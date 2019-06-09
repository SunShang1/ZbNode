/*
 * 文件名称：sysTask.c
 * 文件说明：define all the tasks 
 * 
 */

#include "kernel.h"
#include "sysTask.h"
/********************************************
Defines
********************************************/




/********************************************
Typedefs
********************************************/



/********************************************
* Globals *
********************************************/
extern __task void  SysTimerTask(event_t);
extern __task void  sMacTask(event_t);
extern __task void  sNwkTask(event_t);
extern __task void  AppTask(event_t);
extern __task void UpgradeTask(event_t);
extern __task void netMngrTask(event_t);

#ifdef USHELL
extern __task void uShellTask(event_t event);
#endif


__code const pTaskFn tasksArr[MAX_TASK_NUM] =
{
    SysTimerTask,
    sMacTask,
    sNwkTask,
#ifdef USHELL
    uShellTask,
#endif
    AppTask,
    UpgradeTask,
    
    netMngrTask,
};


/********************************************
* Function defines *
********************************************/