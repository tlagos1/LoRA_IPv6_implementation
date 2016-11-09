
/*Edited by Tomás Lagos*/

#define _BSD_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include "IPv6.h"

#include "LoRaMacCrypto.h"

int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char const *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

RoHC_base *reassemble_lowpan(lowpan_header *lowpan)
{
    uint8_t returnValueBase;
    int i,j;
    int src_size, dst_size, payload_size;

    RoHC_base *aux = (RoHC_base *)malloc(sizeof(RoHC_base));
    if (!lowpan->tf)
    {
        aux->tfn = IPHC_TF_ELIDED;
    }
    else
    {
      return 0;
    }

    aux->nh = IPHC_NH_INLINE;

    if (lowpan->hlim == 1)
    {
        aux->hlim = IPHC_HLIM_1;
        lowpan->hlim = 0;
    }
    else if(lowpan->hlim == 64)
    {
        aux->hlim = IPHC_HLIM_64;
        lowpan->hlim = 0;
    }
    else if (lowpan->hlim == 255)
    {
        aux->hlim = IPHC_HLIM_255;
        lowpan->hlim = 0; 
    }
    else
    {
        aux->hlim = IPHC_HLIM_255; 
    }    
      ///////////////////////////////////////////////////////////
    if (!lowpan->cid)
    {
        aux->cid = IPHC_CID_NO;
    }
    else
    {
        aux->cid = IPHC_CID_YES;
    }

    aux->sac = IPHC_SAC_STATELESS;


    if(sizeof(lowpan->src) == 16)
    {   
        aux->sam = IPHC_SAM_128B;
    }
    else if (sizeof(lowpan->src) == 8)
    {
        aux->sam = IPHC_SAM_64B;
    }
    else if (sizeof(lowpan->src) == 2)
    {
        aux->sam = IPHC_SAM_16B;
    }
    else if (sizeof(lowpan->src) == 0)
    {
        aux->sam = IPHC_SAM_ELIDED;
    }    

    aux->m = IPHC_M_NO;
    aux->dac = IPHC_DAC_STATELESS;

    if(sizeof(lowpan->dst) == 16)
    {
        aux->dam = IPHC_DAM_128B;
    }
    else if (sizeof(lowpan->dst) == 8)
    {
        aux->dam = IPHC_DAM_64B;
    }
    else if (sizeof(lowpan->dst) == 2)
    {
        aux->dam = IPHC_DAM_16B;
    }
    else if (sizeof(lowpan->dst) == 0)
    {
        aux->dam = IPHC_DAM_ELIDED;
    }
    returnValueBase = 0;
    returnValueBase |= (aux->hlim         << 0);
    returnValueBase |= (aux->nh           << 2);
    returnValueBase |= (aux->tfn          << 3);
    returnValueBase |= (IPHC_DISPATCH     << 5);
    returnValueBase |= (PAGE_ONE_DISPATCH << 7);

    aux->returnValue[0] = returnValueBase;

    returnValueBase = 0;
    returnValueBase |= (aux->dam          << 0); 
    returnValueBase |= (aux->dac          << 2);
    returnValueBase |= (aux->m            << 3);
    returnValueBase |= (aux->sam          << 4);
    returnValueBase |= (aux->sac          << 6);
    returnValueBase |= (aux->cid          << 7);
    
    aux->returnValue[1] = returnValueBase;

    aux->returnValue[2] = lowpan->nh;

    j=3;

    src_size = sizeof(lowpan->src.slowpan_addr);
    dst_size = sizeof(lowpan->dst.slowpan_addr);
    payload_size = sizeof(lowpan->payload.slowpan_payload);
	
    for(i= 0; i < src_size; i++)
    {
        aux->returnValue[j] = lowpan->src.slowpan_addr[i];
        j++;
    }
    for(i= 0; i < dst_size; i++)
    {
        aux->returnValue[j] = lowpan->dst.slowpan_addr[i];
        j++;
    }
    for(i=0 ; i < payload_size; i++)
    {
        aux->returnValue[j] = lowpan->payload.slowpan_payload[i];
        j++;
    }
    return aux;
}

RoHC_base *IPv6ToMesh(char ivp6[1500])
{

  int i,j = 0;
    struct ip6_hdr *iph = (struct ip6_hdr *)ivp6;
    if (iph->ip6_dst.s6_addr[0] == 0xff)
    {
      return NULL;
    }
    else
    {
        lowpan_header *lowpanh = (lowpan_header *)malloc(sizeof(lowpan_header));
        lowpanh->nh = iph->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        lowpanh->hlim = iph->ip6_ctlun.ip6_un1.ip6_un1_hlim;
	int ip6_src_len = sizeof(iph->ip6_src);        
	for( i = 0 ; i < ip6_src_len; i++)
        {
		lowpanh->src.slowpan_addr[i] = iph->ip6_src.s6_addr[i];
	}	

        int ip6_dst_len = sizeof(iph->ip6_dst);
        int payload_len = sizeof(lowpanh->payload);

        if(ip6_dst_len == 16)
        {
            for ( i = 8; i < ip6_dst_len; i++)
            {
                lowpanh->dst.slowpan_addr[j] = iph->ip6_dst.s6_addr[i];
                j++;
            }
        }
        for( i = 0; i < payload_len; i++)
        {
            lowpanh->payload.slowpan_payload[i] = ivp6[40+i];
        }
        return reassemble_lowpan(lowpanh);
    }

}



int init_tun()
{
    int tun_fd = 0;
    char tun_name[IFNAMSIZ];

    const char *command;
    command = "ip tuntap add dev tun0 mode tun user root";
    system(command);
    command = "ip link set tun0 up";
    system(command);
    command = "ip -6 addr add BBBB:0000:0000:0000::1/64 dev tun0";
    system(command);

    strcpy(tun_name, "tun0");

    tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface, no Ethernet headers*/
    
    if(tun_fd < 0){
        return -1;
    }

    return tun_fd;
}
