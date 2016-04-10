/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "obdprofile.h"

using namespace util;

//
// Reply error string constants
//
static const char ErrMessage [] = "?";
static const char Err1Message[] = "DATA ERROR";        // Bad ECU response
static const char Err2Message[] = "NO DATA";           // No response
static const char Err3Message[] = "ERROR";             // Got response but invalid
static const char Err4Message[] = "UNABLE TO CONNECT"; // All attempts to connect failed (for auto connect)
static const char Err5Message[] = "FB ERROR";          // Wiring
static const char Err6Message[] = "BUS BUSY";          // Bus collision or busy
static const char Err7Message[] = "BUS ERROR";         // Bus error
static const char Err8Message[] = "DATA ERROR>";       // Checksum
static const char Err0Message[] = "Program Error";     // Wrong coding?


/**
 * Instance accessor
 * @return The OBDProfile instace pointer
 **/
OBDProfile* OBDProfile::instance()
{
    static OBDProfile obdProfile;
    return &obdProfile;
}
/**
 * Construct OBDProfile object
 */
OBDProfile::OBDProfile()
{
    adapter_ = ProtocolAdapter::getAdapter(ADPTR_AUTO);
}

/**
 * Proxies....
 */
void OBDProfile::getProtocolDescription() const
{
    adapter_->getDescription();
}

void OBDProfile::getProtocolDescriptionNum() const
{
    adapter_->getDescriptionNum();
}

void OBDProfile::dumpBuffer()
{
    adapter_->dumpBuffer();
}

/**
 * Set the protocol number
 * @param[in] num The protocol number
 * @param[in] refreshConnection The flag
 * @return The completion status
 */
int OBDProfile::setProtocol(int num, bool refreshConnection)
{
    ProtocolAdapter* pvadapter = adapter_;
    switch (num) {
        case PROT_AUTO:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_AUTO);
            ProtocolAdapter::getAdapter(ADPTR_ISO)->setProtocol(PROT_AUTO);
            break;
        case PROT_J1850_PWM:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_PWM);
            break;
        case PROT_J1850_VPW:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_VPW);
            break;
        case PROT_ISO9141:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_ISO);
            if (refreshConnection)
                adapter_->setProtocol(PROT_ISO9141);
            break;
        case PROT_ISO14230_5BPS:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_ISO);
            if (refreshConnection)
                adapter_->setProtocol(PROT_ISO14230_5BPS);
            break;
        case PROT_ISO14230:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_ISO);
            if (refreshConnection)
                adapter_->setProtocol(PROT_ISO14230);
            break;
        case PROT_ISO15765_1150:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_CAN);
            break;
        case PROT_ISO15765_2950:
            adapter_ = ProtocolAdapter::getAdapter(ADPTR_CAN_EXT);
            break;
        default:
            return REPLY_CMD_WRONG;
    }
    // Do this if only "ATSP" executed
    if (refreshConnection) {
        if (pvadapter != adapter_) { 
            pvadapter->close();
            adapter_->open();
        }
    }
    return REPLY_OK;
}

/**
 * The entry for ECU send/receive function
 * @param[in] cmdString The command
 * @return The status code
 */
void OBDProfile::onRequest(const string& cmdString)
{
    int result = onRequestImpl(cmdString);
    switch(result) {
        case REPLY_CMD_WRONG:
            AdptSendReply(ErrMessage);
            break;
        case REPLY_DATA_ERROR:
            AdptSendReply(Err1Message);
            break;
        case REPLY_NO_DATA:
            AdptSendReply(Err2Message);
            break;
        case REPLY_ERROR:
            AdptSendReply(Err3Message);
            break;
        case REPLY_UNBL_2_CNNCT:
            AdptSendReply(Err4Message);
            break;
        case REPLY_BUS_BUSY:
            AdptSendReply(Err6Message);
            break;        
        case REPLY_BUS_ERROR:
            AdptSendReply(Err7Message);
            break;
        case REPLY_CHKS_ERROR:
            AdptSendReply(Err8Message);
            break;
        case REPLY_WIRING_ERROR:
            AdptSendReply(Err5Message);
            break;        
        case REPLY_NONE:
            break;
        default:
            AdptSendReply(Err0Message);
            break;
    }
}    

/**
 * The actual implementation of request handler
 * @param[in] cmdString The command
 * @return The status code
 */
int OBDProfile::onRequestImpl(const string& cmdString)
{
    const char* OBD_TEST_SEQ = "0100";
    uint8_t data[OBD_IN_MSG_LEN];

    // Buffer overrun check,
    // should be less then (11 * 2) => 22 characters
    if (cmdString.length() > (sizeof(data) * 2)) {
        return REPLY_CMD_WRONG;
    }

    int len = to_bytes(cmdString, data);

    // Valid request length?
    if (!sendLengthCheck(data, len)) {
        return REPLY_DATA_ERROR;
    }

    // The regular flow stops here
    if (adapter_->isConnected()) {
        return adapter_->onRequest(data, len);
    } 

    // The convoluted logic
    //
    bool sendReply = (cmdString == OBD_TEST_SEQ);
    
    int protocol = 0;
    int sts = REPLY_NO_DATA;
    ProtocolAdapter* autoAdapter = ProtocolAdapter::getAdapter(ADPTR_AUTO);
    if (adapter_ == autoAdapter) {
        protocol = autoAdapter->onConnectEcu(sendReply);
    }
    else {
        protocol = adapter_->onConnectEcu(sendReply);
        bool useAutoSP = AdapterConfig::instance()->getBoolProperty(PAR_USE_AUTO_SP);
        if (protocol == 0 && useAutoSP) {
            protocol = autoAdapter->onConnectEcu(sendReply);
        }
    }
    if (protocol) {
        setProtocol(protocol, false);
        if (!sendReply || (protocol >= PROT_ISO9141 && protocol <= PROT_ISO14230)) {
            sts = adapter_->onRequest(data, len);
        }
        else {
            sts = REPLY_NONE; //the command sent already as part of autoconnect
        }
    }
    return sts;
}

/**
 * Check the maximum length for OBD request
 * @param[in] msg The request bytes
 * @param[in] len The request length
 * @return true if set, false otherwise
 */
bool OBDProfile::sendLengthCheck(const uint8_t* msg, int len)
{
    int maxLen = OBD_IN_MSG_DLEN;
    
    // For KWP use maxlen=8
    if (adapter_ ==  ProtocolAdapter::getAdapter(ADPTR_ISO)) {
        maxLen++;
    }

    if ((len == 0) ||len > maxLen) {
        return false;
    }
    return true;
}

int OBDProfile::getProtocol() const
{
    return adapter_->getProtocol();
}

void OBDProfile::closeProtocol()
{
    adapter_->closeProtocol();
}

/**
 * ISO KW1,KW2 display, applies only to 9141/14230 adapter
 */
int OBDProfile::kwDisplay()
{
    ProtocolAdapter::getAdapter(ADPTR_ISO)->kwDisplay();
    return REPLY_OK;
}

/**
 * ISO 9141/14230 hearbeat, implemented only in ISO serial adapter
 */
void OBDProfile::sendHeartBeat()
{
    adapter_->sendHeartBeat();
}

/**
 * Test wiring connectivity for all protocols
 */
void OBDProfile::wiringCheck()
{
    ProtocolAdapter::getAdapter(ADPTR_PWM)->wiringCheck();
    ProtocolAdapter::getAdapter(ADPTR_VPW)->wiringCheck();
    ProtocolAdapter::getAdapter(ADPTR_ISO)->wiringCheck();
    ProtocolAdapter::getAdapter(ADPTR_CAN)->wiringCheck();
}

/**
 * Constructs ProtocolAdater
 */
ProtocolAdapter::ProtocolAdapter()
{ 
    connected_ = false;
    config_ = AdapterConfig::instance();
}
