/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <cstdint>

using namespace std;


struct LPC_MRT_CH_T;
class Timer {
public:
    const static int TIMER0 = 0;
    const static int TIMER1 = 1;
    const static int TIMER2 = 2;
    static Timer* instance(int timerNum);
    void start(uint32_t interval);
    bool isExpired() const;
    uint32_t value();
protected:
    Timer(int timerNum);
    LPC_MRT_CH_T* timer_;
};


class LongTimer {
public:
    static LongTimer* instance();
    void start(uint32_t interval);
    bool isExpired() const;
private:
    LongTimer();
};

// For use with Rx/Tx LEDs
typedef void (*PeriodicCallbackT)();
class PeriodicTimer {
public:
    PeriodicTimer(PeriodicCallbackT callback);
    void start(uint32_t interval);
    void stop();
};

#endif //__TIMER_H__
