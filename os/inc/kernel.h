/*
 *  文件名称：Kernel.h
 *
 *  文件说明：BOS 内核头文件
 *
 */

#ifndef  KERNEL_H
#define  KERNEL_H

#include "pub_def.h"
#include "error.h"


/********************************************
* Defines *
********************************************/

#define  __task            
/********************************************
* Typedefs *
********************************************/
typedef uint16_t        event_t;
typedef void            (*BosTaskHandler_t)(event_t);
typedef __task void (*pTaskFn)(event_t);

/********************************************
* Function defines *
********************************************/

/* */
error_t BosInit(void); 

/* 向一个任务添加一个事件 */
error_t BosEventSend(uint8_t taskId, event_t event);

/* */
void BosScheduler(void);


#endif



