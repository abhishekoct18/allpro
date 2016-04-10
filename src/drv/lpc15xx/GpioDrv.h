/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __GPIO_DRV_H__
#define __GPIO_DRV_H__

#include <cstdint>

using namespace std;

// GPIO Direction
const uint32_t GPIO_INPUT  = 0x0;
const uint32_t GPIO_OUTPUT = 0x1;

// Port attributes
const uint32_t GPIO_OPEN_DRAIN = 0x400;

void GPIOSetDir(uint32_t portNum, uint32_t pinNum, uint32_t dir);
void GPIOPinWrite(uint32_t portNum, uint32_t pinNum, uint32_t val);
void GPIOPinConfig(uint32_t portNum, uint32_t pinNum, uint32_t val);
uint32_t GPIOPinRead(uint32_t port_num, uint32_t pin_num);


#endif //__GPIO_DRV_H__
