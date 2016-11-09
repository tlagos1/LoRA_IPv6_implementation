#include <stdio.h>
#include <stdint.h>
#include "LoRaMacCrypto.h"

#define LORAMAC_PHY_MAXPAYLOAD 255

#define UPLINK 0

#define DOWNLINK 1

int main(int argc, const char * argv[]) {

uint8_t payload[] = {0x40,0x01,0x00,0xFF,0x03,0x80,0x1B,0x00,0x01,0x0D,0x98,0x16,0xDA,0xCA,0x11,0x96,0xA2,0xE3,0xDC,0xDB,0x0A,0xBE,0x22,0xEC,0xA4,0x6F}; // An example of my payload. The payload comes as a base64 encoded string so a prerequisite step is to convert it to hex before supplying it here

uint8_t appPayloadStartIndex = 9;

uint16_t size = 26; // This should be your payload size.

uint8_t key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};

uint32_t address = (uint32_t)payload[1] + ((uint32_t)payload[2]<<8) + ((uint32_t)payload[3]<<16) + ((uint32_t)payload[4]<<24);

printf("0x%x\n", address);
uint8_t dir = UPLINK;


uint32_t sequenceCounter = (uint32_t)payload[6] + ((uint32_t)payload[7]<<8); //FCnt from the LoRa spec

static uint8_t LoRaMacRxPayload[LORAMAC_PHY_MAXPAYLOAD];

LoRaMacPayloadDecrypt(payload + appPayloadStartIndex, size, key, address, dir, sequenceCounter, LoRaMacRxPayload);

for(int count=0; count < size; ++count)
printf("0x%02x ", LoRaMacRxPayload[count]);

printf("\nEnd of Program!\n");
return 0;
}