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

/*Edited by Tomás Lagos*/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define TX_INTERVAL 1000
      
uint8_t DEVEUI[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};

ip6_buffer *ip6;
lowpan_header *lowpan_h;
osjob_t txjob;


// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
};



void tx(char *str, int lenght) {
  int i = 0;
  os_radio(RADIO_RST); // Stop RX first
 // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  
  for(i = 0;i<lenght;i++)
  {
    LMIC.frame[LMIC.dataLen++] = str[i];
  }
  delay(random(500));
  digitalWrite(9, LOW);
  digitalWrite(8, HIGH);   // set the LED on
  
  os_radio(RADIO_TX);
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


/*///////////////////////////////////////////////////////////// RX function ///////////////////////////////////////////////////////////////////////////////*/
static void rx_func (osjob_t* job) 
{
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH); 
  
  char *buffer, RoHC_package[250],*SCHC, LoWPAN_IPHC[250];

  int payload_length,i;
  
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
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  
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
