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

SCHC_data *SCHC;
ip6_header_buffer *ipv6_header_schc;    
NodeList *node_list;
IPv6PackageRawData *ipv6_data;
ICMP6PackageRawData *payload_data;

uint8_t DEVEUI[8] = {0x10, 0x4a, 0x00 ,0x16 ,0xC0 , 0x00, 0x00 , 0x03};

osjob_t txjob;

int solicitation = 1, advertisement = 1;
uint8_t src_NS[16];
uint8_t target_ND[16];


// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
};


void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  os_radio(RADIO_RXON);
}

static void tx_done(osjob_t* job) {
  rx(listening_mode);
}

static void rx_done_listening_mode (osjob_t* job) {
    rx(listening_mode); 
}

static void listening_mode (osjob_t* job) {
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  int ix;  
  int offset = 0;
  uint8_t IPv6[16];
  if(memcmp(DEVEUI, LMIC.frame, 8) == 0)
  {
    offset += 8;

    int SCHC_Next_h = (LMIC.frame[offset] >> 7 )& 1;
    int SCHC_src = (LMIC.frame[offset] >> 6 )& 1;

    if(SCHC_Next_h == 0)
    {
      int SCHC_type = (LMIC.frame[offset] >> 2 )& 7;
      int SCHC_target = (LMIC.frame[offset] >> 1 )& 1;
      
      
      if(SCHC_type == 0)  // echo request
      {
          os_setTimedCallback(&txjob, os_getTime() + ms2osticks(TX_INTERVAL), Echo_replay);
      }
      
      if(SCHC_type == 3)
      {
        //     "Router Advertisement;
        solicitation = 0;
        offset += 1;
        src_NS[0] = 0xfe;
        src_NS[1] = 0x80;
        for(ix = 0; ix < 16; ix++)
        {
            if(ix < 8)
            {
                src_NS[ix+8] =  LMIC.frame[(offset) + ix];
            }                
        }
      }
      if(SCHC_type == 4)
      {
         //     "Neighbor Solicitation;
        memset(src_NS, 0 , 16);
        memset(target_ND, 0 , 16);

        offset += 1;

        if(SCHC_src == 0 && SCHC_target == 0 )
        {
            src_NS[0] = 0xfe;
            src_NS[1] = 0x80;
            target_ND[0] = 0xfe;
            target_ND[1] = 0x80;
            for(ix = 0; ix < 8; ix++)
            {
                src_NS[ix+8] =  LMIC.frame[offset + ix];
                target_ND[ix+8] =  LMIC.frame[(offset + 8) + ix];
            }
            if(memcmp(IPv6_address(DEVEUI, IPv6, 0), target_ND, 16) == 0)
            {
              solicitation = 1;
              os_setTimedCallback(&txjob, os_getTime() + ms2osticks(TX_INTERVAL), Neighbor_Solicitation);
            }
        }
      }

      
      if(SCHC_type == 5)
      {
        offset += 1;
        if(SCHC_src == 0)
        {
          
          if(SCHC_target == 1)
          {
            if(memcmp(IPv6_address(DEVEUI, IPv6, 1),&LMIC.frame[offset], 16 ) == 0)
            {
              advertisement = 0;
            }
          }
          else
          {
            if(memcmp(&IPv6_address(DEVEUI, IPv6, 0)[8],&LMIC.frame[offset], 8 ) == 0)
            {
              advertisement = 0;
              solicitation = 1;
              os_setTimedCallback(&txjob, os_getTime() + ms2osticks(TX_INTERVAL), Neighbor_Solicitation);
            }
            else if(memcmp(&target_ND[8],&LMIC.frame[offset], 8 ) == 0)
            {
             //Serial.println("Neighbor Advertisement <--"); 
              solicitation = 0;
            }
          }
        }
      }
    }
  }
  rx(listening_mode);
}

static void Echo_replay(osjob_t* job) {
  int ix;
  int offset = 0;
  
  offset += 8;
  LMIC.frame[offset] |= (1 << 2);

  tx((char *)LMIC.frame, LMIC.dataLen, tx_done);
  //tx_ping((char *)LMIC.frame,  LMIC.dataLen);
}


static void Neighbor_Advertisement(osjob_t* job) {
  if(advertisement == 1)
  {
    char buffer[200],schc_buffer[200];
    int ix;
    uint8_t IPv6[16];
    uint8_t t_address[16];

    for(ix = 0; ix < 16; ix++)
    {
      t_address[ix] = target_ND[ix];
    }
    //Serial.println("Neighbor Advertisement -->");

    memset(buffer,0,200);
    memcpy(&buffer[0],neighbor_advertisement(buffer,IPv6_address(DEVEUI, IPv6, 0), src_NS, t_address),200);

    SCHC->length = 0;
    SCHC = schc_compression(buffer, schc_buffer, SCHC);
    
    for(ix = 0; ix < 8; ix++)
    {
      buffer[ix] = DEVEUI[ix];
    }
    for(ix = 0; ix < SCHC->length; ix++)
    {
      buffer[ix + 8] = SCHC->buffer[ix];
    }
    tx(buffer, (SCHC->length + 8), tx_done);
   
    os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL ), Neighbor_Advertisement);
    
    
  }
}


static void Neighbor_Solicitation(osjob_t* job){
    if(solicitation == 1)
    {
      
      char buffer[200],schc_buffer[200];
      int ix;
      uint8_t IPv6[16];
      uint8_t mac[6];
  
      for(ix = 0; ix < 6; ix++)
      {
        mac[ix] = DEVEUI[ix + 2];
      }
  
      for(ix = 0; ix < 16; ix++)
      {
        target_ND[ix] = src_NS[ix];
      }
      
      memset(buffer,0,200);
      memcpy(&buffer[0],neighbor_solicitation(schc_buffer, IPv6_address(DEVEUI, IPv6, 1), src_NS, target_ND , mac),200);
  
      SCHC = schc_compression(buffer, schc_buffer, SCHC);
  
      for(ix = 0; ix < 8; ix++)
      {
        buffer[ix] = DEVEUI[ix];
      }
      for(ix = 0; ix < SCHC->length; ix++)
      {
        buffer[ix + 8] = SCHC->buffer[ix];
      }
      tx(buffer, (SCHC->length + 8), tx_done);
        
      os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL), Neighbor_Solicitation);
    }
}

static void Router_Solicitation(osjob_t* job) {
  if(solicitation == 1)
  {
    char buffer[200],schc_buffer[200];
    int ix;
    uint8_t IPv6[16];
 
    //Serial.println("router solicitation -->");
  
    memset(buffer,0,200);
    memcpy(&buffer[0],router_solicitation(IPv6_address(DEVEUI, IPv6, 0), buffer, DEVEUI),200);
      
    SCHC = schc_compression(buffer, schc_buffer, SCHC);
 
    for(ix = 0; ix < 8; ix++)
    {
      buffer[ix] = DEVEUI[ix];
    }
    for(ix = 0; ix < SCHC->length; ix++)
    {
      buffer[ix + 8] = SCHC->buffer[ix];
    }
   
    tx(buffer, (SCHC->length + 8), tx_done);

    os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL), Router_Solicitation);
      
  }
}


void tx(char *str, int lenght, osjobcb_t func) {
  int i = 0;
  os_radio(RADIO_RST); // Stop RX first
 // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  
  for(i = 0;i<lenght;i++)
  {
    LMIC.frame[LMIC.dataLen++] = str[i];
  }
  digitalWrite(9, LOW);
  digitalWrite(8, HIGH);   // set the LED on  
  
  LMIC.osjob.func = func;
  os_radio(RADIO_TX);
}


// application entry point
void setup() {
  Serial.begin(115200);
  Serial.println("start");
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  ipv6_header_schc = (ip6_header_buffer *)malloc(sizeof(ip6_header_buffer));
  
  node_list = (NodeList *)NULL;

  SCHC = (SCHC_data *)malloc(sizeof(SCHC_data));
  
  ipv6_data = (IPv6PackageRawData *)malloc(sizeof(IPv6PackageRawData));

  payload_data = (ICMP6PackageRawData *)malloc(sizeof(ICMP6PackageRawData));

  
  
  // initialize runtime env
  os_init();
  
  // Set up these settings once, and use them for both TX and RX
#if defined(CFG_eu868)
  // Use a frequency in the g3 which allows 10% duty cycling.
  LMIC.freq = 868300000;
#elif defined(CFG_us915)
  LMIC.freq = 902300000;
#endif
  // Maximum TX power
  LMIC.txpow = 25;
  // Use a medium spread factor. This can be increased up to SF12 for
  // better range, but then the interval should be (significantly)
  // lowered to comply with duty cycle limits as well.
  LMIC.datarate = DR_SF10;
  // This sets CR 4/5, BW125 (except for DR_SF7B, which uses BW250)
  LMIC.rps = updr2rps(LMIC.datarate);
  // setup initial job
  
  os_setCallback(&txjob, Router_Solicitation);
}

void loop() {
  // execute scheduled jobs and events
  os_runloop_once();
}
