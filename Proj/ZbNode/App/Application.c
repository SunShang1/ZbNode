/*
 * 
 * 文件名称: App.c
 * 文件说明：系统应用程序
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
#include "ushell.h"
#include "dbg.h"
#include "list.h"
#include "netMngr.h"
#include "attribute.h"
#include "att7053bu.h"
/********************************************
* Defines *
********************************************/
#define APP_RFRECV_EVENT            0x01
#define APP_HEARTBEAT_EVENT         0x02
#define APP_UARTRECV_EVENT          0x03
#define APP_NETRECV_EVENT           0x04


__code const uint32_t macAddr = 0x0300000A; 
/********************************************
* Typedefs *
********************************************/

uint8_t LampPower = 0;

/********************************************
* Globals * 
********************************************/
uint8_t                 rxBuf[128];
uint8_t                 txBuf[128];

static uint16_t MySeq = 0;

static SysTimerID_t            heartBeatTimerId;

static void netDataIndication(void *p);
//static void Uart0RxIndicat(void * p);

static void sysHeaderAdd(sys_pkt_hdr_t *hdr, uint8_t type, uint16_t seq, uint16_t paylen);

static void heartBeatTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
    
    BosEventSend(TASK_APPLICATION_ID,APP_HEARTBEAT_EVENT);
}

static void rebootTimeOutCallBack(SysTimerID_t timeId)
{
    (void)timeId;
    
    halReboot();
}

/* */
error_t ApplicationInit(void)
{
    saddr_t            MyAddr = sMacAddressGet();

    att7053buInit();
    
    /* nbiot init */
    netMngrInit();
    
    netDataIndicationCallBackSet(netDataIndication);

    heartBeatTimerId = SysTimerAlloc();
    
    SysTimerStart(heartBeatTimerId, SYSTIMER_TYPE_ONCE, ONE_SECOND*5, heartBeatTimeOutCallBack);
    
    LampCtrl(100);    
    
    return BOS_EOK;
}

/* */
__task void AppTask(event_t event)
{
    if (event == APP_RFRECV_EVENT)
    {
        saddr_t  srcAddr;
        uint8_t   len;
        uint8_t   cmd;
        uint8_t   *ptr = rxBuf;
        uint8_t *head;
        struct sNwkNpdu_t *npdu;
        
        len = receive(&srcAddr,rxBuf,125,head,NULL);
        
        npdu = (struct sNwkNpdu_t *)(head + MAC_HEAD_LEN);

        if (len == 0)
            return;
        
        cmd = *ptr ++;
        len --;
    }
    else if (event == APP_HEARTBEAT_EVENT)
    {
        sys_pkt_hdr_t       *hdr;
        uint8_t         *ptr;
        
        uint8_t          sum;
        uint8_t plen = 0;
        uint8_t maxSize;
        
        uint32_t    u32tmp;
        
        uint16_t   heartBeat;
        
        LampPower = att7053buPowerGet();
        
//        printf("\r\nPOWER: %dW\r\n\r\n", LampPower);
        
        hdr = (sys_pkt_hdr_t *)BosMalloc(128);
        if (hdr == NULL)
            goto NEXT_HEARTBEAT;
        
        memset(hdr, 0, 128);
    
        ptr = (uint8_t *)(hdr + 1);
    
        maxSize = 128 - sizeof(sys_pkt_hdr_t);
        
        u32tmp = htonl(macAddr);
        memcpy(ptr, &u32tmp, sizeof(u32tmp));
        plen += sizeof(u32tmp);
        
        ptr[plen ++] = 7;    // attr number
        
        plen += attributeGet(ATTR_SWVER, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_GITSHA, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_HEARTBEAT, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_LAMPPOWER, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_LEDBRIGHT, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_ICCID, ptr+plen, maxSize-plen);
        plen += attributeGet(ATTR_SQ, ptr+plen, maxSize-plen);
        
        sysHeaderAdd(hdr, SYSCMD_NODE_STATUS, MySeq ++, plen);
        
        sum = sum8(0, (uint8_t *)hdr, plen + sizeof(sys_pkt_hdr_t));
        ptr[plen ++] = sum;
        
        netSend((uint8_t *)hdr, plen + sizeof(sys_pkt_hdr_t));

        BosFree(hdr);

NEXT_HEARTBEAT:

        nvramSysBlkParamGet(heartBeat, &heartBeat);
            
        SysTimerStart(heartBeatTimerId, SYSTIMER_TYPE_ONCE, ONE_SECOND*heartBeat*60, heartBeatTimeOutCallBack);
    }
    else if (event == APP_NETRECV_EVENT)
    {
        uint8_t *pbuf;
        int len;

        len = netRecv(&pbuf);
        if (len <= 0)
            return;
        
        halUartSend(NBIOT_UART, pbuf, len);
        
        BosFree(pbuf);
    }
}

void sysReboot(void)
{
    SysTimerID_t rebootTimerId = SysTimerAlloc();
    
    SysTimerStart(rebootTimerId, SYSTIMER_TYPE_ONCE, ONE_SECOND>>2, rebootTimeOutCallBack);
}

/* 网络层收到数据之后会回调该函数 */
static void netDataIndication(void *p)
{
    (void)p;
    
    BosEventSend(TASK_APPLICATION_ID, APP_NETRECV_EVENT);
}

static void sysHeaderAdd(sys_pkt_hdr_t *hdr, uint8_t type, uint16_t seq, uint16_t paylen)
{
    if (!hdr)
        return;
    
    memset(hdr, 0, sizeof(sys_pkt_hdr_t));

    memcpy(hdr->prefix, "LAMP", sizeof(hdr->prefix));
    hdr->devId = htonl(macAddr);
    hdr->type = type;
    hdr->seq = htons(seq);
    hdr->len = htons(paylen);
}

void LampCtrl(uint8_t bright)
{    
    if (bright == 0)
    {
        MCU_IO_OUTPUT(0, 6, 1);
    }
    else
    {
        MCU_IO_OUTPUT(0, 6, 0);
    }
    
    McuPwmCfg(500, bright);
}