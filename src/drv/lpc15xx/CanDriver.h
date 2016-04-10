/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __CAND_RIVER_H__
#define __CAND_RIVER_H__

#include <cstdint>

using namespace std;

typedef void *CAN_HANDLE_T;
struct CanMsgBuffer;

class CanDriver {
public:
    static CanDriver* instance();
    static void configure();
    bool send(const CanMsgBuffer* buff);
    bool setFilterAndMask(uint32_t filter, uint32_t mask, bool extended);
    bool isReady() const;
    bool read(CanMsgBuffer* buff);
    bool wakeUp();
    bool sleep();
    void setBitBang(bool val);
    void setBit(uint32_t val);
    uint32_t getBit();
    static CAN_HANDLE_T handle_;
private:
    CanDriver();
    void configRxMsgobj(uint32_t filter, uint32_t mask, uint8_t msgobj, bool can29bit, bool fifoLast);
};

#endif //__CAN_DRIVER_H__
