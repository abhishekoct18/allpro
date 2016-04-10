#include <LPC15xx.h>
#include "GPIODrv.h"
#include "Timer.h"
#include "PwmDriver.h"

const int VregPin  = 5;
const int VregPort = 0;
static volatile uint8_t  timerFlag;
static volatile uint32_t timerVal;
static volatile uint32_t timerVal2;


void PwmDriver::configure()
{
    // VREG control pin
    GPIOSetDir(VregPort, VregPin, GPIO_OUTPUT);
    GPIOPinWrite(VregPort, VregPin, 1);
    
    LPC_SYSCON->SYSAHBCLKCTRL1 |= (1 << 2); // enable the SCT0 clock but keep hold
    LPC_SYSCON->PRESETCTRL1 |=  (1 << 2);
    LPC_SYSCON->PRESETCTRL1 &= ~(1 << 2);
    LPC_INMUX->SCT0_INMUX[0] = 0x00;        // SCT0_IN0 at P0_2
    
    LPC_SCT0->OUTPUT = 0;
    LPC_SWM->PINASSIGN7 &= 0xFFFF00FF; // ASSIGN7
    LPC_SWM->PINASSIGN7 |= 0x00000100; // P0_1 is SCT0_OUT0, ASSIGN7(15:8)
    LPC_SWM->PINASSIGN7 &= 0xFF00FFFF; // 
    LPC_SWM->PINASSIGN7 |= 0x001D0000; // P0_29 is SCT0_OUT1, ASSIGN7(23:16)

    LPC_SCT0->CONFIG = 0x00000E01;
    LPC_SCT0->CTRL   = 0x00040004; 
    LPC_SCT0->CTRL |= (SystemCoreClock/1000000-1) << 5; // set prescaler, SCT clock = 1 MHz
                                              
    LPC_SCT0->REGMODE = (1 << 1) | (1 << 2); // register pair 1 and 2 are capture
    
    // event 0 happens in state 0, 3
    LPC_SCT0->EV0_CTRL  = (0 << 0)  |  // MATCHSEL[3:0]   = related to match 0
                          (1 << 12) |  // COMBMODE[13:12] = uses match condition only
                          (1 << 14) |  // STATELD [14]    = STATEV is loaded into state
                          (0 << 15);   // STATEV  [15]    = new state is 0
                          
    // event 1 only happens in state 0
    LPC_SCT0->EV1_CTRL  = (0 << 6)  |  // IOSEL   [9:6]   = SCT_IN0
                          (1 << 10) |  // IOCOND  [11:10] = rising edge
                          (2 << 12) |  // COMBMODE[13:12] = uses IO condition only
                          (1 << 14) |  // STATELD [14]    = STATEV is loaded into state
                          (0 << 15);   // STATEV[ 15]     = new state is 0

    // event 2 only happens in state 0
    LPC_SCT0->EV2_CTRL  = (0 << 6)  |  // IOSEL   [9:6]   = SCT_IN0
                          (2 << 10) |  // IOCOND  [11:10] = falling edge
                          (2 << 12) |  // COMBMODE[13:12] = uses IO condition only
                          (1 << 14) |  // STATELD [14]    = STATEV is loaded into state
                          (0 << 15);   // STATEV  [15]    = new state is 0
                          
    // event 3 (low0) only happens in state 2
    LPC_SCT0->EV3_CTRL  = (3 << 0)  |  // related to match 3
                          (1 << 12) |  // COMBMODE[13:12] = match condition only
                          (1 << 14) |  // STATELD[14] = STATEV is loaded into state
                          (1 << 15);   // STATEV[15] = new state is 1
    LPC_SCT0->OUT0_CLR |= (1 << 3);    // low0 @ event 3

    // event 4 (high0) only happens in state 1
    LPC_SCT0->EV4_CTRL  = (3 << 0)  |  // related to match 3
                          (1 << 12) |  // COMBMODE[13:12] = match condition only
                          (1 << 14) |  // STATELD[14] = STATEV is loaded into state
                          (2 << 15);   // STATEV[15] = new state is 2
    LPC_SCT0->OUT0_SET |= (1 << 4);    // high0 @ event 4

    // event 5 (low1) only happens in state 4
    LPC_SCT0->EV5_CTRL  = (5 << 0)  |  // related to match 5
                          (1 << 12) |  // COMBMODE[13:12] = match condition only
                          (1 << 14) |  // STATELD[14] = STATEV is loaded into state
                          (3 << 15);   // STATEV[15] = new state is 3
    LPC_SCT0->OUT0_CLR |= (1 << 5);    // low @ event 5
    LPC_SCT0->OUT1_CLR |= (1 << 5);    // low @ event 5
    
    // event 6 (high1) only happens in state 3
    LPC_SCT0->EV6_CTRL  = (6 << 0)  |  // related to match 6
                          (1 << 12) |  // COMBMODE[13:12] = match condition only
                          (1 << 14) |  // STATELD[14] = STATEV is loaded into state
                          (4 << 15);   // STATEV[15] = new state is 4
    LPC_SCT0->OUT0_SET |= (1 << 6);    // high @ event 6
    LPC_SCT0->OUT1_SET |= (1 << 6);    // high @ event 6
    
    LPC_SCT0->CAPCTRL1  = (1 << 1);    // event 1 causes capture 1 to be loaded
    LPC_SCT0->CAPCTRL2  = (1 << 2);    // event 2 causes capture 2 to be loaded
    LPC_SCT0->LIMIT     = 0x0000007E;  // events 1-6 are used as counter limit
    LPC_SCT0->EVEN      = 0x00000007;  // events 0-2 generate interrupts

    NVIC_EnableIRQ(SCT0_IRQn);         // enable SCT interrupt
}

/**
 * Set timeout for PWM protocol operations
 * @param[in] timeout Timeout value
 */
void PwmDriver::setTimeoutPwm(uint32_t timeout)
{
    LPC_SCT0->CTRL |= (1 << 2);    // halt it to avoid hardware exception
    LPC_SCT0->MATCH0    = timeout; // match 0 @ 239/1MHz = 239 uSec (TV3 timeout)
    LPC_SCT0->MATCHREL0 = timeout; // reload MATCH0 when counter reaches the limit condition
}

/**
 * PwmDriver singleton
 * @return The pointer to PwmDriver instance
 */
PwmDriver* PwmDriver::instance()
{
    static PwmDriver instance;
    return &instance;
}

void PwmDriver::stop()
{
    LPC_SCT0->CTRL |= (1 << 2); // halt it
    LPC_SCT0->OUTPUT = 0;
    LPC_SCT0->STATE = 0;
    LPC_SCT0->EV0_STATE = 0; // disable all events
    LPC_SCT0->EV1_STATE = 0;
    LPC_SCT0->EV2_STATE = 0;
    LPC_SCT0->EV3_STATE = 0;
    LPC_SCT0->EV4_STATE = 0;
    LPC_SCT0->EV5_STATE = 0;
    LPC_SCT0->EV6_STATE = 0;
}

/**
 * Wait for J1850 SOF pulse
 * Note: Not using timer event 0 but Timer P2 timeout
 * @param[in] timeout Timeout for SOF
 * @param[in] p2timeout P2 timeout
 * @return The width of SOF pulse candidate, 0xFFFFFFFF if P2 timeout expired
 */
uint32_t PwmDriver::wait4Sof(uint32_t timeout, Timer* p2timer)
{
    // Looking for falling edge interrupt (event 2)
    LPC_SCT0->CTRL |= (1 << 2);  // halt it
    LPC_SCT0->COUNT = 0;
    LPC_SCT0->STATE = 0;
    LPC_SCT0->MATCH0    = timeout;
    LPC_SCT0->MATCHREL0 = timeout;
    LPC_SCT0->EV0_STATE = 0x00;  // event 0 disabled
    LPC_SCT0->EV1_STATE = 0x01;  // event 1 happens in state 0
    LPC_SCT0->EV2_STATE = 0x01;  // event 2 happens in state 0
    LPC_SCT0->CTRL &= ~(1 << 2); // unhalt it
    
    timerFlag = 0;
    while (!(timerFlag & 0x04)) {
        if (p2timer->isExpired())
            return 0xFFFFFFFF;
    }
    return timerVal;
}

/**
 * Timer is running since last wait4Sof call. Get the values for rising or falling edge.
 * The counter is not resetting! It is related to the previous bus change.
 * @return The timer counter value
 */
uint32_t PwmDriver::wait4BusChangeVpw()
{
    // Use both edges and timeout
    LPC_SCT0->EV0_STATE = 0x01;  // event 0 happens in state 0
    LPC_SCT0->EV1_STATE = 0x01;  // event 1 happens in state 0
    LPC_SCT0->EV2_STATE = 0x01;  // event 2 happens in state 0

    timerFlag = 0;
    while (!(timerFlag & 0x07)) // events 0-2
        ; 
    return (timerFlag & 0x01) ? 0 : timerVal;
}

/**
 * Wait for J1850 bus right moment to start transmitting the message
 * @param[in] timeout1 TV6/TP5 timeout value
 * @param[in] timeout2 TVP4/TP6 timeout value
 * @param[in] p2Timer P2 timer pointer
 * @return true if bus ready, false if bus busy
 */
bool PwmDriver::wait4Ready(uint32_t timeout1, uint32_t timeout2, Timer* p2timer)
{
	bool sts = false;

    // Looking for rising edge or timeout interrupt
    LPC_SCT0->CTRL |= (1 << 2);  // halt it
    LPC_SCT0->COUNT = 0;
    LPC_SCT0->STATE = 0;
    LPC_SCT0->EV0_STATE = 0x0;   // event 0 disabled
    LPC_SCT0->EV1_STATE = 0x0;   // event 1 disabled
    LPC_SCT0->EV2_STATE = 0x01;  // event 2 happens in state 0
    LPC_SCT0->CTRL &= ~(1 << 2); // unhalt it by clearing bit 2 of the CTRL register

    while (!p2timer->isExpired()) {
    	// Run as long as bus active
    	if (getBit())
    		continue;
    	// Got the passive bus, measuring timeout1
    	while (getBit() == 0) {
    		if (LPC_SCT0->COUNT > timeout1)
    			goto nxt;
    	}
    }
    goto done;

nxt:
	// Just wait for rising edge or timeout2 expired
    while (LPC_SCT0->COUNT <  timeout2) {
    	if (getBit())
    		break;
    }
    sts = true;
done:
    LPC_SCT0->CTRL |= (1 << 2);  // halt it
    return sts;
}

/**
 * Start the PWM pulse sequence with SOF pulse
 * @param[in] interval The SOD width
 */
void PwmDriver::sendSofVpw(uint32_t interval)
{
    LPC_SCT0->CTRL |= (1 << 2);  // halt it
    LPC_SCT0->EV0_STATE = 0x00;  // event 0 disabled
    LPC_SCT0->EV1_STATE = 0x00;  // event 1 disabled
    LPC_SCT0->EV2_STATE = 0x00;  // event 2 disabled
    LPC_SCT0->EV3_STATE = 0x04;  // event 3 happens in state 2
    LPC_SCT0->EV4_STATE = 0x02;  // event 4 happens in state 1
    LPC_SCT0->COUNT = 0;         // reset counter
    LPC_SCT0->STATE = 2;         // start in state 2
    LPC_SCT0->MATCH3 = interval;
    LPC_SCT0->MATCHREL3 = interval; 
    LPC_SCT0->OUTPUT = 0x01;     // start with high level, output 0
    LPC_SCT0->CTRL &= ~(1 << 2); // unhalt it
}

/**
 * Timer is running. Set the new reload value and wait till it's reloaded.
 * @param[in] interval The pulse width
 */
void PwmDriver::sendPulseVpw(uint32_t interval)
{
    LPC_SCT0->MATCHREL3 = interval;
    volatile uint32_t state = LPC_SCT0->STATE;
    while (LPC_SCT0->STATE == state)
        ;
}

/**
 * Timer is running (state 2). The output0 should be high. Reload the new value for EOD (low output)
 * and let the timer stop on completion.
 */
void PwmDriver::sendEodVpw()
{
    sendPulseVpw(0xFFFFFFFF);  // long pulse
    stop();
}

/**
 * Get the value on the falling edge.
 * @return The pulse width
 */
uint32_t PwmDriver::wait4BusPulsePwm()
{
    // Use falling edge and timeout
    LPC_SCT0->CTRL |= (1 << 2);  // halt it
    LPC_SCT0->COUNT = 0;         // reset counter
    LPC_SCT0->EV0_STATE = 0x01;  // event 0 happens in state 0
    LPC_SCT0->EV1_STATE = 0x01;  // event 1 happens in state 0
    LPC_SCT0->EV2_STATE = 0x01;  // event 2 happens in state 0
    LPC_SCT0->EV5_STATE = 0x00;  // disable event 5
    LPC_SCT0->EV6_STATE = 0x00;  // disable event 6
    LPC_SCT0->CTRL &= ~(1 << 2); // unhalt it
    timerFlag = 0;
    timerVal2 = 0;
    while ((timerFlag & 0x05) == 0)  // events 0,2
        ;
    return (timerFlag & 0x01) ? 0 : timerVal2;
}

/**
 * Send the J1850 PWM pulse
 * @param[in] intrval1 The active part width
 * @param[in] intrval1 The passive part width
 */
void PwmDriver::sendPulsePwm(uint32_t interval1, uint32_t interval2)
{
    sendHalfBit1(interval1);
    sendHalfBit2(interval2);
}

/**
 * Send the active part of J1850 PWM pulse
 * @param[in] interval The active part width
 */
void PwmDriver::sendHalfBit1(uint32_t interval)
{
    interval--;
    if (LPC_SCT0->CTRL & (1 << 2)) {
        LPC_SCT0->EV0_STATE = 0x00;  // disable event 0
        LPC_SCT0->EV1_STATE = 0x00;  // disable event 0
        LPC_SCT0->EV2_STATE = 0x00;  // disable event 0
        LPC_SCT0->EV5_STATE = 0x10;  // event 5 happens in state 4
        LPC_SCT0->EV6_STATE = 0x08;  // event 6 happens in state 3
        LPC_SCT0->COUNT = 0;         // reset counter
        LPC_SCT0->STATE = 4;         // start in state 4
        LPC_SCT0->MATCH5 = interval;
        LPC_SCT0->MATCHREL5 = interval; 
        LPC_SCT0->OUTPUT = 0x03;     // start with high level
        LPC_SCT0->CTRL &= ~(1 << 2); // unhalt it
    }
    else {
        LPC_SCT0->MATCHREL5 = interval;
        volatile uint32_t state = LPC_SCT0->STATE;
        while (LPC_SCT0->STATE == state) 
            ;
    }
}

/**
 * Send the passive part of J1850 PWM pulse
 * @param[in] interval The passive part width
 */
void PwmDriver::sendHalfBit2(uint32_t interval)
{
    interval--;
    LPC_SCT0->MATCHREL6 = interval;
    volatile uint32_t state = LPC_SCT0->STATE;
    while (LPC_SCT0->STATE == state) 
        ;
}

/**
 * Send EOD
 */
void PwmDriver::sendEodPwm()
{
    stop();
}

extern "C" void SCT0_IRQHandler(void)
{
    // event 0, timeout
    uint32_t evflag = LPC_SCT0->EVFLAG;
    if (evflag & 0x01) {
        timerFlag |= 0x01;
        LPC_SCT0->EVFLAG |= 0x01;
    }
    // event 1, rising edge
    if (evflag & 0x02) {
        timerVal = LPC_SCT0->CAP1;
        timerFlag |= 0x02;
        LPC_SCT0->EVFLAG |= 0x02;
    }
    // event 2, falling edge
    if (evflag & 0x04) {
        timerVal = LPC_SCT0->CAP2;
        timerVal2 = timerVal;
        timerFlag |= 0x04;
        LPC_SCT0->EVFLAG |= 0x04;
    }
}

/**
 * Get the port value in bing-bang mode (connectivity testing)
 * @return The driver pin output value
 */
uint32_t PwmDriver::getBit()
{ 
    return LPC_SCT0->INPUT & 0x01;
}

/**
 * Set the port value in bing-bang mode (connectivity testing)
 * @param[in] val The driver pin output value
 */
void PwmDriver::setBit(int val)
{
    LPC_SCT0->CTRL |= (1 << 2);
    if (vpwMode_)
        LPC_SCT0->OUTPUT = val ? 0x01 : 0x00;
    else
        LPC_SCT0->OUTPUT = val ? 0x03 : 0x00;
}

/**
 * Open driver for operation, either J1850 PWM or VPW
 * @param[in] mode true for vpw, false for PWM
 */
void PwmDriver::open(bool vpwMode)
{
    vpwMode_ = vpwMode;
    if (vpwMode) {
        GPIOPinWrite(VregPort, VregPin, 1);
        LPC_INMUX->SCT0_INMUX[0] = 0x00; // SCT0_IN0 at P0_2
    }
    else {
        GPIOPinWrite(VregPort, VregPin, 0);
        LPC_INMUX->SCT0_INMUX[0] = 0x01; // SCT0_IN0 at P0_3
    }
}
