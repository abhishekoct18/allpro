/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ISO_SERIAL_H__ 
#define __ISO_SERIAL_H__

#include "padapter.h"


static const int ECU_SPEED = 10400;

class EcuUart;
class Timer;
class LongTimer;

class IsoSerialAdapter : public ProtocolAdapter {
public:
    friend class ProtocolAdapter;
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual int onRequest(const uint8_t* data, int len);
    virtual int onConnectEcu(bool sendReply);
    virtual void open();
    virtual void close();
    virtual void wiringCheck();
    virtual void setProtocol(int protocol);
    virtual void closeProtocol();
    virtual void sendHeartBeat();
    virtual int getProtocol() const { return protocol_; }
    virtual void kwDisplay();
private:
    IsoSerialAdapter();
    bool ecuSlowInit();
    bool ecuFastInit();
    void setKeepAlive();
    void checkP3Timeout();
    bool isKeepAlive();
    bool sendToEcu(const Ecumsg* msg, int p4Timeout);
    void receiveFromEcu(Ecumsg* msg, int maxLen, int p2Timeout, int p1Timeout);
    void configureProperties();
    int  onConnectEcuSlow(int protocol);
    int  onConnectEcuFast(int protocol);
    int  getP2MaxTimeout() const;
    int  get2MaxLen() const;

    bool     kwCheck_;
    uint8_t  customWkpMsg_[6];
    uint8_t  isoKwrds_[2];
    int      protocol_;
    uint8_t  isoInitByte_;
    uint32_t wakeupTime_;
    EcuUart* uart_;
    LongTimer* keepAliveTimer_;
    Timer*   p3Timer_;
};

#endif //__ISO_SERIAL_H__
