/*
 * 文件名称：time.c
 *
 * 文件说明：CC2530 TIME/PWM 驱动
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
//扩展时钟;
static uint32_t             tickExt;

static pInterruptHandler_t  pSTCompIntHandler;

/********************************************
* Function *
********************************************/
/* sleep timer, for systimer */
error_t McuSleepTimerInit(void)
{
    // timer clock is 32.768kHz

    
    return BOS_EOK;
  
}

tick_t TickGet(void)
{
    uint8_t     hwlock;
    static tick_t   preTick=0;
    tick_t  curTick;
    uint8_t st0,st1,st2;
    
    EnterCritical(hwlock);
    
    st0 = ST0;
    st1 = ST1;
    st2 = ST2;
    
    curTick = tickExt;
    curTick <<= 8;
    curTick |= st2&0xFF;
    curTick <<= 8;
    curTick |= st1&0xFF;
    curTick <<= 8;
    curTick |= st0&0xFF;
    
    if (curTick < preTick)
    {
        tickExt ++;
        curTick += 0x1000000;
    }
    
    preTick = curTick;
    
    ExitCritical(hwlock);
    
    return curTick;
    
}

uint32_t TimeGet(void)  //单位：S
{
    uint8_t     hwlock;
    static tick_t   preTick=0;
    tick_t  curTick;
    uint8_t st0,st1,st2;
    
    EnterCritical(hwlock);
    
    st0 = ST0;
    st1 = ST1;
    st2 = ST2;
    
    curTick = tickExt;
    curTick <<= 8;
    curTick |= st2&0xFF;
    curTick <<= 1;
    curTick |= (st1>>7)&0x01;
    
    if (curTick < preTick)
    {
        tickExt ++;
        curTick += 0x1000000;
    }
    
    preTick = curTick;
    
    ExitCritical(hwlock);
    
    return curTick;
    
}

/* 设置下一次定时器中断时间 */
void SleepTimeCompSet(tick_t   tick)
{
    tick_t next = tick + TickGet();
    uint8_t st0,st1,st2;
    
    st0 = next&0xFF;
    next>>=8;
    st1 = next&0xFF;
    next>>=8;
    st2 = next&0xFF;
    
    while(!(STLOAD&0x01));
    
    ST2 = st2;
    ST1 = st1;
    ST0 = st0;

    STIF = 0;
    STIE = 1;
    // Set conmpare mode with interrupt enabled
    T1CCTL0 = 0x44;
    
}

/* */
void SleepTimeCompCallbackSet(pInterruptHandler_t pCallBack)
{
    pSTCompIntHandler = pCallBack; 
}

HAL_ISR_FUNCTION(ST_ISR,ST_VECTOR)
{
    uint8_t     hwlock;
    
    EnterCritical(hwlock);
    
    if (STIF)
    {
        STIF = 0;
        STIE = 0;
        
        if (pSTCompIntHandler != NULL)
          pSTCompIntHandler( (void *)0 );
    }

    ExitCritical(hwlock);
}



