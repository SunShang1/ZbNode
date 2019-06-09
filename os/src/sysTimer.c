/*
 * �ļ����ƣ�sysTimer.C
 *
 * �ļ�˵����
 *     BOS����ϵͳ��ʱ��������
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
// ʱ�� ״̬ �б�
// timer ״̬���� (��4λstatus������λtype)
typedef uint8_t         SysTimerStatus_t;


typedef void            (*pSysTimerCallBack_t)(SysTimerID_t);

typedef struct  
{
    SysTimerStatus_t    status; /* ��4λ��ʾ���ͣ���4λ��ʾ״̬ */
    tick_t              intervalInTicks;
    tick_t              countDown;
    pSysTimerCallBack_t pCallBack;
} SysTimerTab_t;
/********************************************
* Globals *
********************************************/ 
static uint8_t          numOfActiveTimers = 0;

// ʱ�� �б�
static SysTimerTab_t    SysTimerTable[MAX_SYSTIMERS];

bool_t                   SysTimerTaskIsPending = FALSE;

/********************************************
* Function defines *
********************************************/


static void Time1InterruptHandler(void *pdata);
__task void SysTimerTask(event_t events);
static void SysTimerEnable(SysTimerID_t timerID);
static void SysTimerDisable(SysTimerID_t timerID);

/* ����ʱ��״̬ */
#define SysTimerStatusSet(SysTimerID, SysTimerStatus)   \
  ( SysTimerTable[SysTimerID].status = (SysTimerTable[SysTimerID].status & 0x0f) | (SysTimerStatus & 0xf0) )

/* ����ʱ������ */
#define SysTimerTypeSet(SysTimerID, SysTimerType)  \
  ( SysTimerTable[SysTimerID].status = (SysTimerTable[SysTimerID].status & 0xf0) | (SysTimerType & 0x0f) )  

/* ��ȡʱ��״̬ */
#define SysTimerStatusGet(SysTimerID)  \
  (SysTimerTable[SysTimerID].status & 0xf0)  


/* ��ȡʱ������ */
#define SysTimerTypeGet(SysTimerID)  \
  ( (SysTimerTable[SysTimerID].status & 0x0f) ) 
  

/* ��ʼ��ϵͳʱ��,����ʱ����ƹ������� */
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


/* ��ϵͳ����һ��ʱ�Ӹ����� */
SysTimerID_t SysTimerAlloc(void)
{
    SysTimerID_t id;
  
    //���Ȳ鿴�Ƿ�ʱ���б����Ƿ���ʱ����Դ
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


/* �ͷ�ʱ�� */
void _SysTimerFree(SysTimerID_t timerID)
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerStop(timerID);
    SysTimerTable[timerID].status = SYSTIMER_STATUS_FREE;
}                                       


/* ϵͳʱ���ж���ں��� */
static void Time1InterruptHandler(void *pdata) 
{
    (void)pdata;
    
    SysTimerTaskIsPending = TRUE;
}        

/* ��������ʱ�� */
void SysTimerStart(SysTimerID_t timerID,        \
                   SysTimerType_t type,         \
                   tick_t tick,              \
                   void (*pSysTimerCallBack)(SysTimerID_t) )
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerDisable(timerID);
  
    /* intervalInTicks Ҳ��Ҫ����һ��������ֵ */
    SysTimerTable[timerID].intervalInTicks = tick; //Ŀǰ��δ��������ֵ
  
    SysTimerTable[timerID].countDown = tick;   
  
    SysTimerTypeSet(timerID, type);
  
    SysTimerTable[timerID].pCallBack = pSysTimerCallBack;
  
    SysTimerEnable(timerID);
}                                  

/* ֹͣʱ�� */
void SysTimerStop(SysTimerID_t timerID)
{
    if ( !(timerID < MAX_SYSTIMERS) )
        return;
  
    SysTimerDisable(timerID);
  
    SysTimerTable[timerID].intervalInTicks = 0;
    SysTimerTable[timerID].countDown = 0;
    SysTimerTable[timerID].pCallBack = NULL;  
}

/* ����ʱ�� */
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

/* �ر�ʱ�� */
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
    
        /* ֻ�е�ʱ��״̬Ϊ TIMER_STATUS_ACTIVE ʱ���������*/
        if (SysTimerTable[timerID].countDown > deltaTick)
        {
            SysTimerTable[timerID].countDown -= deltaTick;
            goto CONTINUE;
        }
    
        /* 
         * ���countDown <= deltaTick ��ʾ��ʱʱ���Ѿ����ˣ���Ҫִ�лص����� pCallBack.
         * ��ִ�лص�����֮ǰ����Ҫ�ж��Ƿ���������ʱ����ֻ��һ���ԵĶ�ʱ������������Զ�ʱ����
         * Ҫ���������´ζ�ʱʱ�� countDown
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

    /* �������ҳ������һ�� active timer */  
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
  
    /* �� SysTimerTask �����տ�ʼִ�е�����Ѿ�����һ��ʱ���ˣ�Ҫ�����ʱ��������� */
    EnterCritical(hwlock);
  
    tick2 = TickGet();
    ticksdiff = tick2 - tick1;
  
    /* �鿴��һ��ʱ���ж��Ƿ��Ѿ����� */
    if (nextInterruptTick > ticksdiff)
        nextInterruptTick -= ticksdiff;
    else
        nextInterruptTick = 0;
  
    /* �����һ��ʱ���ж��Ѿ����ˣ������뵱ǰʱ��̫���� */
    if (nextInterruptTick < ONE_MILLISECOND)
        nextInterruptTick = ONE_MILLISECOND;
    
    SleepTimeCompSet(nextInterruptTick);
  
    ExitCritical(hwlock);   
  
} 
