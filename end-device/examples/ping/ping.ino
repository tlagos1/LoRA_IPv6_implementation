/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example transmits data on hardcoded channel and receives data
 * when not transmitting. Running this sketch on two nodes should allow
 * them to communicate.
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#if !defined(DISABLE_INVERT_IQ_ON_RX)
#error This example requires DISABLE_INVERT_IQ_ON_RX to be set. Update \
       config.h in the lmic library to set it.
#endif

ip6_buffer *ip6;
lowpan_header *lowpan_h;

// How often to send a packet. Note that this sketch bypasses the normal
// LMIC duty cycle limiting, so when you change anything in this sketch
// (payload length, frequency, spreading factor), be sure to check if
// this interval should not also be increased.
// See this spreadsheet for an easy airtime and duty cycle calculator:
// https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc 
#define TX_INTERVAL 4000

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
};


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }


osjob_t txjob;
osjob_t timeoutjob;
static void tx_func (osjob_t* job);


void tx(char *str, int lenght) {
  //Serial.print("lala");
  int i = 0;
  os_radio(RADIO_RST); // Stop RX first
  delay(1); // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  for(i = 0;i<lenght;i++)
  {
    LMIC.frame[LMIC.dataLen++] = str[i];
  }
  os_radio(RADIO_TX);
  //Serial.print("lalo");
}

void rx(osjobcb_t func) {
  //os_radio(RADIO_RXON);
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  // Enable "continuous" RX (e.g. without a timeout, still stops after
  // receiving a packet)
  os_radio(RADIO_RXON);
  // Serial.println("start");
}

static void rxtimeout_func(osjob_t *job) {
  digitalWrite(LED_BUILTIN, LOW); // off
}

static void rx_func (osjob_t* job) {
  // Serial.print("asdasd");
  // Blink once to confirm reception and then keep the led on
  char *buffer,*RoHC_package;
  int lenght = 0,i;

  buffer = IPv6Rx((char *)LMIC.frame,LMIC.dataLen,3,ip6);
  
  lenght = ((int)buffer[5]);

  RoHC_package = IPv6ToMesh(buffer,(lenght + 40),lowpan_h);
  //for(i = 0;i<LMIC.dataLen;i++)
  //{
  //  Serial.print((uint8_t)RoHC_package[i]);
  //  Serial.print("-"); 
  //}
  //Serial.print("\n");
  tx(RoHC_package,(LMIC.dataLen));
    
  os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), tx_rx_function);

}


static void tx_rx_function(osjob_t* job)
{
   //Serial.println("start");
  rx(rx_func);
  //os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), tx_rx_function);
}

// application entry point
void setup() {
  //Serial.begin(115200);
  //Serial.println("start");
  pinMode(LED_BUILTIN, OUTPUT);

  ip6 = (ip6_buffer *)malloc(sizeof(ip6_buffer));
  lowpan_h = (lowpan_header *)malloc(sizeof(lowpan_header));
  // initialize runtime env
  os_init();
  
  // Set up these settings once, and use them for both TX and RX

#if defined(CFG_eu868)
  // Use a frequency in the g3 which allows 10% duty cycling.
  LMIC.freq = 868000000;
#elif defined(CFG_us915)
  LMIC.freq = 902300000;
#endif
   
  // Maximum TX power
  LMIC.txpow = 27;
  // Use a medium spread factor. This can be increased up to SF12 for
  // better range, but then the interval should be (significantly)
  // lowered to comply with duty cycle limits as well.
  LMIC.datarate = DR_SF7 ;
  // This sets CR 4/5, BW125 (except for DR_SF7B, which uses BW250)
  LMIC.rps = updr2rps(LMIC.datarate);
  // setup initial job
  os_setCallback(&txjob, tx_rx_function);
  //Serial.println("start");
}

void loop() {
  // execute scheduled jobs and events
  os_runloop_once();
}
