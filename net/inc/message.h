/*
 *
 * �ļ����ƣ�message.h
 * �ļ�˵����smartNet ��������֡�ṹ����
 * 
 */
 

#ifndef     __MESSAGE_H__
#define     __MESSAGE_H__

#include "pub_def.h"
#include "board.h"

/* ��������֡�ṹ���� */
struct Msg_t    
{
    uint8_t     msgLen;    
    uint8_t     *pdata;
};


/* ��������֡����ڵ㶨�� */
struct MsgSendLinkNode_t
{
    struct MsgSendLinkNode_t    *next;
    
    /* ����ʱ�� */
    tick_t                      sendTick;
    
    /* ���ͻص��������ɹ���ʧ�� */
    void                        (*pMacSendOutCallBack)(struct MsgSendLinkNode_t *, bool_t);
    
    uint8_t                     retry;
    
    struct Msg_t                msg;
};


/* ��������֡����ڵ㶨�� */
struct MsgRecvLinkNode_t
{
    struct MsgRecvLinkNode_t   *next;
    
    /* ����ʱ�� */
    tick_t                      rcvTick;
    
    uint8_t                     rssi;
    
    struct Msg_t               msg;
};




#endif