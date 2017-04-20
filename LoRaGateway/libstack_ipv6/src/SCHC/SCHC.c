
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "IPv6.h"
#include "SCHC.h"




char *schc_compression(char *buffer,char *schc_buffer, ip6_header_buffer *ipv6_header_schc)
{
    uint8_t rule_id = 0; 

    int schc_offset = 1; // offset bytes for the return buffer (after rule_id)
    int buffer_offset = 0;

    char link_local_feight_bytes[8] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0};
    char multicast_feight_bytes[8] = {0xff, 0x02, 0, 0, 0, 0, 0, 0};

    // header first 4 bytes

    ipv6_header_schc->version = (buffer[0] >> 4) & 15;
    ipv6_header_schc->tclass = 0; // traffic class will be set in 0
    ipv6_header_schc->flabel = 0; // flow label will be set in 0

    // header next 4 bytes

    ipv6_header_schc->paylength = (buffer[4] << 8) | buffer[5];
    memcpy(&ipv6_header_schc->nheader,&buffer[6],1);
    memcpy(&ipv6_header_schc->hlimit,&buffer[7],1);
    
    // source address and destination address

    memcpy(&ipv6_header_schc->ip6_src,&buffer[8],16);
    memcpy(&ipv6_header_schc->ip6_dst,&buffer[24],16);
    
    buffer_offset += 40;

    //fields
    // *The payload length is used only to get the payload
        // - version:
    if(ipv6_header_schc->version == 6)
    {   
        // - traffic class elided

        // - flow label elided

        // - next header

        if(ipv6_header_schc->nheader == 58)
        {   
            rule_id |= (0 << 6); // ==> 00 ICMP
        }

        // hop limit is omitted

        // - source address
        if(memcmp(ipv6_header_schc->ip6_src.s6_addr,link_local_feight_bytes,8) == 0)
        {

            rule_id |= (0 << 5); 
            memcpy(&schc_buffer[schc_offset], &ipv6_header_schc->ip6_src.s6_addr[8],8);
            
            schc_offset+=8; 
        }
        else
        {
            rule_id |= (1 << 5);
            memcpy(&schc_buffer[schc_offset], &ipv6_header_schc->ip6_src,16);

            schc_offset+=16;
        }

        // - destination address

        if(memcmp(multicast_feight_bytes, ipv6_header_schc->ip6_dst.s6_addr,8) == 0)
        {
            rule_id |= (0 << 4);
            memcpy(&schc_buffer[schc_offset], &ipv6_header_schc->ip6_dst.s6_addr[8],8);

            schc_offset+=8;

        }
        else
        {
            rule_id |= (1 << 4);
            if(memcmp(ipv6_header_schc->ip6_dst.s6_addr,link_local_feight_bytes,8) == 0)
            {
                rule_id |= (0 << 3);
                memcpy(&schc_buffer[schc_offset], &ipv6_header_schc->ip6_dst.s6_addr[8],8);

                schc_offset+=8;
            }
            else
            {
                rule_id |= (1 << 3);
                memcpy(&schc_buffer[schc_offset], &ipv6_header_schc->ip6_dst,16);

                schc_offset+=16;      
            }

        }
        
        schc_buffer[0] = rule_id;
        
        ////////////////////////////////////ICMPv6//////////////////////////////
        if(ipv6_header_schc->nheader == 58)
        {
            uint8_t request = 0x80, reply = 0x81 , rsolicitation = 0x85, radvertisement = 0x86, nsolicitation = 0x87, nadvertisement = 0x88, redirect = 0x89;

            //ICMP TYPE
            if(memcmp(&buffer[buffer_offset], &request ,1) == 0)
            {
                rule_id |= (0 << 0); //==> 000 echo request
            }
            else if(memcmp(&buffer[buffer_offset], &reply ,1) == 0)
            {
                rule_id |= (1 << 0); //==> 001 echo reply
            }
            else if(memcmp(&buffer[buffer_offset], &rsolicitation ,1) == 0)
            {
                rule_id |= (2 << 0); //==> 010 router solicitation
            }
            else if(memcmp(&buffer[buffer_offset], &radvertisement ,1) == 0)
            {
                rule_id |= (3 << 0); //==> 011 router advertisement
            }
            else if(memcmp(&buffer[buffer_offset], &nsolicitation ,1) == 0)
            {
                rule_id |= (4 << 0); //==> 100 neighbor solicitation
            }
            else if(memcmp(&buffer[buffer_offset], &nadvertisement ,1) == 0)
            {
                rule_id |= (5 << 0); //==> 101 neighbor advertisement
            }
            else if(memcmp(&buffer[buffer_offset], &redirect ,1) == 0)
            {
                rule_id |= (6 << 0); //==> 110 redirect
            }

            schc_buffer[0] = rule_id;

            //type
            buffer_offset += 1;

            // code is elided
            buffer_offset += 1;

            // checksum can be compute
            buffer_offset += 2; 

            if(memcmp(&buffer[buffer_offset - 4], &request ,1) == 0 || memcmp(&buffer[buffer_offset - 4], &reply ,1) == 0) // ----------> echo request or echo reply
            {              
                // identifier
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],2);
                schc_offset += 2;
                buffer_offset += 2;

                // - sequece number
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],2);
                schc_offset += 2;
                buffer_offset += 2;
            }
            else if(memcmp(&buffer[buffer_offset - 4], &rsolicitation ,1) == 0)
            {
                //reserved
                buffer_offset += 4;   
            }
            else if(memcmp(&buffer[buffer_offset - 4], &radvertisement ,1) == 0)
            {
                //Cur Hop Limit elided
                buffer_offset += 1;

                //Autoconfig Flags elided
                buffer_offset += 1;

                //Router Lifetime elided
                buffer_offset += 2;

                //Reachable Time elided
                buffer_offset += 4;

                //Retrans Timer elided
                buffer_offset += 4;

                //option
                //-type
                schc_buffer[schc_offset] = buffer[buffer_offset];
                schc_offset += 1;
                buffer_offset += 1;

                //-length elided
                buffer_offset += 1;

                //lynk - layer
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],6);
                schc_offset += 6;
                buffer_offset += 6;

            }
            else if(memcmp(&buffer[buffer_offset - 4], &nsolicitation ,1) == 0)
            {
                //reserved elided
                buffer_offset += 4;

                //target address elided
                buffer_offset += 16;

                //option
                //type
                schc_buffer[schc_offset] = buffer[buffer_offset];
                buffer_offset += 1;
                schc_offset += 1;

                //length elided
                buffer_offset += 1;

                //lynk - layer
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],6);
                buffer_offset += 6;
                schc_offset += 6;

            }
            else if(memcmp(&buffer[buffer_offset], &nadvertisement ,1) == 0)
            {
                //flags elided
                buffer_offset += 4;

                //target address
                buffer_offset += 16;

                //option elided                

            }
            else if(memcmp(&buffer[buffer_offset], &redirect ,1) == 0)
            {
                // reserved
                buffer_offset += 4;

                // targer address
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],16);
                buffer_offset += 16;
                schc_offset += 16;

                // destination address
                buffer_offset += 16;

                //option
                //type
                schc_buffer[schc_offset] = buffer[buffer_offset];
                buffer_offset += 1;
                schc_offset += 1;

                //length elided
                buffer_offset += 1;

                //lynk - layer
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],6);
                buffer_offset += 6;
                schc_offset += 6;
            }
            // - payload
            memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],((ipv6_header_schc->paylength + 40) - buffer_offset));                


            return schc_buffer;
        }   
    }
    
    return NULL;

}


char *icmp_decompression(char *schc_buffer, char *buffer, int schc_offset, int buffer_offset, int packet_size)
{
    uint16_t payload_length = 0;
    uint16_t checksum = 0;

    int icmp = (schc_buffer[0] >> 0) & 7;

    // type
    if(icmp == 0)
    {
        buffer[buffer_offset] = 0x80; // echo request
    }
    else if(icmp == 1)
    {
        buffer[buffer_offset] = 0x81; // echo reply
    }
    else if(icmp == 2)
    {
        buffer[buffer_offset] = 0x85; // router solicitation
    }
    else if(icmp == 3)
    {
        buffer[buffer_offset] = 0x86; // router advertisement
    }
    else if(icmp == 4)
    {
        buffer[buffer_offset] = 0x87; // neighbor solicitation
    }
    else if(icmp == 5)
    {
        buffer[buffer_offset] = 0x88; // neighbor advertisement
    }
    else if(icmp == 6)
    {
        buffer[buffer_offset] = 0x89; // redirect
    }
    buffer_offset += 1;
    
    // code
    buffer[buffer_offset] = 0;
    buffer_offset += 1;

    // checksum will be calculate at the end

    buffer_offset += 2;  
    

    if(icmp == 0 || icmp == 1) // if it is echo request or reply
    {
        // identifier
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],2);
        buffer_offset += 2;
        schc_offset += 2;  

        // sequence number
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],2);
        buffer_offset += 2;
        schc_offset += 2;

        // payload
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset], (packet_size - schc_offset));
        buffer_offset += (packet_size - schc_offset);

    }
    else if(icmp == 2) // if it is router solicitation
    {
        // reseved
        buffer[buffer_offset] = 0;
        buffer[buffer_offset + 1] = 0;
        buffer[buffer_offset + 2] = 0;
        buffer[buffer_offset + 3] = 0;
        buffer_offset += 4;
    }
    
    else if(icmp == 3) // if it is router advertisement
    {
        uint16_t router_lifetime = 64800;
        uint32_t reachable_time = 30000;
        uint32_t retrans_time = 1000;

        //Cur Hop Limit
        buffer[buffer_offset] = 0;
        buffer_offset += 1;

        //Autoconfig Flags
        buffer[buffer_offset] = 0;
        buffer_offset += 1;

        //Router Lifetime
        buffer[buffer_offset] |= (router_lifetime >> 8) & 255;
        buffer[buffer_offset + 1] |= (router_lifetime) & 255;
        buffer_offset += 2;

        //Reachable Time
        buffer[buffer_offset] |= (reachable_time >> 24) & 255;
        buffer[buffer_offset + 1] |= (reachable_time >> 16) & 255;
        buffer[buffer_offset + 2] |= (reachable_time >> 8) & 255;
        buffer[buffer_offset + 3] |= (reachable_time) & 255;
        buffer_offset += 4;

        //Retrans Timer
        buffer[buffer_offset] |= (retrans_time >> 24) & 255;
        buffer[buffer_offset + 1] |= (retrans_time >> 16) & 255;
        buffer[buffer_offset + 2] |= (retrans_time >> 8) & 255;
        buffer[buffer_offset + 3] |= (retrans_time) & 255;
        buffer_offset += 4;

        //option
        //-type
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],1);
        buffer_offset += 1;
        schc_offset += 1;

        //-length
        buffer[buffer_offset] = 1;
        buffer_offset += 1;

        //-link-layer
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],6);
        buffer_offset += 6; 

    }

    else if(icmp == 4) // if it is neighbor solicitation
    {
        int i;

        // reserved
        buffer[buffer_offset] = 0;
        buffer[buffer_offset+1] = 0;
        buffer[buffer_offset+2] = 0;
        buffer[buffer_offset+3] = 0;
        buffer_offset += 4;  

        // target value
        for(i = 0; i < 16; i++)
        {
            buffer[buffer_offset + i] = buffer[24 + i];
        }
        buffer_offset += 16;

        // option 
        //-type
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],1);
        buffer_offset += 1;
        schc_offset += 1;

        //-length
        buffer[buffer_offset] = 1;
        buffer_offset += 1;

        //-link-layer
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],6);
        buffer_offset += 6; 

    }
    else if(icmp == 5) // if it is neighbor advertisement
    {
        int i;

        // flags
        buffer[buffer_offset] = 1;
        buffer[buffer_offset+1] = 1;
        buffer[buffer_offset+2] = 0;
        buffer[buffer_offset+3] = 0;
        buffer_offset += 4;  

        // target value
        for(i = 0; i < 16; i++)
        {
            buffer[buffer_offset + i] = buffer[8 + i];
        }
        buffer_offset += 16;

        // option elided
    }
    else if(icmp == 6) // redirect
    {
        int i;

        // reserved
        buffer[buffer_offset] = 0;
        buffer[buffer_offset+1] = 0;
        buffer[buffer_offset+2] = 0;
        buffer[buffer_offset+3] = 0;
        buffer_offset += 4;  

        // target value
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],16);
        buffer_offset += 16;
        schc_offset += 16;

        // destination address
        for(i = 0; i < 16; i++)
        {
            buffer[buffer_offset + i] = buffer[24 + i];
        }
        buffer_offset += 16;

        // option 
        //-type
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],1);
        buffer_offset += 1;
        schc_offset += 1;

        //-length
        buffer[buffer_offset] = 1;
        buffer_offset += 1;

        //-link-layer
        memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],6);
        buffer_offset += 6; 

        // option elided
    }

    // payload lenngth = (packet_size - schc_offset) + 8
    payload_length = (buffer_offset - 40);
    buffer[4] = (payload_length >> 8) & 255;
    buffer[5] = (payload_length >> 0) & 255;    

    // checksum
    checksum = checksum_icmpv6(buffer,(buffer_offset));

    buffer[42] = (checksum >> 8) & 255;
    buffer[43] = (checksum >> 0) & 255;
    
    return buffer;
}

char *schc_decompression(char *schc_buffer, char *buffer, int packet_size)
{
    int schc_offset = 1;
    int buffer_offset = 0; 

    char link_local_feight_bytes[8] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0};
    char multicast_feight_bytes[8] = {0xff, 0x02, 0, 0, 0, 0, 0, 0};

    int schc_header = (schc_buffer[0] >> 6) & 3;
    int schc_src = (schc_buffer[0] >> 5) & 1;
    int multicast = (schc_buffer[0] >> 4) & 1;
    int schc_dst = (schc_buffer[0] >> 3) & 1;

    //4 bytes who were elided 
    buffer[0] = 0x60;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;

    buffer_offset += 4;

    // payload length will be calculate at the end

    buffer_offset += 2;

    if(schc_header == 0)
    {
        buffer[buffer_offset] = 0x3a;
        buffer_offset += 1;
    }
    
    // hop limit 
    buffer[buffer_offset] = 1;
    buffer_offset += 1;

    if(schc_src == 0) // => source address link local
    {
        memcpy(&buffer[buffer_offset], &link_local_feight_bytes[0], 8);
        buffer_offset += 8;
        
        memcpy(&buffer[buffer_offset], &schc_buffer[schc_offset], 8);
        buffer_offset += 8;
        schc_offset += 8;
    }
    else // => source address global
    {   
        memcpy(&buffer[buffer_offset], &schc_buffer[schc_offset], 16);
        buffer_offset += 16;
        schc_offset += 16;   
    }

    if(multicast == 0) // => it is multicast
    {
        memcpy(&buffer[buffer_offset], &multicast_feight_bytes[0], 8);
        buffer_offset += 8;
        
        memcpy(&buffer[buffer_offset], &schc_buffer[schc_offset], 8);
        buffer_offset += 8;
        schc_offset += 8;
    }
    else // => it is not multicast
    {
        if(schc_dst == 0) // destination address is link local
        {
            memcpy(&buffer[buffer_offset], &link_local_feight_bytes[0], 8);
            buffer_offset += 8;
            
            memcpy(&buffer[buffer_offset], &schc_buffer[schc_offset], 8);
            buffer_offset += 8;
            schc_offset += 8;
        }
        else  // destination address is global
        {   
            memcpy(&buffer[buffer_offset], &schc_buffer[schc_offset], 16);
            buffer_offset += 16;
            schc_offset += 16;   
        }   
    }
    
    if(schc_header == 0)
    {
        icmp_decompression(schc_buffer, buffer, schc_offset, buffer_offset, packet_size);
    }
    return buffer;
}

