#ifndef __UART_LPC15xx_H__
#define __UART_LPC15xx_H__

#include <LPC15xx.h>
#include <cstdint>
#include <romapi_15xx.h>

using namespace std;

/**
 * UART CFG register definitions
 */
const uint32_t UART_CFG_ENABLE         (0x01 << 0);
const uint32_t UART_CFG_DATALEN_7      (0x00 << 2);	    // UART 7 bit length mode
const uint32_t UART_CFG_DATALEN_8      (0x01 << 2);	    // UART 8 bit length mode
const uint32_t UART_CFG_DATALEN_9      (0x02 << 2);	    // UART 9 bit length mode
const uint32_t UART_CFG_PARITY_NONE    (0x00 << 4);	    // No parity
const uint32_t UART_CFG_PARITY_EVEN    (0x02 << 4);	    // Even parity
const uint32_t UART_CFG_PARITY_ODD     (0x03 << 4);	    // Odd parity
const uint32_t UART_CFG_STOPLEN_1      (0x00 << 6);	    // UART One Stop Bit Select
const uint32_t UART_CFG_STOPLEN_2      (0x01 << 6);   	// UART Two Stop Bits Select
const uint32_t UART_MODE_32K           (0x01 << 7);	    // Selects the 32 kHz clock from the RTC oscillator as the clock source to the BRG
const uint32_t UART_CFG_CTSEN          (0x01 << 9);	    // CTS enable bit
const uint32_t UART_CFG_SYNCEN         (0x01 << 11);    // Synchronous mode enable bit
const uint32_t UART_CFG_CLKPOL         (0x01 << 12);	// Un_RXD rising edge sample enable bit
const uint32_t UART_CFG_SYNCMST        (0x01 << 14);	// Select master mode (synchronous mode) enable bit
const uint32_t UART_CFG_LOOP           (0x01 << 15);	// Loopback mode enable bit

/**
 * UART STAT register definitions
 */
const uint32_t UART_STAT_RXRDY       = (0x01 << 0);		// Receiver ready 
const uint32_t UART_STAT_RXIDLE      = (0x01 << 1);		// Receiver idle 
const uint32_t UART_STAT_TXRDY       = (0x01 << 2);		// Transmitter ready for data
const uint32_t UART_STAT_TXIDLE      = (0x01 << 3);		// Transmitter idle
const uint32_t UART_STAT_CTS         = (0x01 << 4);		// Status of CTS signal
const uint32_t UART_STAT_DELTACTS    = (0x01 << 5);		// Change in CTS state
const uint32_t UART_STAT_TXDISINT    = (0x01 << 6);		// Transmitter disabled
const uint32_t UART_STAT_OVERRUNINT  = (0x01 << 8);		// Overrun Error interrupt flag
const uint32_t UART_STAT_RXBRK       = (0x01 << 10);	// Received break
const uint32_t UART_STAT_DELTARXBRK  = (0x01 << 11);	// Change in receive break detection
const uint32_t UART_STAT_START       = (0x01 << 12);	// Start detected
const uint32_t UART_STAT_FRM_ERRINT  = (0x01 << 13);	// Framing Error interrupt flag
const uint32_t UART_STAT_PAR_ERRINT  = (0x01 << 14);	// Parity Error interrupt flag
const uint32_t UART_STAT_RXNOISEINT  = (0x01 << 15);	// Received Noise interrupt flag

/**
 * UART INTENSET/INTENCLR register definitions
 */
const uint32_t UART_INTEN_RXRDY      = (0x01 << 0);		// Receive Ready interrupt
const uint32_t UART_INTEN_TXRDY      = (0x01 << 2);		// Transmit Ready interrupt
const uint32_t UART_INTEN_DELTACTS   = (0x01 << 5);		// Change in CTS state interrupt
const uint32_t UART_INTEN_TXDIS      = (0x01 << 6);		// Transmitter disable interrupt
const uint32_t UART_INTEN_OVERRUN    = (0x01 << 8);		// Overrun error interrupt
const uint32_t UART_INTEN_DELTARXBRK = (0x01 << 11);	// Change in receiver break detection interrupt
const uint32_t UART_INTEN_START      = (0x01 << 12);	// Start detect interrupt
const uint32_t UART_INTEN_FRAMERR    = (0x01 << 13);	// Frame error interrupt
const uint32_t UART_INTEN_PARITYERR  = (0x01 << 14);	// Parity error interrupt
const uint32_t UART_INTEN_RXNOISE    = (0x01 << 15);	// Received noise interrupt


inline void UARTIntEnable(LPC_USART0_Type *pUART, uint32_t intMask)
{
	pUART->INTENSET = intMask;
}

inline void UARTIntDisable(LPC_USART0_Type *pUART, uint32_t intMask)
{
	pUART->INTENCLR = intMask;
}

inline uint32_t UARTGetStatus(LPC_USART0_Type *pUART)
{
	return pUART->STAT;
}

inline uint32_t UARTGetIntStatus(LPC_USART0_Type *pUART)
{
	return pUART->INTSTAT;
}

inline uint32_t UARTGetIntsEnabled(LPC_USART0_Type *pUART)
{
	return pUART->INTENSET;
}

inline void UARTSendByte(LPC_USART0_Type *pUART, uint8_t data)
{
	pUART->TXDATA = (uint32_t) data;
}

inline uint8_t UARTReadByte(LPC_USART0_Type *pUART)
{
	return pUART->RXDATA & 0xFF;
}

inline void UART_Enable(LPC_USART0_Type *pUART)
{
	pUART->CFG |= UART_CFG_ENABLE;
}

/**
 * Disable the UART
 * @param[in] pUART pointer to selected UARTx peripheral
 */
inline void UART_Disable(LPC_USART0_Type *pUART)
{
	pUART->CFG &= ~UART_CFG_ENABLE;
}

#endif //__UART_LPC15xx_H__

