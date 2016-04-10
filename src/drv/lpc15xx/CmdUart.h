/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __CMD_UART_H__
#define __CMD_UART_H__

#include <cstdint>
#include <lstring.h>

using namespace std;

const int TX_BUFFER_LEN = 100;

typedef bool (*UartRecvHandler)(uint8_t ch);

class CmdUart {
public:
    static CmdUart* instance();
    static void configure();
    void irqHandler();
    void init(uint32_t speed);
    void send(const util::string& str);
    void send(uint8_t ch);
    bool ready() const { return ready_; }
    void ready(bool val) { ready_ = val; }
    void handler(UartRecvHandler handler) { handler_ = handler; }
private:
    CmdUart();
    void txIrqHandler();
    void rxIrqHandler();

    char txData_[TX_BUFFER_LEN];
    util::string    rdData_;
    uint16_t        txLen_;
    uint16_t        txPos_;
    volatile bool   ready_;
    UartRecvHandler handler_;
};


#endif //__CMD_UART_H__
