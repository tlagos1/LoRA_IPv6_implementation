
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <stdint.h>

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
    char chksum[64];

    for(i = 8; i < 40 ; i++)
    {
         chksum[j] = buffer[i];  
         j++;
    }
    for(i = 0; i < 2 ; i++)
    {
         chksum[j] = 0;  
         j++;
    }
    aux |= buffer[4] << 8;
    aux |= buffer[5] << 0; 

    chksum[j] = aux/256;j++;

    chksum[j] = aux%256;j++;
    
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
        j++  ;
    }
    return checksum ((uint16_t *)chksum, j); 
}
