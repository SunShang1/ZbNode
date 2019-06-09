/*
 * 
 * 文件名称: attribute.h
 * 文件说明：设备属性描述头文件
 *
 */

#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

#include "pub_def.h"
#include "error.h"
#include "sysTimer.h" 

/********************************************
* Defines *
********************************************/



/********************************************
* Typedefs *
********************************************/
enum
{
    ATTR_SWVER    = 0,
    ATTR_HWVER,
    ATTR_GITSHA,
    ATTR_HEARTBEAT,
    
    ATTR_LAMPPOWER = 15,
    
    ATTR_LEDMODE    = 25,
    ATTR_LEDBRIGHT  = 26,
    
    ATTR_ICCID    = 34,
    
    ATTR_SQ      = 42,
    ATTR_IMEI    = 43,
    
    ATTR_MAXNUM,
};

/********************************************
* Globals * 
********************************************/


/********************************************
* Function defines *
********************************************/
uint8_t attributeGet(uint8_t attr, uint8_t *ptr, uint8_t bufSize);
error_t attributeSet(uint8_t *ptr, uint8_t attrLen, uint8_t auth);
error_t attributeList(uint8_t *ptr, uint8_t bufSize, uint8_t *attr, uint8_t *attrLen);
void *attributeSearch(uint8_t dstAttr, uint8_t *buf, uint8_t bufSize);

#endif
