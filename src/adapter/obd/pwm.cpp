/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <memory>
#include <adaptertypes.h>
#include <GpioDrv.h>
#include <Timer.h>
#include <PwmDriver.h>
#include "j1850.h"
#include "pwm.h"

using namespace util;
    
PwmAdapter::PwmAdapter()
{
    driver_ = PwmDriver::instance();
    timer_ = Timer::instance(0);
}

/**
 * PWM adapter init
 */
void PwmAdapter::open()
{
    driver_->open(false);
}

/**
 * On adapter close
 */
void PwmAdapter::close()
{
    connected_ = false;
}

/**
 * Send a byte over PWM
 * @param[in] val Byte to send
 * @returns true if OK, false if arbitration was lost
 */
bool PwmAdapter::sendByte(uint8_t val)
{
    for (int i = 0; i < 8; i++) {
        if (val & 0x80) {
            // Send "1"
            driver_->sendPulsePwm(TP1_TX_NOM, TP3_TX_NOM-TP1_TX_NOM);
        }
        else {
            // Send "0"
            driver_->sendPulsePwm(TP2_TX_NOM, TP3_TX_NOM-TP2_TX_NOM);
        }
        //if (driver_->getBit()) {
        //    return false; // Arbitration lost
        //}
        val = val << 1;
    }
    return true;
}

/**
 * Send PWM SOF pulse
 */
void PwmAdapter::sendSof()
{
    driver_->stop();
    driver_->sendPulsePwm(TP7_TX_NOM, TP4_TX_NOM-TP7_TX_NOM);
}

/**
 * Receive a byte over PWM
 * @param[out] val Byte to receive
 * @return 1 if success, 0 if timeout, -1 if timing error
 */
int PwmAdapter::receiveByte(uint8_t& val)
{
    val = 0;
    for (int i = 0; i < 8; i++) {
        val = val << 1;
        uint32_t pulse = driver_->wait4BusPulsePwm();
        if (pulse == 0)
            return 0;  // Timeout on TP3_RX_MAX
        else if (pulse > TP2_RX_MAX)
            return -1; // Timing error
        if (pulse <= TP2_RX_MIN) {
            val = val | 0x01;
        }            
    }
    return 1;
}

int PwmAdapter::getIfr()
{
    uint8_t ifr = 0;
    driver_->setTimeoutPwm(TP4_RX_MAX);
    return receiveByte(ifr);
}

/**
 * Send buffer to ECU using PWM
 * @param[in] msg Message to send
 * @return  1 if success, -1 if arbitration lost or bus busy, 0 if error getting IFR
 */
int PwmAdapter::sendToEcu(const Ecumsg* msg)
{
    // For buffer dump
    insertToHistory(msg);

    // Wait for bus to be inactive
    //
    if(!driver_->wait4Ready(TP5_TX_MIN, TP6_TX_NOM, timer_)) {
        driver_->stop();
        return -1; // Bus busy
    }
    
    TX_LED(true);  // Turn the transmit LED on
    
    sendSof();
    
    for (int i = 0; i < msg->length(); i++) {
        if (!sendByte(msg->data()[i])) {
            TX_LED(false); // Turn the transmit LED off
            driver_->stop();
            return -1;     // Lost arbitration
        }
    }
    
    driver_->sendEodPwm();
    TX_LED(false); // Turn the transmit LED off

    // Get IFR byte
    int ifrStatus = getIfr();
    return ifrStatus;
}

/**
 * Run as long as SOF active
 * @return 0 if timeout, 1 if success, -1 bus error
 */
bool PwmAdapter::waitForSof()
{
    for (;;) {
        uint32_t val = driver_->wait4Sof(TP7_RX_MAX, timer_);
        if (val == 0xFFFFFFFF) // P2 timer expired
            return false;
        if (val >= TP7_RX_MIN) // val >= TP7_RX_MIN && <= TP7_RX_MAX
            break;
    }
    return true;
}

void PwmAdapter::sendIfr()
{
    Delay1us(15);
    sendByte(0xF1);
}

/**
 * Receives a sequence of bytes from the VPW ECU
 * @param[out] msg Message to receive
 * @param[int] maxlen Maximum message length
 * @return 0 if timeout, 1 if OK, -1 if bus error
 */
int PwmAdapter::receiveFromEcu(Ecumsg* msg, int maxLen)
{
    msg->length(0); // Reset the buffer byte length
    
    // Wait SOF
    if (!waitForSof())
        return 0;

    RX_LED(1); // Turn the receive LED on

    int i;
    uint8_t len = 0;
    driver_->setTimeoutPwm(TP3_RX_MAX);
    
    for(i = 0; i < maxLen; i++) { // Only retrieve maxLen bytes
        int sts = receiveByte(msg->data()[i]);
        if (sts == 0)
            break;
        if (sts < 0)
            goto exte; // Timing issue
        len++;
    }
    driver_->stop();
    
    sendIfr();
    driver_->stop();

    msg->length(i);
    RX_LED(false);        // Turn the receive LED off
    appendToHistory(msg); // Save data for buffer dump
    return 1;

exte: // Invalid pulse width, BUS_ERROR
    driver_->stop();
    RX_LED(false);        // Turn the receive LED off
    return -1;
}

/**
 * PWM request handler
 * @param[in] data command 
 * @param[in] data Data bytes
 * @return The completion status
 */
int PwmAdapter::onRequest(const uint8_t* data, int len)
{
    return requestImpl(data, len, true);
}

/**
 * Will try to send PID0 to query the PWM protocol
 * @param[in] sendReply Send reply flag
 * @return PROT_J1850_PWM if ECU is supporting PWM protocol, 0 otherwise
 */
int PwmAdapter::onConnectEcu(bool sendReply)
{
    uint8_t testSeq[] = { 0x01, 0x00 };

    open();
    int reply = requestImpl(testSeq, sizeof(testSeq), sendReply);

    connected_ = (reply == REPLY_NONE);
    if (!connected_) {
        close(); // Close only if not succeeded
    }
    return connected_ ? PROT_J1850_PWM : 0;
}

int PwmAdapter::requestImpl(const uint8_t* data, int len, bool sendReply)
{
    int p2Timeout = getP2MaxTimeout();
    bool gotReply = false;
    util::string str;
    
    unique_ptr<Ecumsg> msg(Ecumsg::instance(Ecumsg::PWM));
    
    msg->setData(data, len);
    msg->addHeaderAndChecksum();

    // Calculate expected 2nd byte reply 
    uint8_t expct2ndByte = msg->data()[1] + 1;
    
    // Set the request operation timeout P2
    timer_->start(p2Timeout);
    switch (sendToEcu(msg.get())) {
        case 0:               // Bus idle, no IFR received
            return REPLY_NO_DATA;
        case -1:              // Arbitration lost
            return REPLY_BUS_BUSY;
    }

    // Set the reply operation timeout
    timer_->start(p2Timeout);
    do {
        int sts = receiveFromEcu(msg.get(), OBD_IN_MSG_LEN); 
        if (sts == -1) { // Bus timing error
            return REPLY_BUS_ERROR;
        }    
        if (sts == 0) {  // Timeout
            break;
        }        
        if (msg->length() < OBD2_BYTES_MIN || msg->length() > OBD2_BYTES_MAX) {
            continue;
        }
        if (msg->data()[1] != expct2ndByte) { // Ignore all replies but expected
            continue;
        }
        
        // OK, got OBD message, reset timer
        timer_->start(p2Timeout);
        
        // Extract the ISO message if option "Send Header" not set
        if (!config_->getBoolProperty(PAR_HEADER_SHOW)) {
            // Was the message OK?
            if (!msg->stripHeaderAndChecksum()) {
                return REPLY_CHKS_ERROR;
            }
        }

        if (sendReply && msg->length() > 0) {
            msg->toString(str);
            AdptSendReply(str);
        }
        str.resize(0);
        gotReply = true;
    } while(!timer_->isExpired());
        
    // Reply
    return gotReply ? REPLY_NONE : REPLY_NO_DATA;
}

/**
 * Use either ISO default or custom value
 * @return Timeout value
 */
int PwmAdapter::getP2MaxTimeout() const
{
    int p2Timeout = config_->getIntProperty(PAR_TIMEOUT);
    return p2Timeout ? p2Timeout : P2_J1850;
}

/**
 * Response for "ATDP"
 */
void PwmAdapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, SAE J1850 PWM" : "SAE J1850 PWM");
}

/**
 * Response for "ATDPN"
 */
void PwmAdapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A1" : "1");
}

/**
 * Test wiring connectivity for PWM
 */
void PwmAdapter::wiringCheck()
{    
    open();
    driver_->setBit(1);
    Delay1ms(1);
    if (driver_->getBit() != 1) {
        AdptSendReply("PWM wiring failed [1->0]");
        goto ext;
    }

    driver_->setBit(0);
    Delay1ms(1);
    if (driver_->getBit() == 0) {
        AdptSendReply("PWM wiring is OK");
    }
    else {
        AdptSendReply("PWM wiring failed [0->1]");
    }

ext:
    driver_->setBit(0);
    close();
}
