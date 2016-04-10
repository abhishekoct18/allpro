/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __J1850_DEFINES_H__ 
#define __J1850_DEFINES_H__

//
// SAE J1850 timeouts definition
//

enum J1850Limits {
    J1850_BYTES_MIN =  1,
    J1850_BYTES_MAX = 12,
    OBD2_BYTES_MIN  =  5, // 3(header) + 1(data) + 1(checksum)
    OBD2_BYTES_MAX  = 11  // 3(header) + 7(data) + 1(checksum)
};
    
// J1850 Timeouts
//
enum J1850Timeouts {
    P2_J1850 = 100 // in msec
};

// VPW Timeouts, in usec
//
enum VpwTimeouts {
    TV1_TX_NOM  =   64,
    TV2_TX_NOM  =  128,
    TV3_TX_NOM  =  200, 
    TV4_TX_MIN  =  261, 
    TV6_TX_MIN  =  280,
    TV5_TX_NOM  =  300,
    TV5_TX_MAX  =  5000,
    TV6_TX_NOM  =  300,
    TV1_TX_ADJ  =   64,
    TV2_TX_ADJ  =  128,
    TV1_RX_MIN  =   34,
    TV2_RX_MAX  =  163,
    TV3_RX_MIN  =  163,
    TV3_RX_MAX  =  239,
    TV5_RX_MIN  =  239,
    TV6_RX_MIN  =  280,
    VPW_RX_MID  =   96
};

// PWM Timeouts, in usec
//
enum PwmTimeouts {
    TP1_TX_NOM  =   8,
    TP2_TX_NOM  =  16,
    TP3_TX_NOM  =  24,
    TP4_TX_NOM  =  48,
    TP5_TX_MIN  =  70,
    TP6_TX_NOM  =  96,
    TP7_TX_NOM  =  32,
    TP8_TX_NOM  =  40,
    TP9_TX_NOM  = 120,
    TP1_RX_MAX  =  10,
    TP2_RX_MIN  =  12,
    TP2_RX_MAX  =  19,
    TP3_RX_MAX  =  27,
    TP4_RX_MIN  =  46, 
    TP4_RX_MAX  =  63,
    TP7_RX_MIN  =  30,
    TP7_RX_MAX  =  35,
    TP8_RX_MAX  =  43
};

#endif //__J1850_DEFINES_H__
