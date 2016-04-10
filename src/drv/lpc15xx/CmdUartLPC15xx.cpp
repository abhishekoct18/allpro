/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include "UartLPC15xx.h"
#include "GPIODrv.h"
#include "CmdUart.h"

using namespace std;

const int TxPin = 15;
const int RxPin = 14;
const int RxPort = 0;
const int TxPort = 0;
const uint32_t PinAssign = ((RxPin << 8) + (RxPort * 32)) | (TxPin  + (TxPort * 32));

/**
 * Constructor
 */
CmdUart::CmdUart()
  : txLen_(0),
    txPos_(0),
    ready_(false),
    handler_(0)
{
}

/**
 * CmdUart singleton
 */
CmdUart* CmdUart::instance()
{
    static CmdUart instance;
    return &instance;;
}

/**
 * Configure UART0
 */
void CmdUart::configure()
{
    // Enable UART0 clock
    LPC_SYSCON->SYSAHBCLKCTRL1 |=  (1 << 17);
    LPC_SYSCON->PRESETCTRL1    |=  (1 << 17);
    LPC_SYSCON->PRESETCTRL1    &= ~(1 << 17);
    LPC_SYSCON->UARTCLKDIV = 1;

    GPIOPinConfig(RxPort, RxPin, 0);
    GPIOPinConfig(TxPort, TxPin, 0); // open-drain

    LPC_SWM->PINASSIGN0 &= 0xFFFF0000;
    LPC_SWM->PINASSIGN0 |= PinAssign;
}

/**
 * Use UART ROM API to configuring speed and interrupt for UART0,
 * discard the allocated UART memory block afterwards
 * @parameter[in] speed Speed to configure
 */
void CmdUart::init(uint32_t speed)
{
    const int UART_MEM_LEN = 40;

    // Temporary allocate UART API block
    uint8_t uartMem[UART_MEM_LEN];

    NVIC_DisableIRQ(UART0_IRQn);

    // Setup the UART handle
    UART_HANDLE_T uartHandle =
        LPC_UARTD_API->uart_setup(reinterpret_cast<uint32_t>(LPC_USART0), uartMem);

    UART_CONFIG_T cfg = {
        SystemCoreClock, // CPU_CLK frequency in Hz
        speed,           // Baud Rate in Hz
        1,               // 8N1
        0,               // Asynchronous Mode
        NO_ERR_EN        // Enable No Errors
    };

    // Initialize the UART with the configuration parameters
    LPC_UARTD_API->uart_init(uartHandle, &cfg);

    NVIC_EnableIRQ(UART0_IRQn);
    UARTIntEnable(LPC_USART0, UART_INTEN_RXRDY);
}

/**
 * CmdUart TX handler
 */
void CmdUart::txIrqHandler()
{
    // Fill TX until full or until TX buffer is empty
    if (txPos_ < txLen_) {
        if (UARTGetStatus(LPC_USART0) & UART_STAT_TXRDY) {
            UARTSendByte(LPC_USART0, txData_[txPos_++]);
        }
    }
    else {
        UARTIntDisable(LPC_USART0, UART_INTEN_TXRDY);
    }
}

/**
 * CmdUart RX handler
 */
void CmdUart::rxIrqHandler()
{
    if (!(UARTGetStatus(LPC_USART0) & UART_STAT_RXRDY))
        return;

    uint8_t ch = UARTReadByte(LPC_USART0);
    if (handler_)
        ready_ = (*handler_)(ch);
}


/**
 * CmdUart IRQ handler
 */
void CmdUart::irqHandler()
{
    if (UARTGetIntsEnabled(LPC_USART0) & UART_INTEN_TXRDY) {
        txIrqHandler();
    }
    if (UARTGetIntsEnabled(LPC_USART0) & UART_INTEN_RXRDY) {
        rxIrqHandler();
    }
}


/**
 * Send one character, blocking call for echo purposes
 * @parameter[in] ch Character to send
 */
void CmdUart::send(uint8_t ch) 
{
    while (!(UARTGetStatus(LPC_USART0) & UART_STAT_TXRDY))
        ;
    UARTSendByte(LPC_USART0, ch);
}

/**
 * Send the string asynch
 * @parameter[in] str String to send
 */
void CmdUart::send(const util::string& str)
{
    // wait for TX interrupt disabled when the previous transmission completed
    while ((UARTGetIntsEnabled(LPC_USART0) & UART_INTEN_TXRDY)) {
        ;
    }

    // start the new transmission 
    txPos_ = 0;
    txLen_ = str.length();
    memcpy(txData_, str.c_str(), txLen_);
    UARTIntEnable(LPC_USART0, UART_INTEN_TXRDY);
}

/**
 * UART0 IRQ Handler, redirect to irqHandler
 */
extern "C" void UART0_IRQHandler(void)
{
    if (CmdUart::instance())
        CmdUart::instance()->irqHandler();
}
