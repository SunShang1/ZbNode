/*
 * 文件名称：sNwk.c
 * 文件说明：smartNet协议栈网络层实现
 * 
 */
 
#include "pub_def.h"
#include "smartNet.cfg"
#include "sNwk.h"
#include "BosMem.h"
#include "Kernel.h"
#include "SysTimer.h"
#include "sysTask.h"

#ifdef USHELL
#include "ush_cmd.h"
#endif

#include <stdlib.h>
/********************************************
Defines
********************************************/

#define SNWK_MSG_SEND_EVENT                 0x01
#define SNWK_MSG_RECV_EVENT                 0x02

#define SNWK_MAX_RETRY_TIMES                3

#define SNWK_ROUTE_TAB_SIZE                 32
#define SNWK_SEQ_LIST_SIZE                  32
   
#define SMARTNET_HEAD_LEN                   (MAC_HEAD_LEN + sizeof(struct sNwkNpdu_t))

/********************************************
Typedefs
********************************************/
/* 网络帧类型定义 */
enum
{
    SNWK_DATA_TYPE  =                   0,
    SNWK_CMD_TYPE
};

typedef struct
{
    saddr_t     addr;
    uint8_t     seqNum;
}sNwkSeqList_t;

/* 路由表定义 */
struct  sNwkRouteTable_t
{
    /* 目标地址 */
    saddr_t                 dstAddr;
    /* 下一跳地址 */
    saddr_t                 nextHop;
};

/********************************************
* Globals *
********************************************/

/* 网络帧序列号 */
static  uint8_t                     MySeqNum;
/* 记录前几次的数据包序列号，以用来判断数据包的重复接收, 初始化为0 */
static  sNwkSeqList_t               sNwkSeqList[SNWK_SEQ_LIST_SIZE];
/* 范围为:0 ~ (SNWK_SEQ_LIST_SIZE - 1) */    
static  uint8_t                     sNwkSeqIndex;
    
/* 发送数据帧链表/队列 */
static struct MsgSendLinkNode_t     *sNwkMsgSendLinkHead;
/* */
static uint8_t                      sNwkSendLinkCnt;
/* 接收数据帧链表/队列 */
static struct MsgRecvLinkNode_t     *sNwkMsgRecvLinkHead;
/* 送往上层的数据帧链表/队列 */
static struct MsgRecvLinkNode_t     *sNwkAppDataLinkHead;
/* 回调函数 */
static DataIndicationCallBack_t     sNwkDataIndication;

/* 路由表 */
static struct sNwkRouteTable_t      sNwkRouteTab[SNWK_ROUTE_TAB_SIZE];

/********************************************
* Function defines *
********************************************/

/* */
__task void sNwkTask(event_t);

/* */
static void sMacDataIndication(void *p);

/* 数据发送返回函数 */
static void sMacDataSendCallBack(struct MsgSendLinkNode_t *node, bool_t result);

/* */
error_t sNwkInit(void)
{
    sMacInit();

    sNwkMsgRecvLinkHead = NULL;
    sNwkAppDataLinkHead = NULL;
    sNwkMsgSendLinkHead = NULL;
    
    sNwkSendLinkCnt = 0;
    
    sNwkDataIndication = NULL;
    
    MySeqNum = halRfGetRandomByte();
    sNwkSeqIndex = 0;
    memset(sNwkSeqList, 0xFF, sizeof(sNwkSeqList));

    memset(sNwkRouteTab, 0xFF, sizeof(sNwkRouteTab));
    
    sMacDataIndicationCallBackSet(sMacDataIndication);

    return BOS_EOK;      
} 

/* 网络层发送 */
error_t  sNwkSend(saddr_t dstAddr, uint8_t *buf, uint8_t len, uint8_t radius)
{
    saddr_t                     MyAddr;
    struct MsgSendLinkNode_t   *node;
    uint8_t                     *pdata;
    struct sNwkNpdu_t          *npdu;
    uint8_t                     i;
    
    if (len == 0)
        return BOS_EOK;
    
    if (len > SNWK_MAX_PAYLOAD)
        return - BOS_EOVERLEN;
    
    /* 获取本节点的MAC地址 */
    MyAddr = sMacAddressGet();
    
    /* 如果目标地址就是本节点地址，直接返回成功 */
    if (MyAddr == dstAddr)
        return BOS_EOK;
    
    if ( !(sNwkSendLinkCnt < MAX_SEND_LINK_CNT) )
        return -BOS_ENOMEM;
        
    node = BosMalloc(sizeof(struct MsgSendLinkNode_t));
    if (node == NULL)
        return -BOS_ENOMEM;
        
    pdata = BosMalloc(len + sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN);
    if (pdata == NULL)
    {
        BosFree(node);
        return -BOS_ENOMEM;
    }
    
    BosMemSet(pdata, 0x0, sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN);
    
    //数据拷贝
    for (i = 0 ; i < len; i ++)
    {
        pdata[i + sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN] = buf[i];
    }
    
    node->next = NULL;
    node->pMacSendOutCallBack = sMacDataSendCallBack;
    node->sendTick = TickGet();
    node->retry = SNWK_MAX_RETRY_TIMES;
    node->msg.pdata = pdata;
    node->msg.msgLen = len + sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN;
    
    npdu = (struct sNwkNpdu_t *)(&pdata[MAC_HEAD_LEN]);
    npdu->frameCtrl.bits.frameType = SNWK_DATA_TYPE;
    npdu->frameCtrl.bits.version = 0x01;
    npdu->nwkDstAddr = dstAddr;
    npdu->nwkSrcAddr = MyAddr;
    
    if ( (radius == 0) || (radius > DEF_MAX_RADIUS) )
        npdu->radius = (DEF_MAX_RADIUS<<4)+DEF_MAX_RADIUS;
    else
        npdu->radius = (radius<<4)+radius;
    
    npdu->seqNum = MySeqNum ++;
    
    if (sNwkMsgSendLinkHead == NULL)
    {
        sNwkMsgSendLinkHead = node;
    }
    else
    {
        struct MsgSendLinkNode_t    *TailOfNwkMsgLink;
        
        for (TailOfNwkMsgLink = sNwkMsgSendLinkHead; TailOfNwkMsgLink->next != NULL; TailOfNwkMsgLink = TailOfNwkMsgLink->next);
         
        TailOfNwkMsgLink->next = node;
    }
    
    sNwkSendLinkCnt ++;
    
    BosEventSend(TASK_SNWK_ID, SNWK_MSG_SEND_EVENT);
    
    return SUCCESS;
}

/* 网络层接收 */
uint8_t sNwkRecv(saddr_t *srcAddr,         \
                 uint8_t *buf,              \
                 uint8_t bufSize,           \
                 uint8_t *head,             \
                 uint8_t *rssi)
{
     struct MsgRecvLinkNode_t       *pMsgNode;
     uint8_t                        *pdata;
     uint8_t                        len;
     uint8_t                        i;
      
     if (sNwkAppDataLinkHead == NULL)
        return 0;
     
     pMsgNode = sNwkAppDataLinkHead;
     sNwkAppDataLinkHead = sNwkAppDataLinkHead->next;
     pMsgNode->next = NULL;
     
     pdata = pMsgNode->msg.pdata;
     
     if (srcAddr != NULL)
        *srcAddr = ((struct sNwkNpdu_t *)(&pdata[MAC_HEAD_LEN]))->nwkSrcAddr;
     
     if (rssi != NULL)
        *rssi = pMsgNode->rssi;
     
     if (head != NULL)
     {
        for (i = sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN; i > 0; i --)
            *head ++ = *pdata ++;
     }
     
     len = pMsgNode->msg.msgLen - sizeof(struct sNwkNpdu_t) - MAC_HEAD_LEN;
     
     len = (len > bufSize)?bufSize:len;
     pdata = &pMsgNode->msg.pdata[sizeof(struct sNwkNpdu_t) + MAC_HEAD_LEN]; 
     
     for (i = len; i > 0; i --)
     {
        *buf ++ = *pdata ++;
     }
     
     BosFree(pMsgNode->msg.pdata);
     BosFree(pMsgNode);
     
     if (sNwkAppDataLinkHead != NULL)
     {
        if (sNwkDataIndication != NULL)
            sNwkDataIndication(NULL);
     }
     
     return len;
        
}

/* */
__task void sNwkTask(event_t event)
{
    if (event == SNWK_MSG_SEND_EVENT)
    {
        struct sNwkNpdu_t   *npdu;
        struct sMacMpdu_t   *mpdu;
        saddr_t              nextHop;
        
        //找出有效的待发送的数据帧
        for (;;)
        {
            struct MsgSendLinkNode_t    *MsgNode;
            
            if (sNwkMsgSendLinkHead == NULL)
            {
                return;
            }
        
            if (sNwkMsgSendLinkHead->retry != 0)
                break;
            
           //重复多次都发送失败，直接丢弃该帧
            MsgNode = sNwkMsgSendLinkHead;
            sNwkMsgSendLinkHead = sNwkMsgSendLinkHead->next;
                
            BosFree(MsgNode->msg.pdata);
            BosFree(MsgNode); 
            
            sNwkSendLinkCnt --;           
        }
        
        /* 获取当前需要发送的数据的网络层帧头 */    
        npdu = (struct sNwkNpdu_t*)(&(sNwkMsgSendLinkHead->msg.pdata[MAC_HEAD_LEN]));
        mpdu = (struct sMacMpdu_t *)(sNwkMsgSendLinkHead->msg.pdata);
        
        /* 获取下一跳地址 */
        if (mpdu->macDstAddr == 0xFFFFFFFF)
            nextHop = 0xFFFFFFFF;
        else
            nextHop = sNwkGetNextHop(npdu->nwkDstAddr);

        {
            struct MsgSendLinkNode_t            *next;
            
            next = sNwkMsgSendLinkHead->next;    
            sNwkMsgSendLinkHead->retry --;
            
            if (sMacSend(nextHop, sNwkMsgSendLinkHead) == SUCCESS)
            {
                sNwkMsgSendLinkHead = next;
                
                // 如果发送成功，那么继续发送下一个数据包
                if (sNwkMsgSendLinkHead != NULL)
                {
                    BosEventSend(TASK_SNWK_ID,SNWK_MSG_SEND_EVENT);
                }
            }
            else
            {
                /* 重新触发发送任务 */
                BosEventSend(TASK_SNWK_ID,SNWK_MSG_SEND_EVENT);
            }
        }
    }
    
    /* 收到数据处理 */
    else if (event == SNWK_MSG_RECV_EVENT)
    {
        uint8_t                      *pdata;
        struct sNwkNpdu_t           *npdu;
        struct MsgRecvLinkNode_t    *sNwkCurMsgNode;
        uint16_t                     MyAddr;
        uint8_t                      i;
        bool_t                       MemUsedFlag = FALSE;
        
        if (sNwkMsgRecvLinkHead == NULL)
        {
            
            return;
        }
        
        MyAddr = sMacAddressGet();
        
        sNwkCurMsgNode = sNwkMsgRecvLinkHead;        
        sNwkMsgRecvLinkHead = sNwkMsgRecvLinkHead->next;
        sNwkCurMsgNode->next = NULL;
        
        if (sNwkCurMsgNode->msg.msgLen < SMARTNET_HEAD_LEN)
            goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
        
        pdata = sNwkCurMsgNode->msg.pdata;
        npdu = (struct sNwkNpdu_t *)(&pdata[MAC_HEAD_LEN]);
        
        /* 判断地址是否为有效 */
        if ( (npdu->nwkSrcAddr == 0xFFFFFFFF)    ||\
             (npdu->nwkSrcAddr == MyAddr)  )
        {
            goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
        }
        
        /* 判断是否是重复收到的帧 */
        if (npdu->nwkSrcAddr != 0xFFFFFFFF)
        {
            for (i = 0; i < SNWK_SEQ_LIST_SIZE; i ++)
            {
                if((sNwkSeqList[i].addr == npdu->nwkSrcAddr) && (sNwkSeqList[i].seqNum == npdu->seqNum))
                {
                    goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
                }
            }
        
            /* 保存序列号 */
            sNwkSeqList[sNwkSeqIndex].seqNum = npdu->seqNum;
            sNwkSeqList[sNwkSeqIndex].addr = npdu->nwkSrcAddr;
        
            if (++sNwkSeqIndex == SNWK_SEQ_LIST_SIZE)
                sNwkSeqIndex = 0;
        }
        
        if (((npdu->radius)&0x0F) > 0)
            npdu->radius --;
        
        /* 如果是一个数据帧 */
        if (npdu->frameCtrl.bits.frameType == SNWK_DATA_TYPE)
        {    
            /* 目标地址是否为广播或者本节点地址 */    
            if ( (0xFFFFFFFF == npdu->nwkDstAddr)  || \
                 (npdu->nwkDstAddr == MyAddr)   )
            {
                if (sNwkAppDataLinkHead == NULL)
                {
                    sNwkAppDataLinkHead = sNwkCurMsgNode;
                }
                else
                {
                    struct MsgRecvLinkNode_t        *node;
                    
                    for (node = sNwkAppDataLinkHead; node->next != NULL; node = node->next);
                    
                    node->next = sNwkCurMsgNode;
                }
                
                if (sNwkDataIndication != NULL)
                {
                    sNwkDataIndication(NULL);
                    MemUsedFlag = TRUE;
                }
            }
            
            /* 如果目标地址不是本节点地址且支持路由功能 */
            if ( (sMacDeviceTypeGet() == ROUTER) && (npdu->nwkDstAddr != MyAddr) )
            {
                struct MsgSendLinkNode_t        *node;                 
                
                if (((npdu->radius)&0x0F) == 0)
                {
                    goto  CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
                }
                
                if ( !(sNwkSendLinkCnt<MAX_SEND_LINK_CNT) )
                {
                    goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV; 
                }
                
                node = BosMalloc(sizeof(struct MsgSendLinkNode_t));
                if (node == NULL)
                {
                    goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
                }
                
                node->next = NULL;
                node->pMacSendOutCallBack = sMacDataSendCallBack;
                node->sendTick = TickGet() + (halRfGetRandomWord()%50)*4*ONE_MILLISECOND;
                node->retry = SNWK_MAX_RETRY_TIMES;
                node->msg.msgLen = sNwkCurMsgNode->msg.msgLen;
                
                if (MemUsedFlag == TRUE)
                {
                    node->msg.pdata = BosMalloc(node->msg.msgLen);
                    if (node->msg.pdata == NULL)
                    {
                        BosFree(node);
                        goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
                    }
                
                    for (i = 0; i < node->msg.msgLen; i ++)
                    {
                        node->msg.pdata[i] = pdata[i];
                    } 
                }
                else
                {
                    node->msg.pdata = pdata;
                    BosFree(sNwkCurMsgNode);
                    MemUsedFlag = TRUE;
                }
                
                if (sNwkMsgSendLinkHead == NULL)
                {
                    sNwkMsgSendLinkHead = node;
                }
                else
                {
                    struct MsgSendLinkNode_t    *TailOfNwkMsgLink;
                     
                    for (TailOfNwkMsgLink = sNwkMsgSendLinkHead; TailOfNwkMsgLink->next != NULL; TailOfNwkMsgLink = TailOfNwkMsgLink->next);
         
                    TailOfNwkMsgLink->next = node;
                }
                
                sNwkSendLinkCnt ++;
    
                BosEventSend(TASK_SNWK_ID, SNWK_MSG_SEND_EVENT); 
            }          
        } // end of if (npdu->frameCtrl.bits.frameType == SNWK_DATA_TYPE)
   
CHECK_AND_RETURN_FROM_SNWK_MSG_RECV:
        
        if (!MemUsedFlag)
        {
            BosFree(sNwkCurMsgNode);
            BosFree(pdata);
        }
        
        if (sNwkMsgRecvLinkHead != NULL)
            BosEventSend(TASK_SNWK_ID,SNWK_MSG_RECV_EVENT);                      
    }               
        
}

/* */
void sNwkDataIndicationCallBackSet(DataIndicationCallBack_t p)
{
    sNwkDataIndication = p;
}

/* */
static void sMacDataIndication(void *p)
{
    struct MsgRecvLinkNode_t *pMsgNode=p;
      
    if (sNwkMsgRecvLinkHead == NULL)
    {
        sNwkMsgRecvLinkHead = pMsgNode;
    }
    else
    {
        struct MsgRecvLinkNode_t    *pNode;
        
        for (pNode = sNwkMsgRecvLinkHead; pNode->next != NULL; pNode = pNode->next);
        
        pNode->next = pMsgNode;
    }
    
    pMsgNode->next = NULL;
    
    BosEventSend(TASK_SNWK_ID, SNWK_MSG_RECV_EVENT);   
    
}

/* */
saddr_t sNwkGetNextHop(saddr_t dstAddr)
{
    saddr_t    nextHop = 0xFFFFFFFF;
    int i;

    if (dstAddr == 0xFFFFFFFF)
        return 0xFFFFFFFF;

    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr == dstAddr)
        {
            nextHop = sNwkRouteTab[i].nextHop;
            break;
        }
    }

    return  nextHop;
}

/* 往路由表中添加一个新的路由 */
error_t sNwkAddRoute(saddr_t dstAddr, saddr_t nextHop)
{
    int i;
    
    /* 先查找是否已经存在 */
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr == 0xFFFFFFFF)
            continue;

        if (sNwkRouteTab[i].dstAddr == dstAddr)
        {
            //更新路由表
            sNwkRouteTab[i].nextHop = nextHop;

            return BOS_EOK;
        }
    }

    /* 查找一个没有用过的 sNwkRouteTab */
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr != 0xFFFFFFFF)
            continue;
        
        sNwkRouteTab[i].dstAddr = dstAddr;
        sNwkRouteTab[i].nextHop = nextHop;
        
        return BOS_EOK;
    }
    
    /* 如果都被占用了 */
    return -BOS_ENOMEM;
}

void sNwkDelateRoute(saddr_t addr)
{
    int i;

    /* 先查找是否已经存在 */
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr == 0xFFFFFFFF)
            continue;

        if (sNwkRouteTab[i].nextHop == addr)
        {
            sNwkRouteTab[i].dstAddr = 0xFFFFFFFF;
        }
    }
}

/* 数据发送返回函数 */
static void sMacDataSendCallBack(struct MsgSendLinkNode_t *node, bool_t result)
{
    struct sMacMpdu_t           *mpdu;
            
    if (result)
    {
        BosFree(node->msg.pdata);
        BosFree(node);
        sNwkSendLinkCnt --;

        return;
    }

    /* send fail */
    /* change to broadcast send */
    node->next = NULL;
    node->retry = SNWK_MAX_RETRY_TIMES;

    mpdu = (struct sMacMpdu_t *)(node->msg.pdata);
    mpdu->macDstAddr = 0xFFFFFFFF;

    if (sNwkMsgSendLinkHead == NULL)
    {
        sNwkMsgSendLinkHead = node;
    }
    else
    {
        struct MsgSendLinkNode_t    *TailOfNwkMsgLink;

        for (TailOfNwkMsgLink = sNwkMsgSendLinkHead; TailOfNwkMsgLink->next != NULL; TailOfNwkMsgLink = TailOfNwkMsgLink->next);
        
        TailOfNwkMsgLink->next = node;
    }

    BosEventSend(TASK_SNWK_ID, SNWK_MSG_SEND_EVENT);

}

#ifdef USHELL
int do_listroute(cmd_tbl_t * cmdtp, int argc, char *argv[])
{
    int i;
    
    if (argc != 1)
    {
        printf("Unknown command '%s' - try 'help'\r\n", argv[0]);
        
        return -1;
    }
    
    printf("dest\t\tnexthop\t\thopcnt\n");
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr == 0xFFFFFFFF)
            continue;
        
        printf("0x%04x%04x\t\t0x%04x%04x\n", (uint16_t)(sNwkRouteTab[i].dstAddr>>16), (uint16_t)(sNwkRouteTab[i].dstAddr),
                                                   (uint16_t)(sNwkRouteTab[i].nextHop>>16), (uint16_t)(sNwkRouteTab[i].nextHop));
    }
    
    return 0;
}
#endif
