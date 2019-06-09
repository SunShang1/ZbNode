/*
 * 文件名称：Kernel.C
 *
 * 文件说明：BOS 操作系统内核
 */

#include "hal_mcu.h"
#include "kernel.h"

/********************************************
* Defines *
********************************************/
#define  BOS_MAX_TASKS                  40

/********************************************
* Typedefs *
********************************************/
struct BosTaskTab_t
{
    uint8_t             taskId;
    event_t             event;
};



/********************************************
* Globals *
********************************************/ 
//任务队列
struct BosTaskTab_t     BosTaskTable[BOS_MAX_TASKS];
static uint8_t          BosTaskTabRead;
static uint8_t          BosTaskTabWrite; 
static uint8_t          TaskNum;

extern bool_t           SysTimerTaskIsPending; 
extern __code const pTaskFn   tasksArr[];

/********************************************
* Function defines *
********************************************/

/* 初始化操作系统 */
error_t BosInit(void) 
{
    // 初始化任务列表 和 优先级列表
    uint8_t  i;
  
    for (i = 0; i < BOS_MAX_TASKS; i ++)
    {
        BosTaskTable[i].taskId = 0xFF;
    }
    
    BosTaskTabRead = BosTaskTabWrite = 0;
    TaskNum = 0;
    
    return BOS_EOK;

}                                                       

/* 向一个任务添加一个事件 */
error_t BosEventSend(uint8_t taskId, event_t event)
{
    error_t     error = -BOS_ERROR;
    
    uint8_t     hwlock;

    EnterCritical(hwlock);
    if (TaskNum < BOS_MAX_TASKS)
    {
        BosTaskTable[BosTaskTabWrite].taskId = taskId;
        BosTaskTable[BosTaskTabWrite].event = event;
        
        if (++BosTaskTabWrite == BOS_MAX_TASKS)
            BosTaskTabWrite = 0;
        
        TaskNum ++;
        
        error = BOS_EOK;
    }
    ExitCritical(hwlock);
    
    return error;
}                 
 
/* 任务调度 */
void BosScheduler(void) 
{
    BosTaskHandler_t    BosTask;
    event_t             event;
  
    uint8_t             hwlock;
    
    extern __task void  SysTimerTask(event_t);
    
    
    for (;;)
    {
     //   ClrWdt();
        
        BosTask = NULL;
        
        EnterCritical(hwlock);
        if (SysTimerTaskIsPending == TRUE)
        {
            BosTask = SysTimerTask;
        }
        else
        {
            if (TaskNum > 0)
            {
                BosTask = tasksArr[BosTaskTable[BosTaskTabRead].taskId];
                event = BosTaskTable[BosTaskTabRead].event;
                
                if (++BosTaskTabRead == BOS_MAX_TASKS)
                    BosTaskTabRead = 0;
                
                TaskNum --;
            }
        }
        
        if (BosTask == NULL)
        {
     //       wait(); //没有其他任务，修眠
      
            ExitCritical(hwlock); //这里可以不要，但是考虑到 ENTER 和 EXIT 需要成对出现，所以最好加上
            continue;
        }            
        ExitCritical(hwlock);
        
        (*BosTask)(event);
    }
}
