/*
 * 文件名称：sMac.c
 * 文件说明：smartNet协议栈MAC层实现
 * 
 */
 
#include "pub_def.h"
#include "smartNet.cfg"
#include "sMac.h"
#include "sNwk.h"
#include "BosMem.h"
#include "Kernel.h"
#include "SysTimer.h"
#include "board.h"
#include "inet.h"
#include "crc.h"
#include "sysTask.h"
#include "list.h"
#include "dbg.h"

#ifdef USHELL
#include "ush_cmd.h"
#endif

#include <stdlib.h>
/********************************************
Defines
********************************************/
#define SMAC_MSG_SEND_EVENT         0x01
#define SMAC_MSG_RECV_EVENT         0x02

#define SMAC_SEARCH_INIT_EVENT      0x03
#define SMAC_SEARCH_REQ_EVENT       0x04
#define SMAC_PINGPONG_EVENT         0x05
#define SMAC_JOIN_REQ_EVENT         0x07
#define SMAC_SEARCH_STOP_EVENT      0x08

#define SMAC_MAX_RETRY_TIMES        6

/* ACK等待超时时间，单位：mS */
#define ACK_TIMEOUT                 (ONE_MILLISECOND * 100)
/********************************************
Typedefs
********************************************/
/* 节点网络信息描述 */
struct sMacNetInfo_t
{
    uint8_t                         netStatus;
    
    saddr_t                         father;
    saddr_t                         coordinator;
    
    uint16_t                        panId;
    uint8_t                         channel;
    uint8_t                         depth;
    
    uint8_t                         rssi;
    uint8_t                         rxCntInPercent;
};

/* 搜索响应的节点列表 */
struct sMacSearchListNode_t
{
    list_t                          link;
    
    uint8_t                         type;    
    saddr_t                         address;
    
    saddr_t                         coordinator;
    
    uint16_t                        panId;
    
    uint8_t                         channel;
    uint8_t                         depth;
    uint8_t                         sonNum;
    
    uint8_t                         rssi;
    uint8_t                         rxCntInPercent;
};

struct sMacSearchHandle_t
{
    //search
    uint8_t                         startChannel;
    uint8_t                         endChannel;    
    uint8_t                         retry;
    
    //search list
    uint8_t                         searchCnt;
    list_t                          searchList;
    uint8_t                         handleTimerId;
    
    //pingpong
    uint8_t                         nodeIndex;
    uint8_t                         pingSeq;
//    uint8_t                         handleTimerId;
//    
//    //join request
//    uint8_t                         nodeIndex;
//    uint8_t                         retry;
//    uint8_t                         handleTimerId;
    
};

/* 子节点信息列表 */
struct sMacSonListNode_t
{
    list_t                          link;
    
    uint8_t                         type;    
    saddr_t                         sonAddr;
    
    //最大允许连续发送失败次数，超过这个次数就判断为设备失联
#define MAX_SON_RETRY               3
    uint8_t                         retry;
};

struct sMacSonHandle_t
{
    uint8_t                         sonNum;
    
    list_t                          sonList;
};
/********************************************
* Globals *
********************************************/
static struct sMacNetInfo_t        netInfo;

static struct sMacSonHandle_t      *sMacSonHandle;

static struct sMacSearchHandle_t   *sMacSearchHandle;

static void (*sMacNetAutoCB)(uint8_t status);
/* MAC帧序列号 */
static  uint8_t                     MySeqNum;

static bool_t                       AckIsPending;
static uint8_t                      AckSeqNum;
static SysTimerID_t                 AckTimerId;

/* 发送数据帧链表/队列 */
static struct MsgSendLinkNode_t     *sMacMsgSendLinkHead;
/* 接收数据帧链表/队列*/
volatile struct MsgRecvLinkNode_t   *sMacMsgRecvLinkHead;
/* */
volatile uint8_t                    RecvLinkCnt;


static DataIndicationCallBack_t     sMacDataIndication;

/* 数据发送定时器 */
static SysTimerID_t                 MsgSendTimerId;


/********************************************
* Function defines *
********************************************/
static error_t  PhySend(uint8_t *, uint8_t);
//extern uint8_t  PhyRxRssiGet(void);

static void RfRxIndicateIsr(void);

/* */
__task void sMacTask(event_t);

/* */
static void PhyDataIndication(void *);

/* */
static error_t sMacCmdNetSearchReq(void);

/* */
static error_t sMacCmdNetSearchResp(saddr_t);

/* */
static error_t sMacCmdPingPong(uint8_t cmd, saddr_t dstAddr, uint8_t seq);

/* */
static error_t sMacCmdJoinReq(saddr_t dstAddr,  uint8_t* payload, uint8_t len);

/* */
static error_t sMacCmdJoinResp(saddr_t dstAddr, uint8_t* payload, uint8_t len);

/* 网络搜索定时 */
static void sMacNetSearchTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
  
    BosEventSend(TASK_SMAC_ID,SMAC_SEARCH_REQ_EVENT);
}

/* 网络PING PONG定时 */
static void sMacPingTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
  
    BosEventSend(TASK_SMAC_ID,SMAC_PINGPONG_EVENT);
}

static void sMacJoinReqTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
  
    BosEventSend(TASK_SMAC_ID,SMAC_JOIN_REQ_EVENT);
}

/* */
error_t sMacInit(void)
{
    uint8_t power;
    
    /* RF init */
    halRfInit();
    
    memset(&netInfo, 0, sizeof(netInfo));
    nvramSysBlkParamGet(RadioChannel, &netInfo.channel);       
    halRfSetChannel(netInfo.channel);
    
    /* */
    if (sMacDeviceTypeGet() == COORDINATOR)
    {
        //creat net
        do
        {
            netInfo.panId = halRfGetRandomByte();
            netInfo.panId <<= 8;
            netInfo.panId += halRfGetRandomByte();
        }while(netInfo.panId == 0xFFFF);
        
        netInfo.netStatus = STATUS_LINKED;
    }

    nvramSysBlkParamGet(RadioPower, &power); 
    if (power > HAL_RF_TXPOWER_20_DBM)
        power = HAL_RF_TXPOWER_20_DBM;
    halRfSetTxPower(power);
    
    halRfRxInterruptConfig(RfRxIndicateIsr);
    
    halRfReceiveOn();
    
    sMacMsgSendLinkHead = NULL;
    sMacMsgRecvLinkHead = NULL;
    
    RecvLinkCnt = 0;
    
    MySeqNum = halRfGetRandomByte();

    AckIsPending = FALSE;
    AckSeqNum = 0;
    AckTimerId = SysTimerAlloc();
    
    sMacDataIndication = NULL;
    
    MsgSendTimerId = SysTimerAlloc();
    
    sMacSearchHandle = NULL;
    sMacSonHandle = NULL;
    
    if (sMacDeviceTypeGet() != COORDINATOR)
    {
        sMacNetAutoCB = NULL;
    }
    
    if (sMacDeviceTypeGet() != ENDDEVICE)
    {
        sMacSonHandle = BosMalloc(sizeof(struct sMacSonHandle_t));
        BosMemSet(sMacSonHandle, 0, sizeof(struct sMacSonHandle_t));
        
        INIT_LIST_HEAD(&(sMacSonHandle->sonList));
    }

    return SUCCESS;
}

/* 定时检查ACK */
static void AckTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
    
    if (!AckIsPending)
        return;
    
    AckIsPending = FALSE;    
    BosEventSend(TASK_SMAC_ID,SMAC_MSG_SEND_EVENT);
}

static void MsgSendTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
    
    BosEventSend(TASK_SMAC_ID,SMAC_MSG_SEND_EVENT);
}

/* */
__task void sMacTask(event_t    event)
{
    if (event == SMAC_MSG_SEND_EVENT)
    {
        struct MsgSendLinkNode_t    *MsgNode;
        struct MsgSendLinkNode_t    *preMsgNode;
        struct sMacMpdu_t           *sMpdu;
        tick_t                       curTick;
        tick_t                       nextTick;

        if (AckIsPending)
            return;
        
        curTick = TickGet();
        nextTick = (tick_t)(-1L); 
        
        preMsgNode = NULL;
        MsgNode = sMacMsgSendLinkHead;
        
        for (;;)
        {
            if (sMacMsgSendLinkHead == NULL)
            {
                return;
            }
            
            if (sMacMsgSendLinkHead->retry == 0)
            {
                //重复多次都发送失败，调用回调函数
                MsgNode = sMacMsgSendLinkHead;
                sMacMsgSendLinkHead = sMacMsgSendLinkHead->next;
            
                sMpdu = (struct sMacMpdu_t *)MsgNode->msg.pdata;
                
                if ( (sMpdu->frameCtrl.bits.frameType == SMAC_DATA_TYPE)  ||\
                     (sMpdu->frameCtrl.bits.frameType == SMAC_CMD_TYPE)  )
                {
                    if (sMpdu->macDstAddr != 0xFFFFFFFF)
                    {
                        struct sMacSonListNode_t    *node;
                        list_t          *entry, *n;
                        
                        //son retry --
                        list_for_each_safe(entry, n, &(sMacSonHandle->sonList))
                        {
                            node = list_entry(entry, struct sMacSonListNode_t, link);
                            
                            if (node->sonAddr != sMpdu->macDstAddr)
                                continue;
                                                   
                            if (node->retry == 0)
                            {
                                list_del(entry);
                            
                                BosFree(node);
                                
                                sNwkDelateRoute(node->sonAddr);
                                
                                sMacSonHandle->sonNum --;
                            }
                        }
                    }
                }
            
                if(MsgNode->pMacSendOutCallBack != NULL)
                {
                    MsgNode->pMacSendOutCallBack(MsgNode,FALSE);
                }
                else
                {
                    BosFree(MsgNode->msg.pdata);
                    BosFree(MsgNode);
                }
            }
            else
            {
                if ( (curTick - MsgNode->sendTick) < (((tick_t)(-1L))>>1) )
                {
                    // 发送时间到，准备发送当前数据
                    if (preMsgNode != NULL)
                    {
                        preMsgNode->next = MsgNode->next;
                        MsgNode->next = sMacMsgSendLinkHead;
                        sMacMsgSendLinkHead = MsgNode;
                    }
                    
                    break;
                }
                else
                {
                    //发送时间没有到达，继续往下找
                    if (nextTick > (MsgNode->sendTick - curTick))
                        nextTick = MsgNode->sendTick - curTick;
                    
                    preMsgNode = MsgNode;
                    MsgNode = MsgNode->next;
                    
                    if (MsgNode == NULL)
                    {
                        SysTimerStart(MsgSendTimerId, SYSTIMER_TYPE_ONCE, nextTick, MsgSendTimeOutCallBack);
                        return;
                    }
                }
            }              
                
        }
        
        sMpdu = (struct sMacMpdu_t *)sMacMsgSendLinkHead->msg.pdata;
        
        sMacMsgSendLinkHead->retry --;
        
        if (BOS_EOK == PhySend((uint8_t *)sMpdu, sMacMsgSendLinkHead->msg.msgLen))
        {         
            if (sMpdu->frameCtrl.bits.ackRequest == TRUE)
            {
                AckIsPending = TRUE;
                AckSeqNum = sMpdu->seqNum;
                
                /* 开启定时器 */
                SysTimerStart(AckTimerId, SYSTIMER_TYPE_ONCE, ACK_TIMEOUT, AckTimeOutCallBack);
            }
            else
            {
                /* 如果不需要ACK，可以直接删除msg了 */
                MsgNode = sMacMsgSendLinkHead;
                sMacMsgSendLinkHead = sMacMsgSendLinkHead->next;
                
                if (MsgNode->pMacSendOutCallBack != NULL)
                {
                    MsgNode->pMacSendOutCallBack(MsgNode, TRUE);
                }
                else
                {
                    BosFree(MsgNode->msg.pdata);
                    BosFree(MsgNode); 
                }
            }
               
        }
        else
        {
            sMacMsgSendLinkHead->sendTick = curTick;// + (ONE_MILLISECOND<<6);
          
            MsgNode = sMacMsgSendLinkHead;
            sMacMsgSendLinkHead = sMacMsgSendLinkHead->next;
                
            if (MsgNode->pMacSendOutCallBack != NULL)
            {
                MsgNode->pMacSendOutCallBack(MsgNode, FALSE);
            }
            else
            {
                BosFree(MsgNode->msg.pdata);
                BosFree(MsgNode); 
            }
        }    
    
        if (sMacMsgSendLinkHead != NULL)
        {
            BosEventSend(TASK_SMAC_ID, SMAC_MSG_SEND_EVENT); 
        }
    }    
    else if (event == SMAC_MSG_RECV_EVENT)
    {
        uint8_t                     *pdata;
        struct sMacMpdu_t           *rMpdu;
        struct MsgRecvLinkNode_t    *sMacCurMsgNode;
        bool_t                      MemUsedFlag = FALSE;
        saddr_t                     myAddr;

        uint8_t hwlock;
        
        myAddr = sMacAddressGet();
        
        EnterCritical(hwlock);
        if (sMacMsgRecvLinkHead == NULL)
        {
            ExitCritical(hwlock);
            return;
        }
        
        sMacCurMsgNode = (struct MsgRecvLinkNode_t *)sMacMsgRecvLinkHead;
        sMacMsgRecvLinkHead = sMacMsgRecvLinkHead->next;
        RecvLinkCnt --;
        ExitCritical(hwlock);
        
        sMacCurMsgNode->next = NULL;
        
        pdata = sMacCurMsgNode->msg.pdata;
        rMpdu = (struct sMacMpdu_t *)pdata;  
        
        /* channel check */
        if (rMpdu->frameCtrl.bits.channel != netInfo.channel)
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
        
        /* panid check */
        if ( (rMpdu->panId != netInfo.panId) &&
             (rMpdu->panId != 0xFFFF) )
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;

#ifdef USHELL        
        if (neighborCheck(rMpdu->macSrcAddr) != 0)
                goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
#endif
        
        /* 如果目标地址既不是广播地址，又不是 me */
        if ( (rMpdu->macDstAddr != 0xFFFFFFFF) && (rMpdu->macDstAddr != myAddr) )
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
        
        /* 如果源地址为 me 或 广播地址 */
        if ( (rMpdu->macSrcAddr == 0xFFFFFFFF) || (rMpdu->macSrcAddr == myAddr) )
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;

        /* ACK 帧 */
        if (rMpdu->frameCtrl.bits.frameType == SMAC_ACK_TYPE)
        {   
            struct MsgSendLinkNode_t    *MsgNode;
            struct sMacAck_t            *ack;
            
            if (sMacCurMsgNode->msg.msgLen != sizeof(struct sMacAck_t))
                goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
                
            ack = (struct sMacAck_t *)pdata;
            
            if (!AckIsPending)    
                goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
            
            if (ack->seqNum != AckSeqNum)
                goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
            
            /* 成功等待到ACK */
            AckIsPending = FALSE;
            
            MsgNode = sMacMsgSendLinkHead;
            sMacMsgSendLinkHead = sMacMsgSendLinkHead->next;
            
            if (MsgNode->pMacSendOutCallBack != NULL)
            {
                MsgNode->pMacSendOutCallBack(MsgNode,TRUE);
            }
            else
            {
                BosFree(MsgNode->msg.pdata);
                BosFree(MsgNode);
            }
            
            BosEventSend(TASK_SMAC_ID, SMAC_MSG_SEND_EVENT);   
            
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;   
        }
                   
        /* 如果AckRequest, 那么发送ACK 帧 */
        if (rMpdu->frameCtrl.bits.ackRequest == TRUE)
        {
            struct Msg_t        Msg;
            struct sMacAck_t    *ack;
            
            Msg.msgLen = sizeof(struct sMacAck_t);
            Msg.pdata = BosMalloc(Msg.msgLen);
            
            if (Msg.pdata == NULL)
            {
                goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
            }
            
            BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
            ack = (struct sMacAck_t *)Msg.pdata;
            
            ack->frameCtrl.u16Val = 0;
            ack->frameCtrl.bits.channel = netInfo.channel;
            ack->frameCtrl.bits.frameType = SMAC_ACK_TYPE;
            ack->frameCtrl.bits.framePending = FALSE;
            ack->frameCtrl.bits.ackRequest = FALSE;
            ack->frameCtrl.bits.version = SMAC_FRAME_VERSION;
            ack->seqNum = rMpdu->seqNum;
            
            ack->panId = rMpdu->panId; 
            ack->macDstAddr = rMpdu->macSrcAddr;
            ack->macSrcAddr = myAddr;
            
            PhySend(Msg.pdata, Msg.msgLen);
            
            BosFree(ack);
        }
        
        /* 数据帧 */
        if (rMpdu->frameCtrl.bits.frameType == SMAC_DATA_TYPE)
        {
            if (sMacDataIndication != NULL)
                sMacDataIndication(sMacCurMsgNode);
            
            MemUsedFlag = TRUE;
            
            goto CHECK_AND_RETURN_FROM_SMAC_MSG_RECV;
        }
        
        /* 命令帧 */
        if (rMpdu->frameCtrl.bits.frameType == SMAC_CMD_TYPE)
        {
            uint8_t     cmd;
            uint8_t     *payload;
            
            payload = rMpdu->pdata;            
            cmd = *payload ++;
            
            switch(cmd)
            {
                case CMD_SEARCH_REQ:
                {
                    if (sMacDeviceTypeGet() == ENDDEVICE)
                        break;
                    if ( (sMacDeviceTypeGet() == ROUTER) && (netInfo.netStatus != STATUS_LINKED) )
                        break;
                    
                    sMacCmdNetSearchResp(rMpdu->macSrcAddr);
                    
                    break;
                }
                case CMD_SEARCH_RESP:
                {
                    if (netInfo.netStatus == STATUS_SEARCH)
                    {
                        struct sMacSearchListNode_t    *new, *old;
                        list_t          *entry, *n;
                        saddr_t         coor;
                        uint16_t        panid;
                        
                        dbg("rx search resp from 0x%04x%04x\n", (uint16_t)((rMpdu->macSrcAddr)>>16), (uint16_t)(rMpdu->macSrcAddr));
                        
                        new = BosMalloc(sizeof(struct sMacSearchListNode_t));
                        if (new == NULL)
                            break;
                        
                        BosMemSet(new, 0, sizeof(struct sMacSearchListNode_t));
                        
                        new->type = *payload ++;
                        
                        new->address = rMpdu->macSrcAddr;
                        
                        memcpy(&coor, payload, sizeof(saddr_t));
                        payload += sizeof(saddr_t);
                        new->coordinator = ntohl(coor);
                        
                        memcpy(&panid, payload, sizeof(panid));
                        payload += sizeof(panid);
                        new->panId = ntohs(panid);
                        
                        new->channel = rMpdu->frameCtrl.bits.channel;
                        
                        new->depth = *payload ++;
                        
                        new->sonNum = *payload ++;
                        
                        new->rssi = sMacCurMsgNode->rssi;
                
                        list_for_each_safe(entry, n, &(sMacSearchHandle->searchList))
                        {
                            old = list_entry(entry, struct sMacSearchListNode_t, link);
                            
                            if (old->address != new->address)
                                continue;
                            
                            list_del(entry);
                            
                            BosFree(old);
                            
                            sMacSearchHandle->searchCnt --;
                            
                            break;
                        }
                        
                        list_add_tail(&new->link, &(sMacSearchHandle->searchList));
                        
                        sMacSearchHandle->searchCnt ++;
                    }
                    
                    break;
                }
                case CMD_PING:
                {
                    if ( (sMacDeviceTypeGet() != ENDDEVICE) && (netInfo.netStatus == STATUS_LINKED) )
                    {
                        uint8_t seq;
                        
                        seq = rMpdu->pdata[1];
                        
                        sMacCmdPingPong(CMD_PONG, rMpdu->macSrcAddr, seq);
                        
                        dbg("pong 0x%04x%04x in %d\n", (uint16_t)((rMpdu->macSrcAddr)>>16), (uint16_t)(rMpdu->macSrcAddr), seq);
                    }
                    
                    break;
                }
                case CMD_PONG:
                {
                    if ( (sMacDeviceTypeGet() != COORDINATOR) && (netInfo.netStatus == STATUS_LQ_CHECK) )
                    {
                        struct sMacSearchListNode_t    *node;
                        list_t          *entry, *n;
                        uint8_t         i = 0;
        
                        if (rMpdu->pdata[1] != sMacSearchHandle->pingSeq)
                            break;
                          
                        if (sMacSearchHandle->searchCnt == 0)
                        {
                            break;
                        }
        
                        list_for_each_safe(entry, n, &(sMacSearchHandle->searchList))
                        {
                            if (i != sMacSearchHandle->nodeIndex)
                                continue;
                            
                            node = list_entry(entry, struct sMacSearchListNode_t, link);
                            
                            if (node->address != rMpdu->macSrcAddr)
                                break;
                            
                            node->rxCntInPercent ++;
                            node->rssi = sMacCurMsgNode->rssi;
                            
                            break;
                        }
                    }
                    
                    break;
                }
                case CMD_JOIN_REQ:
                {
                    struct sMacSonListNode_t       *node;
                    list_t                  *entry, *n;
                    
                    struct sMacCmdJoinReq_t        *req;
                    
                    req = (struct sMacCmdJoinReq_t *)(rMpdu->pdata+1);
        
                    if ( (sMacDeviceTypeGet() == ROUTER) && (netInfo.netStatus == STATUS_LINKED) )
                    {
                        //1. if father is me,
                        //2. father is a junior
                        if (ntohl(req->fatherAddr) == sMacAddressGet())
                        {
                            sMacCmdJoinReq(netInfo.father, (uint8_t *)req, sizeof(struct sMacCmdJoinReq_t));
                                
                            break;
                        }
                        else
                        {
                            list_for_each_safe(entry, n, &(sMacSonHandle->sonList))
                            {
                                node = list_entry(entry, struct sMacSonListNode_t, link);
                                
                                if (node->sonAddr == rMpdu->macSrcAddr)
                                {
                                    sMacCmdJoinReq(netInfo.father, (uint8_t *)req, sizeof(struct sMacCmdJoinReq_t));
                                    
                                    break;
                                }
                            }
                        }
                    }
                    else if (sMacDeviceTypeGet() == COORDINATOR)
                    {
                        struct sMacCmdJoinResp_t   *resp;
                        
                        resp = BosMalloc(sizeof(struct sMacCmdJoinResp_t));
                        if (resp != NULL)
                        {
                            memset(resp, 0, sizeof(struct sMacCmdJoinResp_t));
                            resp->result = 1;
                            //TODO: 
                            //check if access to join
                            // if not ,reject and return
                          
                            // if access
                            //1. if father is me,  or
                            //2. father is a junior
                            if (ntohl(req->fatherAddr) == sMacAddressGet())
                            {
                                //check if this node is in my son list
                                list_for_each_safe(entry, n, &(sMacSonHandle->sonList))
                                {
                                    node = list_entry(entry, struct sMacSonListNode_t, link);
                                
                                    if (node->sonAddr == ntohl(req->devAddr))
                                    {
                                        list_del(entry);
                                        
                                        BosFree(node);
                                        
                                        sMacSonHandle->sonNum --;
                                        
                                        break;
                                    }
                                }
                                
                                node = BosMalloc(sizeof(struct sMacSonListNode_t));
                                if (node != NULL)
                                {
                                    //add node to son list
                                    node->retry = MAX_SON_RETRY;
                                    node->type = req->type;
                                    node->sonAddr = ntohl(req->devAddr);
                                    
                                    list_add_tail(&node->link, &sMacSonHandle->sonList);
                                    sMacSonHandle->sonNum ++;
                                    
                                    //add node to route table
                                    sNwkAddRoute(ntohl(req->devAddr), ntohl(req->devAddr));
                                    
                                    //request
                                    resp->result = 0;
                                    memcpy((uint8_t *)&(resp->result) + 1, rMpdu->pdata+1, sizeof(struct sMacCmdJoinReq_t));
                                    
                                    sMacCmdJoinResp(rMpdu->macSrcAddr, (uint8_t *)resp, sizeof(struct sMacCmdJoinResp_t));
                                }
                            }
                            else
                            {
                                list_for_each_safe(entry, n, &(sMacSonHandle->sonList))
                                {
                                    node = list_entry(entry, struct sMacSonListNode_t, link);
                                    
                                    if (node->sonAddr == rMpdu->macSrcAddr)
                                    {
                                        //add node to route table
                                        sNwkAddRoute(ntohl(req->devAddr), node->sonAddr);
                                        
                                        //request
                                        resp->result = 0;
                                        memcpy((uint8_t *)&(resp->result) + 1, rMpdu->pdata+1, sizeof(struct sMacCmdJoinReq_t));
                                        
                                        sMacCmdJoinResp(rMpdu->macSrcAddr, (uint8_t *)resp, sizeof(struct sMacCmdJoinResp_t));
                                        
                                        break;
                                    }
                                }
                            }
                            
                            BosFree(resp);
                        }
                    }
                    
                    break;
                }
                case CMD_JOIN_RESP:
                {
                    struct sMacCmdJoinResp_t       *resp;
                    resp = (struct sMacCmdJoinResp_t *)(rMpdu->pdata+1);
            
                    //1. the join request node is me or
                    //2. the join request node is a junior
                    if (ntohl(resp->devAddr) == sMacAddressGet())
                    {
                        struct sMacSearchListNode_t    *node;
                        list_t                  *entry, *n;
                    
                        if (netInfo.netStatus != STATUS_LINK_REQ)
                            break;
                        
                        list_for_each_safe(entry, n, &(sMacSearchHandle->searchList))
                        {
                            node = list_entry(entry, struct sMacSearchListNode_t, link);
                            
                            if ( (node->address == ntohl(resp->fatherAddr)) && 
                                 (node->address == rMpdu->macSrcAddr) )
                            {
                                //check result
                                if (resp->result == 0)
                                {
                                    SysTimerStop(sMacSearchHandle->handleTimerId);
                                    
                                    netInfo.netStatus = STATUS_LINKED;
                                    
                                    netInfo.father = node->address;
                                    netInfo.coordinator = node->coordinator;
                                    
                                    netInfo.panId = node->panId;
                                    netInfo.channel = node->channel;
                                    
                                    netInfo.depth = node->depth + 1;
                                    netInfo.rssi = node->rssi;
                                    netInfo.rxCntInPercent = node->rxCntInPercent;
                                    
                                    halRfSetChannel(netInfo.channel);
                                    
                                    list_del(entry);
                                    
                                    BosFree(node);
                                    
                                    BosEventSend(TASK_SMAC_ID,SMAC_SEARCH_STOP_EVENT);
                                    
                                    if (sMacNetAutoCB != NULL)
                                    {
                                        sMacNetAutoCB(0);
                                        sMacNetAutoCB = NULL;
                                    }
                                    
                                    //add route table
                                    sNwkAddRoute(netInfo.father, netInfo.father);
                                    sNwkAddRoute(netInfo.coordinator, netInfo.father);
                                    
                                    dbg("Join success.\n");
                                    dbg("\tPanId: 0x%04x.\n", netInfo.panId);
                                    dbg("\tChannel: %d\n", netInfo.channel);
                                    dbg("\tCoor: 0x%04x%04x\n", (uint16_t)((netInfo.coordinator)>>16), (uint16_t)(netInfo.coordinator));
                                    dbg("\tFather: 0x%04x%04x\n", (uint16_t)((netInfo.father)>>16), (uint16_t)(netInfo.father));
                                    dbg("\tDepth: %d\n", netInfo.depth);
                                    dbg("\tRxCnt: %d\n", netInfo.rxCntInPercent); 
                                }
                                
                                break;
                            }
                        }
                    }
                    else
                    {
                        saddr_t dstAddr;
                        
                        //1. father is me
                        //2. father is a junior
                        if (ntohl(resp->fatherAddr) == sMacAddressGet())
                        {
                            struct sMacSonListNode_t       *node;
                            list_t                  *entry, *n;
                    
                            //add route to table
                            sNwkAddRoute(ntohl(resp->devAddr), ntohl(resp->devAddr));
                            
                            //add to son list
                            //check if this node is in my son list
                            list_for_each_safe(entry, n, &(sMacSonHandle->sonList))
                            {
                                node = list_entry(entry, struct sMacSonListNode_t, link);
                                
                                if (node->sonAddr == ntohl(resp->devAddr))
                                {
                                    list_del(entry);
                                        
                                    BosFree(node);
                                        
                                    sMacSonHandle->sonNum --;
                                        
                                    break;
                                }
                            }
                            
                            if (resp->result == 0)//success
                            {
                                node = BosMalloc(sizeof(struct sMacSonListNode_t));
                            
                                if (node != NULL)
                                {
                                    //add node to son list
                                    node->retry = MAX_SON_RETRY;
                                    node->type = resp->type;
                                    node->sonAddr = ntohl(resp->devAddr);
                                        
                                    list_add_tail(&node->link, &sMacSonHandle->sonList);
                                    sMacSonHandle->sonNum ++;
                                        
                                    //add node to route table
                                    sNwkAddRoute(ntohl(resp->devAddr), ntohl(resp->devAddr));
                                }
                            }
                                    
                            //response
                            sMacCmdJoinResp(ntohl(resp->devAddr), (uint8_t *)resp, sizeof(struct sMacCmdJoinResp_t));
                        }
                        else
                        {                          
                            dstAddr = sNwkGetNextHop(ntohl(resp->fatherAddr));
                            if (dstAddr != 0xFFFFFFFF)
                            {
                                sMacCmdJoinResp(dstAddr, (uint8_t *)resp, sizeof(struct sMacCmdJoinResp_t));
                                
                                //add node to route table
                                sNwkAddRoute(ntohl(resp->devAddr), dstAddr);
                            }
                        }
                    }
                    
                    break;
                }
                default:
                    break;
              
            }
        }            
       
CHECK_AND_RETURN_FROM_SMAC_MSG_RECV:
        
        if (!MemUsedFlag)
        {
            BosFree(sMacCurMsgNode->msg.pdata);
            BosFree(sMacCurMsgNode);
        }
        
        EnterCritical(hwlock);
        
        if (sMacMsgRecvLinkHead != NULL)
            BosEventSend(TASK_SMAC_ID,SMAC_MSG_RECV_EVENT);
        
        ExitCritical(hwlock);        
    }
    else if (event == SMAC_SEARCH_INIT_EVENT)
    {
        sMacSearchHandle = BosMalloc(sizeof(struct sMacSearchHandle_t));
        BosMemSet(sMacSearchHandle, 0, sizeof(struct sMacSearchHandle_t));

        sMacSearchHandle->startChannel = netInfo.channel;
        sMacSearchHandle->endChannel = ((netInfo.channel+15)>MAX_RADIO_CHANNEL)?(netInfo.channel+15-(MAX_RADIO_CHANNEL-MIN_RADIO_CHANNEL+1)):(netInfo.channel+15);
        sMacSearchHandle->retry = 0;
        
        sMacSearchHandle->searchCnt = 0;
        INIT_LIST_HEAD(&(sMacSearchHandle->searchList));
        sMacSearchHandle->handleTimerId = SysTimerAlloc();
        
        netInfo.netStatus = STATUS_SEARCH;

        SysTimerStart(sMacSearchHandle->handleTimerId, SYSTIMER_TYPE_ONCE, ONE_MILLISECOND*100, sMacNetSearchTimeOutCallBack);
    }
    else if (event == SMAC_SEARCH_REQ_EVENT)
    {
#define RETRY_PER_CHANNEL       3
        
        if (netInfo.netStatus != STATUS_SEARCH)
            return;
        
        SysTimerStop(sMacSearchHandle->handleTimerId);
        
        if ( (netInfo.channel == sMacSearchHandle->endChannel) &&
             (sMacSearchHandle->retry == RETRY_PER_CHANNEL) )
        {
            //搜索结束
            netInfo.netStatus = STATUS_LQ_CHECK;
            
            sMacSearchHandle->retry = 0;
            sMacSearchHandle->nodeIndex = 0;
            sMacSearchHandle->pingSeq = 0;
        
            BosEventSend(TASK_SMAC_ID,SMAC_PINGPONG_EVENT);
            
            return;
        }
        
        if ( sMacSearchHandle->retry == RETRY_PER_CHANNEL )
        {
            sMacSearchHandle->retry = 0;
            netInfo.channel ++;
            
            if (netInfo.channel > MAX_RADIO_CHANNEL)
                netInfo.channel = MIN_RADIO_CHANNEL;
            
            halRfSetChannel(netInfo.channel);
        }
        
        sMacCmdNetSearchReq();
        
        sMacSearchHandle->retry ++;
        
        SysTimerStart(sMacSearchHandle->handleTimerId, SYSTIMER_TYPE_ONCE, ONE_MILLISECOND*100, sMacNetSearchTimeOutCallBack);
        
#undef RETRY_PER_CHANNEL
    }
    else if (event == SMAC_PINGPONG_EVENT)
    {
#define MAX_PING_COUNT       100
      
        struct sMacSearchListNode_t    *node;
        list_t                  *entry, *n;
        uint8_t                 i = 0;
        
        SysTimerStop(sMacSearchHandle->handleTimerId);
        
        if (sMacSearchHandle->searchCnt == 0)
        {
            BosEventSend(TASK_SMAC_ID,SMAC_SEARCH_STOP_EVENT);
            return;
        }
        
        if ( (sMacSearchHandle->nodeIndex == sMacSearchHandle->searchCnt - 1) &&
             (sMacSearchHandle->pingSeq == MAX_PING_COUNT) )
        {
            //冒泡法排序
            for (uint8_t i = 0; i < sMacSearchHandle->searchCnt - 1; i ++)
            {
                struct sMacSearchListNode_t    *a, *b;
                list_t          *tmp;
                
                for (uint8_t j = 0; j < sMacSearchHandle->searchCnt - 1 - i; j ++)
                {
                    a = list_entry(entry, struct sMacSearchListNode_t, link);
                    b = list_entry(n, struct sMacSearchListNode_t, link);
                    
                    if ( (a->rxCntInPercent < b->rxCntInPercent) ||
                         ( (a->rxCntInPercent == b->rxCntInPercent) && (a->depth > b->depth) ) )
                    {
                        list_del(entry);
                        list_add(entry, n);
                        
                        tmp = entry;
                        entry = n;
                        n = tmp;
                    }
                    
                    entry = n;
                    n = n->next;
                }
            }
            
            sMacSearchHandle->retry = 0;
            
            netInfo.netStatus = STATUS_LINK_REQ;
            
            BosEventSend(TASK_SMAC_ID,SMAC_JOIN_REQ_EVENT);
            return;
        }
        
        if (sMacSearchHandle->pingSeq == MAX_PING_COUNT)
        {
            sMacSearchHandle->nodeIndex ++;
            sMacSearchHandle->pingSeq = 0;
        }
        
        list_for_each_safe(entry, n, &(sMacSearchHandle->searchList))
        {
            if (i ++ != sMacSearchHandle->nodeIndex)
                continue;
            
            node = list_entry(entry, struct sMacSearchListNode_t, link);
            
            if (netInfo.channel != node->channel)
            {
                netInfo.channel = node->channel;
                halRfSetChannel(netInfo.channel);
            }
            
            sMacSearchHandle->pingSeq ++;
            
            sMacCmdPingPong(CMD_PING, node->address, sMacSearchHandle->pingSeq);
            
            dbg("ping 0x%04x%04x in %d\n", (uint16_t)((node->address)>>16), (uint16_t)(node->address), sMacSearchHandle->pingSeq);
            
            SysTimerStart(sMacSearchHandle->handleTimerId, SYSTIMER_TYPE_ONCE, ONE_MILLISECOND*20, sMacPingTimeOutCallBack);
            
            break;
        }
    }
    else if (event == SMAC_JOIN_REQ_EVENT)
    {
#define MAX_JOIN_REQ_CNT        3
        
        struct sMacSearchListNode_t    *node;
        list_t                          *entry;
        struct sMacCmdJoinReq_t        *req;
        
        SysTimerStop(sMacSearchHandle->handleTimerId);
                
        if (sMacSearchHandle->retry == MAX_JOIN_REQ_CNT)
        {
            entry = sMacSearchHandle->searchList.next;
            list_del(entry);
            
            sMacSearchHandle->searchCnt --;
            sMacSearchHandle->retry = 0;
        }
      
        if (sMacSearchHandle->searchCnt == 0)
        {
            if (sMacNetAutoCB != NULL)
            {
                sMacNetAutoCB(1);
                sMacNetAutoCB = NULL;
            }
            
            BosEventSend(TASK_SMAC_ID,SMAC_SEARCH_STOP_EVENT);
            return;
        }
        
        entry = sMacSearchHandle->searchList.next;
        node = list_entry(entry, struct sMacSearchListNode_t, link);
        
        req = (struct sMacCmdJoinReq_t *)BosMalloc(sizeof(struct sMacCmdJoinReq_t));
        if (req != NULL)
        {
            req->type = sMacDeviceTypeGet();
            
            //my address
            req->devAddr = htonl(sMacAddressGet());
            
            //father address
            req->fatherAddr = htonl(node->address);
            
            sMacCmdJoinReq(node->address, (uint8_t *)req, sizeof(struct sMacCmdJoinReq_t));
            
            BosFree(req);
        }
        
        sMacSearchHandle->retry ++;
        
        SysTimerStart(sMacSearchHandle->handleTimerId, SYSTIMER_TYPE_ONCE, ONE_SECOND, sMacJoinReqTimeOutCallBack);
    }
    else if (event == SMAC_SEARCH_STOP_EVENT)
    {
        if (sMacSearchHandle != NULL)
        {
            struct sMacSearchListNode_t    *node;
            list_t                  *entry, *n;
            
            list_for_each_safe(entry, n, &(sMacSearchHandle->searchList))
            {
                node = list_entry(entry, struct sMacSearchListNode_t, link);
                            
                list_del(entry);
                
                BosFree(node);
                            
                sMacSearchHandle->searchCnt --;
            }
            
            SysTimerFree(sMacSearchHandle->handleTimerId);
            
            BosFree(sMacSearchHandle);
            
        }
        
        if (netInfo.netStatus != STATUS_LINKED)
            netInfo.netStatus = STATUS_UNLINK;
    }
}

/* */
error_t sMacNetAutoStart(void (*pCallBack)(uint8_t status))
{
    if (sMacDeviceTypeGet() == COORDINATOR)
        return - BOS_ERROR;
      
    if (netInfo.netStatus != STATUS_UNLINK)
        return - BOS_ERROR;
    
    sMacNetAutoCB = pCallBack;
    
    BosEventSend(TASK_SMAC_ID,SMAC_SEARCH_INIT_EVENT);
    
    return BOS_EOK;
}

/* */
uint8_t sMacDeviceTypeGet(void)
{
    return ROUTER;
}

/* */
saddr_t sMacAddressGet(void)
{
    saddr_t  addr;
    
    nvramSysBlkParamGet(devId, &addr);
    
    return addr;
}

/* */
saddr_t sMacCoordinatorAddressGet(void)
{
    if (netInfo.netStatus == STATUS_LINKED)
        return netInfo.coordinator;
     
    return 0xFFFFFFFF;
}

/* */
saddr_t sMacFatherAddressGet(void)
{
    if (netInfo.netStatus == STATUS_LINKED)
        return netInfo.father;
     
    return 0xFFFFFFFF;
}

uint8_t sMacLinkStatusGet(void)
{
    return netInfo.netStatus;
}

saddr_t sMacCoorAddrGet(void)
{
    return netInfo.coordinator;
}

saddr_t sMacFatherAddrGet(void)
{
    return netInfo.father;
}

uint16_t sMacLinkSQGet(void)
{
    return (netInfo.rssi<<8)+netInfo.rxCntInPercent;
}

uint8_t sMacSonAddrGet(uint8_t *buf, uint8_t bufSize)
{
    struct sMacSonListNode_t    *node;
    list_t          *entry;
    
    if (buf == NULL)
        return 0;
    
    if (netInfo.netStatus != STATUS_LINKED)
        return 0;
    
    if (bufSize < sizeof(saddr_t)*(sMacSonHandle->sonNum))
        return 0;
    
    list_for_each(entry, &(sMacSonHandle->sonList))
    {
        saddr_t addr;
        
        node = list_entry(entry, struct sMacSonListNode_t, link);
                            
        addr = node->sonAddr;
        
        addr = htonl(addr);
        
        memcpy(buf, &addr, sizeof(addr));
        
        buf += sizeof(addr);
    }
    
    return sMacSonHandle->sonNum;
}

/* */
error_t sMacSend(saddr_t dstAddr, struct MsgSendLinkNode_t *pMsgNode)
{
    struct sMacMpdu_t   *mpdu;
    
    pMsgNode->next = NULL;
    pMsgNode->retry = SMAC_MAX_RETRY_TIMES;
    mpdu = (struct sMacMpdu_t *)pMsgNode->msg.pdata;
    
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_DATA_TYPE;
    mpdu->frameCtrl.bits.security = FALSE;
    mpdu->frameCtrl.bits.framePending = 0;
    
    if (dstAddr == 0xFFFFFFFF)
        mpdu->frameCtrl.bits.ackRequest = FALSE;
    else
        mpdu->frameCtrl.bits.ackRequest = TRUE;

    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    
    mpdu->panId = netInfo.panId; 
    mpdu->macDstAddr = dstAddr;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->seqNum = MySeqNum ++;

    if (sMacMsgSendLinkHead == NULL)
    {
        sMacMsgSendLinkHead = pMsgNode;
    }
    else
    {
        struct MsgSendLinkNode_t    *TailOfMacMsgLink;
        
        for (TailOfMacMsgLink = sMacMsgSendLinkHead; TailOfMacMsgLink->next != NULL; TailOfMacMsgLink = TailOfMacMsgLink->next);
         
        TailOfMacMsgLink->next = pMsgNode;
    }
    
    BosEventSend(TASK_SMAC_ID, SMAC_MSG_SEND_EVENT);
    
    return SUCCESS;
}

static error_t sMacCmdNetSearchReq(void)
{
    struct Msg_t        Msg;
    struct sMacMpdu_t   *mpdu;
            
    Msg.msgLen = sizeof(struct sMacMpdu_t) + 1;
    Msg.pdata = BosMalloc(Msg.msgLen);
            
    if (Msg.pdata == NULL)
    {
        return - BOS_ENOMEM;
    }
            
    BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
    mpdu = (struct sMacMpdu_t *)Msg.pdata;
            
    mpdu->frameCtrl.u16Val = 0;
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_CMD_TYPE;
    mpdu->frameCtrl.bits.framePending = FALSE;
    mpdu->frameCtrl.bits.ackRequest = FALSE;
    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    mpdu->seqNum = MySeqNum ++;
            
    mpdu->panId = 0xFFFF; 
    mpdu->macDstAddr = 0xFFFFFFFF;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->pdata[0] = CMD_SEARCH_REQ;
            
    PhySend(Msg.pdata, Msg.msgLen);
            
    BosFree(mpdu);
    
    dbg("search ch %d\n", netInfo.channel);
    
    return BOS_EOK;
}

/* */
static error_t sMacCmdNetSearchResp(saddr_t dstAddr)
{
    struct Msg_t        Msg;
    struct sMacMpdu_t   *mpdu;
    struct sMacCmdSearchResp_t *resp;
            
    Msg.msgLen = sizeof(struct sMacMpdu_t) + 1 + sizeof(struct sMacCmdSearchResp_t);
    Msg.pdata = BosMalloc(Msg.msgLen);
            
    if (Msg.pdata == NULL)
    {
        return - BOS_ENOMEM;
    }
            
    BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
    mpdu = (struct sMacMpdu_t *)Msg.pdata;
            
    mpdu->frameCtrl.u16Val = 0;
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_CMD_TYPE;
    mpdu->frameCtrl.bits.framePending = FALSE;
    mpdu->frameCtrl.bits.ackRequest = FALSE;
    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    mpdu->seqNum = MySeqNum ++;
            
    mpdu->panId = 0xFFFF; 
    mpdu->macDstAddr = dstAddr;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->pdata[0] = CMD_SEARCH_RESP;
    
    resp = (struct sMacCmdSearchResp_t *)(mpdu->pdata + 1);
    
    if (sMacDeviceTypeGet() == COORDINATOR)
    {
        resp->type = COORDINATOR;    
        resp->coor = htonl(sMacAddressGet());       
        resp->panId = htons(netInfo.panId);        
        resp->depth = 0;        
        resp->sonNum = sMacSonHandle->sonNum;
    }
    else //router
    {
        resp->type = ROUTER;    
        resp->coor = htonl(sMacCoordinatorAddressGet());       
        resp->panId = htons(netInfo.panId);        
        resp->depth = netInfo.depth;        
        resp->sonNum = sMacSonHandle->sonNum;
    }
            
    PhySend(Msg.pdata, Msg.msgLen);
            
    BosFree(mpdu);
    
    dbg("rx search from 0x%04d%04x\n", (uint16_t)(dstAddr>>16), (uint16_t)dstAddr);
    
    return BOS_EOK;
}

/* */
static error_t sMacCmdPingPong(uint8_t cmd, saddr_t dstAddr, uint8_t seq)
{
    struct Msg_t        Msg;
    struct sMacMpdu_t   *mpdu;
            
    Msg.msgLen = sizeof(struct sMacMpdu_t) + 1 + sizeof(seq);
    Msg.pdata = BosMalloc(Msg.msgLen);
            
    if (Msg.pdata == NULL)
    {
        return - BOS_ENOMEM;
    }
            
    BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
    mpdu = (struct sMacMpdu_t *)Msg.pdata;
            
    mpdu->frameCtrl.u16Val = 0;
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_CMD_TYPE;
    mpdu->frameCtrl.bits.framePending = FALSE;
    mpdu->frameCtrl.bits.ackRequest = FALSE;
    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    mpdu->seqNum = MySeqNum ++;
            
    mpdu->panId = 0xFFFF;
    mpdu->macDstAddr = dstAddr;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->pdata[0] = cmd;
    
    mpdu->pdata[1] = seq;
            
    PhySend(Msg.pdata, Msg.msgLen);
            
    BosFree(mpdu);
    
    return BOS_EOK;
}

/* */
static error_t sMacCmdJoinReq(saddr_t dstAddr,  uint8_t* payload, uint8_t len)
{
    struct Msg_t        Msg;
    struct sMacMpdu_t   *mpdu;
            
    Msg.msgLen = sizeof(struct sMacMpdu_t) + 1 + len;
    Msg.pdata = BosMalloc(Msg.msgLen);
            
    if (Msg.pdata == NULL)
    {
        return - BOS_ENOMEM;
    }
            
    BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
    mpdu = (struct sMacMpdu_t *)Msg.pdata;
            
    mpdu->frameCtrl.u16Val = 0;
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_CMD_TYPE;
    mpdu->frameCtrl.bits.framePending = FALSE;
    mpdu->frameCtrl.bits.ackRequest = FALSE;
    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    mpdu->seqNum = MySeqNum ++;
            
    mpdu->panId = (netInfo.netStatus != STATUS_LINKED)?0xFFFF:netInfo.panId;
    
    mpdu->macDstAddr = dstAddr;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->pdata[0] = CMD_JOIN_REQ;
   
    memcpy(mpdu->pdata+1, payload, len);
            
    PhySend(Msg.pdata, Msg.msgLen);
            
    BosFree(mpdu);
    
    dbg("send join req to 0x%04x%04x\n", (uint16_t)(dstAddr>>16), (uint16_t)dstAddr);
    
    return BOS_EOK;
}

/* */
static error_t sMacCmdJoinResp(saddr_t dstAddr, uint8_t* payload, uint8_t len)
{
    struct Msg_t        Msg;
    struct sMacMpdu_t   *mpdu;
            
    Msg.msgLen = sizeof(struct sMacMpdu_t) + 1 + len;
    Msg.pdata = BosMalloc(Msg.msgLen);
            
    if (Msg.pdata == NULL)
    {
        return - BOS_ENOMEM;
    }
            
    BosMemSet(Msg.pdata, 0x0, Msg.msgLen);
            
    mpdu = (struct sMacMpdu_t *)Msg.pdata;
            
    mpdu->frameCtrl.u16Val = 0;
    mpdu->frameCtrl.bits.channel = netInfo.channel;
    mpdu->frameCtrl.bits.frameType = SMAC_CMD_TYPE;
    mpdu->frameCtrl.bits.framePending = FALSE;
    mpdu->frameCtrl.bits.ackRequest = FALSE;
    mpdu->frameCtrl.bits.version = SMAC_FRAME_VERSION;
    mpdu->seqNum = MySeqNum ++;
            
    mpdu->panId = 0xFFFF;
    mpdu->macDstAddr = dstAddr;
    mpdu->macSrcAddr = sMacAddressGet();
    
    mpdu->pdata[0] = CMD_JOIN_RESP;
    
    memcpy(mpdu->pdata+1, payload, len);
            
    PhySend(Msg.pdata, Msg.msgLen);
            
    BosFree(mpdu);
    
    dbg("send join resp to 0x%04x%04x\n", (uint16_t)(dstAddr>>16), (uint16_t)dstAddr);
    
    return BOS_EOK;
}

/* */
void sMacDataIndicationCallBackSet(DataIndicationCallBack_t p)
{
    sMacDataIndication = p;
}

/* 接收回调函数 */
static void PhyDataIndication(void *p)
{
    struct MsgRecvLinkNode_t   *pMsgNode;
    uint8_t                     *pdata;
    uint8_t                     len;
    uint8_t                     crc_status;    
    uint32_t                    crc;
    
    (void)p;
    
    // Read payload length.
    halRfReadRxBuf(&len,1);
    len &= 0x7F; // Ignore MSB

    if ( (len <= sizeof(struct sMacAck_t) + sizeof(crc))  ||
         (!(RecvLinkCnt<MAX_RECV_LINK_CNT)) )
    {
        return;
    }
    
    pMsgNode = BosMalloc(sizeof(struct MsgRecvLinkNode_t));
    if (pMsgNode == NULL)
    {
        return;
    }
    
    pMsgNode->msg.msgLen = len - 2;
    
    pMsgNode->next = NULL;
    pMsgNode->rcvTick = TickGet();
    
    pdata = BosMalloc(pMsgNode->msg.msgLen);
    if (pdata == NULL)
    {
        BosFree(pMsgNode);
        
        return;
    }
    
    pMsgNode->msg.pdata = pdata;
    
    halRfReadRxBuf(pdata, pMsgNode->msg.msgLen);
    
    halRfReadRxBuf(&pMsgNode->rssi, 1);

    /* hardware crc check */
    halRfReadRxBuf(&crc_status, 1);
    if ((crc_status&0x80) != 0x80)
    {
        BosFree(pdata);
        BosFree(pMsgNode);
        
        return;
    }
    
    /* soft crc check */
    crc = crc32(0, pdata, pMsgNode->msg.msgLen - sizeof(crc));
    crc = htonl(crc);
    if (memcmp(&crc, &pdata[pMsgNode->msg.msgLen - sizeof(crc)], sizeof(crc)) != 0)
    {
        BosFree(pdata);
        BosFree(pMsgNode);
        
        return;
    }
    
    pMsgNode->msg.msgLen -= sizeof(crc);
    
    pMsgNode->rssi += halRfGetRssiOffset();
    
    if (sMacMsgRecvLinkHead == NULL)
    {
        sMacMsgRecvLinkHead = pMsgNode;
    }
    else
    {
        struct MsgRecvLinkNode_t    *pNode;
        
        for (pNode = (struct MsgRecvLinkNode_t *)sMacMsgRecvLinkHead; pNode->next != NULL; pNode = pNode->next);
        
        pNode->next = pMsgNode;
    }
    RecvLinkCnt ++;
    
    BosEventSend(TASK_SMAC_ID, SMAC_MSG_RECV_EVENT);
}
        
static error_t  PhySend(uint8_t *buf, uint8_t len)
{
#define  macMinBE    6
#define  macMaxBE    9
#define  macMaxCSMABackoff  3

    uint8_t NB=0;
    uint8_t BE=macMinBE;
    
    uint8_t *tmpBuf;
    uint32_t    crc;
    
    tmpBuf = BosMalloc(len + sizeof(crc));
    
    if (tmpBuf == NULL)
        return -BOS_ERROR;
    
    memcpy(tmpBuf, buf, len);
    crc = crc32(0, buf, len);
    crc = htonl(crc);
    memcpy(tmpBuf+len, &crc, sizeof(crc));
  
    // Wait until the transceiver is idle
    halRfWaitTransceiverReady();
    
    // Turn off RX frame done interrupt to avoid interference on the SPI interface
    halRfDisableRxInterrupt();
    halRfWriteTxBuf(tmpBuf, len+sizeof(crc));
    halRfEnableRxInterrupt();
    
    BosFree(tmpBuf);
    
    do
    {
        tick_t curTick;
        tick_t delay;
        
        delay = halRfGetRandomByte() & ((1<<BE)-1);
        delay *= (ONE_MILLISECOND/16);
        
        curTick = TickGet();
        
        while ((TickGet() - curTick) < delay);
        
        if (halRfTransmit() == SUCCESS)
            return BOS_EOK;
        
        NB ++;
        BE ++;
        
        BE = (BE > macMaxBE) ? macMaxBE:BE;
        
    }while(NB < macMaxCSMABackoff);
      
    
    return -BOS_ERROR;
}

/***********************************************************************************
* @fn          RfRxIndicateIsr
*
* @brief       Interrupt service routine for received frame from radio
*              (either data or acknowlegdement)
*
* @param       none
*
* @return      none
*/
static void RfRxIndicateIsr(void)
{
    // Clear interrupt and disable new RX frame done interrupt
    halRfDisableRxInterrupt();
    
    PhyDataIndication(NULL);
    
    halRfEnableRxInterrupt();
}
