/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __CAN_HISTORY_H__ 
#define __CAN_HISTORY_H__

#include <adaptertypes.h>

using namespace util;

typedef struct {
    uint32_t id;
    bool dir;
    bool ext;
    uint8_t dlc;
    uint8_t mid;
    uint8_t data[8];
} MsgEntry;

struct CanMsgBuffer;

class CanHistory {
public:
	CanHistory() : currMsgPos_(0), numOfEntries_(0) {}
	void dumpCurrentBuffer();
	void add2Buffer(const CanMsgBuffer* buff, bool dir, uint8_t mid)
;
private:
	const static int HISTORY_LEN = 16;
	int      currMsgPos_;
	int      numOfEntries_;
	MsgEntry msglog_[HISTORY_LEN];
};


#endif //__CAN_HISTORY_H
