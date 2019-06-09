/*
 * Copyrite (C) 2010, BeeLinker
 *
 * �ļ����ƣ�sNwk.h
 * �ļ�˵����smartNet NWK��Э��ͷ�ļ�
 * 
 */
 
#ifndef __SNWK_H__
#define __SNWK_H__

#include "saddr.h"
#include "error.h"
#include "sMac.h"

/********************************************
* Defines *
********************************************/
#define SNWK_MAX_PAYLOAD				(119-sizeof(struct sNwkNpdu_t)-MAC_HEAD_LEN) //119-11-13


/********************************************
* Typedefs *
********************************************/
/* ��������֡�ֶζ��� */
union sNwkFrameCtrl_t
{
    struct
    {
        // 0x00: Data; 0x01: Command; 0x10/0x11: reserved
        uint8_t           frameType   :2;
        uint8_t           version     :4;
        uint8_t                       :2;
    }bits;
    
    uint8_t        u8Val;
};   

/* �����ͨ��֡֡�ṹ */
struct sNwkNpdu_t
{
    /* header */
    union sNwkFrameCtrl_t   frameCtrl;
    saddr_t                  nwkDstAddr;
    saddr_t                  nwkSrcAddr;
    uint8_t                  radius;
    uint8_t                  seqNum;
    
    /* payload */
    uint8_t                  pdata[0];
    
};

/********************************************
* Function defines *
********************************************/
/* */
error_t sNwkInit(void);

/* */
error_t  sNwkSend(saddr_t dstAddr, uint8_t *buf, uint8_t len, uint8_t radius);

/* */
uint8_t sNwkRecv(saddr_t *, uint8_t *, uint8_t, uint8_t *, uint8_t *);

/* */
void sNwkDataIndicationCallBackSet(DataIndicationCallBack_t p);

/* */
saddr_t sNwkGetNextHop(saddr_t dstAddr);

/* */
error_t sNwkAddRoute(saddr_t dstAddr, saddr_t nextHop);

/* */
void sNwkDelateRoute(saddr_t addr);

#endif
