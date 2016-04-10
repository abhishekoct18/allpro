/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __LED_H__ 
#define __LED_H__

#include <cstdint>

using namespace std;

class PeriodicTimer;

class AdptLED {
public:
    static void configure();
    static AdptLED* instance();
    void startTimer();
    void stopTimer();
    void blinkTx();
    void blinkRx();
private:
    AdptLED();
    static void TimerCallback();
    static volatile uint32_t txCount_;
    static volatile uint32_t rxCount_;
    PeriodicTimer* timer_;
};


#endif //__LED_H__
