/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <LPC15xx.h>
#include "Timer.h"

const uint32_t tickDiv = (SystemCoreClock / 1000);

struct LPC_MRT_CH_T {
    __IO uint32_t INTVAL; // Timer interval register
    __O  uint32_t TIMER;  // Timer register
    __IO uint32_t CTRL;   // Timer control register
    __IO uint32_t STAT;   // Timer status register
};

/**
 * Construct the Timer object
 * @param[in] timerNum Logical timer number (0..2)
 */
Timer::Timer(int timerNum)
{
    timer_ = reinterpret_cast<LPC_MRT_CH_T*>(LPC_MRT_BASE) + timerNum;
    timer_->CTRL = 0x02; // one-shot mode
}

/**
 * Start/restart the timer, interval <= 349 ms
 * @param[in] interval Timer interval in milliseconds
 */
void Timer::start(uint32_t interval) 
{
    uint32_t val = tickDiv * interval;
    timer_->STAT |= 0x01; // Clear interrupt flag
    timer_->INTVAL = val | 0x80000000;
}

/**
 * Check if timer is still running
 * @return Timer interrupt status (false or true)
 */
bool Timer::isExpired() const
{
    return !(timer_->STAT & 0x02);
}

/**
 * Factory method to construct the Timer object
 * @param[in] timerNum Logical timer number (0..1)
 * @return Timer pointer
 */
Timer* Timer::instance(int timerNum)
{
    static Timer timer0(0);
    static Timer timer1(1);
    
    switch (timerNum) {
      case Timer::TIMER0:
          return &timer0;
      
      case Timer::TIMER1: 
          return &timer1;
        
      default:
        return 0;
    }
}

extern "C" void RIT_IRQHandler(void)
{
    LPC_RIT->CTRL &= 0x07;
}

/**
 * Construct the LongTimer object
 */
LongTimer::LongTimer()
{
    LPC_RIT->CTRL = 0;
    LPC_RIT->MASK = LPC_RIT->MASK_H = 0;
    NVIC_EnableIRQ(RIT_IRQn);
}

/**
 * Start/restart the timer
 * @param[in] interval Timer interval in milliseconds
 */
void LongTimer::start(uint32_t interval)
{
    uint64_t compval = static_cast<uint64_t>(tickDiv) * interval;
    LPC_RIT->CTRL = 0;
    LPC_RIT->COUNTER = LPC_RIT->COUNTER_H = 0;
    LPC_RIT->COMPVAL = compval & 0xFFFFFFFF;
    LPC_RIT->COMPVAL_H = compval >> 32;
    LPC_RIT->CTRL = 0x09;
}

/**
 * Check if timer interrupt occurred
 * @return Timer interrupt status, false/true
 */
bool LongTimer::isExpired() const
{
    return !(LPC_RIT->CTRL & 0x08);
}

/**
 * Instance method for LongTimer object
 * @return LongTimer pointer
 */
LongTimer* LongTimer::instance()
{
    static LongTimer timer;
    return &timer;
}

static PeriodicCallbackT irqCallback;

extern "C" void MRT_IRQHandler(void)
{
    uint32_t irqFlag = LPC_MRT->IRQ_FLAG;
    //LPC_MRT->IRQ_FLAG = irqFlag;

    if ((irqFlag & 0x08) && irqCallback) {
        LPC_MRT->IRQ_FLAG |= 0x08;
        (*irqCallback)();
    }
}

/**
 * Construct the PeriodicTimer instance
 * @param[in] callback Timer callback handler
 */
PeriodicTimer::PeriodicTimer(PeriodicCallbackT callback)
{
    irqCallback = callback;
    LPC_MRT->CTRL3 = 0x0;
    NVIC_EnableIRQ(MRT_IRQn);
}

/**
 * Start/restart the timer
 * @param[in] interval Timer interval in milliseconds
 */
void PeriodicTimer::start(uint32_t interval)
{
    uint32_t val = tickDiv * interval;
    LPC_MRT->CTRL3 = 0x1;  // repeated mode with interrupt
    LPC_MRT->STAT3 |= 0x1; // Clear interrupt flag
    LPC_MRT->INTVAL3 = val | 0x80000000;
}

/**
 *  Stop the timer
 */
void PeriodicTimer::stop()
{
    LPC_MRT->CTRL3 = 0x0;
}
