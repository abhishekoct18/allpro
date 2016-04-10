/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __PROTOCOL_ADAPTER_H__
#define __PROTOCOL_ADAPTER_H__

#include <adaptertypes.h>
#include <ecumsg.h>

// Command results
//
enum ReplyTypes {
    REPLY_OK =  1,
    REPLY_CMD_WRONG,
    REPLY_DATA_ERROR,
    REPLY_NO_DATA,
    REPLY_ERROR,
    REPLY_UNBL_2_CNNCT,
    REPLY_NONE,
    REPLY_BUS_BUSY,
    REPLY_BUS_ERROR,
    REPLY_CHKS_ERROR,
    REPLY_WIRING_ERROR
};

// Protocols
//
enum ProtocolTypes {
   PROT_AUTO,
   PROT_J1850_PWM,
   PROT_J1850_VPW,
   PROT_ISO9141,
   PROT_ISO14230_5BPS,
   PROT_ISO14230,
   PROT_ISO15765_1150,
   PROT_ISO15765_2950,
   PROT_ISO15765_1125,
   PROT_ISO15765_2925
};

// Adapters
//
enum AdapterTypes {
   ADPTR_AUTO,
   ADPTR_PWM,
   ADPTR_VPW,
   ADPTR_ISO,
   ADPTR_CAN,
   ADPTR_CAN_EXT
};

class ProtocolAdapter {
public:
    static ProtocolAdapter* getAdapter(int adapterType);
    virtual int onConnectEcu(bool sendReply) = 0;
    virtual int onRequest(const uint8_t* data, int len) = 0;
    virtual void getDescription() = 0;
    virtual void getDescriptionNum() = 0;
    virtual void dumpBuffer();
    virtual void setProtocol(int protocol) { connected_ = true; }
    virtual void closeProtocol() { connected_ = false; }
    virtual void open() { connected_ = false; }
    virtual void close() {}
    virtual void wiringCheck() = 0;
    virtual void sendHeartBeat() {}
    virtual int getProtocol() const = 0;
    virtual void kwDisplay() {}
    bool isConnected() const { return connected_; }
protected:
    static void insertToHistory(const Ecumsg* msg);
    static void appendToHistory(const Ecumsg* msg);
    ProtocolAdapter();
    bool           connected_;
    AdapterConfig* config_;
private:
    const static int HISTORY_LEN = 256;
    const static int ITEM_LEN    = 16;
    static int     historyPos_;
    static uint8_t history_[HISTORY_LEN];
};

#endif //__PROTOCOL_ADAPTER_H__
