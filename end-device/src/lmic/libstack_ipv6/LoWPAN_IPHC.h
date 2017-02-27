/*Edited by Tomás Lagos*/

#define IPv6_HEADER_LEN            40
#define PAGE_ONE_DISPATCH          0xF1
#define IPHC_DISPATCH              3


#define IPv6_HEADER_LEN            40
#define PAGE_ONE_DISPATCH          0xF1
#define IPHC_DISPATCH              3


#define IPHC_TF_4B                 0
#define IPHC_TF_3B                 1
#define IPHC_TF_1B                 2
#define IPHC_TF_ELIDED             3

#define IPHC_NH_INLINE             0
#define IPHC_NH_COMPRESSED         1

#define IPHC_HLIM_INLINE           0
#define IPHC_HLIM_1                1
#define IPHC_HLIM_64               2
#define IPHC_HLIM_255              3 

#define IPHC_CID_NO                0
#define IPHC_CID_YES               1

#define IPHC_SAC_STATELESS         0
#define IPHC_SAC_STATEFUL          1

#define IPHC_SAM_128B              0
#define IPHC_SAM_64B               1
#define IPHC_SAM_16B               2
#define IPHC_SAM_ELIDED            3

#define IPHC_M_NO                  0
#define IPHC_M_YES                 1

#define IPHC_DAC_STATELESS         0
#define IPHC_DAC_STATEFUL          1

#define IPHC_DAM_128B              0
#define IPHC_DAM_64B               1
#define IPHC_DAM_16B               2
#define IPHC_DAM_ELIDED            3



typedef struct ip6_hdr {
    union {
        struct ip6_hdrctl {
            uint32_t ip6_un1_flow;  /* 20 bits of flow-ID */
            uint16_t ip6_un1_plen;  /* payload length */
            uint8_t  ip6_un1_nxt;   /* next header */
            uint8_t  ip6_un1_hlim;  /* hop limit */
        } ip6_un1;
        uint8_t ip6_un2_vfc;    /* 4 bits version, top 4 bits class */
    } ip6_ctlun;
    
    struct in6_addr
    {
        uint8_t s6_addr[16];
    }ip6_src, ip6_dst;
    
    struct SRHC_payload
    {
        uint8_t s6_payload[64]; 
    } payload;
} ip6_buffer;



typedef struct lowpan
{
    uint8_t  tf;
    uint8_t  nh;
    uint8_t  hlim;
    uint8_t  cid;
    struct lowpan_addr1
    {
      uint8_t slowpan_addr[16];     
    } src;
    struct lowpan_addr2
    {
      uint8_t slowpan_addr[16];
    } dst;
    struct lowpan_payload
    {
      uint8_t slowpan_payload[64];  // 27 header --- max 64 LoRa payload
    }payload;
}lowpan_header;

typedef struct RoHC
{
    uint8_t tfn:2;
    uint8_t nh:1;
    uint8_t hlim:2;
    uint8_t cid:1;
    uint8_t sac:1;
    uint8_t sam:2;
    uint8_t m:1;
    uint8_t dac:1;
    uint8_t dam:2;  
} RoHC_base;


char *IPv6ToMesh(char *, int,lowpan_header *);
 
char *IPv6Rx(char *, int, int,ip6_buffer *,uint8_t *);
