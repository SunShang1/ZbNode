/*
 * Copyrite (C) 2013, xxx Co., Ltd.
 * 
 * 文件名称: adc.c
 * 文件说明：adc驱动
 *
 *
 * 版本信息：
 * v0.0.1       wzy         2013/02/24
 *
 */


/********************************************
* Include *
********************************************/
#include "hal_adc.h"


/********************************************
* Defines *
********************************************/
#define ADCCON1_EOC                       0x80
#define ADCCON1_ST                        0x40
#define ADCCON1_STSEL                     (0x03 << 4)
    #define ADCCON1_STSEL_P2_0                (0x00 << 4)   // External trigger on P2.0
    #define ADCCON1_STSEL_FULL_SPEED          (0x01 << 4)   // Do not wait for triggers
    #define ADCCON1_STSEL_T1C0_CMP_EVT        (0x02 << 4)   // Timer 1 ch0 compare event
    #define ADCCON1_STSEL_ST                  (0x03 << 4)   // ADCCON1.ST = 1

#define ADCCON2_SREF                      (0x03 << 6)   // bit mask, select reference voltage
    #define ADCCON2_SREF_1_25V                (0x00 << 6)   // Internal reference 1.15 V
    #define ADCCON2_SREF_P0_7                 (0x01 << 6)   // External reference on AIN7 pin
    #define ADCCON2_SREF_AVDD                 (0x02 << 6)   // AVDD5 pin
    #define ADCCON2_SREF_P0_6_P0_7            (0x03 << 6)   // External reference on AIN6-AIN7 differential input

#define ADCCON2_SDIV                      (0x03 << 4)   // bit mask, decimation rate
    #define ADCCON2_SDIV_64                   (0x00 << 4)   // 7 bits ENOB
    #define ADCCON2_SDIV_128                  (0x01 << 4)   // 9 bits ENOB
    #define ADCCON2_SDIV_256                  (0x02 << 4)   // 10 bits ENOB
    #define ADCCON2_SDIV_512                  (0x03 << 4)   // 12 bits ENOB


/********************************************
* Typedefs *
********************************************/


/********************************************
* Globals *
********************************************/


/********************************************
* Functions *
********************************************/
/* */
void halAdcInit(ADC_In_TypeDef in)
{
    APCFG |= (1<<in);
}

static int16_t AdcSingleGet(ADC_In_TypeDef in)
{
    static int16_t    val;
    
    ADCCON3 = ADCCON2_SREF_AVDD | ADCCON2_SDIV_512 | in;
    
    ADCCON1 |= ADCCON1_ST;
    while( !(ADCCON1 & ADCCON1_EOC));
    
    val = ((uint16_t)ADCH << 8) & 0xff00;
    val |= ADCL;
    
    val >>= 2;
    
    return val;   
}

/* */
int16_t halAdcGet(ADC_In_TypeDef in)
{
    int16_t    val = 0;
    uint8_t     i;
   
    for (i = 0; i < 4; i ++)
    {
        val += AdcSingleGet(in);
    }
    
    val >>= 2;
    
    return val;
    
}
    
    
