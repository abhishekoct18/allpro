/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include "canmsgbuffer.h"

using namespace std;


CanMsgBuffer::CanMsgBuffer() 
: id(0), extended(false), dlc(0), msgnum(0)
{
    memset(data, 0, sizeof (data));
}

CanMsgBuffer::CanMsgBuffer(uint32_t _id, bool _extended, uint8_t _dlc, 
    uint8_t _data0, 
    uint8_t _data1,
    uint8_t _data2, 
    uint8_t _data3, 
    uint8_t _data4, 
    uint8_t _data5, 
    uint8_t _data6,
    uint8_t _data7) 
{
    id = _id;
    extended = _extended;
    dlc = _dlc;
    data[0] = _data0;
    data[1] = _data1;
    data[2] = _data2;
    data[3] = _data3;
    data[4] = _data4;
    data[5] = _data5;
    data[6] = _data6;
    data[7] = _data7;
}
