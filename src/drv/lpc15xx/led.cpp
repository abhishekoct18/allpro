/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <Timer.h>
#include <GPIODrv.h>
#include <adaptertypes.h>
#include "led.h"

const int TimerBlinkNum = 10; // The blink longevity
const int TimerInteral  = 10; // 10ms

volatile uint32_t AdptLED::txCount_;
volatile uint32_t AdptLED::rxCount_;

/**
 * Configure GPIO for RX and TX LEDs
 */
void AdptLED::configure()
{
    GPIOPinConfig(RX_LED_PORT, RX_LED_NUM, GPIO_OPEN_DRAIN);
    GPIOPinConfig(TX_LED_PORT, TX_LED_NUM, GPIO_OPEN_DRAIN);
    GPIOSetDir(RX_LED_PORT, RX_LED_NUM, GPIO_OUTPUT);  // yellow
    GPIOSetDir(TX_LED_PORT, TX_LED_NUM, GPIO_OUTPUT);  // red
    RX_LED(0);
    TX_LED(0);
    txCount_ = rxCount_ = 0;
}

/**
 * AdptLED singleton
 * @return The AdptLED class instance
 */
AdptLED* AdptLED::instance()
{
    static AdptLED instance;
    return &instance;;
}

/**
 * Constructo AdpLED object
 */
AdptLED::AdptLED()
{
    timer_ = new PeriodicTimer(TimerCallback);
}

/**
 * Start the LED periodic timer with TimerInteral interval
 */
void AdptLED::startTimer()
{
    timer_->start(TimerInteral);
}

/**
 * Stop the LED periodic timer
 */
void AdptLED::stopTimer()
{
    timer_->stop();
}

/**
 * Initiate the TX LED blink
 */
void AdptLED::blinkTx()
{
    txCount_ = TimerBlinkNum;
    TX_LED(1);
}

/**
 * Initiate the RX LED blink
 */
void AdptLED::blinkRx()
{
    rxCount_ = TimerBlinkNum;
    RX_LED(1);
}

/**
 * Periodic timer callback function, decrement the LED tick counters
 */
void AdptLED::TimerCallback()
{
    if (rxCount_ && (--rxCount_ == 0)) {
        RX_LED(0);
    }
    if (txCount_ && (--txCount_ == 0)) {
        TX_LED(0);
    }
}
