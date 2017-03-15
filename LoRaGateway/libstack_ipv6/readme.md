# LoRa IPv6 library

## Introduction

The LoRa IPv6 library is compose by the following folders

* IPv6: Contains basic functions for IPv6
* LoWPAN_IPHC: Contains the functions to implement IPv6 over 6LoWPAN 
* SCHC: Contains the functions to implement stactic context header compresion over 6LoWPAN.
* tun_tap: Contains the functions to work with a virtual network tunnel.

## Library functions 
### 1. LoWPAN_IPHC

* **IPv6ToMesh**: This function dissects the IPv6 header to assemble it later in the 6LoWPAN format using the function **reassemble_lowpan** 
    * **reassemble_lowpan**: It build the 6LoWPAN format with the IPv6 header components.
* **IPv6Rx**: It receive the 6LoWPAN buffer and decode it in a IPv6 buffer using the function **DecodeIPv6**.
    * **DecodeIPv6**: It decode 6LoWPAN in IPv6. 

### 2. SCHC

* **SCHC_TX**: Using the function **check_rule**, this function compress the 6LoWPAN header in a  rule with the size of 1 Byte.
    * **check_rule**: This function check if the 6LoWPAN header is in one of the SCHC rules. If exist return the rule.
* **SCHC_RX**: This function receive the SCHC buffer and translates it to 6LowPAN buffer.
 
### 3. IPv6

* **checksum_icmpv6**: It makes the 2 Bytes checksum for ICMPv6.
* **checksum_schc**: It makes the 2 Bytes checksum for the SCHC buffer to check if it is correct.

### 4. tun_tap

* **init_tun**: This function start the virtual network tunnel for the gateway with the interface **tun0**. The global address is **bbbb::1** and the link local address is **fe80::1**.

## SCHC rules
### Rule 1

    Type Link-local (unicast)

    Last 8 Bytes
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 1;  dst[7] = 3;
        
        Traffic class = 00
        
        Next header = ICMPv6
        
        Hop limit = 64
        
### Rule 2

    Type Link-local (unicast)

    Last 8 Bytes
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 3;  dst[7] = 1;
        
        Traffic class = 00
        
        Next header = ICMPv6
        
        Hop limit = 64
                
### Rule 3

    Type Link-local (unicast)

    Last 8 Bytes
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 1;  dst[7] = 2;
        
        Traffic class = 00
        
        Next header = ICMPv6
        
        Hop limit = 64

### Rule 4

    Type Link-local (unicast)

    Last 8 Bytes
        src[0] = 0;  dst[0] = 0;
        src[1] = 0;  dst[1] = 0;
        src[2] = 0;  dst[2] = 0;
        src[3] = 0;  dst[3] = 0;
        src[4] = 0;  dst[4] = 0;
        src[5] = 0;  dst[5] = 0;
        src[6] = 0;  dst[6] = 0;
        src[7] = 2;  dst[7] = 1;
        
        Traffic class = 00
        
        Next header = ICMPv6
        
        Hop limit = 64        
### Rule 5
    Type Link-local (Multicast)

    Last 8 Bytes for src and last Byte for dst
        src[0] = 0;  dst[0] = 1;
        src[1] = 0;  
        src[2] = 0;  
        src[3] = 0;  
        src[4] = 0;  
        src[5] = 0;  
        src[6] = 0;  
        src[7] = 1;  
        
        Traffic class = 00
        
        Next header = ICMPv6
        
        Hop limit = 1

