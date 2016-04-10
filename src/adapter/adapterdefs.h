/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ADAPTER_DEFS_H__ 
#define __ADAPTER_DEFS_H__

#ifdef  __CC_ARM
  #define unique_ptr auto_ptr
  #define nullptr    NULL
#elif  __GNUC__ 
#endif

const int TX_LED_PORT =  0;
const int RX_LED_PORT =  0;
const int TX_LED_NUM  = 12;
const int RX_LED_NUM  = 11;

#endif //__ADAPTER_DEFS_H__
