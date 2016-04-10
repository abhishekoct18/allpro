/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "padapter.h"
#include "autoadapter.h"
#include "vpw.h"
#include "pwm.h"
#include "isoserial.h"
#include "isocan.h"

using namespace util;

uint8_t ProtocolAdapter::history_[];
int     ProtocolAdapter::historyPos_;

/**
 * ProtocolAdapter object factory
 * @param[in] adapterType The adapter type number
 * @return The ProtocolAdapter pointer
 **/
ProtocolAdapter* ProtocolAdapter::getAdapter(int adapterType)
{
    static VpwAdapter  vpwAdapter;
    static PwmAdapter  pwmAdapter;
    static AutoAdapter autoAdapter;
    static IsoSerialAdapter isoAdapter;
    static IsoCan11Adapter canAdapter;
    static IsoCan29Adapter canExtAdapter;

    switch (adapterType) {
        case ADPTR_AUTO:
            return &autoAdapter;
        case ADPTR_VPW:
            return &vpwAdapter;
        case ADPTR_PWM:
            return &pwmAdapter;
        case ADPTR_ISO:
            return &isoAdapter;
        case ADPTR_CAN:
            return &canAdapter;
        case ADPTR_CAN_EXT:
            return &canExtAdapter;
        default:
            return nullptr;
    }
}

/**
 * Start the new history trail
 * @param[in] msg Message to insert
 */
void ProtocolAdapter::insertToHistory(const Ecumsg* msg)
{
    historyPos_ = 0;
    memset(history_, 0, sizeof(history_));
    appendToHistory(msg);
}

/**
 * Append the message to history
 * @param[in] msg Message to append
 */
void ProtocolAdapter::appendToHistory(const Ecumsg* msg)
{
    if (!msg->length())
        return; //Do not append empty

    if ((historyPos_ + ITEM_LEN) > sizeof(history_)) {
        historyPos_ = 0; // Reset position
    }
    history_[historyPos_++] = msg->length();
    memcpy(&history_[historyPos_], msg->data(), msg->length());
    historyPos_ += (ITEM_LEN - 1);
}

/**
 * Print the current history for debug purposes
 */
void ProtocolAdapter::dumpBuffer()
{
    for (int i = 0; i < sizeof(history_); i += ITEM_LEN) {
        string str;
        to_ascii(history_ + i, ITEM_LEN, str);
        AdptSendReply(str);
    }
}
