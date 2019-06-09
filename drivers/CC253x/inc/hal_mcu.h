/*
 *
 * 文件名称：mcu.h
 *
 * 文件说明：CC2530 MCU驱动头文件
 * 
 *
 */  


#ifndef   MCU_H
#define   MCU_H

#include "pub_def.h"
#include "hal_int.h"
#include "hal_cc8051.h"
#include "hal_clock.h"


/********************************************
* Defines *
********************************************/ 




/********************************************
* Typedefs *
********************************************/
typedef void  (*pInterruptHandler_t)(void *pdata); 



/********************************************
* Function *
********************************************/
void halMcuInit(void);

void EnableInterrupts(void);

void DisableInterrupts(void);

void halReboot(void);

#define EnterCritical(lock)     HAL_INT_LOCK(lock)

#define ExitCritical(lock)      HAL_INT_UNLOCK(lock)

#define EnterIsr(status)          HAL_ENTER_ISR(status)

#define ExitIsr(status)             HAL_EXIT_ISR(status)


#endif
