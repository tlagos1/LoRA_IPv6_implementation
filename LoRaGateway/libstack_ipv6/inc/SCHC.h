
/*Edited by Tomás Lagos*/

typedef struct ipv6_hdr {
   uint8_t version:4;
   uint8_t tclass;
   uint32_t flabel:20;
   uint16_t paylength;
   uint8_t nheader;
   uint8_t hlimit;

   struct v6_addr
   {
       uint8_t s6_addr[16];
   }ip6_src, ip6_dst;
    
} ip6_header_buffer;

char *schc_compression(char *, char *, ip6_header_buffer *);
char *schc_decompression(char *, char *, int);
char *SCHC_TX(char *, int);
char *SCHC_RX(char *, int);