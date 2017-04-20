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

ip6_header_buffer *ipv6_header_schc;    
NodeList *node_list;

uint8_t DEVEUI[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03};

osjob_t txjob;

int rs,ns;
int first_time = 1;

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


static void ndiscovery_rx (osjob_t* job) {
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH); 

  uint8_t mac_address[6];  
  uint16_t payload_size;
  char buffer[200], payload[200];
  uint8_t IPv6[16];

  int j;
  int offset = 40;

  uint8_t router_advertisement = 0x86,neighbor_advertisement = 0x88;
  
  memcpy(&buffer[0],schc_decompression((char *)LMIC.frame, buffer, LMIC.dataLen), 200); 

  payload_size = buffer[4] << 8 | buffer[5];  

  if(memcmp(&buffer[offset], &router_advertisement, 1) == 0)
  {
   
    // type
    offset +=1;

    // code
    offset +=1;

    // checksum
    offset +=2;

    //cur hop limit
    offset +=1;

    //Autoconfig Flags
    offset +=1;

    //Router Lifetime
    offset +=2;

    //Reachable Time
    offset +=4;

    //Retrans Timer
    offset +=4;

    // option 
    // type
    if(buffer[offset] == 1)
    {
      offset += 1;
      if(buffer[offset] == 1)
      {
        offset += 1;
        memcpy(&mac_address[0], &buffer[offset] ,6);
      
        add_node(&node_list, mac_address, 1);
      
        rs = 1;
        ns = 0;
      }
    }
    
  }

  if(memcmp(&buffer[offset], &neighbor_advertisement, 1) == 0)
  {
    
    // type
    offset +=1;

    // code
    offset +=1;

    // checksum
    offset +=2;

    // flags
    offset +=4;

    // target address
    if(memcmp(&node_list->IPv6.addr[0], &buffer[offset], 16) == 0)
    {
       ns = 1;
    }
     
  }
 
 // rx(ndiscovery_rx);
}


static void nd_tx_done_func (osjob_t* job) {
  
  rx(ndiscovery_rx);
}

static void ndiscovery_tx(osjob_t* job) {
  
  char buffer[200],schc_buffer[48];
  int size_schc = 0;
  int j;
  uint8_t IPv6[16];
  
  if(rs == 0) // router solicitation
  {
    Serial.println("router solicitation");
    size_schc = 1 + 8 + 8 ; // id + last src + last dst
    memcpy(&buffer[0],router_solicitation(IPv6_address(DEVEUI, IPv6), buffer),200);
  }  
  if(ns == 0) // neighbor solicitation
  {
    Serial.println("neighbor solicitation");
    uint8_t gateway_address[16];
    uint8_t mac_addr[6]; 
    
    for(j = 0; j < 16; j++)
    {
      gateway_address[j] = node_list->IPv6.addr[j];
      if(j >= 2 && j <= 7)
      {
        mac_addr[j-2] = DEVEUI[j];
      }
    }
    size_schc = 1 + 8 + 8 + 1 + 6 ; // id + last src + last dst + option type + link layer
    memcpy(&buffer[0],neighbor_solicitation(buffer, IPv6_address(DEVEUI, IPv6), gateway_address, mac_addr),200);
    
  }
  
  memcpy(&buffer[0],schc_compression(buffer, schc_buffer, ipv6_header_schc),200);
    
  tx(buffer, size_schc, nd_tx_done_func);

  if(rs == 0 || ns == 0) // router solicitation or neighbor solicitation
  {
    os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), ndiscovery_tx);
  }
  else
  {
    os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), tx_rx_function);
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
  delay(random(500));
  digitalWrite(9, LOW);
  digitalWrite(8, HIGH);   // set the LED on

  if(rs == 0 || ns == 0) // router solicitation || neighbor solicitation
  {
    LMIC.osjob.func = func;
  }
  
  os_radio(RADIO_TX);
}




/*///////////////////////////////////////////////////////////// RX function ///////////////////////////////////////////////////////////////////////////////*/
static void rx_func (osjob_t* job) 
{
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH); 

  
  uint8_t gateway_address[16];
  int j;
  uint8_t IPv6[16];
  

  for(j = 0; j < 16; j++)
  {
      gateway_address[j] = node_list->IPv6.addr[j];
  }
  char buffer[200],schc_buffer[200];

  int packet_size = 0, schc_size = 0;


  
  
  if(LMIC.frame[0] == 0x10)
  {
    packet_size = 40 + ((LMIC.dataLen + 8) - (1 + 8 + 8 + 2 + 2));
  }

  memcpy(&buffer[0],schc_decompression((char *)LMIC.frame, buffer, LMIC.dataLen), 200); 

  for(j = 0; j < 58; j++)
  {
     Serial.print((uint8_t)buffer[j]);
     Serial.print("-");
  }  
  Serial.println();
  
  if(node_list != NULL) // if the node have the the gateway do echo reply
  {   
    memcpy(&buffer[0],icmp_reply(buffer, IPv6_address(DEVEUI, IPv6),  gateway_address),200);
    
    if(buffer[0] != 0)
    {
     schc_size = (1 + 8 + 8 + 2 + 2 + (packet_size - 40 - 8));
  
     memcpy(&buffer[0], schc_compression(buffer, schc_buffer, ipv6_header_schc), 200);
        
     tx(buffer,LMIC.dataLen,tx_rx_function); 

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

  ipv6_header_schc = (ip6_header_buffer *)malloc(sizeof(ip6_header_buffer));
  node_list = (NodeList *)NULL; 
  // initialize runtime env
  os_init();
  
  rs = 0;  
  ns = 1;
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
  
  os_setCallback(&txjob, ndiscovery_tx);
  //Serial.println("start");
}

void loop() {
  // execute scheduled jobs and events
  os_runloop_once();
}
