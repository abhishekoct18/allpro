/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __CAN_MSG_BUFFER_H__
#define __CAN_MSG_BUFFER_H__

#include <cstdint>

using namespace std;

//
// To exchange messages with CAN controller
//

struct CanMsgBuffer {
    const static uint8_t DefaultByte = 0x55;
    CanMsgBuffer();
    CanMsgBuffer(uint32_t _id, bool _extended, uint8_t _dlc, 
        uint8_t _data0, 
        uint8_t _data1 = DefaultByte,
        uint8_t _data2 = DefaultByte, 
        uint8_t _data3 = DefaultByte, 
        uint8_t _data4 = DefaultByte, 
        uint8_t _data5 = DefaultByte, 
        uint8_t _data6 = DefaultByte,
        uint8_t _data7 = DefaultByte);
    uint32_t id;
    bool    extended;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t msgnum;
};

#endif //__CAN_MSG_BUFFER_H__

