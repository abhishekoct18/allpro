/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include "canhistory.h"
#include "canmsgbuffer.h"

using namespace std;
using namespace util;

/**
 * Display the message history
 */
void CanHistory::dumpCurrentBuffer()
{
    const int can11Pos = 5;
    const int can29Pos = 10;
    int i, endp;
    
    // Calculate the end of the buffer
    if (numOfEntries_ <= HISTORY_LEN)
        i = endp = 0;
    else 
        i = endp = currMsgPos_;
    
    // Calculate the message ID length, 11bit or 29bit
    bool extended = false;
    int pos1 = can11Pos; int j = i;
    do {
        if (msglog_[j].ext) { pos1 = can29Pos; extended = true; break; }
        // Advance the position
        j = (j == HISTORY_LEN-1) ? 0 : j + 1;        
    } while (j != endp);

    const int pos2 = pos1 + 3;
    const int pos3 = pos2 + 3;
    string out;
    
    do {
        out.resize(0);
        CanIDToString(msglog_[i].id, out, extended);
        out.resize(pos1, ' ');
        out += msglog_[i].dir ? 'S' : 'R';
        out.resize(pos2, ' ');
        out += msglog_[i].dlc + '0';
        out.resize(pos3, ' ');
        to_ascii(msglog_[i].data, 8, out);
        out += "  -> ";
        to_ascii(&msglog_[i].mid, 1, out);
        
        AdptSendReply(out);
        // Advance the position
        i = (i == HISTORY_LEN-1) ? 0 : i + 1;
    } while (i != endp);
}

/**
 * Add the CAN message to history log
 * @param[in] buff The CanMsgBuffer pointer to add
 * @param[in] dir The direction, false - receive, true send
 * @param[in] mid CAN receiver message buffer id
 */
void CanHistory::add2Buffer(const CanMsgBuffer* buff, bool dir, uint8_t mid)
{
    int i = currMsgPos_++;

    msglog_[i].id = buff->id;
    msglog_[i].dir = dir;
    msglog_[i].ext = buff->extended;
    msglog_[i].dlc = buff->dlc;
    memcpy(msglog_[i].data, buff->data, sizeof(buff->data));
    msglog_[i].mid = mid;

    if (currMsgPos_ >= HISTORY_LEN) { // curMsgPos = [0...15]
        currMsgPos_ = 0;
    }
    numOfEntries_++;
}
