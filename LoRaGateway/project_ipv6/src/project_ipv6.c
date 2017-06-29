/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Configure LoRa concentrator and record received packets in a log file

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont

edited by Tomás Lagos
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#define _BSD_SOURCE

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* atoi */
#include <pthread.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>


#include "loragw_hal.h"
#include "IPv6.h"
#include "tun_tap.h"
#include "parson.h"
#include "SCHC.h"




/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MSG(args...)    fprintf(stderr,"loragw_pkt_logger: " args) /* message that is destined to the user */

#define TX_RF_CHAIN          0    /* TX only supported on radio B */
#define DEFAULT_RSSI_OFFSET  0.0
#define DEFAULT_MODULATION   "LORA"
#define DEFAULT_BR_KBPS      0.68375
#define DEFAULT_FDEV_KHZ     25

#define DEFAULT_NOTCH_FREQ          129000U /* 129 kHz */


pthread_t tid[2];
pthread_mutex_t lock;

int exit_program = 0;

NodeList *list;
IPv6PackageRawData *IPv6_data;
ICMP6PackageRawData *ICMP6_data;
SCHC_data *SCHC;
Saved_addr *S_addr;

uint8_t DEVEUI[8]={0x10, 0x4a, 0x00, 0x16, 0xC0, 0x00, 0x00, 0x01};
uint8_t ETH_mac[6]={0x33, 0x33, 0x00, 0x00, 0x00, 0x02};

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES (GLOBAL) ------------------------------------------- */

/* signal handling variables */
struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */

/* configuration variables needed by the application  */
uint64_t lgwm = 0; /* LoRa gateway MAC address */
char lgwm_str[17];


/* clock and log file management */
time_t now_time;
time_t log_start_time;
FILE * log_file = NULL;
char log_file_name[64];

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

int parse_SX1301_configuration();

int parse_gateway_configuration(const char * conf_file);

void open_log(void);

void usage (void);


/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

struct value_tx
{ 
    int net_fd;
    struct lgw_pkt_tx_s txpkt;
    int delay;
    char *schc_buffer;
};

struct value_rx
{
    struct lgw_pkt_tx_s txpkt;
    int net_fd; 
    char *ifname;
    char *schc_buffer;

};


int parse_SX1301_configuration(const char * conf_file) {
    int i;
    const char conf_obj[] = "SX1301_conf";
    char param_name[32]; /* used to generate variable parameter names */
    const char *str; /* used to store string value from JSON object */
    struct lgw_conf_board_s boardconf;
    struct lgw_conf_rxrf_s rfconf;
    struct lgw_conf_rxif_s ifconf;
    JSON_Value *root_val;
    JSON_Object *root = NULL;
    JSON_Object *conf = NULL;
    JSON_Value *val;
    uint32_t sf, bw;

    /* try to parse JSON */
    root_val = json_parse_file_with_comments(conf_file);
    root = json_value_get_object(root_val);
    if (root == NULL) {
        MSG("ERROR: %s id not a valid JSON file\n", conf_file);
        exit(EXIT_FAILURE);
    }
    conf = json_object_get_object(root, conf_obj);
    if (conf == NULL) {
        MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
        return -1;
    } else {
        MSG("INFO: %s does contain a JSON object named %s, parsing SX1301 parameters\n", conf_file, conf_obj);
    }

    /* set board configuration */
    memset(&boardconf, 0, sizeof boardconf); /* initialize configuration structure */
    val = json_object_get_value(conf, "lorawan_public"); /* fetch value (if possible) */
    if (json_value_get_type(val) == JSONBoolean) {
        boardconf.lorawan_public = (bool)json_value_get_boolean(val);
    } else {
        MSG("WARNING: Data type for lorawan_public seems wrong, please check\n");
        boardconf.lorawan_public = false;
    }
    val = json_object_get_value(conf, "clksrc"); /* fetch value (if possible) */
    if (json_value_get_type(val) == JSONNumber) {
        boardconf.clksrc = (uint8_t)json_value_get_number(val);
    } else {
        MSG("WARNING: Data type for clksrc seems wrong, please check\n");
        boardconf.clksrc = 0;
    }
    MSG("INFO: lorawan_public %d, clksrc %d\n", boardconf.lorawan_public, boardconf.clksrc);
    /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
                MSG("WARNING: Failed to configure board\n");
    }

    /* set configuration for RF chains */
    for (i = 0; i < LGW_RF_CHAIN_NB; ++i) {
        memset(&rfconf, 0, sizeof(rfconf)); /* initialize configuration structure */
        sprintf(param_name, "radio_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
        if (json_value_get_type(val) != JSONObject) {
            MSG("INFO: no configuration for radio %i\n", i);
            continue;
        }
        /* there is an object to configure that radio, let's parse it */
        sprintf(param_name, "radio_%i.enable", i);
        val = json_object_dotget_value(conf, param_name);
        if (json_value_get_type(val) == JSONBoolean) {
            rfconf.enable = (bool)json_value_get_boolean(val);
        } else {
            rfconf.enable = false;
        }
        if (rfconf.enable == false) { /* radio disabled, nothing else to parse */
            MSG("INFO: radio %i disabled\n", i);
        } else  { /* radio enabled, will parse the other parameters */
            snprintf(param_name, sizeof param_name, "radio_%i.freq", i);
            rfconf.freq_hz = (uint32_t)json_object_dotget_number(conf, param_name);
            snprintf(param_name, sizeof param_name, "radio_%i.rssi_offset", i);
            rfconf.rssi_offset = (float)json_object_dotget_number(conf, param_name);
            snprintf(param_name, sizeof param_name, "radio_%i.type", i);
            str = json_object_dotget_string(conf, param_name);
            if (!strncmp(str, "SX1255", 6)) {
                rfconf.type = LGW_RADIO_TYPE_SX1255;
            } else if (!strncmp(str, "SX1257", 6)) {
                rfconf.type = LGW_RADIO_TYPE_SX1257;
            } else {
                MSG("WARNING: invalid radio type: %s (should be SX1255 or SX1257)\n", str);
            }
            snprintf(param_name, sizeof param_name, "radio_%i.tx_enable", i);
            val = json_object_dotget_value(conf, param_name);
            if (json_value_get_type(val) == JSONBoolean) {
                rfconf.tx_enable = (bool)json_value_get_boolean(val);
            } else {
                rfconf.tx_enable = false;
            }
            MSG("INFO: radio %i enabled (type %s), center frequency %u, RSSI offset %f, tx enabled %d\n", i, str, rfconf.freq_hz, rfconf.rssi_offset, rfconf.tx_enable);
        }
        /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) {
            MSG("WARNING: invalid configuration for radio %i\n", i);
        }
    }

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
    for (i = 0; i < LGW_MULTI_NB; ++i) {
        memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
        sprintf(param_name, "chan_multiSF_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
        if (json_value_get_type(val) != JSONObject) {
            MSG("INFO: no configuration for LoRa multi-SF channel %i\n", i);
            continue;
        }
        /* there is an object to configure that LoRa multi-SF channel, let's parse it */
        sprintf(param_name, "chan_multiSF_%i.enable", i);
        val = json_object_dotget_value(conf, param_name);
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) { /* LoRa multi-SF channel disabled, nothing else to parse */
            MSG("INFO: LoRa multi-SF channel %i disabled\n", i);
        } else  { /* LoRa multi-SF channel enabled, will parse the other parameters */
            sprintf(param_name, "chan_multiSF_%i.radio", i);
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, param_name);
            sprintf(param_name, "chan_multiSF_%i.if", i);
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, param_name);
            // TODO: handle individual SF enabling and disabling (spread_factor)
            MSG("INFO: LoRa multi-SF channel %i enabled, radio %i selected, IF %i Hz, 125 kHz bandwidth, SF 7 to 12\n", i, ifconf.rf_chain, ifconf.freq_hz);
        }
        /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_rxif_setconf(i, ifconf) != LGW_HAL_SUCCESS) {
            MSG("WARNING: invalid configuration for LoRa multi-SF channel %i\n", i);
        }
    }

    /* set configuration for LoRa standard channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_Lora_std"); /* fetch value (if possible) */
    if (json_value_get_type(val) != JSONObject) {
        MSG("INFO: no configuration for LoRa standard channel\n");
    } else {
        val = json_object_dotget_value(conf, "chan_Lora_std.enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) {
            MSG("INFO: LoRa standard channel %i disabled\n", i);
        } else  {
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.radio");
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_Lora_std.if");
            bw = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.bandwidth");
            switch(bw) {
                case 500000: ifconf.bandwidth = BW_500KHZ; break;
                case 250000: ifconf.bandwidth = BW_250KHZ; break;
                case 125000: ifconf.bandwidth = BW_125KHZ; break;
                default: ifconf.bandwidth = BW_UNDEFINED;
            }
            sf = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.spread_factor");
            switch(sf) {
                case  7: ifconf.datarate = DR_LORA_SF7;  break;
                case  8: ifconf.datarate = DR_LORA_SF8;  break;
                case  9: ifconf.datarate = DR_LORA_SF9;  break;
                case 10: ifconf.datarate = DR_LORA_SF10; break;
                case 11: ifconf.datarate = DR_LORA_SF11; break;
                case 12: ifconf.datarate = DR_LORA_SF12; break;
                default: ifconf.datarate = DR_UNDEFINED;
            }
            MSG("INFO: LoRa standard channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, SF %u\n", ifconf.rf_chain, ifconf.freq_hz, bw, sf);
        }
        if (lgw_rxif_setconf(8, ifconf) != LGW_HAL_SUCCESS) {
            MSG("WARNING: invalid configuration for LoRa standard channel\n");
        }
    }

    /* set configuration for FSK channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_FSK"); /* fetch value (if possible) */
    if (json_value_get_type(val) != JSONObject) {
        MSG("INFO: no configuration for FSK channel\n");
    } else {
        val = json_object_dotget_value(conf, "chan_FSK.enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) {
            MSG("INFO: FSK channel %i disabled\n", i);
        } else  {
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_FSK.radio");
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_FSK.if");
            bw = (uint32_t)json_object_dotget_number(conf, "chan_FSK.bandwidth");
            if      (bw <= 7800)   ifconf.bandwidth = BW_7K8HZ;
            else if (bw <= 15600)  ifconf.bandwidth = BW_15K6HZ;
            else if (bw <= 31200)  ifconf.bandwidth = BW_31K2HZ;
            else if (bw <= 62500)  ifconf.bandwidth = BW_62K5HZ;
            else if (bw <= 125000) ifconf.bandwidth = BW_125KHZ;
            else if (bw <= 250000) ifconf.bandwidth = BW_250KHZ;
            else if (bw <= 500000) ifconf.bandwidth = BW_500KHZ;
            else ifconf.bandwidth = BW_UNDEFINED;
            ifconf.datarate = (uint32_t)json_object_dotget_number(conf, "chan_FSK.datarate");
            MSG("INFO: FSK channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, %u bps datarate\n", ifconf.rf_chain, ifconf.freq_hz, bw, ifconf.datarate);
        }
        if (lgw_rxif_setconf(9, ifconf) != LGW_HAL_SUCCESS) {
            MSG("WARNING: invalid configuration for FSK channel\n");
        }
    }
    json_value_free(root_val);
    return 0;
}

int parse_gateway_configuration(const char * conf_file) {
    const char conf_obj[] = "gateway_conf";
    JSON_Value *root_val;
    JSON_Object *root = NULL;
    JSON_Object *conf = NULL;
    const char *str; /* pointer to sub-strings in the JSON data */
    unsigned long long ull = 0;

    /* try to parse JSON */
    root_val = json_parse_file_with_comments(conf_file);
    root = json_value_get_object(root_val);
    if (root == NULL) {
        MSG("ERROR: %s id not a valid JSON file\n", conf_file);
        exit(EXIT_FAILURE);
    }
    conf = json_object_get_object(root, conf_obj);
    if (conf == NULL) {
        MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
        return -1;
    } else {
        MSG("INFO: %s does contain a JSON object named %s, parsing gateway parameters\n", conf_file, conf_obj);
    }

    /* getting network parameters (only those necessary for the packet logger) */
    str = json_object_get_string(conf, "gateway_ID");
    if (str != NULL) {
        sscanf(str, "%llx", &ull);
        lgwm = ull;
        MSG("INFO: gateway MAC address is configured to %016llX\n", ull);
    }

    json_value_free(root_val);
    return 0;
}
void *menu()
{

    int option;
    int i;
    NodeList *aux;
    while(true)
    {
        printf("\n\n\n");
        printf("======================================================================\n");
        printf("--------------------------Project IPv6--------------------------------\n");
        printf("======================================================================\n\n\n");
        printf("1) Show connected devices\n");
        //printf("2) stop LoRa concentrator\n");
        printf("Option: ");
        
        scanf("%i",&option);
 
        switch(option)
        {
            case 1:
                aux = check_head(list);
                printf("\n\n|             IPv6 global           |          IPv6 link - local        |     mac     |\n");
                while(aux != NULL)
                {
                    printf("| ");
                    for (i = 0; i < 16; ++i)
                    {
                        if((i + 1) == 16)
                        {
                             printf("%x",aux->IPv6_global.addr[i]);
                        }
                        else
                        {
                            printf("%x-",aux->IPv6_global.addr[i]);
                        }

                    }
                    printf(" | ");
                    for (i = 0; i < 16; ++i)
                    {
                        if((i + 1) == 16)
                        {
                             printf("%x",aux->IPv6_link_local.addr[i]);
                        }
                        else
                        {
                            printf("%x-",aux->IPv6_link_local.addr[i]);
                        }

                    }
                    printf(" | ");
                    for (i = 0; i < 6; ++i)
                    {
                        if((i + 1) == 6)
                        {
                             printf("%x",aux->mac.addr[i]);
                        }
                        else
                        {
                            printf("%x-",aux->mac.addr[i]);
                        }
                    }
                    printf(" |\n");
                    aux = aux->next;
                }
            /*case 2:
                pthread_mutex_lock(&lock);
                    exit_program = 1; 
                pthread_mutex_unlock(&lock);*/
                              
        }
    }

}

void *send_package(void *data)
{
    struct value_tx *value_tx = data;
    struct lgw_pkt_tx_s txpkt = value_tx->txpkt;

    char buffer[1024];
    char schc_buffer[1024];
    char *recived_package;
    int ix;
    NodeList *aux, *aux_info;
    
    memset(buffer,0,1024);

    int net_fd = value_tx->net_fd;
    
    while(true)
    { 
        recived_package = recive_raw_package_by_interface(net_fd, buffer);
        if(recived_package != NULL)
        {
            aux = check_head(list);

            memcpy(&buffer[0],&recived_package[0], 1024);    

            IPv6_data = get_IPv6_data_by_raw_package(buffer, IPv6_data);

            ICMP6_data = get_ICMP6_data_by_raw_payload(buffer, ICMP6_data);

            while(aux!=NULL)
            {
                if(memcmp(aux->IPv6_link_local.addr, IPv6_data->IPv6_dst.addr, 16) == 0 || memcmp(aux->IPv6_global.addr, IPv6_data->IPv6_dst.addr, 16) == 0)
                {
                    if(buffer[40] == 0x80)
                    {
                        printf("request -->\n");
                        SCHC = schc_compression(buffer, schc_buffer, SCHC);

                        aux_info = get_info_by_IPv6(list, IPv6_data->IPv6_dst.addr);
                        
                        pthread_mutex_lock(&lock);
                            S_addr = input_addr(aux_info->eui.addr, IPv6_data->IPv6_src.addr, IPv6_data->IPv6_dst.addr, S_addr, 0);
                        pthread_mutex_unlock(&lock);    

                        memset(buffer, 0, 1024);
                        txpkt.size = 0;
                        for(ix = 0;ix < 8;ix++)
                        {   
                            buffer[ix] = aux_info->eui.addr[ix];
                            txpkt.size ++;
                        }
                        for (ix = 0;ix < SCHC->length; ix++)
                        {
                            buffer[ix+8] = (char)SCHC->buffer[ix];
                            txpkt.size ++;
                        }

                        memcpy(txpkt.payload,buffer,txpkt.size);
                        lgw_send(txpkt);

                    }
                    if(buffer[40] == 0x86)
                    {    
                        printf("Router Advertisement -->\n");
                        for(ix = 0; ix < 6; ix++)
                        {
                            ETH_mac[ix] = ICMP6_data->mac[ix];
                        }

                        pthread_mutex_lock(&lock);

                            list = add_MAC_node(list, ICMP6_data->mac , 1);
                            list = add_IPv6_node(list, IPv6_data->IPv6_src.addr, ICMP6_data->mac);
                        
                        pthread_mutex_unlock(&lock);
                        
                        SCHC = schc_compression(buffer, schc_buffer, SCHC);

                        aux_info = get_info_by_IPv6(list, IPv6_data->IPv6_dst.addr);

                        if(aux_info != NULL)
                        {   
                            memset(buffer, 0, 1024);
                            txpkt.size = 0;
                            for(ix = 0;ix < 8;ix++)
                            {   
                                buffer[ix] = aux_info->eui.addr[ix];
                                txpkt.size ++;
                            }
                            for (ix = 0;ix < SCHC->length; ix++)
                            {
                                buffer[ix+8] = (char)SCHC->buffer[ix];
                                txpkt.size ++;
                            }
                            
                            memcpy(txpkt.payload,buffer,txpkt.size);
                            lgw_send(txpkt);
                        }      
                    }

                    if(buffer[40] == 0x87)
                    {
                        printf("Neighbor solicitation -->\n");
                        
                        SCHC = schc_compression(buffer, schc_buffer, SCHC);

                        aux_info = get_info_by_IPv6(list, IPv6_data->IPv6_dst.addr);

                        pthread_mutex_lock(&lock);
                            S_addr = input_addr(aux_info->eui.addr, IPv6_data->IPv6_src.addr, IPv6_data->IPv6_dst.addr, S_addr, 1);
                        pthread_mutex_unlock(&lock);
                        
                        memset(buffer, 0, 1024);
                        txpkt.size = 0;
                        for(ix = 0;ix < 8;ix++)
                        {   
                            buffer[ix] = aux_info->eui.addr[ix];
                            txpkt.size ++;
                        }
                        for (ix = 0;ix < SCHC->length; ix++)
                        {
                            buffer[ix+8] = (char)SCHC->buffer[ix];
                            txpkt.size ++;
                        }

                        memcpy(txpkt.payload,buffer,txpkt.size);
                        lgw_send(txpkt);
                    }
                    if(buffer[40] == 0x88)
                    {
                        printf("Neighbor Advertisement -->\n");
                        SCHC = schc_compression(buffer, schc_buffer, SCHC);

                        memset(buffer, 0, 1024);
                        txpkt.size = 0;
                        for(ix = 0;ix < 8;ix++)
                        {   
                            buffer[ix] = aux_info->eui.addr[ix];
                            txpkt.size ++;
                        }
                        for (ix = 0;ix < SCHC->length; ix++)
                        {
                            buffer[ix+8] = (char)SCHC->buffer[ix];
                            txpkt.size ++;
                        }

                        memcpy(txpkt.payload,buffer,txpkt.size);
                        lgw_send(txpkt);
                    }

                    break;
                }
                else
                {
                    aux = aux->next;
                }
            }
        }
    }
}

void *recive_package(void *data)
{
    uint8_t EUI_node[8];
    char schc_buffer[1500];
    uint8_t src_addr[16];

    struct value_rx *value_rx = data;

    struct lgw_pkt_rx_s *p;
    struct lgw_pkt_rx_s rxpkt[16];

    struct lgw_pkt_tx_s txpkt = value_rx->txpkt;
     
    int net_fd = value_rx->net_fd;
    char *ifname = value_rx->ifname;  
 
    int nb_pkt;

    NodeList *aux_list;
    Saved_addr *aux_S_addr;

    int ix,j;
    uint8_t target_addr[16];
    char buffer[1500];
  
    while(true)
    {
        /*offset = 40;*/
        nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
        /* log packets */
        if(nb_pkt > 0)
        {
            for (j = 0; j < nb_pkt; j++)
            {
                p = &rxpkt[0];
                
                if(p->status == STAT_CRC_OK && memcmp(&p->payload[0],&DEVEUI[0],2) == 0)
                {     
                    memset(EUI_node,0,8);
                    memset(buffer,0,1500);
                    memset(schc_buffer, 0, 1500);
                    memset(src_addr, 0, 16);
                    
                    memcpy(&EUI_node[0],&p->payload[0],8); 
                    memcpy(&schc_buffer[0],&p->payload[8],(p->size - 8)); 

                    pthread_mutex_lock(&lock);
                        list = add_EUI_node(list, EUI_node, 0);
                    pthread_mutex_unlock(&lock);

                    aux_list = get_info_by_eui(list, EUI_node);
                    

                    aux_S_addr = S_addr;

                    if(((schc_buffer[0] >> 2) & 7) == 1)
                    {
                        aux_S_addr = output_addr(EUI_node, S_addr, 0);
                        if(aux_S_addr != NULL)
                        {
                            printf("reply <--\n");
                            memcpy(&buffer[0],schc_decompression(schc_buffer, buffer, (p->size - 8), aux_S_addr->dst_addr.addr, aux_S_addr->src_addr.addr, aux_list->mac.addr), 1500);
                        }
                    }
                    if(((schc_buffer[0] >> 2) & 7) == 2)
                    {
                        printf("router solicitation  <-- \n");
                        memcpy(&buffer[0],schc_decompression(schc_buffer, buffer, (p->size - 8), NULL, NULL, aux_list->mac.addr), 1500);
                    }
                    else if(((schc_buffer[0] >> 2) & 7) == 4)
                    {
                        printf("Neighbor solicitation  <-- \n");
                        if(p->size == 33)
                        {
                            memset(target_addr, 0 ,16);
                            target_addr[0] = 0xfe;
                            target_addr[1] = 0x80;
                            for(ix = 0; ix < 8; ix++)
                            {
                                target_addr[ix + 8] = schc_buffer[ix + 17];
                            }
                            memcpy(&buffer[0],schc_decompression(schc_buffer, buffer, (p->size - 8), NULL, target_addr, aux_list->mac.addr), 1500);
                        }    
                    }
                    else if(((schc_buffer[0] >> 2) & 7) == 5)
                    {
                        printf("Neighbor Advertisement <-- \n");
                        txpkt.size = 0;
                        for(ix = 0 ; ix < 8; ix++)
                        {
                            txpkt.payload[ix] = EUI_node[ix];
                        }
                        txpkt.size += 8;
                        for(ix = 0 ; ix < (p->size - 8); ix++)
                        {
                            txpkt.payload[txpkt.size + ix] = schc_buffer[ix];
                        }
                        txpkt.size += (p->size - 8);
                        lgw_send(txpkt);

                        aux_S_addr = output_addr(EUI_node, S_addr, 1);
                        if(aux_S_addr != NULL)
                        {
                            memcpy(&buffer[0],schc_decompression(schc_buffer, buffer, (p->size - 8), aux_S_addr->dst_addr.addr, aux_S_addr->src_addr.addr, aux_list->mac.addr), 1500);
                        }

                    }
                    
                    for(ix = 0; ix < 16; ix++)
                    {
                        src_addr[ix] = buffer[ix + 8];
                    }
                    pthread_mutex_lock(&lock);
                        list = add_IPv6_node(list, src_addr, aux_list->mac.addr);
                    pthread_mutex_unlock(&lock);
                   

                    if(send_raw_package(buffer, get_info_by_eui(list, EUI_node)->mac.addr , ETH_mac, ifname, net_fd) < 0)
                    {
                        printf("Error sending raw socket\n");
                    }
                }
            } 
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

int main()
{
    int i;
    int delay =1000;
    struct lgw_conf_rxrf_s rfconf;
    struct lgw_pkt_tx_s txpkt;
    uint32_t f_target = 868300000;
    int pow = 14;
    char mod[64] = DEFAULT_MODULATION;
    uint32_t tx_notch_freq = DEFAULT_NOTCH_FREQ;
    int bw = 125;
    int sf = 7;

    int cr = 1;
    bool invert = false;
    int preamb = 8;
    int pl_size = 52;
    float br_kbps = DEFAULT_BR_KBPS;
    uint8_t fdev_khz = DEFAULT_FDEV_KHZ;
    int net_fd;

    enum lgw_radio_type_e radio_type = LGW_RADIO_TYPE_SX1257;

    char *schc_buffer = malloc(sizeof(char*));

    list  = NULL;

    IPv6_data = (IPv6PackageRawData *)malloc(sizeof(IPv6PackageRawData));

    ICMP6_data = (ICMP6PackageRawData *)malloc(sizeof(ICMP6PackageRawData)); 

    SCHC = (SCHC_data *)malloc(sizeof(SCHC_data));
    
     
    const char global_conf_fname[] = "global_conf.json";
    
    /* configuration files management */
    
    parse_SX1301_configuration(global_conf_fname);
    //parse_gateway_configuration(global_conf_fname);
    
    char *ifname = "eth0";
    net_fd = init_socket();


    if(net_fd == -1)
    {
        printf("%s\n","error socket");
        return -1;
    }

    
    /* starting the concentrator */
    i = lgw_start();
    if (i == LGW_HAL_SUCCESS) {
        MSG("INFO: concentrator started, packet can now be received\n");
    } else {
        MSG("ERROR: failed to start the concentrator\n");
        return EXIT_FAILURE;
    }

    /* opening log file and writing CSV header*/

    memset(&txpkt, 0, sizeof(txpkt));
    txpkt.freq_hz = f_target;
    txpkt.tx_mode = IMMEDIATE;
    txpkt.rf_chain = TX_RF_CHAIN;
    txpkt.rf_power = pow;
    if( strcmp( mod, "FSK" ) == 0 ) {
        txpkt.modulation = MOD_FSK;
        txpkt.datarate = br_kbps * 1e3;
        txpkt.f_dev = fdev_khz;
    } else {
        txpkt.modulation = MOD_LORA;
        switch (bw) {
            case 125: txpkt.bandwidth = BW_125KHZ; break;
            case 250: txpkt.bandwidth = BW_250KHZ; break;
            case 500: txpkt.bandwidth = BW_500KHZ; break;
            default:
                MSG("ERROR: invalid 'bw' variable\n");
                return EXIT_FAILURE;
        }
        switch (sf) {
            case  7: txpkt.datarate = DR_LORA_SF7;  break;
            case  8: txpkt.datarate = DR_LORA_SF8;  break;
            case  9: txpkt.datarate = DR_LORA_SF9;  break;
            case 10: txpkt.datarate = DR_LORA_SF10; break;
            case 11: txpkt.datarate = DR_LORA_SF11; break;
            case 12: txpkt.datarate = DR_LORA_SF12; break;
            default:
                MSG("ERROR: invalid 'sf' variable\n");
                return EXIT_FAILURE;
        }
        switch (cr) {
            case 1: txpkt.coderate = CR_LORA_4_5; break;
            case 2: txpkt.coderate = CR_LORA_4_6; break;
            case 3: txpkt.coderate = CR_LORA_4_7; break;
            case 4: txpkt.coderate = CR_LORA_4_8; break;
            default:
                MSG("ERROR: invalid 'cr' variable\n");
                return EXIT_FAILURE;
        }
    }


    txpkt.invert_pol = invert;
    txpkt.preamble = preamb;
    txpkt.size = pl_size;

    struct value_tx *value_tx = malloc(sizeof(struct value_tx));

    value_tx->net_fd = net_fd;
    value_tx->txpkt = txpkt;
    value_tx->delay = delay;
    value_tx->schc_buffer = schc_buffer;
    
    struct value_rx *value_rx = malloc(sizeof(struct value_rx));

    value_rx->txpkt = txpkt;
    value_rx->net_fd = net_fd;
    value_rx->schc_buffer = schc_buffer;
    value_rx->ifname = ifname;
    
    /*////////////////////////////////////////threads TX RX //////////////////////////////////////////////////////////////////////*/

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    pthread_create(&(tid[0]), NULL, send_package, value_tx);

    pthread_create(&(tid[1]), NULL, recive_package, value_rx);

    menu();
    
    pthread_mutex_destroy(&lock);
    /*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
    
    if (exit_program == 1) {
        /* clean up before leaving */
        i = lgw_stop();
        if (i == LGW_HAL_SUCCESS) {
            MSG("INFO: concentrator stopped successfully\n");
        } else {
            MSG("WARNING: failed to stop concentrator successfully\n");
        }
    }

    MSG("INFO: Exiting packet logger program\n");
    return EXIT_SUCCESS;
}

/* --- EOF ------------------------------------------------------------------ */
