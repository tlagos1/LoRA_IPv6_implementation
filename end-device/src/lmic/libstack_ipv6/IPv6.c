
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h> 

#include "SCHC.h"
#include "IPv6.h"


ip6_header_buffer *ipv6_header = NULL;

uint16_t checksum (uint16_t *addr, int len)
{

  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;
  uint8_t first,second;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  first = (sum >> 8) & 255;
  second = (sum ) & 255;

  sum = 0;

  sum |= (second << 8);
  sum |= first << 0; 
  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

uint16_t checksum_icmpv6(char *buffer,int lenght_buffer)
{
    uint8_t aux = 0; 
    int i,j = 0;
    char chksum[200];

    for(i = 8; i < 40 ; i++)
    {
         chksum[j] = buffer[i];  // addrs
         j++;
    }
    for(i = 0; i < 2 ; i++)
    {
         chksum[j] = 0;  
         j++;
    }
    aux |= buffer[4] << 8;
    aux |= buffer[5] << 0; 

    chksum[j] = aux/256;j++; // length

    chksum[j] = aux%256;j++; // length
    
    for(i = 0; i < 3 ; i++)
    {
         chksum[j] = 0;  
         j++;
    }
    chksum[j] = 58;j++;
    
    for(i = 40; i < 42; i++)
    {
        chksum[j] = buffer[i];
        j++;
    }
    for(i = 44; i < 48; i++)
    {
        chksum[j] = buffer[i];
        j++;
    }
    for(i = 0; i < 2 ; i++)
    {
        chksum[j] = 0;  
        j++;
    }
    for(i = 48; i < (lenght_buffer) ; i++)
    {
        chksum[j] = buffer[i];
        j++;
    }
    
    return checksum ((uint16_t *)chksum, j); 
}


uint8_t *IPv6_address(uint8_t *last_B_addr, uint8_t *IPv6_addr)
{
  uint8_t first_B_addr[8] = {0xfe, 0x80, 0 ,0 ,0 ,0 ,0 ,0};
  uint8_t aux_last_B_addr[8];

  int i;
  for(i = 0; i < 8; i++)
  {
    aux_last_B_addr[i] = last_B_addr[i];
  }

  memcpy(&IPv6_addr[0],&first_B_addr[0],8);
  memcpy(&IPv6_addr[8],&aux_last_B_addr[0],8);

  return IPv6_addr;
}


NodeList *get_info_by_IPv6(NodeList *list, uint8_t *IPv6)
{ 
  if(list != NULL)
  {
    while(list != NULL)
    {
      if(memcmp(&list->IPv6.addr[0], &IPv6[0], 16) == 0)
      {
        return list;
      }
      list = list->back;
    }
  } 
  return NULL;
}

NodeList *get_info_by_mac(NodeList *list, uint8_t *mac)
{
  if(list != NULL)
  {
    while(list != NULL)
    {
      if(memcmp(&list->mac.addr[0], &mac[0], 6) == 0)
      {
        return list;
      }
      list = list->back;
    }
  } 
  return NULL;
}

void add_node(NodeList **list, uint8_t *mac, int type)
{
  int i;
  uint8_t link_local_ften_bytes[10] = {0xfe, 0x80, 0 ,0 ,0 ,0 ,0 ,0 ,0 , 0};
  uint8_t mac_aux[6];

  for(i = 0 ; i < 6; i++)
  {
    mac_aux[i] = mac[i];
  }

  if((*list) == NULL)
  {

    (*list) = (NodeList *)malloc(sizeof(NodeList));

    memcpy(&(*list)->IPv6,&link_local_ften_bytes[0],10);
    memcpy(&(*list)->IPv6.addr[10],&mac_aux[0],6);

    memcpy(&(*list)->mac.addr[0],&mac[0],6);

    (*list)->type = type;
    (*list)->next = NULL;
    (*list)->back = NULL;
  }
  else
  {
    if(memcmp(&(*list)->mac.addr[0],&mac[0],6) != 0)
    {
      NodeList **aux = list;
      while((*aux)->next != NULL)
      {  
        if(memcmp(&(*aux)->mac.addr[0],&mac[0],6) == 0)
        {
            return;
        }   
        *aux = (*aux)->next;
      }
    
      (*aux)->next = (NodeList *)malloc(sizeof(NodeList));
      

      if((*aux)->next != NULL)
      {
        (*aux)->next->back = *aux;

        memcpy(&(*aux)->next->IPv6,&link_local_ften_bytes[0],10);
        memcpy(&(*aux)->next->IPv6.addr[10],&mac_aux[0],6);

        memcpy(&(*aux)->next->mac.addr[0],&mac[0],6);

        (*aux)->next->type = type;
        (*aux)->next->next = NULL;
      }
/*      while((*aux)->back != NULL)
      {
        *aux = (*aux)->back;
      }*/
    }
  }
  
}


char *icmp_reply(char *buffer, uint8_t *src_address, uint8_t *dst_address)
{
  int offset = 0;
  uint16_t checksum = 0;
  uint16_t payload_length = 0;
  uint8_t multicast[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01}; 


  // first 8 bytes elided  
  
  offset += 8;

  // src elided

  offset += 16;

  // dst address

  if(memcmp(&buffer[offset],&src_address[0],16) == 0 || memcmp(&buffer[offset],&multicast[0],16) == 0)// unicast or multicast
  {
    offset -= 16;

    // src address
    memcpy(&buffer[offset], &src_address[0],16);
    offset += 16;

    // dst address
    memcpy(&buffer[offset], &dst_address[0],16);
    offset += 16;

    // echo reply
    buffer[offset] = 0x81;

    payload_length = buffer[4] << 8 | buffer[5];

    checksum = checksum_icmpv6(buffer,(payload_length + 40));
        
    buffer[42] = (checksum >> 8) & 255;
    buffer[43] = (checksum >> 0) & 255;

    return buffer;    
  }

  buffer[0] = 0;
  
  return buffer;

}

char *router_solicitation(uint8_t *src_address, char *buffer)
{
  int offset = 0;
  uint8_t dst_address[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02};
  uint16_t payload_length = 8;
  uint16_t checksum = 0;
  
  // version, traffic class, flow label
  buffer[0] = 0x60;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  offset += 4;

  // payload length
  buffer[offset] = (payload_length >> 8) & 255;
  buffer[offset + 1] = (payload_length >> 0) & 255;

  offset += 2;

  // next header
  buffer[offset] = 0x3a;
  
  offset += 1;  

  // hop limit
  buffer[offset] = 1;

  offset += 1;  

  //soure address
  memcpy(&buffer[offset], &src_address[0],16);

  offset += 16;

  //destination address
  memcpy(&buffer[offset], &dst_address[0],16);

  offset += 16;

  // ICMPv6
  // type
  buffer[offset] = 0x85;

  offset += 1;
  
  // code
  buffer[offset] = 0;
  offset += 1;

  // checksum will be calculate at the end

  offset += 2;

  //reserved
  
  buffer[offset] = 0;
  buffer[offset+1] = 0;
  buffer[offset+2] = 0;
  buffer[offset+3] = 0;

  offset += 4;  

  // checksum
  checksum = checksum_icmpv6(buffer,(offset));
  buffer[42] = (checksum >> 8) & 255;
  buffer[43] = (checksum >> 0) & 255;

  return buffer;
}

char *router_advertisement(char *buffer, uint8_t *IPv6_address, uint8_t *mac_address)
{
  int offset = 0;
  uint16_t checksum = 0;
  uint16_t payload_length = 0;
  uint8_t dst_address[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01};
  uint16_t router_lifetime = 64800;
  uint32_t reachable_time = 30000;
  uint32_t retrans_time = 1000;

  // first 8 bytes elided - payload length will be calculate at the end
  offset += 8;

  // set src address with the gateway address  
  memcpy(&buffer[offset],&IPv6_address[0],16);
  offset += 16;

  // set dst address with node multicast
  memcpy(&buffer[offset],&dst_address[0],16);
  offset += 16;
  
  // router advertisement
  buffer[offset] = 0x86;
  offset += 1;

  // code
  buffer[offset] = 0;
  offset += 1;

  // checksum will be calucate at the end
  offset += 2;

  // Cur hop limit
  buffer[offset] = 0;
  offset += 1;

  //Autoconfig flags will be 0 node IPv6 is not dhcp6
  buffer[offset] = 0;
  offset += 1;

  // Router Lifetime max 18 hours
  buffer[offset] |= (router_lifetime >> 8) & 255;
  buffer[offset + 1] |= (router_lifetime) & 255;
  offset += 2;

  // Reachable Time  30000 ms
  buffer[offset] |= (reachable_time >> 24) & 255;
  buffer[offset + 1] |= (reachable_time >> 16) & 255;
  buffer[offset + 2] |= (reachable_time >> 8) & 255;
  buffer[offset + 3] |= (reachable_time) & 255;
  offset += 4;
  
  //Retrans Timer 1000 ms
  buffer[offset] |= (retrans_time >> 24) & 255;
  buffer[offset + 1] |= (retrans_time >> 16) & 255;
  buffer[offset + 2] |= (retrans_time >> 8) & 255;
  buffer[offset + 3] |= (retrans_time) & 255;
  offset += 4;

  //option - mac address
  buffer[offset] = 0x01;
  offset += 1;

  //length
  buffer[offset] = 0x01;
  offset += 1;

  // link-layer
  memcpy(&buffer[offset], &mac_address[0],6);
  offset += 6; 

  payload_length = (offset - 40);
  buffer[4] = (payload_length >> 8) & 255;
  buffer[5] = (payload_length >> 0) & 255;

  // checksum
  checksum = checksum_icmpv6(buffer,(offset));
  buffer[42] = (checksum >> 8) & 255;
  buffer[43] = (checksum >> 0) & 255; 


  return buffer;

}

char *neighbor_solicitation(char *buffer, uint8_t *IPv6_address, uint8_t *IPv6_dst, uint8_t *mac_address)
{
  int offset = 0;
  uint16_t payload_length = 0;
  uint16_t checksum = 0;
  uint8_t target_addres[16];

  int i;
  for (i = 0; i < 16; i++)
  {
    target_addres[i] = IPv6_dst[i];
  }

  // version, traffic class, flow label
  buffer[0] = 0x60;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  offset += 4;

  // payload lenght will be calulate at the end

  offset += 2;

  // next header
  buffer[offset] = 0x3a;
  
  offset += 1;  

  // hop limit
  buffer[offset] = 1;

  offset += 1;  

  //soure address
  memcpy(&buffer[offset], &IPv6_address[0],16);

  offset += 16;

  //destination address
  memcpy(&buffer[offset], &IPv6_dst[0],16);

  offset += 16;

  // ICMPv6
  // type
  buffer[offset] = 0x87;

  offset += 1;
  
  // code
  buffer[offset] = 0;
  offset += 1;

  // checksum will be calculate at the end

  offset += 2;

  //reserved
  
  buffer[offset] = 0;
  buffer[offset+1] = 0;
  buffer[offset+2] = 0;
  buffer[offset+3] = 0;

  offset += 4;  

  //target address
  memcpy(&buffer[offset],&target_addres[0],16);
  offset += 16;  

  //option
  // - type
  buffer[offset] = 1;
  offset += 1;

  // -length
  buffer[offset] = 1;
  offset += 1;

  // -link-layer
  memcpy(&buffer[offset],&mac_address[0],6);
  offset += 6;

  // payload length
  payload_length = (offset - 40);
  buffer[4] = (payload_length >> 8) & 255;
  buffer[5] = (payload_length >> 0) & 255;

  // checksum
  checksum = checksum_icmpv6(buffer,(offset));
  buffer[42] = (checksum >> 8) & 255;
  buffer[43] = (checksum >> 0) & 255;

  return buffer;
}

char *neighbor_advertisement(char *buffer, uint8_t *IPv6_address, uint8_t *dst_address)
{
  int offset = 0;
  uint16_t payload_length = 0;
  uint16_t checksum = 0;
  uint8_t target_addres[16];

  int i;
  for (i = 0; i < 16; i++)
  {
    target_addres[i] = IPv6_address[i];
  }

  // version, traffic class, flow label
  buffer[0] = 0x60;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  offset += 4;

  // payload lenght will be calulate at the end

  offset += 2;

  // next header
  buffer[offset] = 0x3a;
  
  offset += 1;  

  // hop limit
  buffer[offset] = 1;

  offset += 1;  

  //soure address
  memcpy(&buffer[offset], &IPv6_address[0],16);

  offset += 16;

  //destination address
  memcpy(&buffer[offset], &dst_address[0],16);

  offset += 16;

  // ICMPv6
  // type
  buffer[offset] = 0x88;

  offset += 1;
  
  // code
  buffer[offset] = 0;
  offset += 1;

  // checksum will be calculate at the end

  offset += 2;

  //flags
  buffer[offset] = 0;
  buffer[offset] |= 1 << 7; // 1 is gateway - 0 is node
  buffer[offset] |= 1 << 6; // 1 is neighbor solicitation answer
  buffer[offset] |= 0 << 5; // 1 rewrite cache from source device - 0 else
  buffer[offset + 1] = 0;  // reserved
  buffer[offset + 2] = 0;  // reserved
  buffer[offset + 3] = 0;  // reserved

  offset += 4;  

  //target address
  memcpy(&buffer[offset],&target_addres[0],16);
  offset += 16;  

  //option - elided
  
  // payload length
  payload_length = (offset - 40);
  buffer[4] = (payload_length >> 8) & 255;
  buffer[5] = (payload_length >> 0) & 255;

  // checksum
  checksum = checksum_icmpv6(buffer,(offset));
  buffer[42] = (checksum >> 8) & 255;
  buffer[43] = (checksum >> 0) & 255;

  return buffer;   
}

// optional
char *redirect(char *buffer, uint8_t *IPv6_address, uint8_t *dst_address, uint8_t *target_addres, uint8_t *mac_address)
{
  int offset = 0;
  uint16_t payload_length = 0;
  uint16_t checksum = 0;
  uint8_t dst_addres_copy[16];

  int i;
  for (i = 0; i < 16; i++)
  {
    dst_addres_copy[i] = dst_address[i];
  }

  // version, traffic class, flow label
  buffer[0] = 0x60;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  offset += 4;

  // payload lenght will be calulate at the end

  offset += 2;

  // next header
  buffer[offset] = 0x3a;
  
  offset += 1;  

  // hop limit
  buffer[offset] = 1;

  offset += 1;  

  //soure address
  memcpy(&buffer[offset], &IPv6_address[0],16);

  offset += 16;

  //destination address
  memcpy(&buffer[offset], &dst_address[0],16);

  offset += 16;

  // ICMPv6
  // type
  buffer[offset] = 0x89;

  offset += 1;
  
  // code
  buffer[offset] = 0;
  offset += 1;

  // checksum will be calculate at the end

  offset += 2;

  // reserved
  buffer[offset] = 0;
  buffer[offset + 1] = 0;
  buffer[offset + 2] = 0;
  buffer[offset + 3] = 0;
  
  offset += 4;

  // target address
  memcpy(&buffer[offset], &target_addres[0],16);

  offset += 16;

  // destination address
  memcpy(&buffer[offset], &dst_addres_copy[0],16);

  offset += 16;

  // option
  // - type
  buffer[offset] = 1;
  offset += 1;

  // -length
  buffer[offset] = 1;
  offset += 1;

  // -link-layer
  memcpy(&buffer[offset],&mac_address[0],6);
  offset += 6;

  // payload length
  payload_length = (offset - 40);
  buffer[4] = (payload_length >> 8) & 255;
  buffer[5] = (payload_length >> 0) & 255;

  // checksum
  checksum = checksum_icmpv6(buffer,(offset));
  buffer[42] = (checksum >> 8) & 255;
  buffer[43] = (checksum >> 0) & 255;

  return buffer;
}