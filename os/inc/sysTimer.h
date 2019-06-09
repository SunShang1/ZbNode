/*
 * Copyrite (C) 2010, BeeLinker
 *
 * �ļ����ƣ�SysTimer.h
 *
 * �ļ�˵����system timer ͷ�ļ�
 * 
 * �汾��Ϣ��
 * v0.1      wzy        2010/06/24
 * v0.2      wzy        2011/09/24
 *
 */
 
#ifndef     SYS_TIMER_H
#define     SYS_TIMER_H
 
#include    "pub_def.h"
#include    "hal_time.h"


/********************************************
* Defines *
********************************************/
#define SYSTIMER_STATUS_FREE           0x00
#define SYSTIMER_STATUS_ACTIVE         0x20
#define SYSTIMER_STATUS_READY          0x40
#define SYSTIMER_STATUS_INACTIVE       0x80

#define SYSTIMER_TYPE_INTERVAL         0x01
#define SYSTIMER_TYPE_ONCE             0x02

#define INVALID_SYSTIMER_ID            255

/********************************************
* Typedefs *
********************************************/
typedef uint8_t     SysTimerID_t;

// ����timer����
typedef uint8_t     SysTimerType_t;

/********************************************
* Globals *
********************************************/ 
  


/********************************************
* Function defines *
********************************************/
/* */
error_t SysTimerInit(void);

/* */
SysTimerID_t SysTimerAlloc(void);

/* */
void _SysTimerFree(SysTimerID_t);

#define SysTimerFree(timeId)  \
do  \
{   \
    _SysTimerFree(timeId);  \
    timeId = INVALID_SYSTIMER_ID;   \
}while(0)
    

/* */
void SysTimerStart(SysTimerID_t, SysTimerType_t, tick_t, void (*pTimerCallBack)(SysTimerID_t) ); 

/* ֹͣʱ�� */
void SysTimerStop(SysTimerID_t);
 
#endif