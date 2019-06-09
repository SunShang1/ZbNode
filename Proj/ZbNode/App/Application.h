/*
 *
 * 文件名称：Application.h
 * 文件说明：头文件
 * 
 */
 
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "release.h"
#include "nvram.h"
#include "BosMem.h"
#include "smartNet.h"
#include "sysTask.h"

/********************************************
* Defines *
********************************************/
/* 射频命令 */
enum
{
    SYSCMD_NODE_STATUS                = 0x06,
    

};

enum
{
    CTRL_REBOOT                         = 0x00,
    CTRL_FACTORYRESET,
    CTRL_SLEEP,
};



/********************************************
* Typedefs *
********************************************/
typedef struct
{
    uint8_t prefix[4];
    uint8_t reserved[4]; 
    uint32_t    devId;
    uint8_t type;
    uint16_t seq;
    uint16_t len;
    uint8_t payload[0];
}sys_pkt_hdr_t;


/********************************************
* Function defines *
********************************************/
error_t upgradeInit(void);

void uartCmdExe(uint8_t type, uint8_t *payLoad, uint8_t payLen);

int newImgReleaseNotify(uint8_t *payLoad, uint8_t payLen);

void newImgGetResp(uint8_t *payLoad, uint8_t payLen);

void LampCtrl(uint8_t bright);

void sysReboot(void);

#endif
