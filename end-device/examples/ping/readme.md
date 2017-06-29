# LoRa IPv6 library

## Introduction

The LoRa IPv6 library is compose by the following folders

* IPv6: Contains basic functions for IPv6
* SCHC: Contains the functions to implement stactic context header compresion over IPv6.
* tun_tap: Contains the functions to work with a virtual network tunnel.

## Library functions  

### 1. SCHC

* **schc_compression**: This function compress all the redundant information of the ICMPv6 header (IPv6 header + ICMPv6 header) and return a SCHC rule to send.
    
* **schc_decompression**: This function take the SCHC rule and transform it in a ICMPv6 packet to work with it.
 
### 2. IPv6

* **checksum_icmpv6**: Prepares the ICMPv6 packet to be processed in the **checksum** function.
* **icmp_reply**: It makes an echo reply packet with the data of the echo request packet. 
* **router_solicitation**: Generates a router solicitation packet.
* **router_advertisement**: Generates a router advertisement packet.
* **neighbor_solicitation**: Generates a neighbor solicitation packet.
* **neighbor_advertisement**: Generates a neighbor advertisement packet.
* **redirect**: Generates a redirect packet.
* **add_node**: Adds the discovered node in a linked list.
* **get_info_by_IPv6**: Returns the node info asked by IPv6.
* **get_info_by_mac**: Returns the node info asked by mac.

### 3. tun_tap

* **init_tun**: This function start the virtual network tunnel for the gateway with the interface **tun0**. The global address is **bbbb::1** and the link local address is **fe80::1**.

## SCHC rule structure

Next Header | Source Address | Reserved | ICMPv6 | Concatenate 
--- | --- | --- | --- | --- 
1 bits | 1 bit | 1 bit | 4 bit | 1 bit |

* The first 2 bits correspond to the next header where:
    * 0 -> ICMPv6
    * 1 -> reserved
* The next bit correspond to the source address where:
    * 0 -> link-local address
    * 1 -> global address
* The next bit correspond for future aplications:
    * 0 -> is multicast
    * 1 -> is not multicast
In case that the first bits is 0 : 
* The next 3 bits correspond to the ICMPv6 type and the last bit to the Target Address where:
	* 000:0 -> echo request
	* 001:0 -> echo reply 
	* 010:0 -> router solicitation
	* 011:0 -> router advertisement
	* 100:b -> neighbor solicitation
		* b = 0 -> Link-local target address
		* b = 1 -> global target address
	* 101:b -> neighbor advertisement
		* b = 0 -> Link-local target address
		* b = 1 -> global target address    
 	* 110:0 -> redirect
	* 111:0 -> reserved
* The next bit correspond for concatenation:
	* 0 -> single package
	* 1 -> There is a concatenate package next to the original 

## SCHC rule tables

| IPv6 header | schc rule | Description |
 --- | --- | ---
 Version | Elided | Redundant Data
 Traffic Class | Elided | Redundant Data
 Flow Label | Elided | Redundant Data
 Payload Length | Elided | Can be calculate
 Next Header | Elided | In SCHC rule
 Hop limit | Elided | Redundant Data
 Source Address | **Send** | last 8 bytes if it's link - local else full 16 bytes if it's global
 Destination Address | Elided | The Gateway keep the address
 
 | Echo Request - Reply | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Identifier | **Send** | Ping ID 
 Sequence Number| **Send** | Always different
 
| Router Solicitation | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Reserved | Elided | Redundant Data 
 MAC | **Send** | to be resolve
 
| Router Advertisement | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Cur Hop | Elided | Redundant Data 
 Autoconfig Flags | Elided | Redundant Data 
 Router Lifetime | Elided | Redundant Data 
 Reachable Time | Elided | Redundant Data 
 Retrans Timer | Elided | Redundant Data 
 Option Type | Elided | Redundant Data
 Option Length | Elided | Redundant Data 
 Option Lynk - Layer| **Send** | gateway mac
 
 | Neighbor Solicitation | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Reserved | Elided | Redundant Data 
 Target address | **Send** | who am I asking for 
 Option Type | Elided | what option type 
 Option Length | Elided | Redundant Data 
 Option Lynk - Layer| Elided | node mac 
 
 | Neighbor Advertisement | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Flags | Elided | Redundant Data 
 Target Address | **Send** | Who am I reply 
 Option | Elided | Redundant Data 
 
  | Redirect | schc rule | Description |
 --- | --- | ---
 Type | Elided | In SCHC rule
 Code | Elided | Redundant Data
 Checksum | Elided | Can be calculate
 Reserved | Elided | Redundant Data 
 Target Address | **Send** | 16 bytes for the new address
 Destination Address | Elided | Redundant Data 
 Option Type | **Send** | what option type 
 Option Length | Elided | Redundant Data 
 Option Lynk - Layer| **Send** | node mac 
 
