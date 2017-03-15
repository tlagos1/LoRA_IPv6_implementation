
/*Edited by Tomás Lagos*/

#define _BSD_SOURCE

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "LoWPAN_IPHC.h"
#include "IPv6.h"

RoHC_base *HRbase = NULL;
ip6_buffer *decode = NULL;
ip6_buffer *auxRX = NULL;
char *buffer = NULL;

char *reassemble_lowpan(lowpan_header *lowpan, int payload_lenght, int src_lenght, int dst_lenght)
{
    uint8_t returnValueBase;
    int j, packet_size = 0;

    RoHC_base *aux = (RoHC_base *)malloc(sizeof(RoHC_base));
    if(buffer == NULL)
    {
        buffer = malloc(sizeof(char *));
    }

    /*/////////////////////////////////////// 6LoWPAN tf /////////////////////////////////////////////*/
    
    aux->tfn = IPHC_TF_ELIDED;

    /*/////////////////////////////////////// 6LoWPAN nh /////////////////////////////////////////////*/

    aux->nh = IPHC_NH_INLINE;

    /*/////////////////////////////////////// 6LoWPAN hlim /////////////////////////////////////////////*/

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

    /*/////////////////////////////////////// 6LoWPAN cid /////////////////////////////////////////////*/

    aux->cid = IPHC_CID_NO;

    /*/////////////////////////////////////// 6LoWPAN sac /////////////////////////////////////////////*/

    aux->sac = IPHC_SAC_STATELESS;

    /*/////////////////////////////////////// 6LoWPAN sam /////////////////////////////////////////////*/

    if(src_lenght == 16)
    {   
        aux->sam = IPHC_SAM_128B;
    }
    else if (src_lenght == 8)
    {
        aux->sam = IPHC_SAM_64B;
    }
    else if (src_lenght == 2)
    {
        aux->sam = IPHC_SAM_16B;
    }
    else if (src_lenght == 0)
    {
        aux->sam = IPHC_SAM_ELIDED;
    }    

    /*/////////////////////////////////////// 6LoWPAN m /////////////////////////////////////////////*/

    if(dst_lenght == 1)
    {
        aux->m = IPHC_M_YES; 
    }
    else
    {
       aux->m = IPHC_M_NO; 
    }
    
    /*/////////////////////////////////////// 6LoWPAN dac /////////////////////////////////////////////*/
    
    aux->dac = IPHC_DAC_STATELESS;

    /*/////////////////////////////////////// 6LoWPAN dam /////////////////////////////////////////////*/

    if(aux->m == 0 && aux->dac == 0)
    {
        if(dst_lenght == 16)
        {
            aux->dam = IPHC_DAM_128B;
        }
        else if (dst_lenght == 8)
        {
            aux->dam = IPHC_DAM_64B;
        }
        else if (dst_lenght == 2)
        {
            aux->dam = IPHC_DAM_16B;
        }
        else if (dst_lenght == 0)
        {
            aux->dam = IPHC_DAM_ELIDED;
        }    
    }
    else if(aux->m == 1 && aux->dac == 0)
    {
        aux->dam = 3;
    }
    
    /*/////////////////////////////////////// 6LoWPAN first 2 bytes buffer /////////////////////////////////////////////*/

    returnValueBase = 0;
    returnValueBase |= (aux->hlim         << 0);
    returnValueBase |= (aux->nh           << 2);
    returnValueBase |= (aux->tfn          << 3);
    returnValueBase |= (1                 << 5);
    returnValueBase |= (1                 << 6);

    buffer[0] = returnValueBase;
    packet_size++;

    returnValueBase = 0;
    returnValueBase |= (aux->dam          << 0); 
    returnValueBase |= (aux->dac          << 2);
    returnValueBase |= (aux->m            << 3);
    returnValueBase |= (aux->sam          << 4);
    returnValueBase |= (aux->sac          << 6);
    returnValueBase |= (aux->cid          << 7);
    
    buffer[1] = returnValueBase;
    packet_size++;

    buffer[2] = lowpan->nh;
    packet_size++;

    /*/////////////////////////////////////// 6LoWPAN leftover bytes buffer /////////////////////////////////////////////*/

    j = 3;

    memcpy(&buffer[j],lowpan->src.slowpan_addr,src_lenght);
    j += src_lenght;
    packet_size += src_lenght;
    

    memcpy(&buffer[j],lowpan->dst.slowpan_addr,dst_lenght);
    j += dst_lenght;
    packet_size += dst_lenght;


    memcpy(&buffer[j],lowpan->payload.slowpan_payload,payload_lenght);
    j += payload_lenght;
    packet_size += payload_lenght;
    

    if(packet_size > 64)
    {
        free(aux);
        return NULL;
    }
    else
    {
        free(aux);
        return buffer;    
    }  
}

char *IPv6ToMesh(char *buffer, int payload_length, lowpan_header *lowpanh)
{

    ip6_buffer *iph = (ip6_buffer *)buffer;

    lowpanh->nh = iph->ip6_ctlun.ip6_un1.ip6_un1_nxt;
    lowpanh->hlim = iph->ip6_ctlun.ip6_un1.ip6_un1_hlim;

    int ip6_src_len = 0,ip6_dst_len = 0;

    /*/////////////////////////////////////// source address /////////////////////////////////////////////*/
    
    if(iph->ip6_src.s6_addr[0] == 0xfe && iph->ip6_src.s6_addr[1] == 0x80)
    {
        memcpy(lowpanh->src.slowpan_addr,&iph->ip6_src.s6_addr[8],8);
        ip6_src_len = 8;
    }
    else
    {
        memcpy(lowpanh->src.slowpan_addr,&iph->ip6_src.s6_addr[0],16);
        ip6_src_len = 16;
    }


    /*/////////////////////////////////////// destination address /////////////////////////////////////////////*/

    if(iph->ip6_dst.s6_addr[0] == 0xfe && iph->ip6_dst.s6_addr[1] == 0x80)
    {
        memcpy(lowpanh->dst.slowpan_addr,&iph->ip6_dst.s6_addr[8],8);
        ip6_dst_len = 8;
    }

    else if(iph->ip6_dst.s6_addr[0] == 0xff && iph->ip6_dst.s6_addr[1] == 0x02 && iph->ip6_dst.s6_addr[15] == 0x01)
    {
        memcpy(lowpanh->dst.slowpan_addr,&iph->ip6_dst.s6_addr[15],1);
        ip6_dst_len = 1;
    }
    else
    {
        memcpy(lowpanh->dst.slowpan_addr,&iph->ip6_dst.s6_addr[0],16);
        ip6_dst_len = 16;
    }

    /*/////////////////////////////////////// payload /////////////////////////////////////////////*/

    memcpy(lowpanh->payload.slowpan_payload,&buffer[40],payload_length);

    
    buffer = reassemble_lowpan(lowpanh, payload_length, ip6_src_len, ip6_dst_len);

    if( buffer != NULL)
    {
        return buffer;    
    }
    else
    {
        return NULL;
    }  
    
}

ip6_buffer *DecodeIPv6(char *buffer,int payload_length, uint8_t *IPv6_addr)
{
    int i,j,k;
    
    if(!decode)
    {
        decode = (ip6_buffer *) malloc(sizeof(ip6_buffer));
        HRbase = (RoHC_base *)malloc(8*sizeof(RoHC_base));
    }
    HRbase->tfn= (buffer[0] >> 3) & 3;
    HRbase->nh = (buffer[0] >> 2) & 1; 
    HRbase->hlim = buffer[0] & 3; 
    HRbase->cid = (buffer[1] >> 7) & 1;
    HRbase->sac = (buffer[1] >> 6) & 1;
    HRbase->sam = (buffer[1] >> 4) & 3;
    HRbase->m = (buffer[1] >> 3) & 1;
    HRbase->dac = (buffer[1] >> 2) & 1;
    HRbase->dam = buffer[1] & 3;

    decode->ip6_ctlun.ip6_un1.ip6_un1_flow = 0x60;

    /*/////////////////////////////////////// 6LoWPAN tf /////////////////////////////////////////////*/
    
    if(HRbase->tfn == 3)
    {
        j = 2;
    }

    /*/////////////////////////////////////// 6LoWPAN nh /////////////////////////////////////////////*/

    if(HRbase->nh == 0)
    {
        decode->ip6_ctlun.ip6_un1.ip6_un1_nxt =buffer[2] ;
        j++;
    }

    /*/////////////////////////////////////// 6LoWPAN hlim /////////////////////////////////////////////*/

    if(HRbase->hlim == 2)
    {
        decode->ip6_ctlun.ip6_un1.ip6_un1_hlim = 64;
    }
    else if(HRbase->hlim == 1)
    {
        decode->ip6_ctlun.ip6_un1.ip6_un1_hlim = 1;
    }

    /*/////////////////////////////////////// 6LoWPAN cid /////////////////////////////////////////////*/
    
    if(HRbase->cid == 1)
    {
       j += 8;
    }

    /*/////////////////////////////////////// 6LoWPAN sam - sac /////////////////////////////////////////////*/
    
    if(HRbase->sam == 0)
    {
        if(HRbase->sac == 0)
        {
            memcpy(decode->ip6_src.s6_addr,&buffer[j],16);
            j += 16;
        }
    }
    else if(HRbase->sam == 1)
    {
        if(HRbase->sac == 0)
        {
            decode->ip6_src.s6_addr[0] = 0xfe;
            decode->ip6_src.s6_addr[1] = 0x80;
            for(i = 2; i < 8; i++)
            {
                decode->ip6_src.s6_addr[i] = 0;
            }
            memcpy(&decode->ip6_src.s6_addr[8],&buffer[j],8);
            j += 8;
        }
    }

    /*/////////////////////////////////////// 6LoWPAN m - dac - dam /////////////////////////////////////////////*/

    if(HRbase->m == 0 && HRbase->dac == 0)
    {
        if(HRbase->dam == 1)
        {
            decode->ip6_dst.s6_addr[0] = 0xfe;
            decode->ip6_dst.s6_addr[1] = 0x80;

            for(i = 2; i < 8; i++)
            {
                decode->ip6_dst.s6_addr[i] = 0;
            }
            memcpy(&decode->ip6_dst.s6_addr[8],&buffer[j],8);
            j += 8;  
        }
    }
    else if(HRbase->m == 1 && HRbase->dac == 0)
    {
        if(HRbase->dam == 3)
        {
            memcpy(&decode->ip6_dst.s6_addr,IPv6_addr,16);
            j += 1;  
        }
    }


    /*/////////////////////////////////////// 6LoWPAN payload /////////////////////////////////////////////*/

    k=j;

    decode->ip6_ctlun.ip6_un1.ip6_un1_plen = 0 ;
    decode->ip6_ctlun.ip6_un1.ip6_un1_plen |= payload_length << 8 ;

    
    memcpy(decode->payload.s6_payload,&buffer[k],(decode->ip6_ctlun.ip6_un1.ip6_un1_plen >> 8));
    
    return decode;
}


char *IPv6Rx(char *buffer, int payload_length, int tun_fd, ip6_buffer *dec,uint8_t *IPv6_addr)
{
    int i,j = 0; 
    int IP = 1;
    uint16_t gen_checksum = 0;
    uint16_t payload_len;
    char *ptr;


    if(!auxRX)
    {
        auxRX = (ip6_buffer *)malloc(sizeof(ip6_buffer));
    }


    ptr = &buffer[0];
    

    dec = DecodeIPv6(buffer, payload_length, IPv6_addr);

   
    for(i = 0; i < 16; i++)
    {
        if(IPv6_addr[i] != dec->ip6_dst.s6_addr[i])
        {
            IP = -1;
        }
    }
    
    if(IP == 1)
    {
        if(dec->ip6_ctlun.ip6_un1.ip6_un1_nxt == 58)
        {
            if(dec->payload.s6_payload[0] == 0x80)
            {
                dec->payload.s6_payload[0] = 0x81;
                auxRX->ip6_src = dec->ip6_src;
                dec->ip6_src = dec->ip6_dst;
                dec->ip6_dst = auxRX->ip6_src;
                
                
                memcpy(&ptr[0], &dec->ip6_ctlun.ip6_un1.ip6_un1_flow, sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_flow));
                ptr += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_flow);  
                j += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_flow); 

                payload_len = dec->ip6_ctlun.ip6_un1.ip6_un1_plen >> 8 & 255;
                memcpy(ptr, &dec->ip6_ctlun.ip6_un1.ip6_un1_plen, sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_plen));
                ptr += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_plen);  
                j += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_plen);

                memcpy(ptr, &dec->ip6_ctlun.ip6_un1.ip6_un1_nxt, sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_nxt));
                ptr += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_nxt);  
                j += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_nxt);

                memcpy(ptr, &dec->ip6_ctlun.ip6_un1.ip6_un1_hlim, sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_hlim));
                ptr += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_hlim);  
                j += sizeof (dec->ip6_ctlun.ip6_un1.ip6_un1_hlim);  
                        
                memcpy(ptr, &dec->ip6_src.s6_addr, sizeof (dec->ip6_src.s6_addr));
                ptr += sizeof (dec->ip6_src.s6_addr);
                j += sizeof (dec->ip6_src.s6_addr);   

                memcpy(ptr, &dec->ip6_dst.s6_addr, sizeof (dec->ip6_dst.s6_addr));
                ptr += sizeof (dec->ip6_dst.s6_addr);
                j += sizeof (dec->ip6_dst.s6_addr);  

                memcpy(ptr, &dec->payload.s6_payload, payload_len);
                ptr += payload_len;
                j += payload_len; 

                gen_checksum = checksum_icmpv6(buffer,(j));
                buffer[42] = (gen_checksum >> 8) & 255;
                buffer[43] = (gen_checksum) & 255;
                
                return buffer;

            }
            else
            {
                memcpy(&ptr[0], &decode->ip6_ctlun.ip6_un1.ip6_un1_flow, sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_flow));
                ptr += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_flow);  
                j += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_flow);  

                payload_len = decode->ip6_ctlun.ip6_un1.ip6_un1_plen >> 8 & 255;
                memcpy(ptr, &decode->ip6_ctlun.ip6_un1.ip6_un1_plen, sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_plen));
                ptr += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_plen);  
                j += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_plen);

                memcpy(ptr, &decode->ip6_ctlun.ip6_un1.ip6_un1_nxt, sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_nxt));
                ptr += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_nxt);  
                j += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_nxt);

                memcpy(ptr, &decode->ip6_ctlun.ip6_un1.ip6_un1_hlim, sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_hlim));
                ptr += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_hlim);  
                j += sizeof (decode->ip6_ctlun.ip6_un1.ip6_un1_hlim);

                memcpy(ptr, &decode->ip6_src.s6_addr, sizeof (decode->ip6_src.s6_addr));
                ptr += sizeof (decode->ip6_src.s6_addr);
                j += sizeof (decode->ip6_src.s6_addr);    

                memcpy(ptr, &decode->ip6_dst.s6_addr, payload_len);
                ptr += sizeof (decode->ip6_dst.s6_addr);
                j += sizeof (decode->ip6_dst.s6_addr);

                memcpy(ptr, &decode->payload.s6_payload, sizeof(decode->payload.s6_payload));
                ptr += payload_len;
                j += payload_len; 

                //write(tun_fd, buffer, j );
            }    
        }
    }
    
    return NULL;
}
