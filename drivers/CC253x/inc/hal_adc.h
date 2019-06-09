/*
 * Copyrite (C) 2013, xxx Co., Ltd.
 * 
 * 文件名称: adc.h
 * 文件说明：adc驱动头文件
 *
 *
 * 版本信息：
 * v0.0.1       wzy         2013/02/24
 *
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

/********************************************
* Include *
********************************************/
#include "pub_def.h"
#include "ioCC2530.h"

/********************************************
* Defines *
********************************************/
//p00 p01
#define SENSOR_CMAX     13       //0.005V
#define SENSOR_CMIN     1        //0.0004V

//p02
#define SENSOR_VMAX      1990   //0.8V
#define SENSOR_VMIN      993    //0.4V


/********************************************
* Typedefs *
********************************************/
typedef enum
{
    ADC_In_0 = 0x00,
    ADC_In_1 = 0x01,
    ADC_In_2 = 0x02,
    ADC_In_3 = 0x03,
    ADC_In_4 = 0x04,
    ADC_In_5 = 0x05,
    ADC_In_6 = 0x06,
    ADC_In_7 = 0x07,
    ADC_In_8 = 0x08,
}ADC_In_TypeDef;

#define ADC_SENSOR_CL_PORT  ADC_In_0
#define ADC_SENSOR_CP_PORT  ADC_In_1
#define ADC_SENSOR_C_PORT   ADC_In_8

#define ADC_SENSOR_V_PORT   ADC_In_2

#define ADC_CURRENT_PORT    ADC_In_6
#define ADC_PHOTO_PORT      ADC_In_7
/********************************************
* Globals *
********************************************/


/********************************************
* Functions *
********************************************/
/* */
void halAdcInit(ADC_In_TypeDef in);

/* */
int16_t halAdcGet(ADC_In_TypeDef in);

#endif