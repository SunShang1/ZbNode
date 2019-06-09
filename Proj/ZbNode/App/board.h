/*
 *
 * �ļ����ƣ�board.h
 *
 * �ļ�˵����CC2530 Ӳ����ӿڶ���ͷ�ļ�
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
