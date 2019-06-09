/*
 *  Copyrite (C) 2010, BeeLinker
 *
 *  �ļ����ƣ�BosMem.h
 *  �ļ�˵����BOS �ڴ����ͷ�ļ�
 *
 *
 *  �汾��Ϣ��
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



