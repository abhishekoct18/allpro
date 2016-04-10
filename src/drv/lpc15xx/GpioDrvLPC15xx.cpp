/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <LPC15xx.h>
#include "GpioDrv.h"

struct LPC_IOCON_T {  // LPC15XX IOCON Structure
    __IO uint32_t PIO[3][32];
};

static LPC_IOCON_T* LPCIOCon = (LPC_IOCON_T*) LPC_IOCON;

/**
 * Setting GPIO pin direction
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] dir GPIO_DIR_INPUT, GPIO_DIR_OUTPUT
 */
void GPIOSetDir(uint32_t portNum, uint32_t pinNum, uint32_t dir)
{
    if (dir) {
        LPC_GPIO_PORT->DIR[portNum] |= (1UL << pinNum);
    }
    else {
        LPC_GPIO_PORT->DIR[portNum] &= ~(1UL << pinNum);
    }
}

/**
 * Setting the port pin value
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] val Port pin value (0 or 1)
 */
void GPIOPinWrite(uint32_t portNum, uint32_t pinNum, uint32_t val)
{
    if (val) {
        LPC_GPIO_PORT->SET[portNum] = (1UL << pinNum);
    }
    else {
        LPC_GPIO_PORT->CLR[portNum] = (1UL << pinNum);
    }
}

/**
 * Read port pin
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @return pin value (0 or 1)
 */
uint32_t GPIOPinRead (uint32_t portNum, uint32_t pinNum)
{
    return (LPC_GPIO_PORT->PIN[portNum] & (1UL << pinNum)) ? 1 : 0;
}


/**
 * Setting CPU-specific port attributes, like open drain and etc.
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] val Port attribute
 */
void GPIOPinConfig(uint32_t portNum, uint32_t pinNum, uint32_t val)
{
    LPCIOCon->PIO[portNum][pinNum] = val;
}
