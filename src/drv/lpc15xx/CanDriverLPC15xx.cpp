/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <cstdlib>
#include <adaptertypes.h>
#include <LPC15xx.h>
#include <romapi_15xx.h>
#include "CanDriver.h"
#include "GPIODrv.h"
#include <canmsgbuffer.h>
#include <led.h>

using namespace std;

const int TxPin = 18;
const int RxPin =  9;
const int RxPort = 0;
const int TxPort = 0;
const uint32_t PinAssign = ((RxPin << 16) + (RxPort * 32)) | ((TxPin << 8)  + (TxPort * 32));
const uint32_t CAN_MSGOBJ_STD = 0x00000000;
const uint32_t CAN_MSGOBJ_EXT = 0x20000000;
const int FIFO_NUM = 10;
CAN_HANDLE_T CanDriver::handle_;
static volatile uint32_t msgBitMask;

// C-CAN callbacks
extern "C" {
    void C_CAN0_IRQHandler(void)
    {
        LPC_CAND_API->hwCAN_Isr(CanDriver::handle_);
    }

    void CAN_rx(uint8_t objNum) {
        // Blink LED from here, when RX operation is completed
        AdptLED::instance()->blinkRx();
        
        // Just set bitmask
        msgBitMask |= (1 << objNum);
    }

    void CAN_tx(uint8_t msgObjNum)
    {
        // Blink LED from here, when TX operation is completed
        AdptLED::instance()->blinkTx();
    }

    void CAN_error(uint32_t errorInfo)
    {
    }
}

/**
 * Make the particular message buffer part of the FIFO
 * @parameter msgobj C-CAN message object number
 */
static void SetFIFOItem(uint32_t msgobj) 
{   // p458,
    const uint32_t IFCREQ_BUSY = 0x8000;
    const uint32_t CMD_CTRL    = (1 << 4); // 1 is transfer the CTRL bit to the message object, 0 is not
    const uint32_t CMD_WR      = (1 << 7);
    const uint32_t DLC_LEN     = 8;
    const uint32_t RXIE        = (1 << 10);
    const uint32_t UMASK       = (1 << 12);

    // Write only control bits
    // Note: message number CANIF1_CMDREQ.MN starts with 1
    LPC_C_CAN0->CANIF1_CMDMSK_W = CMD_WR | CMD_CTRL;
    LPC_C_CAN0->CANIF1_MCTRL = DLC_LEN | RXIE | UMASK;
    LPC_C_CAN0->CANIF1_CMDREQ = msgobj + 1;    // Start message transfer
    while(LPC_C_CAN0->CANIF1_CMDREQ & IFCREQ_BUSY);
}

/**
 * Convert CAN_MSG_OBJ to CanMsgBuffer
 * @parameter   msg1   CAN_MSG_OBJ instance
 * @parameter   msg2   CanMsgBuffer instance 
 */
static void CanNative2Msg(const CAN_MSG_OBJ* msg1, CanMsgBuffer* msg2)
{
    msg2->id = msg1->mode_id & 0x1FFFFFFF;
    msg2->dlc = msg1->dlc;
    msg2->extended = (msg1->mode_id & CAN_MSGOBJ_EXT) ? true : false;
    memset(msg2->data, 0, 8);
    memcpy(msg2->data, msg1->data, msg1->dlc);
    msg2->msgnum = msg1->msgobj;
}

/**
 * Convert CanMsgBuffer to CAN_MSG_OBJ
 * @parameter   msg1   CanMsgBuffer instance
 * @parameter   msg2   CAN_MSG_OBJ instance
 */
static void CanMsg2Native(const CanMsgBuffer* msg1, CAN_MSG_OBJ* msg2, uint8_t msgobj)
{
    msg2->mode_id = msg1->id | (msg1->extended ? CAN_MSGOBJ_EXT : CAN_MSGOBJ_STD);
    msg2->mask = 0x0;
    msg2->dlc = msg1->dlc;
    memcpy(msg2->data, msg1->data, 8);
    msg2->msgobj = msgobj;
}

/**
 * Configuring CanDriver
 */
void CanDriver::configure()
{
    LPC_SYSCON->SYSAHBCLKCTRL1 |=  (1 << 7);
    LPC_SYSCON->PRESETCTRL1    |=  (1 << 7);
    LPC_SYSCON->PRESETCTRL1    &= ~(1 << 7);

    GPIOPinConfig(RxPort, RxPin, 0);
    GPIOPinConfig(TxPort, TxPin, 0);

    GPIOSetDir(RxPort, RxPin, GPIO_INPUT);
    GPIOSetDir(TxPort, TxPin, GPIO_OUTPUT);

    LPC_SWM->PINASSIGN6 &= 0xFF0000FF;
    LPC_SWM->PINASSIGN6 |= PinAssign;
}

/**
 * CanDriver singleton
 * @return The pointer to CandRiver instance
 */
CanDriver* CanDriver::instance()
{
    static CanDriver instance;
    return &instance;
}

/**
 * Intialize the CAN controller and interrupt handler
 */
CanDriver::CanDriver()
{
    const int MAX_CAN_PARAM_SIZE = 124;
    static uint32_t canApiMem[MAX_CAN_PARAM_SIZE];

    // Initialize the CAN controller 500kbps
    // tq=125nS, T1=12, T2=3, SJW=3, 500 kBit/s
    CAN_CFG canConfig = { 0, 0x00002B85UL, 1 };
    CAN_CALLBACKS callbacks = { &CAN_rx, &CAN_tx, &CAN_error };

    CAN_API_INIT_PARAM_T apiInitCfg = {
        reinterpret_cast<uint32_t>(canApiMem),
        LPC_C_CAN0_BASE,
        &canConfig,
        &callbacks,
        nullptr,
        nullptr
    };

    if (LPC_CAND_API->hwCAN_Init(&handle_, &apiInitCfg) != 0) {
         while (1) {
            __WFI(); // Go to sleep
        }
    }

    // Enable the CAN Interrupt
    NVIC_EnableIRQ(C_CAN0_IRQn);
}

/**
 * Transmits a sequence of bytes to the ECU over CAN bus
 * @parameter   buff   CanMsgBuffer instance
 * @return the send operation completion status
 */
bool CanDriver::send(const CanMsgBuffer* buff)
{
    static CAN_MSG_OBJ msg;

    //Send using msgobj 0
    CanMsg2Native(buff, &msg, 0);

    // Note: can_transmit seems to be non-blocking call,
    // allocate CAN_MSG_OBJ on the heap for now.
    // Would need to use CAN_tx() status callback to make a blocking call
#ifdef WAIT_FOR_TRANSMISSION_COMPLETE
    uint32_t checkMask = 0x1 << msg.msgobj;
    while (LPC_CAN->TXRQST & checkMask) {
        ;
    }
#endif

    LPC_CAND_API->hwCAN_MsgTransmit(handle_, &msg);
    return true;
}

/**
 * Set the configuration for receiving messages
 *
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   msgobj    C-CAN message object number
 * @parameter   extended  CAN extended message flag
 * @parameter   fifoLast  last FIFO message buffer flag
 */
void CanDriver::configRxMsgobj(uint32_t filter, uint32_t mask, uint8_t msgobj, bool extended, bool fifoLast)
{
    CAN_MSG_OBJ msg;

    msg.msgobj = msgobj;
    msg.mode_id = filter | (extended ? CAN_MSGOBJ_EXT : CAN_MSGOBJ_STD);
    msg.mask = mask;
    LPC_CAND_API->hwCAN_ConfigRxmsgobj(handle_, &msg);
    if (!fifoLast) {
        SetFIFOItem(msgobj);
    }
}

/**
 * Set the CAN filter for FIFO buffer
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   extended  CAN extended message flag
 * @return  the operation completion status
 */
bool CanDriver::setFilterAndMask(uint32_t filter, uint32_t mask, bool extended)
{
    // Set the FIFO buffer, starting with obj 1
    for (int msgobj = 1; msgobj <= FIFO_NUM; msgobj++) {
        bool fifoLast = (msgobj == FIFO_NUM);
        configRxMsgobj(filter, mask, msgobj, extended, fifoLast);
        if (!fifoLast) {
            SetFIFOItem(msgobj);
        }
    }
    return true;
}

/**
 * Read the CAN frame from FIFO buffer
 * @return  true if read the frame / false if no frame
 */
bool CanDriver::read(CanMsgBuffer* buff)
{
    CAN_MSG_OBJ msg;
    uint32_t mask = msgBitMask;
    for (int i = 1; i < 32; i++) {
        uint32_t val = 1 << i;
        if (val & mask) {
            msgBitMask &= ~val;
            msg.msgobj = i;
            LPC_CAND_API->hwCAN_MsgReceive(CanDriver::handle_, &msg);
            CanNative2Msg(&msg, buff);
            return true;
        }
    }
    return false;
}

/**
 * Read CAN frame received status
 * @return  true/false
 */
bool CanDriver::isReady() const
{
    return (msgBitMask != 0L);
}

/**
 * Wakes up the CAN peripheral from sleep mode
 * @return  true/false
 */
bool CanDriver::wakeUp()
{
    return true;
}

/**
 * Enters the sleep (low power) mode
 * @return  true/false
 */
bool CanDriver::sleep()
{
    return false;
}

/**
 * Switch on/off CAN and let the CAN pins controlled directly (testing mode)
 * @parameter  val  CAN testing mode flag 
 */
void CanDriver::setBitBang(bool val)
{
    if (!val) {
        LPC_SWM->PINASSIGN6 &= 0xFF0000FF;
        LPC_SWM->PINASSIGN6 |= PinAssign;
    }
    else {
        LPC_SWM->PINASSIGN6 |= 0x00FFFF00;
    }
}

/**
 * Set the CAN transmitter pin status
 * @parameter  bit  CAN TX pin value
 */
void CanDriver::setBit(uint32_t bit)
{
    GPIOPinWrite(TxPort, TxPin, bit);
}

/**
 * Read CAN RX pin status
 * @return pin status, 1 if set, 0 otherwise
 */
uint32_t CanDriver::getBit()
{
    return GPIOPinRead(RxPort, RxPin);
}
