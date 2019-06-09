/*
 * 
 * 文件名称: upgrade.c
 * 文件说明：固件升级
 *
 */

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "sysTimer.h"
#include "kernel.h"
#include "Application.h"
#include "crc.h"
#include "inet.h"
#include "attribute.h"

 /********************************************
* Defines *
********************************************/
#define UPGRADE_NEWIMGREQ_EVENT         0x01



#define MAX_PAYLOAD_LEN				    84


/********************************************
* Typedefs *
********************************************/
enum
{
    STATUS_IDLE         = 0x00,
    STATUS_UPGRADING,
};

typedef struct
{
    uint8_t         status;
    
    uint8_t         major;
    uint8_t         minor;
    
    uint16_t        size;    
    uint16_t        offset;    
    uint16_t        expect;
    
    SysTimerID_t    upgradeTimerId;
    
    uint8_t         retry;
    
}newImgInfo_t;

typedef struct
{
    uint16_t    icrc;
    uint16_t    isize;
} imgHdr_t;
/********************************************
* Globals * 
********************************************/
newImgInfo_t    newImg;


/********************************************
* Function defines *
********************************************/
/* */
__task void UpgradeTask(event_t);

static void upgradeTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
    
    BosEventSend(TASK_UPGRADE_ID, UPGRADE_NEWIMGREQ_EVENT);
}

/* */
error_t upgradeInit(void)
{
    memset(&newImg, 0, sizeof(newImg));
    
    newImg.status = STATUS_IDLE;

    return BOS_EOK;
}

/* */
int newImgReleaseNotify(uint8_t *payLoad, uint8_t payLen)
{
    if (newImg.status == STATUS_UPGRADING)
        return 1;
    
    newImg.major = *payLoad ++;
    newImg.minor = *payLoad ++;
                
    memcpy(&newImg.size, payLoad, sizeof(payLoad));
    newImg.size = ntohs(newImg.size);
    
    if ( (newImg.major == SW_VER_MAJOR) &&
         (newImg.minor == SW_VER_MINOR) )
        return 2;
    
    /* start upgrading */
    newImg.status = STATUS_UPGRADING;
    
    newImg.upgradeTimerId = SysTimerAlloc();
    
    SysTimerStart(newImg.upgradeTimerId, SYSTIMER_TYPE_INTERVAL, ONE_MILLISECOND*100, upgradeTimeOutCallBack);
    
    newImg.retry = 100;
                
    return BOS_EOK;
}

void newImgGetResp(uint8_t *payLoad, uint8_t payLen)
{
  
}

__task void UpgradeTask(event_t event)
{
  
}


