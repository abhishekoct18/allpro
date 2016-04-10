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
#include "vpw.h"

using namespace util;

VpwAdapter::VpwAdapter()
{
    driver_ = PwmDriver::instance();
    timer_ = Timer::instance(0);
}

/**
 * VPW adapter init
 */
void VpwAdapter::open()
{
    driver_->open(true);
}

/**
 * On adapter close
 */
void VpwAdapter::close()
{
    connected_ = false;
}

/**
 * Send buffer to ECU using VPW
 * @param[in] msg Message to send
 * @return  1 if success, -1 if arbitration lost, 0 if bus busy
 */
int VpwAdapter::sendToEcu(const Ecumsg* msg)
{
    insertToHistory(msg); // Buffer dump
    
    // Wait for bus to be inactive
    //
    if (!driver_->wait4Ready(TV6_TX_NOM, TV4_TX_MIN, timer_)) {
        return 0;
    }

    TX_LED(true);  // Turn the transmit LED on

    // SOF pulse
    driver_->sendSofVpw(TV3_TX_NOM); // 200us
  
    for (int i = 0; i < msg->length(); i++) {
        uint8_t ch = msg->data()[i];  // sent next byte in buffer
        int bits = 8;

        while (bits--) {  // send each bit in the byte
            if (bits & 0x01) { // Passive, set bus to 0
                if (ch & 0x80)  {
                    driver_->sendPulseVpw(TV2_TX_NOM); // 128us
                    Delay1us(TV2_TX_NOM / 2);
                }
                else {
                    driver_->sendPulseVpw(TV1_TX_NOM); // 64us
                    Delay1us(TV1_TX_NOM / 2);
                }
                if (driver_->getBit()) {
                    driver_->stop();
                    TX_LED(0); // Turn the transmit LED off
                    return -1; // Lost arbitration
                }
            }
            else {  // Active, set bus to 1
                if (ch & 0x80) {
                    driver_->sendPulseVpw(TV1_TX_NOM); // 64us
                }
                else {
                    driver_->sendPulseVpw(TV2_TX_NOM); // 128us
                }
            }
            ch <<= 1;
        }
    }
    
    driver_->sendEodVpw();
    TX_LED(false); // Turn the transmit LED off
    return 1;
}

/**
 * Wait for VPW SOF pulse to arrive
 * @return true if SOF, false if timeout
 * Note: Using timer_ object is for max P2 timeout
 */
bool VpwAdapter::waitForSof()
{
    for (;;) {
        uint32_t val = driver_->wait4Sof(TV3_RX_MAX, timer_);
        if (val == 0xFFFFFFFF) // P2 timer expired
            return false;
        if (val >= TV3_RX_MIN) 
            break;
    }
    return true;
}

/**
 * Receives a sequence of bytes from the VPW ECU
 * @return 0 if timeout, 1 if OK, -1 if bus error
 */
int VpwAdapter::receiveFromEcu(Ecumsg* msg, int maxLen)
{
    uint8_t* ptr = msg->data();
    msg->length(0); // Reset the buffer byte length
    
    // Wait SOF
    if (!waitForSof()) {
        return 0; // Timeout, no SOF
    }

    RX_LED(1); // Turn the receive LED on

    int i;
    for(i = 0; i < maxLen; i++) { // Only retrieve maxLen bytes
        int bits = 8;
        uint8_t ch = 0;
        bool state = false;
        uint32_t pulse = 0;

        while (bits--) {
            ch <<= 1;
            pulse = driver_->wait4BusChangeVpw();
            if (!pulse) {
                goto extr; // EOD Max expired, terminate receive on timeout
            }
            state = !state;
            if (pulse < TV1_RX_MIN) {
                goto exte; // pulse is too short
            }
            if (pulse > TV2_RX_MAX) {
                goto exte; // pulse is too long
            }
            ch |= (pulse > VPW_RX_MID) ? 1 : 0;
        }
        *(ptr++) = ch ^ 0x55;
    }

extr: // ok
    driver_->stop();
    msg->length(i);
    RX_LED(false);        // Turn the receive LED off
    appendToHistory(msg); // Save data for buffer dump
    return 1;

exte: // Invalid pulse width, BUS_ERROR
    driver_->stop();
    RX_LED(false);  // Turn the receive LED off
    return -1;
}

/**
 * VPW request handler
 * @param[in] data command 
 * @param[in] data Data bytes
 * @return The completion status
 */
int VpwAdapter::onRequest(const uint8_t* data, int len)
{
    return requestImpl(data, len, true);
}

/**
 * Will try to send PID0 to query the VPW protocol
 * @param[in] sendReply Send reply flag
 * @return PROT_J1850_VPW if ECU is supporting PWM protocol, 0 otherwise
 */
int VpwAdapter::onConnectEcu(bool sendReply)
{
    uint8_t testSeq[] = { 0x01, 0x00 };

    open();
    int reply = requestImpl(testSeq, sizeof(testSeq), sendReply);

    connected_ = (reply == REPLY_NONE);
    if (!connected_) {
        close(); // Close only if not succeeded
    }
    return connected_ ? PROT_J1850_VPW : 0;
}

int VpwAdapter::requestImpl(const uint8_t* data, int len, bool sendReply)
{
    int p2Timeout = getP2MaxTimeout();
    bool gotReply = false;
    util::string str;

    unique_ptr<Ecumsg> msg(Ecumsg::instance(Ecumsg::VPW));

    msg->setData(data, len);
    msg->addHeaderAndChecksum();

    // Calculate expected 2nd byte reply
    uint8_t expct2ndByte = msg->data()[1] + 1;
    
    // Set the request operation timeout P2
    timer_->start(p2Timeout);
    
    switch (sendToEcu(msg.get())) {
        case 0:               // Bus busy, no SOF found
        case -1:              // Arbitration lost
            return REPLY_BUS_BUSY;
    }

    // Set the reply operation timeout
    timer_->start(p2Timeout);
    do {
        int sts = receiveFromEcu(msg.get(), OBD_IN_MSG_LEN);
        if (sts == -1) { // Bus timing error
            continue;
        }
        if (sts == 0) {  // Timeout
            break;
        }
        if (msg->length() < OBD2_BYTES_MIN || msg->length() > OBD2_BYTES_MAX) {
            continue;
        }
        if (msg->data()[1] != expct2ndByte) { // ignore all replies but expected
            continue;
        }

        // OK, got OBD message, reset Timer
        timer_->start(p2Timeout);

        // Extract the ISO message if option "Send Header" not set
        if (!config_->getBoolProperty(PAR_HEADER_SHOW)) {
            // Was the message OK?
            if (!msg->stripHeaderAndChecksum()) {
                return REPLY_CHKS_ERROR;
            }
        }

        if (sendReply) {
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
int VpwAdapter::getP2MaxTimeout() const
{
    int p2Timeout = config_->getIntProperty(PAR_TIMEOUT);
    return p2Timeout ? p2Timeout : P2_J1850;
}

/**
 * Response for "ATDP"
 */
void VpwAdapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, SAE J1850 VPW" : "SAE J1850 VPW");
}

/**
 * Response for "ATDPN"
 */
void VpwAdapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A2" : "2");
}

/**
 * Test wiring connectivity for VPW
 */
void VpwAdapter::wiringCheck()
{
    open();
    driver_->setBit(1);
    Delay1ms(1);
    if (driver_->getBit() != 1) {
        AdptSendReply("VPW wiring failed [1->0]");
        goto ext;
    }

    driver_->setBit(0);
    Delay1ms(1);
    if (driver_->getBit() == 0) {
        AdptSendReply("VPW wiring is OK");
    }
    else {
        AdptSendReply("VPW wiring failed [0->1]");
    }

ext:
    driver_->setBit(0); // Just in case
    close();
}

//-------------------------------------------------------------------------
//
//   +---------+  +----+    +--+  +-----+  +----+    +--+-----+
//   |         |  |    |    |  |  |     |  |    |    |  |XXXXX|
// --+         +--+    +----+  +--+     +--+    +----+  +-----+-----------
//   |   SOF   | 0|  0 |  1 | 1| 0|   0 | 0|  0 |  1 | 1| ... |   EOD   |
//
//-------------------------------------------------------------------------
