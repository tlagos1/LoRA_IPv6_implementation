
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
      uint8_t slowpan_addr[8];
    } dst;
    struct lowpan_payload
    {
      uint8_t slowpan_payload[25];  // 27 header --- max 52 LoRa payload
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

    uint8_t returnValue[52];
} RoHC_base;


RoHC_base *IPv6ToMesh(char *);

int init_tun();
