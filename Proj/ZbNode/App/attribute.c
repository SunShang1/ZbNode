/*
 * attr.c
 * Description: attribute managerment for system
 */

#include <string.h>
#include "attribute.h"
#include "Application.h"
#include "release.h"
#include "inet.h"

/********************************************
* Defines *
********************************************/


/********************************************
* Typedefs *
********************************************/


/********************************************
* Globals * 
********************************************/
__code const uint8_t SizeOfAttr[ATTR_MAXNUM] = 
{
2,//    ATTR_SWVER    = 0,
1,//    ATTR_HWVER,
4,//    ATTR_GITSHA,
2,//    ATTR_HEARTBEAT,
//    
1,//    ATTR_LAMPPOWER = 15,
//    
1,//    ATTR_LEDMODE    = 25,
1,//    ATTR_LEDBRIGHT  = 26,
//    
20,//    ATTR_ICCID    = 34,
//    
1,//    ATTR_SQ      = 42,
15,//    ATTR_IMEI    = 43,
};

extern char    iccid[20];

extern uint8_t   rssi;

extern uint8_t LampPower;
/********************************************
* Function defines *
********************************************/
uint8_t attributeGet(uint8_t attr, uint8_t *ptr, uint8_t bufSize)
{
    uint8_t git[4] = {SOFT_SHA_HEX};
    uint8_t len = 0;
    uint8_t *tmpBuf;
    
    if ((tmpBuf = BosMalloc(32)) == NULL)
        return 0;

    tmpBuf[len ++] = attr;
    
    switch(attr)
    {
        case ATTR_SWVER:
            tmpBuf[len ++] = FW_TYPE;
            tmpBuf[len ++] = SW_VER_MAJOR;
            tmpBuf[len ++] = SW_VER_MINOR;
            break;
            
        case ATTR_HWVER:
            nvramSysBlkParamGet(hwVer, tmpBuf + len);
            len ++;
            break;
            
        case ATTR_GITSHA:
            memcpy(tmpBuf+len, git, sizeof(git));
            len += sizeof(git);
            break;
            
        case ATTR_HEARTBEAT:
        {
            uint16_t   heartBeat;
            
            nvramSysBlkParamGet(heartBeat, &heartBeat);
            heartBeat = htons(heartBeat);
            
            memcpy(tmpBuf+len, &heartBeat, sizeof(heartBeat));
            
            len += sizeof(heartBeat);
            break;
        }
        
        case ATTR_LAMPPOWER:
            tmpBuf[len ++] = 150;
            break;

        case ATTR_LEDBRIGHT:
            tmpBuf[len ++] = 0x0A;
            break;
            
        case ATTR_ICCID:
            memcpy(tmpBuf+len, iccid, sizeof(iccid));
            len += sizeof(iccid);
            break;
       
        case ATTR_SQ:
            tmpBuf[len ++] = rssi;
            break;
            
        default:
            len --;
            break;
    }
    
    if (len <= bufSize)
        memcpy(ptr, tmpBuf, len);
    else
        len = 0;
    
    BosFree(tmpBuf);
    return len;
}

// auth: 0 -- usart
//       1 -- rf
error_t attributeSet(uint8_t *ptr, uint8_t attrLen, uint8_t auth)
{    
    error_t err = BOS_EOK;    
    uint8_t attrNum;
    int8_t len = (int8_t)attrLen;
    nvSysBlk_t *sysBlk;
    uint8_t write = 0;
    
    if ((ptr == NULL) || (attrLen == 0))
        return -BOS_ERROR;
    
    attrNum = *ptr ++;
    len --;
    
    if (nvramBlkParamSetInit(NV_BLK_SYS, (uint8_t **)&sysBlk) < 0)
        return -BOS_ERROR;

    while((attrNum --)&&(err == BOS_EOK))
    {
        uint8_t attr = *ptr ++;
        
        len --;
        if (len <= 0)
        {
            err = -BOS_ERROR;
        }
        
        if (err != BOS_EOK)
            break;
        
        switch(attr)
        {
            default:
                err = -BOS_ERROR;
                break;
        }
    }
    
    if (err != BOS_EOK)
    {
        nvramBlkParamSetFinal(NV_BLK_SYS, (uint8_t *)sysBlk, 0);
    }
    else
    {
        nvramBlkParamSetFinal(NV_BLK_SYS, (uint8_t *)sysBlk, write);
    }
    
    return err;
}

error_t attributeList(uint8_t *ptr, uint8_t bufSize, uint8_t *attr, uint8_t *attrLen)
{    
    int8_t len = (int8_t)bufSize;
    uint8_t attrIndex;
    
    if ((ptr == NULL) || (bufSize == 0))
        return -BOS_ERROR;
    
    attrIndex = *ptr ++;    
    len --;
    
    if (attrIndex >= ATTR_MAXNUM)
        return -BOS_ERROR;
    
    *attrLen = 1+SizeOfAttr[attrIndex];
    
    len -= (*attrLen-1);
    
    if (len >= 0)
    {
        *attr = attrIndex;
        
        return BOS_EOK;
    }
    
    return -BOS_ERROR;
}


