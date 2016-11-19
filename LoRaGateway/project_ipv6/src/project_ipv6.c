/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo
Description:
    Send a bunch of packets on a settable frequency
License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont
*/

/*Edited by Tomás Lagos*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* exit codes */
#include <getopt.h>     /* getopt_long */

#include "loragw_hal.h"
#include "loragw_aux.h"

#include "LoWPAN_HC.h"
#include "tun_tap.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MSG(args...) fprintf(stderr, args) /* message that is destined to the user */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define TX_RF_CHAIN          0    /* TX only supported on radio A */
#define DEFAULT_RSSI_OFFSET  0.0
#define DEFAULT_MODULATION   "LORA"
#define DEFAULT_BR_KBPS      50
#define DEFAULT_FDEV_KHZ     25

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES (GLOBAL) ------------------------------------------- */

/* signal handling variables */
struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* TX gain LUT table */
static struct lgw_tx_gain_lut_s txgain_lut = {
    .size = 5,
    .lut[0] = {
        .dig_gain = 0,
        .pa_gain = 0,
        .dac_gain = 3,
        .mix_gain = 12,
        .rf_power = 0
    },
    .lut[1] = {
        .dig_gain = 0,
        .pa_gain = 1,
        .dac_gain = 3,
        .mix_gain = 12,
        .rf_power = 10
    },
    .lut[2] = {
        .dig_gain = 0,
        .pa_gain = 2,
        .dac_gain = 3,
        .mix_gain = 10,
        .rf_power = 14
    },
    .lut[3] = {
        .dig_gain = 0,
        .pa_gain = 3,
        .dac_gain = 3,
        .mix_gain = 9,
        .rf_power = 20
    },
    .lut[4] = {
        .dig_gain = 0,
        .pa_gain = 3,
        .dac_gain = 3,
        .mix_gain = 14,
        .rf_power = 27
    }};

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

static void sig_handler(int sigio);


/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void sig_handler(int sigio) {
    if (sigio == SIGQUIT) {
        quit_sig = 1;;
    } else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
        exit_sig = 1;
    }
}


/*void printbitssimple(RoHC_base *n) 
{
    int j;
    uint8_t  i;

    for(j = 0;j < sizeof(n->returnValue);j++)
    {
        i = 1<<7;
        while (i > 0) 
        {
            if (n->returnValue[j] & i)
                printf("1");
            else
                printf("0");
            i >>= 1;
        }
        printf("\n");
    }
    
}*/
/* describe command line options */

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

int main()
{

    char buffer[1500];
    int tun_fd = 0,nread=0;


    int i;
    uint8_t status_var;

    /* user entry parameters */

    /* application parameters */
    char mod[64] = DEFAULT_MODULATION;
    uint32_t f_target = 869525000; /* target frequency - invalid default value, has to be specified by user */
    int sf = 10; /* SF10 by default */
    int cr = 1; /* CR1 aka 4/5 by default */
    int bw = 125; /* 125kHz bandwidth by default */
    int pow = 14; /* 14 dBm by default */
    int preamb = 8; /* 8 symbol preamble by default */
    int pl_size = 52; /* 16 bytes payload by default */
    int delay = 1000; /* 1 second between packets by default */
    int repeat = -1; /* by default, repeat until stopped */
    bool invert = false;
    float br_kbps = DEFAULT_BR_KBPS;
    uint8_t fdev_khz = DEFAULT_FDEV_KHZ;
    bool lbt_enable = false;
    uint32_t lbt_f_target = 0;
    uint32_t lbt_tx_max_time = 4000000;
    uint32_t lbt_sc_time = 5000;
    uint8_t  lbt_rssi_target = 160;
    uint8_t  lbt_nb_channel = 1;

    /* RF configuration (TX fail if RF chain is not enabled) */
    enum lgw_radio_type_e radio_type =  LGW_RADIO_TYPE_SX1257;
    uint8_t clocksource = 1; /* Radio B is source by default */
    struct lgw_conf_board_s boardconf;
    struct lgw_conf_lbt_s lbtconf;
    struct lgw_conf_rxrf_s rfconf;

    /* allocate memory for packet sending */
    struct lgw_pkt_tx_s txpkt; /* array containing 1 outbound packet + metadata */

    /* loop variables (also use as counters in the packet payload) */
    uint16_t cycle_count = 0;

    tun_fd = init_tun();

    /* parse command line options */
    

    /* Summary of packet parameters */
    if (strcmp(mod, "FSK") == 0) {
        printf("Sending %i FSK packets on %u Hz (FDev %u kHz, Bitrate %.2f, %i bytes payload, %i symbols preamble) at %i dBm, with %i ms between each\n", repeat, f_target, fdev_khz, br_kbps, pl_size, preamb, pow, delay);
    } else {
        printf("Sending %i LoRa packets on %u Hz (BW %i kHz, SF %i, CR %i, %i bytes payload, %i symbols preamble) at %i dBm, with %i ms between each\n", repeat, f_target, bw, sf, cr, pl_size, preamb, pow, delay);
    }

    /* configure signal handling */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = sig_handler;
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    /* starting the concentrator */
    /* board config */
    memset(&boardconf, 0, sizeof(boardconf));
    boardconf.lorawan_public = true;
    boardconf.clksrc = clocksource;
    lgw_board_setconf(boardconf);

    /* LBT config */
    if (lbt_enable) {
        memset(&lbtconf, 0, sizeof(lbtconf));
        lbtconf.enable = true;
        lbtconf.rssi_target = lbt_rssi_target;
        lbtconf.scan_time_us = lbt_sc_time;
        lbtconf.nb_channel = lbt_nb_channel;
        lbtconf.start_freq = lbt_f_target;
        lbtconf.tx_delay_1ch_us = lbt_tx_max_time;
        lbtconf.tx_delay_2ch_us = lbt_tx_max_time;
        lgw_lbt_setconf(lbtconf);
    }

    /* RF config */
    memset(&rfconf, 0, sizeof(rfconf));
    rfconf.enable = true;
    rfconf.freq_hz = f_target;
    rfconf.rssi_offset = DEFAULT_RSSI_OFFSET;
    rfconf.type = radio_type;
    for (i = 0; i < LGW_RF_CHAIN_NB; i++) {
        if (i == TX_RF_CHAIN) {
            rfconf.tx_enable = true;
        } else {
            rfconf.tx_enable = false;
        }
        lgw_rxrf_setconf(i, rfconf);
    }

    /* TX gain config */
    lgw_txgain_setconf(&txgain_lut);

    /* Start concentrator */
    i = lgw_start();
    if (i == LGW_HAL_SUCCESS) {
        MSG("INFO: concentrator started, packet can be sent\n");
    } else {
        MSG("ERROR: failed to start the concentrator\n");
        return EXIT_FAILURE;
    }



    /* fill-up payload and parameters */
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

    uint8_t lala[1];

    lala[0] = 0x61;
    lala[1] = 0xBB;

    txpkt.invert_pol = invert;
    txpkt.preamble = preamb;
    txpkt.size = pl_size;
    int j;

    for(j = 0; j < 2; j++ )
    {
        txpkt.payload[j] = lala[j];
    }

    


    cycle_count = 0;
    while ((repeat == -1) || (cycle_count < repeat)) {
       nread = read(tun_fd,buffer,sizeof(buffer));
       if(IPv6ToMesh(buffer,nread) != NULL)
       {

            //printbitssimple(IPv6ToMesh(buffer));
            ++cycle_count;

            /* send packet */
            printf("Sending packet number %u ...", cycle_count);

            i = lgw_send(txpkt); /* non-blocking scheduling of TX packet */
            if (i == LGW_HAL_ERROR) {
                printf("ERROR\n");
                return EXIT_FAILURE;
            } else if (i == LGW_LBT_ISSUE ) {
                printf("Failed: Not allowed (LBT)\n");
            } else {
                /* wait for packet to finish sending */
                do {
                    wait_ms(5);
                    lgw_status(TX_STATUS, &status_var); /* get TX status */
                } while (status_var != TX_FREE);
                printf("OK\n");
            }

            /* wait inter-packet delay */
            wait_ms(delay);

            /* exit loop on user signals */
            if ((quit_sig == 1) || (exit_sig == 1)) {
                break;
            }
        }
    }

    /* clean up before leaving */
    lgw_stop();

    printf("Exiting LoRa concentrator TX test program\n");
    return EXIT_SUCCESS;
}
