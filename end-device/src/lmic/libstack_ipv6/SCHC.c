
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "LoWPAN_IPHC.h"
#include "IPv6.h"

lowpan_header *lowpanSC = NULL;
lowpan_header *SCHC_SAVED = NULL;
RoHC_base *RoV = NULL;

uint8_t check_rule(lowpan_header *lowpan_HC,int src_length,int dst_length)
{
    uint8_t rule;
    int valid_rule = 0;
    uint8_t src[16] = {0};
    uint8_t dst[16] = {0};

   

    /* ///////////////////////////////////////////////// start checking Rule 2//////////////////////////////////////////////*/

    if(src_length == 8 && dst_length == 8)
    {
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 3;  dst[7] = 1;

        if(lowpan_HC->tf == 0)
        {
           valid_rule ++;
        }

        if(lowpan_HC->nh == 58)
        {
            valid_rule++;
        }

        if(lowpan_HC->hlim == 64)
        {
            valid_rule++;
        }

        if(lowpan_HC->cid == 0)
        {
            valid_rule++;
        }

        if(memcmp(&lowpan_HC->src.slowpan_addr[0],&src[0],8) == 0)
        {
            valid_rule++;
            
        }
        if(memcmp(lowpan_HC->dst.slowpan_addr,dst,8) == 0)
        {
            valid_rule++;
        }
        if(valid_rule == 6)
        {
            rule = 2;
            return rule;
        }

    }

     /* ///////////////////////////////////////////////// end checking Rule 2////////////////////////////////////////////// */

    valid_rule = 0;

    /* ///////////////////////////////////////////////// start checking Rule 6//////////////////////////////////////////////*/

    if(src_length == 8 && dst_length == 8)
    {
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 3;  dst[7] = 1;

        if(lowpan_HC->tf == 0)
        {
           valid_rule ++;
        }

        if(lowpan_HC->nh == 58)
        {
            valid_rule++;
        }

        if(lowpan_HC->hlim == 1)
        {
            valid_rule++;
        }

        if(lowpan_HC->cid == 0)
        {
            valid_rule++;
        }

        if(memcmp(&lowpan_HC->src.slowpan_addr[0],&src[0],8) == 0)
        {
            valid_rule++;
            
        }
        if(memcmp(lowpan_HC->dst.slowpan_addr,dst,8) == 0)
        {
            valid_rule++;
        }
        if(valid_rule == 6)
        {
            rule = 7;
            return rule;
        }

    }

     /* ///////////////////////////////////////////////// end checking Rule 6////////////////////////////////////////////// */
    return 0;
}

char *SCHC_TX(char *LoWPAN_HC, int payload_length)
{


    if(lowpanSC == NULL)
    {
        lowpanSC = (lowpan_header *)malloc(sizeof(lowpan_header));
    }

    if(RoV == NULL)
    {
        RoV = (RoHC_base *)malloc(sizeof(RoHC_base));
    }   

    char base[2];
    int j = 0,src_length = 0,dst_length = 0;
    uint8_t rule;
    uint16_t gen_checksum;

    memcpy(base, LoWPAN_HC, 2);
    j += 2;

    RoV->tfn= (base[0] >> 3) & 3;
    RoV->nh = (base[0] >> 2) & 1; 
    RoV->hlim = base[0] & 3; 
    RoV->cid = (base[1] >> 7) & 1;
    RoV->sac = (base[1] >> 6) & 1;
    RoV->sam = (base[1] >> 4) & 3;
    RoV->m = (base[1] >> 3) & 1;
    RoV->dac = (base[1] >> 2) & 1;
    RoV->dam = base[1] & 3;

    /*/////////////////////////////////////// 6LoWPAN tf /////////////////////////////////////////////*/
    if(RoV->tfn == 3)
    {
        lowpanSC->tf = 0;
    }

    /*/////////////////////////////////////// 6LoWPAN nh /////////////////////////////////////////////*/
    if(RoV->nh == 0)
    {
        memcpy(&lowpanSC->nh, &LoWPAN_HC[2], 1);
        j+=1;
    }

    /*/////////////////////////////////////// 6LoWPAN hlim /////////////////////////////////////////////*/
    if(RoV->hlim == 2)
    {
       lowpanSC->hlim = 64;
    }
    else if (RoV->hlim == 1)
    {
        lowpanSC->hlim = 1;
    }

    /*/////////////////////////////////////// 6LoWPAN cid /////////////////////////////////////////////*/
    if(RoV->cid == 1)
    {
       lowpanSC->cid = 8;
    }
    else
    {
        lowpanSC->cid = 0;
    }

    /*/////////////////////////////////////// 6LoWPAN sam - sac /////////////////////////////////////////////*/
    if(RoV->sam == 0)
    {
        if(RoV->sac == 0)
        {
            memcpy(lowpanSC->src.slowpan_addr, &LoWPAN_HC[j], 16);
            src_length = 16;
            j += 16;
        }
    }
    else if(RoV->sam == 1)
    {
        if(RoV->sac == 0)
        {
            memcpy(lowpanSC->src.slowpan_addr, &LoWPAN_HC[j], 8);
            src_length = 8;
            j += 8;
        }
    }


    /*/////////////////////////////////////// 6LoWPAN m - dac - dam /////////////////////////////////////////////*/
    if(RoV->m == 0 && RoV->dac == 0)
    {
        if(RoV->dam == 1)
        {
            memcpy(lowpanSC->dst.slowpan_addr, &LoWPAN_HC[j], 8);
            dst_length = 8; 
            j += 8;
        }
    }
    else if(RoV->m == 1 && RoV->dac == 0)
    {
        if(RoV->dam == 3)
        {
            memcpy(lowpanSC->dst.slowpan_addr, &LoWPAN_HC[j], 1);
            dst_length = 1; 
            j += 1;
        }
    }

    rule = check_rule(lowpanSC, src_length, dst_length);
    
    if(rule != 0)
    {
        memcpy(lowpanSC->payload.slowpan_payload, &LoWPAN_HC[j], payload_length);
        LoWPAN_HC[0] = rule;
        LoWPAN_HC[1] = 0;
        LoWPAN_HC[2] = 0;

        memcpy(&LoWPAN_HC[3], lowpanSC->payload.slowpan_payload, payload_length);

        gen_checksum = checksum_schc(LoWPAN_HC, j);
        LoWPAN_HC[1] = (gen_checksum >> 8) & 255;
        LoWPAN_HC[2] = (gen_checksum) & 255;

        return LoWPAN_HC;
    }
    return NULL;
}



char *SCHC_RX(char *LoWPAN_HC, int payload_length)
{
    int rule;
    int j = 0;
    char payload[250];
    uint8_t nh = 0, hlim = 0;
    uint8_t src_aux[16]={0}, dst_aux[16]={0};

    if(RoV == NULL)
    {
        RoV = (RoHC_base *)malloc(sizeof(RoHC_base));
    }
    
    rule = LoWPAN_HC[0];
    

/* ///////////////////////////////////////////////// start saved Rule 3//////////////////////////////////////////////*/    
    if(rule == 1)
    {

        src_aux[0] = 0;  dst_aux[0] = 0; 
        src_aux[1] = 0;  dst_aux[1] = 0;
        src_aux[2] = 0;  dst_aux[2] = 0;
        src_aux[3] = 0;  dst_aux[3] = 0;
        src_aux[4] = 0;  dst_aux[4] = 0;
        src_aux[5] = 0;  dst_aux[5] = 0;
        src_aux[6] = 0;  dst_aux[6] = 0;
        src_aux[7] = 1;  dst_aux[7] = 3;

        RoV->tfn  = 3;
        RoV->nh   = 0; 
        RoV->hlim = 2; 
        RoV->cid  = 0;
        RoV->sac  = 0;
        RoV->sam  = 1;
        RoV->m    = 0;
        RoV->dac  = 0;
        RoV->dam  = 1;

        nh   = 58;
        hlim = 64;
    }
/* ///////////////////////////////////////////////// end Rule 3//////////////////////////////////////////////*/
/* ///////////////////////////////////////////////// start saved Rule 5//////////////////////////////////////////////*/    
    if(rule == 5)
    {

        src_aux[0] = 0;  dst_aux[0] = 1; 
        src_aux[1] = 0;  
        src_aux[2] = 0;  
        src_aux[3] = 0;  
        src_aux[4] = 0;  
        src_aux[5] = 0;  
        src_aux[6] = 0;  
        src_aux[7] = 1;  

        RoV->tfn  = 3;
        RoV->nh   = 0; 
        RoV->hlim = 1; 
        RoV->cid  = 0;
        RoV->sac  = 0;
        RoV->sam  = 1;
        RoV->m    = 1;
        RoV->dac  = 0;
        RoV->dam  = 3;

        nh   = 58;
        hlim = 1;
    }
/* ///////////////////////////////////////////////// end Rule 5//////////////////////////////////////////////*/     

    memcpy(payload,&LoWPAN_HC[3], payload_length);
 
    LoWPAN_HC[0] = 0;
    LoWPAN_HC[1] = 0;

    LoWPAN_HC[0] |= (RoV->hlim  << 0);
    LoWPAN_HC[0] |= (RoV->nh    << 2);
    LoWPAN_HC[0] |= (RoV->tfn   << 3);
    LoWPAN_HC[0] |= (1          << 5);
    LoWPAN_HC[0] |= (1          << 6);

    LoWPAN_HC[1] |= (RoV->dam   << 0); 
    LoWPAN_HC[1] |= (RoV->dac   << 2);
    LoWPAN_HC[1] |= (RoV->m     << 3);
    LoWPAN_HC[1] |= (RoV->sam   << 4);
    LoWPAN_HC[1] |= (RoV->sac   << 6);
    LoWPAN_HC[1] |= (RoV->cid   << 7);

    /*/////////////////////////////////////// 6LoWPAN tf /////////////////////////////////////////////*/
    
    if(RoV->tfn == 3)
    {
        j = 2;
    }

    /*/////////////////////////////////////// 6LoWPAN nh /////////////////////////////////////////////*/
    
    if(RoV->nh == 0)
    {
        LoWPAN_HC[j] = nh ;
        j++;
    }

    /*/////////////////////////////////////// 6LoWPAN hlim /////////////////////////////////////////////*/
    
    if(RoV->hlim == 0)
    {
        LoWPAN_HC[j] = hlim;
        j++;
    }

    /*/////////////////////////////////////// 6LoWPAN cid /////////////////////////////////////////////*/
    
    if(RoV->cid == 1)
    {
       j += 8;
    }

    /*/////////////////////////////////////// 6LoWPAN sam - sac /////////////////////////////////////////////*/

    if(RoV->sam == 0)
    {
        if(RoV->sac == 0)
        {
            memcpy(&LoWPAN_HC[j], src_aux, 16);
            j += 16;
        }
    }
    else if(RoV->sam == 1)
    {
        if(RoV->sac == 0)
        {
            memcpy(&LoWPAN_HC[j], src_aux, 8);
            j += 8;
        }
    }

    /*/////////////////////////////////////// 6LoWPAN m - dac - dam /////////////////////////////////////////////*/

    if(RoV->m == 0 && RoV->dac == 0)
    {
        if(RoV->dam == 1)
        {
            memcpy(&LoWPAN_HC[j], dst_aux, 8);
            j += 8;
        }
    }
    else if(RoV->m == 1 && RoV->dac == 0)
    {
        if(RoV->dam == 3)
        {
            memcpy(&LoWPAN_HC[j], dst_aux, 1);
            j += 1;
        }
    }

    memcpy(&LoWPAN_HC[j], payload, payload_length);

    return LoWPAN_HC;
}