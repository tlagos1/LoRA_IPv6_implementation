
/*Edited by Tomás Lagos*/
typedef struct SCHC_data
{
   char buffer[64];
   uint16_t length;
}SCHC_data;


SCHC_data *schc_compression(char *, char *, SCHC_data *);
char *schc_decompression(char *, char *, int, uint8_t *, uint8_t *, uint8_t *);
char *SCHC_TX(char *, int);
char *SCHC_RX(char *, int);