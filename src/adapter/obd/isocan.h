//
// IsoCanAdapter structures and definitions
//

#ifndef __ISO_CAN_H__
#define __ISO_CAN_H__

#include "padapter.h"

const int CAN_P2_MAX_TIMEOUT = 50;

class CanDriver;
class CanHistory;
struct CanMsgBuffer;

class IsoCanAdapter : public ProtocolAdapter {
public:
    static const int CANSingleFrame      = 0;
    static const int CANFirstFrame       = 1;
    static const int CANConsecutiveFrame = 2;
    static const int CANFlowControlFrame = 3;
public:
    virtual int onRequest(const uint8_t* data, int len);
    virtual int onConnectEcu(bool sendReply);
    virtual void setFilter(const uint8_t* filter);
    virtual void setMask(const uint8_t* mask);
    virtual void setCanCAF(bool val) {}
    virtual void setPriorityByte(uint8_t val) { canPriority_ = val; }
    virtual void wiringCheck();
    virtual void dumpBuffer();
protected:
    IsoCanAdapter();
    virtual uint32_t getID() const = 0;
    virtual void setFilterAndMask() = 0;
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer) = 0;
    bool sendToEcu(const uint8_t* data, int len);
    bool receiveFromEcu(bool sendReply);
    bool isCustomMask() const { return mask_[0] != 0; }
    bool isCustomFilter() const { return filter_[0] != 0; }
    void processFrame(const CanMsgBuffer* msg);
    void formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str);
    int getP2MaxTimeout() const;
    //
    CanDriver*  driver_;
    CanHistory* history_;
    bool        extended_;
    uint8_t     canPriority_;
    uint8_t     filter_[5];    // 4 bytes + length
    uint8_t     mask_[5];      // 4 bytes + length
};

class IsoCan11Adapter : public IsoCanAdapter {
public:
    IsoCan11Adapter() {}
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void setFilterAndMask();
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer);
    virtual int getProtocol() const { return PROT_ISO15765_1150; }
    virtual void open();
private:
};

class IsoCan29Adapter : public IsoCanAdapter {
public:
    IsoCan29Adapter() { extended_ = true; }
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void setFilterAndMask();
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer);
    virtual int getProtocol() const { return PROT_ISO15765_2950; }
    virtual void open();
private:
};

#endif //__ISO_CAN_H__
