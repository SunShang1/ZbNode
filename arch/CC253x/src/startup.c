/*
 * �ļ����ƣ� startup.c
 *
 * �ļ�˵���� ϵͳ�����ļ�
 */

#include "board.h"
#include "kernel.h"
#include "sysTimer.h"
#include "BosMem.h"
#include "smartNet.h"

static uint8_t    Heap[HEAP_SIZE];


extern error_t ApplicationInit(void);

/* main ���� */
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
    
    /* ���return��Զ���ᵽ�� */
    return;
}