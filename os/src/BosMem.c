/*
 * 文件名称：BosMem.C
 *
 * 文件说明：内存管理
 * 
 */

#include "pub_def.h"
#include "error.h"
#include "hal_mcu.h"

/********************************************
* Defines *
********************************************/
#define MEM_HEAD_LEN        sizeof(struct MemHead_t)



/********************************************
* Typedefs *
********************************************/
struct MemHead_t 
{
    uint16_t    IsUsed:         1;            
    uint16_t    ByteSize:       15;              
};


/********************************************
* Globals *
********************************************/ 
static uint8_t              *HeapBase;
static uint16_t             HeapSize;   //Byte Size


/********************************************
* Function defines *
********************************************/

/* Init the Memory */
error_t HeapInit(void *base, void *end)
{
    struct MemHead_t    *head;
    
    HeapBase = (uint8_t *)base;
    HeapSize = (uint8_t *)end - (uint8_t *)base;
    
    head = (struct MemHead_t *)HeapBase;
        
    head->IsUsed = FALSE;
    head->ByteSize = HeapSize - MEM_HEAD_LEN;
        
    return SUCCESS;
}

/* memory alloc */
void* BosMalloc(const uint16_t  ByteSize)
{
    uint8_t             *MemAddr;
    uint8_t             *HeapLimit;
    struct MemHead_t*   head;
    
    uint8_t             hwlock;
    
    MemAddr = HeapBase;
    HeapLimit = HeapBase + HeapSize; 
    
    for (;;)
    {
        if (MemAddr >= HeapLimit)
        {
            return NULL;
        }
            
        head = (struct MemHead_t *)MemAddr;
        
        EnterCritical(hwlock);
        if ( (head->IsUsed == TRUE) || (head->ByteSize < ByteSize) )
        {
            ExitCritical(hwlock);
            
            MemAddr += (head->ByteSize + MEM_HEAD_LEN);
            continue;
        }
        else
        {
            if (head->ByteSize > ByteSize + MEM_HEAD_LEN)
            {
                struct MemHead_t*    nextHead;
                
                nextHead = (struct MemHead_t *)(MemAddr + MEM_HEAD_LEN + ByteSize);
            
                nextHead->IsUsed = FALSE;
                nextHead->ByteSize = head->ByteSize - ByteSize - MEM_HEAD_LEN;
                head->ByteSize = ByteSize;
            }    
            
            head->IsUsed = TRUE;
            
            ExitCritical(hwlock);
            
            return (MemAddr + MEM_HEAD_LEN);
        }
                
    }
}    

/* free Memory */
void _BosFree(const void *FreeAddr)
{
    struct MemHead_t    *beforeHead;
    struct MemHead_t    *head;
    struct MemHead_t    *nextHead;
    
    uint8_t             *MemAddr;
    uint8_t             *HeapLimit;
    
    uint8_t             hwlock;
    
    MemAddr = HeapBase;
    HeapLimit = HeapBase + HeapSize;   
    head = NULL;
    
    for(;;) 
    {
        beforeHead = head;
        head = (struct MemHead_t *)MemAddr;
        
        if (FreeAddr == MemAddr + MEM_HEAD_LEN)
            break;
        
        MemAddr += (head->ByteSize + MEM_HEAD_LEN);
        
        if (MemAddr >= HeapLimit)
            return;
    }
    
    MemAddr += (head->ByteSize + MEM_HEAD_LEN);
    
    EnterCritical(hwlock);
    
    head->IsUsed = FALSE;

/* if the Mem Before head is not Alloc, combine them */    
    if ( (beforeHead != NULL) && (beforeHead->IsUsed == FALSE) )
    {
        beforeHead->ByteSize += (head->ByteSize + MEM_HEAD_LEN);
        head = beforeHead;
    }

/* if the Mem After head is not Alloc ,combine them too */    
    if (MemAddr >= HeapLimit)
    {
        ExitCritical(hwlock);
        return;
    }

    nextHead = (struct MemHead_t *)MemAddr;
    
    if ( nextHead->IsUsed == FALSE )
        head->ByteSize += (nextHead->ByteSize + MEM_HEAD_LEN); 
    
    ExitCritical(hwlock);
    
    return;

}

/* */
void *BosMemSet(const void *buf, const uint8_t c, const uint16_t len)
{
    uint16_t    i;
    uint8_t     *p = (uint8_t *)buf;
    
    for (i = 0; i < len; i ++)
    {
        *p ++ = c;
    }
    
    return (void *)buf;
}