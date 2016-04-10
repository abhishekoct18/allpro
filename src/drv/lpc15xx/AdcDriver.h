/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ADC_DRIVER_H__ 
#define __ADC_DRIVER_H__

#include <cstdint>

using namespace std;

class AdcDriver {
public:
    static void configure();
    static uint32_t read();
};

#endif
