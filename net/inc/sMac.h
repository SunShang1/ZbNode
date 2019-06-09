/*
 * 文件名称：sMac.h
 * 文件说明：smartNet MAC层协议头文件
 * 
 */
 
#ifndef __SMAC_H__
#define __SMAC_H__

#include "saddr.h"
#include "error.h"
#include "message.h"
#include "nvram.h"
#include "smartNet.cfg"

/********************************************
* Defines *
********************************************/
/* frame type version define */
#define SMAC_FRAME_VERSION      1

/* frame type define */
#define SMAC_BEACON_TYPE        0
#define SMAC_DATA_TYPE          1
#define SMAC_ACK_TYPE           2
#define SMAC_CMD_TYPE           3

/* MAC CMD define */
enum
{
    //SMAC CMD
    CMD_SEARCH_REQ =            0x01,
    CMD_SEARCH_RESP =           0x81,
    
    CMD_PING =                  0x02,
    CMD_PONG =                  0x82,
    
    CMD_JOIN_REQ =              0x03,
    CMD_JOIN_RESP =             0x83,
};

enum
{
    STATUS_UNLINK                   = 0,
    STATUS_SEARCH                   = 1,
    STATUS_LQ_CHECK                 = 2,
    STATUS_LINK_REQ                 = 3,
    STATUS_LINKED                   = 4,
};

#define MAC_HEAD_LEN                                sizeof(struct sMacMpdu_t)

/********************************************
* Typedefs *
********************************************/
/* 媒体介入层控制帧字段定义 */
union sMacFrameCtrl_t
{
    struct 
    {
        uint16_t            channel         :8;
        uint16_t            frameType       :3;
        uint16_t            security        :1;
        uint16_t            framePending    :1;
        uint16_t            ackRequest      :1;
        uint16_t            version         :2;        
    }bits;
    
    uint16_t    u16Val;
};        

/* 媒体接入层通用帧结构 */
struct sMacMpdu_t
{   
    /* header */
    union sMacFrameCtrl_t   frameCtrl;
    uint8_t                  seqNum;
    uint16_t                 panId;
    saddr_t                  macDstAddr;
    saddr_t                  macSrcAddr;

    /* payload */
    uint8_t                  pdata[0];
};

/* 媒体接入层 ACK 帧结构 */
struct sMacAck_t
{   
    /* header */
    union sMacFrameCtrl_t   frameCtrl;
    uint8_t                  seqNum;
    uint16_t                 panId;
    saddr_t                  macDstAddr;
    saddr_t                  macSrcAddr;
};

/* 网络搜索响应命令结构 */
struct sMacCmdSearchResp_t
{
    uint8_t     type;
    
    saddr_t     coor;
    uint16_t    panId;
    
    uint8_t     depth;
    uint8_t     sonNum;
};

struct sMacCmdJoinReq_t
{
    uint8_t     type;
    saddr_t     devAddr;
    saddr_t     fatherAddr;
};

struct sMacCmdJoinResp_t
{
    uint8_t     result;
    uint8_t     type;
    saddr_t     devAddr;
    saddr_t     fatherAddr;
};

/* */
typedef void            (*DataIndicationCallBack_t)(void *);

/* */
typedef void            (*sMacCmdProcess_t)(saddr_t *, uint8_t *, uint8_t);

/********************************************
* Function defines *
********************************************/
/* */
error_t sMacInit(void);

/* */
uint8_t sMacDeviceTypeGet(void);

/* */
saddr_t sMacAddressGet(void);

/* */
saddr_t sMacCoordinatorAddressGet(void);

/* */
saddr_t sMacFatherAddressGet(void);

/* */
uint8_t sMacLinkStatusGet(void);

/* */
error_t sMacSend(saddr_t dstAddr, struct MsgSendLinkNode_t *pMsgNode);

/* */
void sMacDataIndicationCallBackSet(DataIndicationCallBack_t p);

/* */
error_t sMacNetAutoStart(void (*pCallBack)(uint8_t status));

saddr_t sMacCoorAddrGet(void);

saddr_t sMacFatherAddrGet(void);

uint16_t sMacLinkSQGet(void);

uint8_t sMacSonAddrGet(uint8_t *buf, uint8_t bufSize);

#endif
