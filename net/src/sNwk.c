/*
 * �ļ����ƣ�sNwk.c
 * �ļ�˵����smartNetЭ��ջ�����ʵ��
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
/* ����֡���Ͷ��� */
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

/* ·�ɱ��� */
struct  sNwkRouteTable_t
{
    /* Ŀ���ַ */
    saddr_t                 dstAddr;
    /* ��һ����ַ */
    saddr_t                 nextHop;
};

/********************************************
* Globals *
********************************************/

/* ����֡���к� */
static  uint8_t                     MySeqNum;
/* ��¼ǰ���ε����ݰ����кţ��������ж����ݰ����ظ�����, ��ʼ��Ϊ0 */
static  sNwkSeqList_t               sNwkSeqList[SNWK_SEQ_LIST_SIZE];
/* ��ΧΪ:0 ~ (SNWK_SEQ_LIST_SIZE - 1) */    
static  uint8_t                     sNwkSeqIndex;
    
/* ��������֡����/���� */
static struct MsgSendLinkNode_t     *sNwkMsgSendLinkHead;
/* */
static uint8_t                      sNwkSendLinkCnt;
/* ��������֡����/���� */
static struct MsgRecvLinkNode_t     *sNwkMsgRecvLinkHead;
/* �����ϲ������֡����/���� */
static struct MsgRecvLinkNode_t     *sNwkAppDataLinkHead;
/* �ص����� */
static DataIndicationCallBack_t     sNwkDataIndication;

/* ·�ɱ� */
static struct sNwkRouteTable_t      sNwkRouteTab[SNWK_ROUTE_TAB_SIZE];

/********************************************
* Function defines *
********************************************/

/* */
__task void sNwkTask(event_t);

/* */
static void sMacDataIndication(void *p);

/* ���ݷ��ͷ��غ��� */
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

/* ����㷢�� */
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
    
    /* ��ȡ���ڵ��MAC��ַ */
    MyAddr = sMacAddressGet();
    
    /* ���Ŀ���ַ���Ǳ��ڵ��ַ��ֱ�ӷ��سɹ� */
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
    
    //���ݿ���
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

/* �������� */
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
        
        //�ҳ���Ч�Ĵ����͵�����֡
        for (;;)
        {
            struct MsgSendLinkNode_t    *MsgNode;
            
            if (sNwkMsgSendLinkHead == NULL)
            {
                return;
            }
        
            if (sNwkMsgSendLinkHead->retry != 0)
                break;
            
           //�ظ���ζ�����ʧ�ܣ�ֱ�Ӷ�����֡
            MsgNode = sNwkMsgSendLinkHead;
            sNwkMsgSendLinkHead = sNwkMsgSendLinkHead->next;
                
            BosFree(MsgNode->msg.pdata);
            BosFree(MsgNode); 
            
            sNwkSendLinkCnt --;           
        }
        
        /* ��ȡ��ǰ��Ҫ���͵����ݵ������֡ͷ */    
        npdu = (struct sNwkNpdu_t*)(&(sNwkMsgSendLinkHead->msg.pdata[MAC_HEAD_LEN]));
        mpdu = (struct sMacMpdu_t *)(sNwkMsgSendLinkHead->msg.pdata);
        
        /* ��ȡ��һ����ַ */
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
                
                // ������ͳɹ�����ô����������һ�����ݰ�
                if (sNwkMsgSendLinkHead != NULL)
                {
                    BosEventSend(TASK_SNWK_ID,SNWK_MSG_SEND_EVENT);
                }
            }
            else
            {
                /* ���´����������� */
                BosEventSend(TASK_SNWK_ID,SNWK_MSG_SEND_EVENT);
            }
        }
    }
    
    /* �յ����ݴ��� */
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
        
        /* �жϵ�ַ�Ƿ�Ϊ��Ч */
        if ( (npdu->nwkSrcAddr == 0xFFFFFFFF)    ||\
             (npdu->nwkSrcAddr == MyAddr)  )
        {
            goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
        }
        
        /* �ж��Ƿ����ظ��յ���֡ */
        if (npdu->nwkSrcAddr != 0xFFFFFFFF)
        {
            for (i = 0; i < SNWK_SEQ_LIST_SIZE; i ++)
            {
                if((sNwkSeqList[i].addr == npdu->nwkSrcAddr) && (sNwkSeqList[i].seqNum == npdu->seqNum))
                {
                    goto CHECK_AND_RETURN_FROM_SNWK_MSG_RECV;
                }
            }
        
            /* �������к� */
            sNwkSeqList[sNwkSeqIndex].seqNum = npdu->seqNum;
            sNwkSeqList[sNwkSeqIndex].addr = npdu->nwkSrcAddr;
        
            if (++sNwkSeqIndex == SNWK_SEQ_LIST_SIZE)
                sNwkSeqIndex = 0;
        }
        
        if (((npdu->radius)&0x0F) > 0)
            npdu->radius --;
        
        /* �����һ������֡ */
        if (npdu->frameCtrl.bits.frameType == SNWK_DATA_TYPE)
        {    
            /* Ŀ���ַ�Ƿ�Ϊ�㲥���߱��ڵ��ַ */    
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
            
            /* ���Ŀ���ַ���Ǳ��ڵ��ַ��֧��·�ɹ��� */
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

/* ��·�ɱ������һ���µ�·�� */
error_t sNwkAddRoute(saddr_t dstAddr, saddr_t nextHop)
{
    int i;
    
    /* �Ȳ����Ƿ��Ѿ����� */
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr == 0xFFFFFFFF)
            continue;

        if (sNwkRouteTab[i].dstAddr == dstAddr)
        {
            //����·�ɱ�
            sNwkRouteTab[i].nextHop = nextHop;

            return BOS_EOK;
        }
    }

    /* ����һ��û���ù��� sNwkRouteTab */
    for (i = 0; i < SNWK_ROUTE_TAB_SIZE; i ++)
    {
        if (sNwkRouteTab[i].dstAddr != 0xFFFFFFFF)
            continue;
        
        sNwkRouteTab[i].dstAddr = dstAddr;
        sNwkRouteTab[i].nextHop = nextHop;
        
        return BOS_EOK;
    }
    
    /* �������ռ���� */
    return -BOS_ENOMEM;
}

void sNwkDelateRoute(saddr_t addr)
{
    int i;

    /* �Ȳ����Ƿ��Ѿ����� */
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

/* ���ݷ��ͷ��غ��� */
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
