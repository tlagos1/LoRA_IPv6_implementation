
/*Edited by Tomás Lagos*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "IPv6.h"
#include "SCHC.h"




SCHC_data *schc_compression(char *buffer, char *schc_buffer, SCHC_data *SCHC)
{

    uint8_t rule_id = 0; 
    SCHC->length = 0;

    int ix;

    int schc_offset = 0; // offset bytes for the return buffer (after rule_id)
    int buffer_offset = 0;

    char src_aux[16];
    char dst_aux[16];

    char link_local_feight_bytes[8] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0};
    char multicast_feight_bytes[8] = {0xff, 0x02, 0, 0, 0, 0, 0, 0};


    // header first 4 bytes

    uint8_t version = (buffer[0] >> 4) & 15;

    // traffic class will be set in 0
    // flow label will be set in 0

    // header next 4 bytes

    uint8_t paylength = (buffer[4] << 8) | buffer[5];    
    uint8_t nheader = buffer[6];

    // hop limit elided
        
    // source address and destination address

    for(ix = 0; ix < 16; ix++)
    {
        src_aux[ix] = buffer[8+ix];
    }
    for(ix = 0; ix < 16; ix++)
    {
        dst_aux[ix] = buffer[24+ix];
    }

    buffer_offset += 40;

    //fields
    // *The payload length is used only to get the payload
        // - version:
    if(version == 6)
    {   
        // - traffic class elided

        // - flow label elided

        // - next header

        if(nheader == 58)
        {   
            rule_id |= (0 << 7); // ==> 0 ICMP
        }
        else
        {
            rule_id |= (1 << 7); // ==> 1 UDP
        }
        // hop limit is omitted

        // - source address
        if(memcmp(src_aux, link_local_feight_bytes, 8) == 0)
        {
            rule_id |= (0 << 6); 
        }
        else
        {
            rule_id |= (1 << 6);
        }

        
        ////////////////////////////////////ICMPv6//////////////////////////////
        if(nheader == 58)
        {
            uint8_t request = 0x80, reply = 0x81 , rsolicitation = 0x85, radvertisement = 0x86, nsolicitation = 0x87, nadvertisement = 0x88, redirect = 0x89;
            //ICMP TYPE
            if(memcmp(&buffer[buffer_offset], &request ,1) == 0)
            {
                rule_id |= (0 << 2); //==> 000 echo request
            }
            else if(memcmp(&buffer[buffer_offset], &reply ,1) == 0)
            {
                rule_id |= (1 << 2); //==> 001 echo reply
            }
            else if(memcmp(&buffer[buffer_offset], &rsolicitation ,1) == 0)
            {
                rule_id |= (2 << 2); //==> 010 router solicitation
            }
            else if(memcmp(&buffer[buffer_offset], &radvertisement ,1) == 0)
            {
                rule_id |= (3 << 2); //==> 011 router advertisement
            }
            else if(memcmp(&buffer[buffer_offset], &nsolicitation ,1) == 0)
            {
                rule_id |= (4 << 2); //==> 100 neighbor solicitation
            }
            else if(memcmp(&buffer[buffer_offset], &nadvertisement ,1) == 0)
            {
                rule_id |= (5 << 2); //==> 101 neighbor advertisement
            }
            else if(memcmp(&buffer[buffer_offset], &redirect ,1) == 0)
            {
                rule_id |= (6 << 2); //==> 110 redirect
            }

            schc_buffer[schc_offset] = rule_id;
            schc_offset += 1;
            SCHC->length += 1;

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
                SCHC->length += 2;

                // - sequece number
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],2);
                schc_offset += 2;
                buffer_offset += 2;
                SCHC->length += 2;

                //payload
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset], paylength - (buffer_offset - 40));
                SCHC->length += (paylength - (buffer_offset - 40));
            }
            else if(memcmp(&buffer[buffer_offset - 4], &rsolicitation ,1) == 0)
            {
                // send src addr
                for(ix = 0; ix < 8; ix++)
                {
                    schc_buffer[schc_offset + ix] = src_aux[ix + 8];
                }
                schc_offset += 8;
                SCHC->length += 8;

            }
            else if(memcmp(&buffer[buffer_offset - 4], &radvertisement ,1) == 0)
            {

                if(memcmp(src_aux, &link_local_feight_bytes[0], 8) == 0)
                {
                    memcpy(&schc_buffer[schc_offset], &src_aux[8],8);
                    schc_offset += 8;
                    SCHC->length += 8;
                }
                else
                {
                    memcpy(&schc_buffer[schc_offset], &src_aux[0],16);
                    schc_offset += 16;
                    SCHC->length += 16;
                }

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

            }
            else if(memcmp(&buffer[buffer_offset - 4], &nsolicitation ,1) == 0)   // --->  Neighbor Solicitation
            {

                // send src addr
                if(memcmp(src_aux, &link_local_feight_bytes[0], 8) == 0)
                {
                    memcpy(&schc_buffer[schc_offset], &src_aux[8],8);
                    schc_offset += 8;
                    SCHC->length += 8;
                }
                else
                {
                    memcpy(&schc_buffer[schc_offset], &src_aux[0],16);
                    schc_offset += 16;
                    SCHC->length += 16;
                }
                //reserved elided
                buffer_offset += 4;

                //target address
                if(memcmp(&buffer[buffer_offset], &link_local_feight_bytes[0], 8) == 0)
                {
                    schc_buffer[0] |= 0 << 1;
                    memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset+8], 8);
                    schc_offset += 8;
                    SCHC->length += 8;
                }
                else
                {
                    schc_buffer[0] |= 1 << 1;
                    memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset], 16);
                    schc_offset += 16;
                    SCHC->length += 16;
                }
            }
            else if(memcmp(&buffer[buffer_offset - 4], &nadvertisement ,1) == 0)   // --->  Neighbor Advertisement
            {
                //flags elided
                buffer_offset += 4;
                //target address
                if(memcmp(&buffer[buffer_offset], &link_local_feight_bytes[0], 8) == 0)
                {
                    schc_buffer[0] |= 0 << 1;
                    memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset + 8], 8);
                    schc_offset += 8;
                    SCHC->length += 8;
                }
                else
                {
                    schc_buffer[0] |= 1 << 1;
                    memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset], 16);
                    schc_offset += 16;
                    SCHC->length += 16;
                }
            }
            else if(memcmp(&buffer[buffer_offset], &redirect ,1) == 0)
            {
                // reserved
                buffer_offset += 4;

                // targer address
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],16);
                buffer_offset += 16;
                schc_offset += 16;
                SCHC->length += 16;

                // destination address
                buffer_offset += 16;

                //option
                //type
                schc_buffer[schc_offset] = buffer[buffer_offset];
                buffer_offset += 1;
                schc_offset += 1;
                SCHC->length += 1;

                //length elided
                buffer_offset += 1;

                //lynk - layer
                memcpy(&schc_buffer[schc_offset], &buffer[buffer_offset],6);
                buffer_offset += 6;
                schc_offset += 6;
                SCHC->length += 6;
            }
 
            for (ix = 0; ix < SCHC->length; ix++)
            {
                SCHC->buffer[ix] = schc_buffer[ix];
            }
            return SCHC;
        }   
    }
    
    return NULL;

}


char *icmp_decompression(char *schc_buffer, char *buffer, int schc_offset, int buffer_offset, int packet_size)
{
    uint16_t payload_length = 0;
    uint16_t checksum = 0;

    int icmp = (schc_buffer[0] >> 0) & 15;

    // type
    if(icmp == 0)
    {
        buffer[buffer_offset] = 0x80; // echo request
    }
    else if(icmp == 2)
    {
        buffer[buffer_offset] = 0x81; // echo reply
    }
    else if(icmp == 4)
    {
        buffer[buffer_offset] = 0x85; // router solicitation
    }
    else if(icmp == 6)
    {
        buffer[buffer_offset] = 0x86; // router advertisement
    }
    else if(icmp == 8 || icmp == 9)
    {
        buffer[buffer_offset] = 0x87; // neighbor solicitation
    }
    else if(icmp == 10 || icmp == 11)
    {
        buffer[buffer_offset] = 0x88; // neighbor advertisement
    }
    else if(icmp == 12)
    {
        buffer[buffer_offset] = 0x89; // redirect
    }
    buffer_offset += 1;
    
    // code
    buffer[buffer_offset] = 0;
    buffer_offset += 1;

    // checksum will be calculate at the end

    buffer_offset += 2;  
    

    if(icmp == 0 || icmp == 2) // if it is echo request or reply
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
    else if(icmp == 4) // if it is router solicitation
    {
        // reseved
        buffer[buffer_offset] = 0;
        buffer[buffer_offset + 1] = 0;
        buffer[buffer_offset + 2] = 0;
        buffer[buffer_offset + 3] = 0;
        buffer_offset += 4;
    }
    
    else if(icmp == 6) // if it is router advertisement
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

    else if(icmp == 8 || icmp == 9) // if it is neighbor solicitation
    {
        int i;

        // reserved
        buffer[buffer_offset] = 0;
        buffer[buffer_offset+1] = 0;
        buffer[buffer_offset+2] = 0;
        buffer[buffer_offset+3] = 0;
        buffer_offset += 4;  

        // target value
        if(icmp == 8)
        {
            for(i = 0; i < 16; i++)
            {
                buffer[buffer_offset + i] = buffer[24 + i];
            }
            buffer_offset += 16; 
        }
        else
        {
            memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],16);

            buffer_offset += 16; 
            schc_offset += 16;
        }
        
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
    else if(icmp == 10 && icmp == 11) // if it is neighbor advertisement
    {
        int i;

        // flags
        buffer[buffer_offset] = 1;
        buffer[buffer_offset+1] = 1;
        buffer[buffer_offset+2] = 0;
        buffer[buffer_offset+3] = 0;
        buffer_offset += 4;  

        // target value
        if(icmp == 10)
        {
            for(i = 0; i < 16; i++)
            {
                buffer[buffer_offset + i] = buffer[8 + i];
            }
            buffer_offset += 16;
        }
        else
        {
            memcpy(&buffer[buffer_offset],&schc_buffer[schc_offset],16);
            buffer_offset += 16; 
            schc_offset += 16;
        }
        
        // option elided
    }
    else if(icmp == 12) // redirect
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

    int schc_header = (schc_buffer[0] >> 7) & 1;
    int schc_src = (schc_buffer[0] >> 6) & 1;
    int multicast = (schc_buffer[0] >> 5) & 1;
    int schc_dst = (schc_buffer[0] >> 4) & 1;

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
    buffer[buffer_offset] = 255;
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

