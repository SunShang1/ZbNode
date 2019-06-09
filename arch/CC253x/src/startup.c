/*
 * 文件名称： startup.c
 *
 * 文件说明： 系统启动文件
 */

#include "board.h"
#include "kernel.h"
#include "sysTimer.h"
#include "BosMem.h"
#include "smartNet.h"

static uint8_t    Heap[HEAP_SIZE];


extern error_t ApplicationInit(void);

/* main 函数 */
void main(void)
{
    halBoardInit();
    
    HeapInit((void *)Heap, Heap+sizeof(Heap)-1); 
    
    consoleInit();
    
    BosInit();
    
    /* init system time */
    SysTimerInit();
    
    nvramInit();
    
    smartNetInit();
    
    ApplicationInit();
    
    /* enable interrupts */
    EnableInterrupts();    
    
    BosScheduler();
    
    /* 这个return永远不会到达 */
    return;
}