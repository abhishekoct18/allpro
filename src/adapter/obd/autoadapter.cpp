/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "autoadapter.h"

void AutoAdapter::getDescription()
{
    AdptSendReply("AUTO");
}

void AutoAdapter::getDescriptionNum()
{
    AdptSendReply("0");
}

int AutoAdapter::onRequest(const uint8_t* data, int len)
{
    return REPLY_NO_DATA;
}

int AutoAdapter::onConnectEcu(bool sendReply)
{
    // PWM
    int protocol = ProtocolAdapter::getAdapter(ADPTR_PWM)->onConnectEcu(sendReply);
    if (protocol != 0) {
        return protocol;
    }
    // VPW
    protocol = ProtocolAdapter::getAdapter(ADPTR_VPW)->onConnectEcu(sendReply);
    if (protocol != 0)
        return protocol;
    // ISO
    protocol = ProtocolAdapter::getAdapter(ADPTR_ISO)->onConnectEcu(sendReply);
    if (protocol != 0)
        return protocol;
    // CAN
    protocol = ProtocolAdapter::getAdapter(ADPTR_CAN)->onConnectEcu(sendReply);
    if (protocol != 0)
        return protocol;
    // CAN 29
    protocol = ProtocolAdapter::getAdapter(ADPTR_CAN_EXT)->onConnectEcu(sendReply);
    if (protocol != 0)
        return protocol;
    return 0;
}
