/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <LPC15xx.h>
#include <romapi_15xx.h>
#include "AdcDriver.h"

const int RAMBLOCK_H  = 60;
const int BUFFER_SIZE = 12;                 // For 12 channels
const uint32_t ADC_CH = 10;                 // Using P0_0 as ADC0_10
const uint32_t ADC_CLOCK_RATE = 12000000;
const uint32_t ADC_INSEL_ADC0 = (0x0 << 0); // Select ADCn_0 for channel 0

static uint32_t     ramBlock[RAMBLOCK_H];
static ADC_CONFIG_T adcCfg;
static ADC_HANDLE_T adcHandle;
static uint32_t     adcBuffer[BUFFER_SIZE]; // Buffer for ADC data

/**
 * Configuring comparator
 */
void AdcDriver::configure()
{
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 27);
    LPC_SYSCON->PRESETCTRL0 |=  (1 << 27);   // Assert peripheral reset, p46
    LPC_SYSCON->PRESETCTRL0 &= ~(1 << 27);   // De-assert

    //Enable ADC0
    LPC_SWM->PINENABLE0 &= ~(1 << ADC_CH);
    
    //Enable power for ADC0
    LPC_SYSCON->PDRUNCFG &= ~(1 << 10);
    
    //uint32_t adcSize = LPC_ADCD_API->adc_get_mem_size();
    
    // ADC Handle Setup
    adcHandle = LPC_ADCD_API->adc_setup(LPC_ADC0_BASE, reinterpret_cast<uint8_t*>(ramBlock));
    
      // ADC0 Calibration
    adcCfg.system_clock = SystemCoreClock; // System clock
    adcCfg.adc_clock = 500000;             // ADC clock set to 500KHz for calibration
    LPC_ADCD_API->adc_calibration(adcHandle, &adcCfg);

    // ADC0 Config for Init
    adcCfg.system_clock = SystemCoreClock; // System clock
    adcCfg.adc_clock   =  ADC_CLOCK_RATE;  // ADC clock
    adcCfg.async_mode  =  0;  // Synchronous mode
    adcCfg.tenbit_mode =  0;  // 12 Bit ADC mode
    adcCfg.lpwr_mode   =  0;  // Disable low power mode
    adcCfg.input_sel   =  ADC_INSEL_ADC0;
    adcCfg.seqa_ctrl   =  (1 << ADC_CH);
    adcCfg.thrsel      =  0;
    adcCfg.thr0_low    =  0;
    adcCfg.thr0_high   =  0;
    adcCfg.thr1_low    =  0;
    adcCfg.thr1_high   =  0;
    adcCfg.error_en    =  0;  // Overrun interrupt disabled
    adcCfg.thcmp_en    =  0;
    adcCfg.channel_num = ADC_CH + 1; // Channel number is one higher than the maximum channel number used
    LPC_ADCD_API->adc_init(adcHandle, &adcCfg);
}

uint32_t AdcDriver::read()
{
    ADC_PARAM_T param;
    param.buffer       =  adcBuffer;
    param.driver_mode  =  0;       // Polling mode
    param.seqa_hwtrig  =  0;
    param.adc_cfg      =  &adcCfg; // ADC0 Config
    param.comp_flags   =  0;
    param.seqa_callback_pt  = 0;   // Call back for SeqA Interrupt
    param.thcmp_callback_pt = 0;   // Call back for Threshold Compare Interrupt
    LPC_ADCD_API->adc_seqa_read(adcHandle, &param);
    return adcBuffer[ADC_CH];
}
