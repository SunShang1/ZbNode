/*
 *  文件名称：sysTask.h
 *
 *  文件说明：系统任务定义头文件
 *
 */

#ifndef  __SYSTASK_H_
#define  __SYSTASK_H_

#include "pub_def.h"
#include "error.h"


/********************************************
* Defines *
********************************************/
enum
{
    TASK_SYSTIMER_ID            = 0,
    TASK_SMAC_ID,
    TASK_SNWK_ID,
#ifdef USHELL
    TASK_USHELL_ID,
#endif
    TASK_APPLICATION_ID,
    TASK_UPGRADE_ID,
    
    TASK_NETMNGR_ID,
    
    MAX_TASK_NUM,
};

/********************************************
* Typedefs *
********************************************/




/********************************************
* Function defines *
********************************************/




#endif



