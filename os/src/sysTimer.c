/*
 * 文件名称：sysTimer.C
 *
 * 文件说明：
 *     BOS操作系统的时间管理机制
 */

#include "pub_def.h"
#include "board.h"
#include "kernel.h"
#include "sysTimer.h"  


/********************************************
* Defines *
********************************************/
#define MAX_SYSTIMERS              20


#define SYSTIMER_EVENT             0x01

/********************************************
* Typedefs *
********************************************/
// 时钟 状态 列表
// timer 状态定义 (高4位status，低四位type)
typedef uint8_t         SysTimerStatus_t;


typedef void            (*pSysTimerCallBack_t)(SysTimerID_t);

typedef struct  
{
    SysTimerStatus_t    status; /* 低4位表示类型，高4位表示状态 */
    tick_t              intervalInTicks;
    tick_t              countDown;
    pSysTimerCallBack_t pCallBack;
} SysTimerTab_t;
/********************************************
* Globals *
********************************************/ 
static uint8_t          numOfActiveTimers = 0;

// 时钟 列表
static SysTimerTab_t    SysTimerTable[MAX_SYSTIMERS];

bool_t                   SysTimerTaskIsPending = FALSE;

/********************************************
* Function defines *
********************************************/


static void Time1InterruptHandler(void *pdata);
__task void SysTimerTask(event_t events);
static void SysTimerEnable(SysTimerID_t timerID);
static void SysTimerDisable(SysTimerID_t timerID);

/* 设置时钟状态 */
#define SysTimerStatusSet(SysTimerID, SysTimerStatus)   \
  ( SysTimerTable[SysTimerID].status = (SysTimerTable[SysTimerID].status & 0x0f) | (SysTimerStatus & 0xf0) )

/* 设置时钟类型 */
#define SysTimerTypeSet(SysTimerID, SysTimerType)  \
  ( SysTimerTable[SysTimerID].status = (SysTimerTable[SysTimerID].status & 0xf0) | (SysTimerType & 0x0f) )  

/* 获取时钟状态 */
#define SysTimerStatusGet(SysTimerID)  \
  (SysTimerTable[SysTimerID].status & 0xf0)  


/* 获取时钟类型 */
#define SysTimerTypeGet(SysTimerID)  \
  ( (SysTimerTable[SysTimerID].status & 0x0f) ) 
  

/* 初始化系统时钟,创建时间机制管理任务 */
error_t SysTimerInit(void)
{
    SysTimerID_t i;
  
    McuSleepTimerInit();
  
    SleepTimeCompCallbackSet(Time1InterruptHandler);
  
    for (i = 0; i < MAX_SYSTIMERS; ++ i)
    {
        SysTimerTable[i].status = SYSTIMER_STATUS_FREE ;
        SysTimerTable[i].intervalInTicks = 0;
        SysTimerTable[i].countDown = 0;
        SysTimerTable[i].pCallBack = NULL;
    }
  
    return SUCCESS;
}                                       


/* 向系统申请一个时钟给任务 */
SysTimerID_t SysTimerAlloc(void)
{
    SysTimerID_t id;
  
    //首先查看是否时钟列表中是否还有时钟资源
    for (id = 0; id < MAX_SYSTIMERS; id ++) 
    {
        if (SysTimerTable[id].status == SYSTIMER_STATUS_FREE) 
        {
            SysTimerTable[id].status = SYSTIMER_STATUS_INACTIVE;
            
            return id;
        }
    }
  
    return INVALID_SYSTIMER_ID;
}                                       


/* 释放时钟 */
void _SysTimerFree(SysTimerID_t timerID)
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerStop(timerID);
    SysTimerTable[timerID].status = SYSTIMER_STATUS_FREE;
}                                       


/* 系统时钟中断入口函数 */
static void Time1InterruptHandler(void *pdata) 
{
    (void)pdata;
    
    SysTimerTaskIsPending = TRUE;
}        

/* 启动任务时钟 */
void SysTimerStart(SysTimerID_t timerID,        \
                   SysTimerType_t type,         \
                   tick_t tick,              \
                   void (*pSysTimerCallBack)(SysTimerID_t) )
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerDisable(timerID);
  
    /* intervalInTicks 也需要增加一定的修正值 */
    SysTimerTable[timerID].intervalInTicks = tick; //目前尚未增加修正值
  
    SysTimerTable[timerID].countDown = tick;   
  
    SysTimerTypeSet(timerID, type);
  
    SysTimerTable[timerID].pCallBack = pSysTimerCallBack;
  
    SysTimerEnable(timerID);
}                                  

/* 停止时钟 */
void SysTimerStop(SysTimerID_t timerID)
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerDisable(timerID);
  
    SysTimerTable[timerID].intervalInTicks = 0;
    SysTimerTable[timerID].countDown = 0;
    SysTimerTable[timerID].pCallBack = NULL;  
}

/* 启动时钟 */
static void SysTimerEnable(SysTimerID_t timerID)
{    
    uint8_t     hwlock;
  
    EnterCritical(hwlock);
  
    if (SysTimerStatusGet(timerID) == SYSTIMER_STATUS_INACTIVE)
    {      
        numOfActiveTimers ++;
        SysTimerStatusSet(timerID, SYSTIMER_STATUS_READY);
        SysTimerTaskIsPending = TRUE;
    }  
    
    ExitCritical(hwlock);
}

/* 关闭时钟 */
static void SysTimerDisable(SysTimerID_t timerID)
{    
    SysTimerStatus_t    status;
  
    uint8_t             hwlock;
  
    EnterCritical(hwlock);
  
    status = SysTimerStatusGet(timerID);  
    if (status == SYSTIMER_STATUS_ACTIVE || status == SYSTIMER_STATUS_READY) 
    {
        numOfActiveTimers --;
        SysTimerStatusSet(timerID, SYSTIMER_STATUS_INACTIVE);  
    }
  
    ExitCritical(hwlock);
}

/* */
__task void SysTimerTask(event_t event)
{
    tick_t              tick1,	tick2;
    tick_t              deltaTick;
    tick_t              ticksdiff;
    tick_t              nextInterruptTick;
    static tick_t      previousTick = 0;
  
    SysTimerID_t        timerID;  
    SysTimerStatus_t    status;
  
    uint8_t             hwlock;
    
    (void)event;
    
    SysTimerTaskIsPending = FALSE;
    
    tick1 = TickGet();
    deltaTick = tick1 - previousTick;
    previousTick = tick1;
    
    for (timerID = 0; timerID < MAX_SYSTIMERS; timerID ++)
    {
        //ClrWdt();
        
        EnterCritical(hwlock);
        
        status = SysTimerStatusGet(timerID);
        
        if (status == SYSTIMER_STATUS_READY)
        {
            SysTimerStatusSet(timerID, SYSTIMER_STATUS_ACTIVE);
            goto CONTINUE;  
        }
    
        if (status != SYSTIMER_STATUS_ACTIVE)
            goto CONTINUE;
    
        /* 只有当时钟状态为 TIMER_STATUS_ACTIVE 时候才有意义*/
        if (SysTimerTable[timerID].countDown > deltaTick)
        {
            SysTimerTable[timerID].countDown -= deltaTick;
            goto CONTINUE;
        }
    
        /* 
         * 如果countDown <= deltaTick 表示定时时间已经到了，需要执行回调函数 pCallBack.
         * 在执行回调函数之前还需要判断是否是连续定时还是只是一次性的定时，如果是连续性定时还需
         * 要重新设置下次定时时间 countDown
         */
     
        if (SysTimerTypeGet(timerID) == SYSTIMER_TYPE_INTERVAL)
            SysTimerTable[timerID].countDown = SysTimerTable[timerID].intervalInTicks;
        else
            SysTimerDisable(timerID);
     
        if (SysTimerTable[timerID].pCallBack != NULL)
            (SysTimerTable[timerID].pCallBack)(timerID);
     
CONTINUE:
        ExitCritical(hwlock); 
        
    }
    
    //if (numOfActiveTimers == 0)
    //    return;       

    /* 接下来找出最近的一个 active timer */  
    nextInterruptTick = 0xFFFFF;
  
    for (timerID = 0; timerID < MAX_SYSTIMERS; ++timerID) 
    {
       // ClrWdt();
    
        EnterCritical(hwlock);
    
        if (SysTimerStatusGet(timerID) == SYSTIMER_STATUS_ACTIVE) 
        {
            if (nextInterruptTick > SysTimerTable[timerID].countDown) 
            {
                nextInterruptTick = (tick_t)(SysTimerTable[timerID].countDown);
            }
        }
    
        ExitCritical(hwlock);
    }
  
    /* 从 SysTimerTask 函数刚开始执行到这里，已经过了一段时间了，要把这段时间计算在内 */
    EnterCritical(hwlock);
  
    tick2 = TickGet();
    ticksdiff = tick2 - tick1;
  
    /* 查看下一次时间中断是否已经过了 */
    if (nextInterruptTick > ticksdiff)
        nextInterruptTick -= ticksdiff;
    else
        nextInterruptTick = 0;
  
    /* 如果下一次时间中断已经过了，或者离当前时间太近了 */
    if (nextInterruptTick < ONE_MILLISECOND)
        nextInterruptTick = ONE_MILLISECOND;
    
    SleepTimeCompSet(nextInterruptTick);
  
    ExitCritical(hwlock);   
  
} 
