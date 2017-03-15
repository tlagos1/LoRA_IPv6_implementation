# LoRa_IPv6_implementation

## Introduction

LoRa is a Low-power, wide-area network that allows to build systems to obtain data on long distances using low bandwith, providing a long battery life to the nodes. This tecnology has the ability to send short packets using Direct Sequence Spread Spectrum (DSSS) in large time intervals. 

This project aims to implement IPv6 for LoRa networks including  Neighbor Discovery service and basic IPv6 configuration. 

## Objetive

By using RoHC, this implementation will build a table inside the LoRa gateway to map the source IPv6 header to an internal ID which will be used to identify the flow to the end-device.  If the source device changes any section of the header, the GateWay will send that difference to the end device using the same flow ID.

## Components

In order to achieve our goal, we plan to use:

	- 1 Modtronix inAir9 LoRa Modules
	- 1 IC880A module
	- 1 Raspberry Pi 2
	- 1 Arduino uno

## Software Components 

For the initial version of the project, the following software components are planned to be used:

	end-device project - Arduino-LMIC library by matthijskooijman
	https://github.com/matthijskooijman/arduino-lmic

	LoraGateway project - LoRa Gateway project by semtech
	https://github.com/Lora-net/lora_gateway

### 1. IPv6 library: libstack_ipv6

This directory contains the source to use IPv6 in the LoRa devices. It can be found at the following addresses:

	- LoRaGateway/libstack_ipv6/
	- end-device/src/lmic/libstack_ipv6/

This library comes with the implementation of 6LoWPAN, SCHC and basics IPv6 functions explained in detail in its folder. 

### 2. IPv6 programs

For the good operation of IPv6, the following programs were created:

#### 2.1 project_ipv6 

Folder: LoRaGateway/project_ipv6
	
The goal of this program is make the Raspberry Py + IC880A works like a gateway. To make it work,   you must use the following command:

	sudo ./project_ipv6

You can start sending and reciving package after the program says that the concentrator is ready to go.

#### 2.2 ping.ino 

Folder: end-device/examples/ping
	
The goal of this program is make the Arduino UNO + inAir9 works like a LoRa node. To start running the node, you have to upload the file in the Arduino UNO with its corresponding libraries (ends-device library).

In this project, the LoRa node was configured with the IP fe80::3.

### Gateway diagram
<img src="https://github.com/tlagos1/LoRA_IPv6_implementation/blob/develop/images/dia_sistema1.png" data-canonical-src="https://github.com/tlagos1/LoRA_IPv6_implementation/blob/develop/images/dia_sistema1.png" width="700" />

### Node diagram
<img src="https://github.com/tlagos1/LoRA_IPv6_implementation/blob/develop/images/dia_sistema2.png" data-canonical-src="https://github.com/tlagos1/LoRA_IPv6_implementation/blob/develop/images/dia_sistema2.png" width="800" />

### Updates

15/03/2017 : Beginning multicast Development, Readmes Updates. 

27/02/2017 : 6LoWPAN_IPHC and SCHC implementation for ICMP6 communication between LoRa Gateway and LoRa Node.

09/11/2016 : RoHC base and AES decryption added in LoRa GateWay. The hop limit is not configured.
