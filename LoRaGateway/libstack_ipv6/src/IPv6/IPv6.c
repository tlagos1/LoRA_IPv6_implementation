
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h> 
#include <pthread.h>

#include "SCHC.h"
#include "IPv6.h"



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


uint8_t *IPv6_address(uint8_t *last_B_addr, uint8_t *IPv6_addr, int type)
{
  uint8_t first_B_addr[8];
  int ix;
  if(type == 0) // link - local
  {
    first_B_addr[0] = 0xfe;
    first_B_addr[1] = 0x80; 
    for ( ix = 2; ix < 8; ix++)
    {
       first_B_addr[ix] = 0;
    } 
  }
  else
  {
    first_B_addr[0] = 0xaa;
    first_B_addr[1] = 0xaa; 
    for ( ix = 2; ix < 8; ix++)
    {
       first_B_addr[ix] = 0;
    } 
  }
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
  uint8_t IPv6_link_local[8] = {0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00};

  list = check_head(list);
  
  if(list != NULL)
  {
    while(list != NULL)
    {
      if(memcmp(IPv6_link_local, IPv6, 8) == 0)
      {
        if(memcmp(&list->IPv6_link_local.addr[0], &IPv6[0], 16) == 0)
        {
          return list;
        }
      }
      else
      {
        if(memcmp(&list->IPv6_global.addr[0], &IPv6[0], 16) == 0)
        {
          return list;
        }
      }
      list = list->next;
    }
  }
  return NULL;
}

NodeList *get_info_by_eui(NodeList *list, uint8_t *eui)
{
  list = check_head(list);
  if(list != NULL)
  {
    while(list != NULL)
    {
      if(memcmp(&list->eui.addr[0], &eui[0], 8) == 0)
      {
        return list;
      }
      list = list->next;
    }
  } 
  return NULL;
}

NodeList *get_info_by_mac(NodeList *list, uint8_t *mac)
{
  list = check_head(list);
  if(list != NULL)
  {
    while(list != NULL)
    {
      if(memcmp(&list->mac.addr[0], &mac[0], 6) == 0)
      {
        return list;
      }
      list = list->next;
    }
  } 
  return NULL;
}

NodeList *check_head(NodeList *list)
{
  if(list != NULL)
  {
    while(list->back != NULL)
    {
      list = list->back; 
    }
    return list;
  }
  else
  {
    return NULL;
  }
  
}

NodeList *add_IPv6_node(NodeList *list, uint8_t *IPv6, uint8_t * mac)
{
  uint8_t IPv6_link_local[8] = {0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00};
  NodeList *aux;
  aux = check_head(list);
  while(aux != NULL)
  {
    if(memcmp(&aux->mac.addr[0], &mac[0], 6) == 0)
    {
        if(memcmp(&IPv6_link_local[0], &IPv6[0], 8) == 0)
        {
          memcpy(&aux->IPv6_link_local.addr[0], &IPv6[0], 16);
          return aux;
        }
        else
        {
          memcpy(&aux->IPv6_global.addr[0], &IPv6[0], 16);
          return aux;
        }
    }
    else
    {
      aux = aux->next;
    }
  }
  return list;
}


NodeList *add_MAC_node(NodeList *list, uint8_t *mac, int type)
{ 
  int i;
  if(list == NULL)
  {
    list = (NodeList *)malloc(sizeof(NodeList));
    
    for(i = 0; i < 6; i++)
    {
      list->mac.addr[i] = mac[i];
    }

    memset(list->IPv6_link_local.addr,0,16);
    memset(list->IPv6_global.addr,0,16);
    memset(list->eui.addr,0,8);
    list->type = type;
    list->next = NULL;
    list->back = NULL;
    return list;
  }

  else
  {
    NodeList *aux;
    aux = check_head(list);
    while(aux != NULL)
    { 
      if(memcmp(&mac[0],&aux->mac.addr[0],6) == 0)
      {
        return aux;
      }
      else
      {
        if(aux->next == NULL)
        {
          break;
        }
        else
        {
          aux = aux->next;
        }
      }  
    }
    aux->next = (NodeList *)malloc(sizeof(NodeList));
    aux->next->back = aux;

    memset(aux->next->mac.addr, 0, 6);
    for(i = 0; i < 6; i++)
    {
      aux->next->mac.addr[i] = mac[i];
    }
  
    memset(aux->next->IPv6_link_local.addr,0,16);
    memset(aux->next->IPv6_global.addr,0,16);
    memset(aux->next->eui.addr,0,8);
    aux->next->type = type;
    aux->next->next = NULL;
    return aux;
  }
}

NodeList *add_EUI_node(NodeList *list, uint8_t *eui, int type)
{ 
  int i;

  if(list == NULL)
  {
    list = (NodeList *)malloc(sizeof(NodeList));
    
    memset(list->mac.addr, 0, 6);
    list->eui.addr[0] = eui[0];
    list->eui.addr[1] = eui[1];

    for(i = 2; i < 8; i++)
    {
      list->mac.addr[i-2] = eui[i];
      list->eui.addr[i] = eui[i];
    }

    memset(list->IPv6_link_local.addr,0,16);
    memset(list->IPv6_global.addr,0,16);
    list->type = type;
    list->next = NULL;
    list->back = NULL;

    return list;
  }

  else
  {
    NodeList *aux;
    aux = check_head(list);
    while(aux != NULL)
    { 
      if(memcmp(&eui[2],&aux->mac.addr[0],6) == 0)
      {
        return aux;
      }
      else
      {
        if(aux->next == NULL)
        {
          break;
        }
        else
        {
          aux = aux->next;
        }
      }  
    }
    aux->next = (NodeList *)malloc(sizeof(NodeList));
    memset(aux->next->mac.addr, 0, 6);
    memset(aux->next->eui.addr, 0, 8);
    aux->next->back = aux;

    aux->next->eui.addr[0] = eui[0];
    aux->next->eui.addr[1] = eui[1];

    for(i = 2; i < 8; i++)
    {
      aux->next->mac.addr[i-2] = eui[i];
      aux->next->eui.addr[i] = eui[i];
    }

    memset(aux->IPv6_link_local.addr,0,16);
    memset(aux->IPv6_global.addr,0,16);
    aux->next->type = type;
    aux->next->next = NULL;
    return aux;
  }
}

Saved_addr *clean_addr(uint8_t *eui, Saved_addr *S_addr)
{
  S_addr = check_head_Saved_addr(S_addr);
  Saved_addr *aux;
  while(S_addr != NULL)
  {
    if(memcmp(eui, S_addr->eui, 8) == 0)
    {
       if(S_addr->back != NULL && S_addr->next == NULL)
       {
          S_addr = S_addr->back;
          S_addr->next = NULL;
       }
       else if(S_addr->back != NULL && S_addr->next != NULL)
       {
          aux = S_addr->next;
          S_addr = S_addr->back;

          S_addr->next = NULL;
          S_addr->next = aux;
       }
       else if(S_addr->back == NULL && S_addr->next != NULL)
       {
          S_addr = S_addr->next;
          S_addr->back = NULL;
       }
       else if(S_addr->back == NULL && S_addr->next == NULL)
       {
          S_addr = NULL;
       }
    }
    else
    {
      S_addr = S_addr->next;
    }
  }
  return S_addr;
}

Saved_addr *output_addr(uint8_t *eui, Saved_addr *S_addr, int type)
{
  S_addr = check_head_Saved_addr(S_addr);
  while(S_addr != NULL)
  {
    if(memcmp(eui, S_addr->eui, 8) == 0 && S_addr->type == type)
    {
       return S_addr;
    }
    else
    {
      S_addr = S_addr->next;
    }
  }
  return NULL;
}

Saved_addr *check_head_Saved_addr(Saved_addr *S_addr)
{
  if(S_addr != NULL)
  {
    while(S_addr->back != NULL)
    {
      S_addr = S_addr->back; 
    }
    return S_addr;
  }
  else
  {
    return NULL;
  }
}

Saved_addr *input_addr(uint8_t *eui, uint8_t *src, uint8_t *dst, Saved_addr *S_addr, int type)
{
  int ix;
  if(S_addr == NULL)
  {
    S_addr = (Saved_addr *)malloc(sizeof(Saved_addr));
    memset(S_addr->src_addr.addr, 0 , 16);
    memset(S_addr->dst_addr.addr, 0 , 16);
    for(ix = 0; ix < 16; ix ++)
    {
      if(ix < 8)
      {
        S_addr->eui[ix] = eui[ix];
      }
      S_addr->src_addr.addr[ix] = src[ix];
      S_addr->dst_addr.addr[ix] = dst[ix];
    }
    S_addr->type = type;
    S_addr->next = NULL;
    S_addr->back = NULL;
    return S_addr;
  }
  else
  {
    Saved_addr *aux;
    aux = check_head_Saved_addr(S_addr);
    while(aux != NULL)
    {
      if(memcmp(eui,aux->eui,8) == 0 && aux->type == type)
      {
        for(ix = 0; ix < 16; ix ++)
        {
          aux->src_addr.addr[ix] = src[ix];
          aux->dst_addr.addr[ix] = dst[ix];
        }
        return aux;
      }
      else
      {
        if(aux->next == NULL)
        {
          break;
        }
        else
        {
          aux = aux->next;
        }
      } 
    }
    aux->next = (Saved_addr *)malloc(sizeof(Saved_addr));
    aux->next->back = aux;

    for(ix = 0; ix < 16; ix ++)
    {
      if(ix < 8)
      {
        aux->next->eui[ix] = eui[ix];
      }
      aux->next->src_addr.addr[ix] = src[ix];
      aux->next->dst_addr.addr[ix] = dst[ix];
    }
    aux->next->type = type;
    aux->next->next = NULL;
    return aux;
  }
}

char *icmp_reply(char *buffer, uint8_t *src_address)
{
  int offset = 0;
  uint16_t checksum = 0;
  uint16_t payload_length = 0; 
  uint8_t dst_address[16];
  int ix;
  // first 8 bytes elided  
  
  offset += 8;

  // src: saved to be dst

  for(ix = offset; ix < (offset + 16); ix++)
  {
      dst_address[ix-offset] = (uint8_t)buffer[ix];
  }

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


char *router_solicitation(uint8_t *src_address, char *buffer, uint8_t *mac_address)
{
  int offset = 0;
  uint8_t dst_address[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02};
  uint16_t payload_length;
  uint16_t checksum = 0;
  
  

  // version, traffic class, flow label
  buffer[0] = 0x60;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  offset += 4;

  // payload length
  offset += 2;

  // next header
  buffer[offset] = 0x3a;
  
  offset += 1;  

  // hop limit
  buffer[offset] = 255;

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
  buffer[offset] = 255;

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

char *neighbor_advertisement(char *buffer, uint8_t *IPv6_address, uint8_t *dst_address, uint8_t *target_address)
{
  int offset = 0;
  uint16_t payload_length = 0;
  uint16_t checksum = 0;
  
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
  buffer[offset] = 255;

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
  buffer[offset] |= 0 << 7; // 1 is gateway - 0 is node
  buffer[offset] |= 1 << 6; // 1 is neighbor solicitation answer
  buffer[offset] |= 0 << 5; // 1 rewrite cache from source device - 0 else
  buffer[offset + 1] = 0;  // reserved
  buffer[offset + 2] = 0;  // reserved
  buffer[offset + 3] = 0;  // reserved

  offset += 4;  

  //target address
  memcpy(&buffer[offset],&target_address[0],16);
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

IPv6PackageRawData *get_IPv6_data_by_raw_package(char *buffer, IPv6PackageRawData *rawdata)
{
  int offset = 0;
  int ix;

  memset(rawdata->IPv6_src.addr, 0, 16);
  memset(rawdata->IPv6_dst.addr, 0, 16);

  offset += 4; 
  
  rawdata->payload_length = (buffer[offset] << 8 | buffer[offset+1]);
  offset += 2;

  rawdata->next_h = buffer[offset];
  offset += 1;
  
  rawdata->h_limit = buffer[offset];
  offset += 1;
  
  for(ix = 0; ix < 16; ix++)
  {
    rawdata->IPv6_src.addr[ix] = buffer[offset+ix];
  }

  offset += 16;

  for(ix = 0; ix < 16; ix++)
  {
    rawdata->IPv6_dst.addr[ix] = buffer[offset+ix];
  }
  return rawdata;
}

ICMP6PackageRawData *get_ICMP6_data_by_raw_payload(char *buffer, ICMP6PackageRawData *rawpayload)
{

  int offset = 40;
  int ix;

  memset(rawpayload->mac,0,6);
  memset(rawpayload->target,0,16);
  memset(rawpayload->checksum,0,2);

  //type
  rawpayload->type = buffer[offset];
  offset += 1;
  
  // Code Elided
  offset += 1;

  //checksum
  rawpayload->checksum[0] = buffer[offset];
  rawpayload->checksum[1] = buffer[offset + 1];
  offset += 2;

  if(rawpayload->type == 0x85) // router solicitation
  {
    // reserved elided
    offset += 4;

    if(buffer[offset] == 0x01) // mac  
    {
      offset += 1;      
      if(buffer[offset] == 0x01) // mac size; 
      {
          offset += 1;
          for(ix = 0; ix < 6; ix++)
          {
            rawpayload->mac[ix] = buffer[offset + ix];
          }
      }
    }
  }

  else if(rawpayload->type == 0x86) // router advertisement
  {
    // Cur Hop Limit elided
    offset += 1;
    
    // Autoconfig Flags elided
    offset += 1;
    
    // Router Lifetime elided
    offset += 2;
    
    // Reachable Time elided
    offset += 4;

    // Retrans Time elided
    offset += 4;

    if(buffer[offset] == 0x01) // mac  
    {
      offset += 1;      
      if(buffer[offset] == 0x01) // mac size; 
      {
          offset += 1;
          for(ix = 0; ix < 6; ix++)
          {
            rawpayload->mac[ix] = buffer[offset + ix];
          }
      }
    }
  }

  else if(rawpayload->type == 0x87) // neighbor solicitation
  {
    // reserved elided
    offset += 4;

    //target addr
    for(ix = 0; ix < 16; ix ++)
    {
        rawpayload->target[ix] = buffer[offset + ix];
    }
    offset += 16;

    if(buffer[offset] == 0x01) // mac  
    {
      offset += 1;      
      if(buffer[offset] == 0x01) // mac size; 
      {
          offset += 1;
          for(ix = 0; ix < 6; ix++)
          {
            rawpayload->mac[ix] = buffer[offset + ix];
          }
      }
    }
  }
  else if(rawpayload->type == 0x88) // neighbor advertisement
  {
    // flags elided
    offset += 4;


    //target addr
    for(ix = 0; ix < 16; ix ++)
    {
        rawpayload->target[ix] = buffer[offset + ix];
    }
    offset += 16;

    if(buffer[offset] == 0x01) // mac  
    {
      offset += 1;      
      if(buffer[offset] == 0x01) // mac size; 
      {
          offset += 1;
          for(ix = 0; ix < 6; ix++)
          {
            rawpayload->mac[ix] = buffer[offset + ix];
          }
      }
    }
  }
  return rawpayload;
}