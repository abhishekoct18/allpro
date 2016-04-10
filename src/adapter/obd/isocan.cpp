/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <memory>
#include <adaptertypes.h>
#include <Timer.h>
#include <candriver.h>
#include <led.h>
#include "canmsgbuffer.h"
#include "isocan.h"
#include "canhistory.h"

using namespace std;
using namespace util;

const int ISO_CAN_LEN = 7;

IsoCanAdapter::IsoCanAdapter()
{
    extended_ = false;
    canPriority_ = 0;
    filter_[0] = mask_[0] = 0;
    driver_ = CanDriver::instance();
    history_ = new CanHistory();
}

/**

 * Setting CAN custom filter
 * @param[in] filter The filter bytes
 */
void IsoCanAdapter::setFilter(const uint8_t* filter)
{
    memcpy(filter_, filter, sizeof(filter_));
    setFilterAndMask();
}

/**
 * Setting CAN custom mask
 * @param[in] mask The mask bytes
 */
void IsoCanAdapter::setMask(const uint8_t* mask)
{
    memcpy(mask_, mask, sizeof(mask_));
    setFilterAndMask();
}

/**
 * Send buffer to ECU using CAN
 * @param[in] data The message data bytes
 * @param[in] len The message length
 * @return true if OK, false if data issues
 */
bool IsoCanAdapter::sendToEcu(const uint8_t* data, int len)
{
    if (len > ISO_CAN_LEN) {
        return false; // REPLY_DATA_ERROR
    }
    CanMsgBuffer msgBuffer(getID(), extended_, 8, 0);
    msgBuffer.data[0] = len;
    memcpy(msgBuffer.data + 1, data, len);
    
    // Message log
    history_->add2Buffer(&msgBuffer, true, 0);

    if (!driver_->send(&msgBuffer)) { 
        return false; // REPLY_DATA_ERROR
    }
    return true;
}

/**
 * Format reply for "H1" option
 * @param[in] msg CanMsgbuffer instance pointer
 * @param[out] str The output string
 */
void IsoCanAdapter::formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str)
{
    CanIDToString(msg->id, str, msg->extended);
    bool useSpaces = AdapterConfig::instance()->getBoolProperty(PAR_SPACES);
    bool isDLC = AdapterConfig::instance()->getBoolProperty(PAR_CAN_DLC);
    if (useSpaces) {
        str += ' '; // number of chars + space
    }
    if (isDLC) {
        str += msg->dlc + '0'; // add DLC byte
        if (useSpaces) {
            str += ' ';
        }
    }
    to_ascii(msg->data, 8, str);
}

/**
 * Process first/next/single frames
 * @param[in] msg CanMsgbuffer instance pointer
 */
void IsoCanAdapter::processFrame(const CanMsgBuffer* msg)
{
    util::string str;
    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str);
    }
    else {
        to_ascii(msg->data, 8, str);
    }
    AdptSendReply(str);
}

/**
 * Receives a sequence of bytes from the CAN bus
 * @param[in] sendReply send reply to user flag
 * @return true if message received, false otherwise
 */
bool IsoCanAdapter::receiveFromEcu(bool sendReply)
{
    const int p2Timeout = getP2MaxTimeout();
    CanMsgBuffer msgBuffer;
    bool msgReceived = false;
    
    Timer* timer = Timer::instance(0);
    timer->start(p2Timeout);

    do {
        if (!driver_->isReady())
            continue;
        driver_->read(&msgBuffer);
        
        // Message log
        history_->add2Buffer(&msgBuffer, false, msgBuffer.msgnum);
        
        // Reload the timer
        timer->start(p2Timeout);

        msgReceived = true;
        if (!sendReply)
            continue;
        switch ((msgBuffer.data[0] & 0xF0) >> 4) {
            case CANSingleFrame:
                processFrame(&msgBuffer);
                break;
            case CANFirstFrame:
                processFlowFrame(&msgBuffer);
                processFrame(&msgBuffer);
                break;
            case CANConsecutiveFrame:
                processFrame(&msgBuffer);
                break;
        }
    } while (!timer->isExpired());

    return msgReceived;
}


int IsoCanAdapter::getP2MaxTimeout() const
{
    int p2Timeout = config_->getIntProperty(PAR_TIMEOUT); 
    return p2Timeout ? p2Timeout : CAN_P2_MAX_TIMEOUT;
}

/**
 * Global entry ECU send/receive function
 * @param[in] data The message data bytes
 * @param[in] len The message length
 * @return The completion status code
 */
int IsoCanAdapter::onRequest(const uint8_t* data, int len)
{
    if (!sendToEcu(data, len))
        return REPLY_DATA_ERROR;
    return receiveFromEcu(true) ? REPLY_NONE : REPLY_NO_DATA;
}

/**
 * Will try to send PID0 to query the CAN protocol
 * @param[in] sendReply Reply flag
 * @return Protocol value if ECU is supporting CAN protocol, 0 otherwise
 */
int IsoCanAdapter::onConnectEcu(bool sendReply)
{
    CanMsgBuffer msgBuffer(getID(), extended_, 8, 0x02, 0x01, 0x00);

    open();
    if (driver_->send(&msgBuffer)) { 
        if (receiveFromEcu(sendReply)) {
            connected_ = true;
            return extended_ ? PROT_ISO15765_2950 : PROT_ISO15765_1150;
        }
    }
    close(); // Close only if not succeeded
    return 0;
}

/**
 * Print the messages buffer
 */
void IsoCanAdapter::dumpBuffer()
{
    history_->dumpCurrentBuffer();
}

/**
 * Test wiring connectivity for CAN
 */
void IsoCanAdapter::wiringCheck()
{
    bool failed = false;
    driver_->setBitBang(true);

    driver_->setBit(0);
    Delay1ms(1);
    if (driver_->getBit() != 0) {
        AdptSendReply("CAN wiring failed [0->1]");
        failed = true;
    }        

    driver_->setBit(1);
    Delay1ms(1);
    if (driver_->getBit() != 1) {
        AdptSendReply("CAN wiring failed [1->0]");
        failed = true;
    }    
    
    if (!failed) {
        AdptSendReply("CAN wiring is OK");
    }

    driver_->setBitBang(false);
}

/**
 * IsoCan11Adapter class members
 */
void IsoCan11Adapter::open()
{
    setFilterAndMask();
    
    //Start using LED timer
    AdptLED::instance()->startTimer();
}

uint32_t IsoCan11Adapter::getID() const
{ 
    NumericType id;

    const ByteArray* bytes = config_->getBytesProperty(PAR_WM_HEADER);
    if (bytes->length) {
        const uint8_t* customHdr = bytes->data;
        id.bvalue[1] = customHdr[2] & 0x07;
        id.bvalue[0] = customHdr[3];
    }
    else {
        id.lvalue = 0x7DF;
    }
    return id.lvalue;
}

void IsoCan11Adapter::setFilterAndMask()
{
    // Mask 11 bit
    NumericType mask;

    if (isCustomMask()) {
        mask.bvalue[1] = mask_[3] & 0x07;
        mask.bvalue[0] = mask_[4];
    }
    else { // Default mask for 11 bit CAN
        mask.lvalue = 0x7F8;
    }
    // Filter 11 bit
    NumericType filter;
    if (isCustomFilter()) {
        filter.bvalue[1] = filter_[3] & 0x07;
        filter.bvalue[0] = filter_[4];
    }
    else { // Default filter for 11 bit CAN
        filter.lvalue = 0x7E8;
    }
    // Set mask and filter 11 bit
    driver_->setFilterAndMask(filter.lvalue, mask.lvalue, false);
}

void IsoCan11Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    CanMsgBuffer ctrlData(getID(), false, 8, 0x30, 0x0, 0x00);
    ctrlData.id |= (msg->id & 0x07);
    driver_->send(&ctrlData);
}

void IsoCan11Adapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, ISO 15765-4 (CAN 11/500)" : "ISO 15765-4 (CAN 11/500)");
}

void IsoCan11Adapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A6" : "6"); 
}

/**
 * IsoCan29Adapter class members
 */
void IsoCan29Adapter::open()
{
    setFilterAndMask();
    
    // Start using LED timer
    AdptLED::instance()->startTimer();
}

uint32_t IsoCan29Adapter::getID() const
{ 
    NumericType id;

    const ByteArray* bytes = config_->getBytesProperty(PAR_WM_HEADER);
    if (bytes->length) {
        const uint8_t* customHdr = bytes->data;
        id.bvalue[3] = canPriority_;
        id.bvalue[2] = customHdr[1];
        id.bvalue[1] = customHdr[2];
        id.bvalue[0] = customHdr[3];
    }
    else {
        id.lvalue = 0x18DB33F1;
    }
    return id.lvalue;
}

void IsoCan29Adapter::setFilterAndMask() 
{
    // Mask 29 bit
    NumericType mask;

    if (isCustomMask()) {
        mask.bvalue[3] = mask_[1];
        mask.bvalue[2] = mask_[2];
        mask.bvalue[1] = mask_[3];
        mask.bvalue[0] = mask_[4];
    }
    else { // Default for 29 bit CAN
        mask.lvalue = 0x1FFFFF00;
    }
    // Filter 29 bit
    NumericType filter;
    if (isCustomFilter()) {
        filter.bvalue[3] = filter_[1];
        filter.bvalue[2] = filter_[2];
        filter.bvalue[1] = filter_[3];
        filter.bvalue[0] = filter_[4];
    }
    else { // Default for 29 bit CAN
        filter.lvalue = 0x18DAF100;
    }
    // Set mask and filter 29 bit
    driver_->setFilterAndMask(filter.lvalue, mask.lvalue, true);
}

void IsoCan29Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    CanMsgBuffer ctrlData(getID(), true, 8, 0x30, 0x0, 0x00);
    ctrlData.id |= (msg->id & 0xFF) << 8;
    driver_->send(&ctrlData);
}

void IsoCan29Adapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, ISO 15765-4 (CAN 29/500)" : "ISO 15765-4 (CAN 29/500)");
}

void IsoCan29Adapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A7" : "7"); 
}
