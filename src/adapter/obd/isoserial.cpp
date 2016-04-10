/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <memory>
#include <adaptertypes.h>
#include <GpioDrv.h>
#include <Timer.h>
#include <EcuUart.h>
#include "j1979.h"
#include "isoserial.h"

using namespace std;
using namespace util;

static const uint8_t Iso14230Seq[]    = { 0x81 };
static const uint8_t Iso9141Wakeup[]  = { 0x01, 0x00 };
static const uint8_t Iso14230Wakeup[] = { 0x3E };

#define __BELLS_AND_WHISTLES__
#ifdef __BELLS_AND_WHISTLES__
static const char BusInit[] = "BUSINIT: ";
static const char EchoDot[] = ".";
static const char EchoOk[]  = "OK";
#endif

/**
 * Check IS014230 KB1 for supported types
 * @param[in] kb1 Keyword byte one
 * @return true if the header type supported, false - otherwise
 */
static bool CheckIso14230Header(uint8_t kb1) 
{
    // Analyze the header type
    //
    switch (kb1 & 0x0F) {
        case 0x5: //ISO14230_H_TYPE1
        case 0x7:
            return false;
        case 0x6: //ISO14230_H_TYPE2 
            return false;
        case 0x9: //Those are the only ones will do
        case 0xb:
        case 0xd:
        case 0xf:
            return true;
        case 0xa: //ISO14230_H_TYPE4
        case 0xe:
            return false;
        default:
            return false;
    }
}

/**
 * Check Timer to produce P3 timeout between conseqent requests to avoid "NO DATA" error
 */
void IsoSerialAdapter::checkP3Timeout()
{
    while (!p3Timer_->isExpired()) ;
}

/**
 * Reset the keepalive
 */
void IsoSerialAdapter::setKeepAlive() 
{
    if (wakeupTime_) { // If keepalive enabled?
        keepAliveTimer_->start(wakeupTime_);
    }
    p3Timer_->start(P3_MIN_TIMEOUT);
}

bool IsoSerialAdapter::isKeepAlive()
{
    return connected_ && wakeupTime_ && keepAliveTimer_->isExpired();
}
    
/**
 * Constructor, init some members
 */
IsoSerialAdapter::IsoSerialAdapter()
{
    protocol_          = PROT_AUTO;
    uart_              = EcuUart::instance();
    keepAliveTimer_    = LongTimer::instance();
    p3Timer_           = Timer::instance(1);
}

/**
 * Actions on opening ISO serial adapter
 */
void IsoSerialAdapter::open()
{
    uart_->init(ECU_SPEED);
}

/**
 * Actions on closing protocol
 */
void IsoSerialAdapter::closeProtocol()
{ 
    connected_ = false;
}

/**
 * Actions on closing ...
 */

void IsoSerialAdapter::close()
{
    closeProtocol();
}

/**
 * Set the protocol
 * @param[in] protocol The protocol number
 */
void IsoSerialAdapter::setProtocol(int protocol)
{ 
    protocol_ = protocol;
}

/**
 * Transmits a sequence of bytes to the ECU 
 * @param[in] msg Ecumsg instance
 * @param[in] p4timeout The P4 timeout
 * @return true if ok, false if wiring problems
 */
bool IsoSerialAdapter::sendToEcu(const Ecumsg* msg, int p4Timeout)
{
    insertToHistory(msg); // Buffer dump
    
    TX_LED(1); // Turn the transmit LED on

    for (int i = 0; i < msg->length(); i++) {
        uart_->send(msg->data()[i]);
        
        // Get the echo byte, it takes 1.2ms
        if (!uart_->getEcho(msg->data()[i])) {
            TX_LED(0); // Turn the transmit LED off
            return false;
        }    
        // Interbyte delay <P4 = [5-20ms]>
        if (i > 0)
            Delay1ms(p4Timeout);
    } 

    TX_LED(0); // Turn the transmit LED off
    return true;
}

/**
 * Receives a sequence of bytes from the ECU until timeout expired or 
 * the maximum number of bytes received
 * @param[in] msg Ecumsg instance
 * @param[in] maxLen The maximum bytes to receive
 * @param[in] p2timeout The P2 timeout
 * @param[in] p1timeout The P1 timeout
 */
void IsoSerialAdapter::receiveFromEcu(Ecumsg* msg, int maxLen, int p2Timeout, int p1Timeout)
{
    msg->length(0);
    
    // Set the req/reply operation timeout
    Timer* timer = Timer::instance(1);
    timer->start(p2Timeout);
    
    int i = 0;
    for(; i < maxLen; i++) { // Only retrieve maxLen bytes
        // Do we have Rx overrun issue?
        uart_->clear();
        
        // Wait for data to be received
        while(!uart_->ready()) {
            if (timer->isExpired())
                goto extm; // exit by timeout
        }

        (*msg) += uart_->get();
        RX_LED(1); // Turn the receive LED on

        // Reload the timer with p1Timeout
        timer->start(p1Timeout);
    }
extm:
    RX_LED(0); // Turn the receive LED off
    appendToHistory(msg); // Buffer dump
}

/**
 * Performs slow 5bps ISO9141 init
 * @return true if OK, false if wiring error
 */
bool IsoSerialAdapter::ecuSlowInit() 
{
    // Sending 0x33 at 5bps
    const int BIT_INTERVAL = 200; // 200ms
    bool sts = true;

    TX_LED(1); // Turn the transmit LED on

    // Disable USART
    uart_->setBitBang(true);

    uint16_t ch = isoInitByte_;
    ch <<= 1;    // Shift to accommodate start bit
    ch |= 0x200; // Add stop bit

    for (int i = 0; i < 10; i++) {
        uint8_t val = (ch & 0x01);
        uart_->setBit(val);
        Delay1ms(BIT_INTERVAL);
        ch >>= 1;
    }

    TX_LED(0); // Turn the transmit LED off

    // Get the feedback status, the last bit (stop bit)
    if (!uart_->getBit()) {
        sts = false;   // Wiring error, no +12V power?
    }
    
    // Enable USART
    uart_->setBitBang(false);
    
    // Do we have Rx overrun issue?
    uart_->clear();
    
    // Clear Rx FIFO
    uart_->clearRxFifo();

    return sts;
}

/**
 * Performs fast ISO14230 init
 * @return true if OK, false if wiring error
 */
bool IsoSerialAdapter::ecuFastInit()
{
    const int TWuP_INTERVAL = 25; // 25ms
    bool sts = true;
   
    TX_LED(1); // Turn the transmit LED on

    // Disable USART
    uart_->setBitBang(true);

    uart_->setBit(0);
    Delay1ms(TWuP_INTERVAL);

    uart_->setBit(1);
    Delay1ms(TWuP_INTERVAL);
    
    TX_LED(0); // Turn the transmit LED off

    // Get the feedback status
    if (!uart_->getBit()) {
        sts = false;   // Wiring error, no +12V power?
    }
    
    // Enable USART
    uart_->setBitBang(false);
    
    // Do we have overrun issue?
    uart_->clear();
    
    // Clear Rx FIFO
    uart_->clearRxFifo();
    
    return sts;
}

/**
 * Connect to Ecu using slow init protocol (ISO9141)
 * @param[in] protocol The protocol number
 * @return The connection status
 */
int IsoSerialAdapter::onConnectEcuSlow(int protocol)
{
    uint8_t kb1, kb2;
    unique_ptr<Ecumsg> msg(Ecumsg::instance(Ecumsg::ISO9141));

    connected_ = false;

#ifdef __BELLS_AND_WHISTLES__
    if (protocol != PROT_AUTO) {
        AdptSendString(BusInit);
    }
#endif

    // Send 0x33 at 5 bit/s on "K" & "L" lines
    // And switch back to 10400 bit/s
    if (!ecuSlowInit())
        return REPLY_WIRING_ERROR;

#ifdef __BELLS_AND_WHISTLES__
    // Send the first "."
    if (protocol != PROT_AUTO) {
        AdptSendString(EchoDot);
    }
#endif
    
    // Should be reply within <W1 = [60-300ms]>
    // either "0x55 0x08 0x08" or "0x55 0x94 0x94"
    receiveFromEcu(msg.get(), 3, W1_MAX_TIMEOUT, W3_TIMEOUT);
    if (msg->length() == 0) {
        return REPLY_ERROR;
    }     
    
    if (msg->length() != 3 || msg->data()[0] != 0x55) {
        return REPLY_DATA_ERROR;
    }    

    // Save keywords
    isoKwrds_[0] = kb1 = msg->data()[1];
    isoKwrds_[1] = kb2 = msg->data()[2];

#ifdef __BELLS_AND_WHISTLES__
    // Send the second "."
    if (protocol != PROT_AUTO) {
        AdptSendString(EchoDot);
    }
#endif

    // Wait W4 = [25-50ms]
    Delay1ms(W4_TIMEOUT);

    // Send inverted last byte
    msg->data()[0] = ~msg->data()[2]; 
    msg->length(1);
    if (!sendToEcu(msg.get(), P4_TIMEOUT))
        return REPLY_WIRING_ERROR;

    // Should wait for "0xcc" and use timeout <W4 = [25-50ms]>
    // Note: providing 2-nd timeout as placeholder, its not being used as we are waiting for one byte
    receiveFromEcu(msg.get(), 1, W4_MAX_TIMEOUT, W3_TIMEOUT); 
    if (msg->length() == 0) {
        return REPLY_ERROR;
    }        
    
    uint8_t invAddrByte = ~isoInitByte_; // 0xCC
    if (msg->length() != 1 || msg->data()[0] != invAddrByte) { 
        return REPLY_DATA_ERROR;
    }    

#ifdef __BELLS_AND_WHISTLES__
    // Send The third "."
    if (protocol != PROT_AUTO) {
        AdptSendString(EchoDot);
    }
#endif

    bool autoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);

    if (!kwCheck_) { // Not checking keywords
        if (protocol == PROT_ISO9141 || protocol == PROT_AUTO || autoSP) {
            connected_ = true;
            protocol_ = PROT_ISO9141;
        }
        else {// PROT_ISO14230_5BPS
            connected_ = true;
            protocol_ = PROT_ISO14230_5BPS;
        }
     }
     else if (kb2 == 0x08 || kb2 == 0x94) { // ISO9141
         if (protocol == PROT_ISO9141 || protocol == PROT_AUTO || autoSP) {
            connected_ = true;
            protocol_ = PROT_ISO9141;
         }
     }
     else if (kb2 == 0x8F) { // ISO14230
         if (CheckIso14230Header(kb1)) {
             if (protocol == PROT_ISO14230_5BPS || protocol == PROT_AUTO || autoSP) {
                 connected_ = true;
                 protocol_ = PROT_ISO14230_5BPS;
             }
         }
    }

    setKeepAlive();
    
    return connected_ ? REPLY_OK : REPLY_ERROR;
}

/**
 * Connect to Ecu using fast init protocol (ISO14230)
 * @param[in] protocol The protocol number
 * @return The connection status
 */
int IsoSerialAdapter::onConnectEcuFast(int protocol) 
{
    uint8_t kb1;
    int sts = REPLY_ERROR;
    const int p2Timeout = getP2MaxTimeout();
    unique_ptr<Ecumsg> msg(Ecumsg::instance(Ecumsg::ISO14230));

    connected_ = false;

#ifdef __BELLS_AND_WHISTLES__
    if (protocol != PROT_AUTO) {
        AdptSendString(BusInit);
    }
#endif

    // Send wakeup pattern at 10400 bit/s
    if (!ecuFastInit())
        return REPLY_WIRING_ERROR;

    msg->setData(Iso14230Seq, sizeof(Iso14230Seq));
    msg->addHeaderAndChecksum();
    if (!sendToEcu(msg.get(), P4_TIMEOUT))
        return REPLY_WIRING_ERROR;

    for (;;) {
        // maxlen -> (3hdr bytes + 3data bytes + checksum) => 7
        receiveFromEcu(msg.get(), 7, p2Timeout, P1_MAX_TIMEOUT); 
        if (msg->length() == 0)
            break;
        if (connected_)
            continue; // Already connected, just get the rest of replies
        if (msg->length() < 5) {
            sts = REPLY_DATA_ERROR;
            continue;
        }    

        // Calculate the position of reply status
        int n;
        if (msg->data()[0] == 0) // no address, add lenght byte
            n = 2;
        else if ((msg->data()[0] & 0xC0) == 0) // 1 byte header
            n = 1;
        else if ((msg->data()[0] & 0x3F) != 0) // length in format byte
            n = 3;
        else // add length byte
            n = 4;

        // Extract the return status
        if (msg->data()[n] != 0xC1) {
            sts = REPLY_DATA_ERROR;
            continue;
        }             
        
        // Save keywords
        isoKwrds_[0] = kb1 = msg->data()[n+1];
        isoKwrds_[1] = msg->data()[n+2];

        if (kwCheck_) {
            if (CheckIso14230Header(kb1)) {
                connected_ = true;
                protocol_ = PROT_ISO14230;
                sts = REPLY_OK;
            }
            else {
                sts = REPLY_DATA_ERROR;
            }
        }        
        else { // Not checking keyword bytes
            connected_ = true;
            protocol_ = PROT_ISO14230;
            sts = REPLY_OK;
        }
    }
    
    setKeepAlive();
    
    return sts;
}

/**
 * Send wakeup sequence to the ECU
 */
void IsoSerialAdapter::sendHeartBeat()
{ 
    if (!isKeepAlive()) {
        return;
    }

    const uint32_t p2Timeout = getP2MaxTimeout();
    uint8_t msgtype = (protocol_ == PROT_ISO14230) ?  Ecumsg::ISO14230 : Ecumsg::ISO9141;
    unique_ptr<Ecumsg> msg(Ecumsg::instance(msgtype));
    
    if (customWkpMsg_[0]) { // Use custom wakeup seq
        msg->setData(customWkpMsg_ + 1, customWkpMsg_[0]);
    }
    else {
        if (protocol_ == PROT_ISO9141) {
            msg->setData(Iso9141Wakeup, sizeof(Iso9141Wakeup));
        }
        else {        
            msg->setData(Iso14230Wakeup, sizeof(Iso14230Wakeup));
        }
        msg->addHeaderAndChecksum();
    }

    int numReplies = 0;
    if (!sendToEcu(msg.get(), P4_TIMEOUT)) {
        close(); // Beat failed
        return;
    }

    // Wait for multiply replies
    for (int i = 0; ; i++) {            
        receiveFromEcu(msg.get(), OBD_IN_MSG_LEN, p2Timeout, P1_MAX_TIMEOUT);
        if (msg->length() > 0) {
            numReplies++;
        }
        else { // Timeout
            break;
        }
    }         
    
    if (numReplies == 0) {
        close(); // Beat failed
    }
    else {
        setKeepAlive(); // Start measuring P3 timeout again
    }
}

/**
 * ISO serial request handler
 * @param[in] data Data bytes
 * @param[in] len Data length
 * @return The completion status
 */
int IsoSerialAdapter::onRequest(const uint8_t* data, int len)
{ 
    bool reply = false;
    const int p2Timeout = getP2MaxTimeout();
    const int maxLen = get2MaxLen();
    util::string str;
    
    uint8_t msgtype = (protocol_ == PROT_ISO14230) ? Ecumsg::ISO14230 : Ecumsg::ISO9141;
    unique_ptr<Ecumsg> msg(Ecumsg::instance(msgtype));
    
    msg->setData(data, len);
    msg->addHeaderAndChecksum();

    // Ready to send it.. but how about P3 timeout?
    checkP3Timeout();
    
    if (!sendToEcu(msg.get(), P4_TIMEOUT)) {
        return REPLY_WIRING_ERROR;
    }

    // Wait for multiple replies
    for (int i = 0; ; i++) {
        receiveFromEcu(msg.get(), maxLen, p2Timeout, P1_MAX_TIMEOUT); 
        if (msg->length() == 0)
            break;
        if (msg->length() < 5)
            return REPLY_DATA_ERROR;
            
        reply = true; // Mark that we have received reply
        
        // Strip the message header/checksum if option "Send Header" is not set
        if (!config_->getBoolProperty(PAR_HEADER_SHOW)) {
            // Was the message OK?
            if (!msg->stripHeaderAndChecksum()) {
                return REPLY_CHKS_ERROR;
            }
        }

        msg->toString(str);
        AdptSendReply(str); 
        str.resize(0);
    }

    setKeepAlive();
    
    // Do we have at least one reply?    
    if (!reply) {
        return REPLY_NO_DATA;
    }

    return REPLY_NONE;
}

/**
 * Response for "ATDP"
 */
void IsoSerialAdapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    switch (protocol_) {
        case PROT_ISO9141:
            AdptSendReply(useAutoSP ? "AUTO, ISO 9141-2" : "ISO 9141-2");
            break;
        case PROT_ISO14230_5BPS:
            AdptSendReply(useAutoSP ? "AUTO, ISO 14230-4 (KWP 5BAUD)" : "ISO 14230-4 (KWP 5BAUD)");
            break;
        case PROT_ISO14230:
            AdptSendReply(useAutoSP ? "AUTO, ISO 14230-4 (KWP FAST)" : "ISO 14230-4 (KWP FAST)");
            break;
    }
}

/**
 * Display KW1 and KW2
 */
void IsoSerialAdapter::kwDisplay()
{
    util::string str;
    KWordsToString(isoKwrds_, str);
    AdptSendReply(str);
}

/**
 * Response for "ATDPN"
 */
void IsoSerialAdapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    switch (protocol_) {
        case PROT_ISO9141:
            AdptSendReply(useAutoSP ? "A3" : "3"); 
            break;
        case PROT_ISO14230_5BPS:
            AdptSendReply(useAutoSP ? "A4" : "4"); 
            break;
        case PROT_ISO14230:
            AdptSendReply(useAutoSP ? "A5" : "5"); 
            break;
        default:
            AdptSendReply("???"); 
    }
}

/**
 * Do slow/fast ISO 9141/14230 init
 * @param[in] sendReply Send reply flag
 * @return Protocol number if ECU is supporting ISO serial protocols, 0 otherwise
 */
int IsoSerialAdapter::onConnectEcu(bool sendReply)
{
    // If already connected - skip this one
    if (connected_ ) {
        return protocol_;
    }
    
    // Set speed and etc
    configureProperties(); 
    open();

    int requestedProtocol = protocol_;
    
    // Get ISO9141/14230 message formatted
    if (requestedProtocol != PROT_AUTO) {
        switch (requestedProtocol) {
            case PROT_ISO9141:
            case PROT_ISO14230_5BPS:
                onConnectEcuSlow(requestedProtocol);
                break;
            case PROT_ISO14230:
                onConnectEcuFast(requestedProtocol);
                break;
        }
    }
    else {
        onConnectEcuSlow(PROT_AUTO);
        if (!connected_) {
            onConnectEcuFast(PROT_AUTO);
        }
    }

    if (connected_) {
        #ifdef __BELLS_AND_WHISTLES__
        if (requestedProtocol != PROT_AUTO) {
            AdptSendReply(EchoOk);
        }
        #endif
    }
    else {
        close(); // Close only if not succeeded
    }
    return connected_ ? protocol_ : 0;
}

/**
 * Test wiring connectivity for ISO 9141/14230
 */
void IsoSerialAdapter::wiringCheck()
{
    // Disable USART
    uart_->setBitBang(true);

    // Send 1 (set port to 0)
    uart_->setBit(1);
    Delay1ms(1);
    if (uart_->getBit() != 1) {
        AdptSendReply("ISO9141/14230 wiring failed [1]");
        goto ext;
    }        
    
    // Send 0 (set port to 1)
    uart_->setBit(0);
    Delay1ms(1);
    if (uart_->getBit() == 0) {
        AdptSendReply("ISO9141/14230 wiring is OK");
    }    
    else {
        AdptSendReply("ISO9141/14230 wiring failed [0]");
    }    
    
ext:
    uart_->setBit(0);
    
    // Enable USART
    uart_->setBitBang(false);
}

/**
 * The P3Min timeout to use, use either ISO default or custom value
 * @return The timeout value
 */
int IsoSerialAdapter::getP2MaxTimeout() const
{
    int p2Timeout = config_->getIntProperty(PAR_TIMEOUT); 
    return p2Timeout ? p2Timeout : P2_MAX_TIMEOUT;
}

/**
 * Use the Max message length, either OBD standard or maximum allowed by implementation
 * @return The max length value
 */
int IsoSerialAdapter::get2MaxLen() const
{
    bool longAllowed = config_->getBoolProperty(PAR_ALLOW_LONG);
    return longAllowed ? OBD_IN_MSG_LEN + 6 : OBD_IN_MSG_LEN;
}

void IsoSerialAdapter::configureProperties()
{
    // KW 1/0
    kwCheck_ = config_->getBoolProperty(PAR_KW_CHECK);
    
    // WM
    const ByteArray* bytes = config_->getBytesProperty(PAR_WM_HEADER);
    if (bytes->length) {
        customWkpMsg_[0] = bytes->length;
        memcpy(customWkpMsg_ + 1, bytes->data, bytes->length);
    }
    
    // IIA
    uint32_t initByte = config_->getIntProperty(PAR_ISO_INIT_ADDRESS);
    isoInitByte_ = initByte ? initByte : 0x33;
    
    // SW
    uint32_t wakeupTime = config_->getIntProperty(PAR_WAKEUP_VAL);
    wakeupTime_ = wakeupTime ? (wakeupTime * 20) : DEFAULT_WAKEUP_TIME;
}
