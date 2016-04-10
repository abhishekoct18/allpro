/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __PWM_DRIVER_H__ 
#define __PWM_DRIVER_H__

#include <cstdint>

using namespace std;

class Timer;
class PwmDriver {
public:
    static PwmDriver* instance();
    static void configure();
    void open(bool vpwMode);
    void close() {}
    void stop();
    bool wait4Ready(uint32_t timeou1, uint32_t timeout2, Timer* p2timer);
    void setBit(int val);
    uint32_t wait4Sof(uint32_t timeout, Timer* p2timer);
    uint32_t getBit();
    // VPW specific
    uint32_t wait4BusChangeVpw();
    void sendSofVpw(uint32_t interval);
    void sendPulseVpw(uint32_t interval);
    void sendEodVpw();
    // PWM specific
    void setTimeoutPwm(uint32_t timeout);
    uint32_t wait4BusPulsePwm();
    void sendPulsePwm(uint32_t interval1, uint32_t interval2);
    void sendEodPwm();
private:
    void sendHalfBit1(uint32_t interval);
    void sendHalfBit2(uint32_t interval);
    PwmDriver() {}
    bool vpwMode_;
};

#endif //__PWM_DRIVER_H__
