/*
 * 文件名称：hal_pwm.c
 *
 * 文件说明：CC2530 PWM 驱动
 *
 */

#include "pub_def.h"

#include "hal_mcu.h"
#include "hal_time.h"

/********************************************
* Defines *
********************************************/ 




/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/


/********************************************
* Function *
********************************************/
/* timer1, for pwm , pwm pin is P10 */

error_t McuPWMInit(void)
{
    /*
     *  Set Timer1 8M/1 = 8MHz
     */
    T1CTL = 0x00;
    
    T1CCTL2 = 0x2C;
    
    MCU_IO_DIR_OUTPUT(1, 0);    // PWM out pin

    return BOS_EOK;
}

static error_t McuPwmStart(void)
{
    
    P1SEL |= (1<<0);    // P10 for timer1 C2 mode
    PERCFG |= (1<<6);  // alt 2
    
    T1CTL &= ~(0x03<<0);
    T1CTL |= (0x02<<0);    // start timer1

    return BOS_EOK;
}

static error_t McuPwmStop(void)
{
    T1CTL &= ~(0x03<<0);
    
    P1SEL &= ~(1<<0);   // GPIO mode
    
    return BOS_EOK;
    
}

/* 建议：freq > 200Hz */
error_t McuPwmCfg(uint32_t freq, uint8_t duty)
{
    uint32_t mode;
    uint16_t tmp;
    
    if (duty == 0)
    {
        McuPwmStop();
        
        MCU_IO_OUTPUT(1, 0, 1);
        
        return BOS_EOK;
    }
    else if (duty == 100)
    {
        McuPwmStop();
        
        MCU_IO_OUTPUT(1, 0, 0);
        
        return BOS_EOK;
    }
    else if (duty > 100)
        return -BOS_ERROR;
    
    mode = 8000000/freq;
    T1CC0L = (mode-1)&0xFF;
    T1CC0H = ((mode-1)>>8)&0xFF;
    
    
    tmp = (mode*duty+50)/100;
    T1CC2L = tmp&0xFF;
    T1CC2H = (tmp>>8)&0xFF;
    
    
    McuPwmStart();
    
    return BOS_EOK;
}

