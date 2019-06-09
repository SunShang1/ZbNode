/*
 * �ļ����ƣ�baord.c
 *
 * �ļ�˵����Ӳ����ӿڶ����ļ�
 *
 */

#include "board.h"
/********************************************
* Defines *
********************************************/ 




/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/
void halBoardInit(void)
{
    halMcuInit();
    
    McuPWMInit();
}

void consoleInit(void)
{
#ifdef UDBG
    struct uartOpt_t opt;
    
    /* Init uart */
    opt.baudRate=UART_BAUD_115200;
    opt.dataBits=UART_DATA_BIT8;
    opt.stopBits=UART_STOP_BIT1;
    opt.parity='N';
    halUartInit(CONSOLE_UART, &opt);
#endif
}

