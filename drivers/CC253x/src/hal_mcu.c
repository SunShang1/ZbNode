/*
 * 文件名称：mcu.c
 *
 * 文件说明：处理器驱动文件
 *
 */

#include "hal_clock.h"
#include "hal_mcu.h"

/********************************************
* Defines *
********************************************/ 




/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/

void halMcuInit(void)
{
    clockSetMainSrc(CLOCK_SRC_XOSC);
}

void EnableInterrupts(void)
{
    HAL_INT_ON();
}

void DisableInterrupts(void)
{
    HAL_INT_OFF();
}

/* ------------------------------------------------------------------------------------------------
 *                                        Reset Macro
 * ------------------------------------------------------------------------------------------------
 */
#define WD_EN               BV(3)
#define WD_MODE             BV(2)
#define WD_INT_1900_USEC    (BV(0) | BV(1))
#define WD_RESET1           (0xA0 | WD_EN | WD_INT_1900_USEC)
#define WD_RESET2           (0x50 | WD_EN | WD_INT_1900_USEC)
#define WD_KICK()           st( WDCTL = (0xA0 | WDCTL & 0x0F); WDCTL = (0x50 | WDCTL & 0x0F); )

/* disable interrupts, set watchdog timer, wait for reset */
#define HAL_SYSTEM_RESET()  do{ HAL_INT_OFF(); WDCTL = WD_RESET1; WDCTL = WD_RESET2; for(;;); }while(0)

void halReboot(void)
{
    HAL_SYSTEM_RESET();
}
