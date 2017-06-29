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



ip6_buffer *ip6;
lowpan_header *lowpan_h;

uint8_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
// How often to send a packet. Note that this sketch bypasses the normal
// LMIC duty cycle limiting, so when you change anything in this sketch
// (payload length, frequency, spreading factor), be sure to check if
// this interval should not also be increased.
// See this spreadsheet for an easy airtime and duty cycle calculator:
// https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc 
#define TX_INTERVAL 1000

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
  int i = 0;
  os_radio(RADIO_RST); // Stop RX first
 // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  
  for(i = 0;i<lenght;i++)
  {
    LMIC.frame[LMIC.dataLen++] = str[i];
  }
    delay(1);
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


/*///////////////////////////////////////////////////////////// RX function ///////////////////////////////////////////////////////////////////////////////*/
static void rx_func (osjob_t* job) {
  
  char *buffer, RoHC_package[250],*SCHC, LoWPAN_IPHC[250];

  int payload_length;
  
  SCHC = SCHC_RX((char *)LMIC.frame,LMIC.dataLen); // SCHC to 6LoWPAN
   
  if(SCHC != NULL)
  {
     payload_length = LMIC.dataLen - 3;  // 3 bytes are for SCHC rule and checksum;
      
     buffer = IPv6Rx(SCHC, payload_length, 0, ip6, IPv6_address(DEVEUI)); // 6LoWPAN to IPv6 -------- ICMP6 request to ICMP6 replay

     if(buffer != NULL) // if I get a valid IPv6 Buffer
     {
        payload_length = ((int)buffer[5]); // payload length from the new buffer
      
        memcpy(LoWPAN_IPHC, IPv6ToMesh(buffer, payload_length, lowpan_h),250); //  IPv6 to 6LoWpan 
        
        memcpy(RoHC_package, SCHC_TX(LoWPAN_IPHC, payload_length), 250); // 6LoWPAN to SCHC
        
        LMIC.dataLen = payload_length + 3;

        tx(RoHC_package,LMIC.dataLen); // SCHC TX
      }
      
    }
 
  os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), tx_rx_function);
}


static void tx_rx_function(osjob_t* job)
{
  rx(rx_func);
}

// application entry point
void setup() {
  Serial.begin(115200);
  Serial.println("start");
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
  LMIC.txpow = 14;
  // Use a medium spread factor. This can be increased up to SF12 for
  // better range, but then the interval should be (significantly)
  // lowered to comply with duty cycle limits as well.
  LMIC.datarate = DR_SF10;
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
