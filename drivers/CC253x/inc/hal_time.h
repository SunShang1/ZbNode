/*
 * Copyrite (C) 2010, BeeLinker
 *
 * �ļ����ƣ�time.h
 *
 * �ļ�˵����MC13213 TIME/PWM ����ͷ�ļ�
 * 
 * �汾��Ϣ��
 * v0.1      wzy        2007/02/23
 * v0.2      wzy        2011/09/24
 *
 */  


#ifndef   TIME_H
#define   TIME_H

#include "pub_def.h"
#include "hal_mcu.h"
#include "error.h"


/********************************************
* Defines *
********************************************/ 
#define ONE_MILLISECOND             33UL
#define ONE_SECOND                  32768UL
#define ONE_MINUTE                  1966080UL


/********************************************
* Typedefs *
********************************************/
typedef uint32_t    tick_t; 



/********************************************
* Function *
********************************************/
#define Time1IntCallBackSet(x)  Time1Comp0CallbackSet(x)

#define timeout(t)  ((t) - TickGet() > (((uint32_t)(-1))>>1))

/* ��ʱ����ʼ�� */ 
error_t McuSleepTimerInit(void);

tick_t TickGet(void);

/* ��ȡϵͳ��ǰʱ�䣬��λ��S*/
uint32_t TimeGet(void);

void SleepTimeCompSet(tick_t   tick);

/* */
void SleepTimeCompCallbackSet(pInterruptHandler_t pCallBack);

#endif
