/*
 *
 * 文件名称：board.h
 *
 * 文件说明：CC2530 硬件板接口定义头文件
 * 
 *
 */  


#ifndef   BOARD_H
#define   BOARD_H

#include "pub_def.h"
#include "hal_mcu.h"
#include "hal_flash.h"
#include "hal_time.h"
#include "hal_rf.h"
#include "hal_pwm.h"
#include "hal_uart.h"
#include "hal_adc.h"


/********************************************
* Defines *
********************************************/ 
#define CONSOLE_UART        0
#define NBIOT_UART          1


/********************************************
* Typedefs *
********************************************/




/********************************************
* Function *
********************************************/
void halBoardInit(void);

void consoleInit(void);

#endif
