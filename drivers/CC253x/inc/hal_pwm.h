/*
 *
 * �ļ����ƣ�hal_pwm.h
 *
 * �ļ�˵����CC2530 PWM ����ͷ�ļ�
 * 
 *
 */  


#ifndef   __HAL_PWM_H__
#define   __HAL_PWM_H__

#include "pub_def.h"
#include "hal_mcu.h"
#include "error.h"


/********************************************
* Defines *
********************************************/ 



/********************************************
* Typedefs *
********************************************/




/********************************************
* Function *
********************************************/
/* timer1, for pwm , pwm pin is P10 */

error_t McuPWMInit(void);

/* ���飺freq > 200Hz */
error_t McuPwmCfg(uint32_t freq, uint8_t duty);

#endif
