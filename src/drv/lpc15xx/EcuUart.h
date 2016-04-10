/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ECU_UART_H__
#define __ECU_UART_H__

#include <cstdint>
#include <ecumsg.h>

using namespace std;

class EcuUart {
public:
    static EcuUart* instance();
    static void configure();
    void init(uint32_t speed);
    void send(uint8_t byte);
    uint8_t get();
    bool getEcho(uint8_t byte);
    bool ready();
    void clear() {}
    void clearRxFifo() {}
    void setBitBang(bool val);
    void setBit(uint32_t val);
    uint32_t getBit();
private:
    EcuUart() {}
};

#endif //__ECU_UART_H__
