/*
 *  Copyrite (C) 2010, BeeLinker
 *
 *  文件名称：BosMem.h
 *  文件说明：BOS 内存管理头文件
 *
 *
 *  版本信息：
 *  v0.1      wzy         2011/03/11
 *
 */

#ifndef  BOS_MEM_H
#define  BOS_MEM_H

#include "pub_def.h"
#include "error.h"


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
/* Init the Memory */
error_t HeapInit(void *, void *);

/* memory alloc */
void* BosMalloc(const uint16_t);   

/* free Memory */
void _BosFree(const void *);

#define BosFree(ptr)    \
do  \
{ \
    if (ptr != NULL)    \
    {   \
        _BosFree(ptr);  \
        ptr = NULL; \
    }   \
}while(0)

/* */
void *BosMemSet(const void *, const uint8_t, const uint16_t);



#endif



