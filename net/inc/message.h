/*
 *
 * 文件名称：message.h
 * 文件说明：smartNet 无线数据帧结构定义
 * 
 */
 

#ifndef     __MESSAGE_H__
#define     __MESSAGE_H__

#include "pub_def.h"
#include "board.h"

/* 无线数据帧结构定义 */
struct Msg_t    
{
    uint8_t     msgLen;    
    uint8_t     *pdata;
};


/* 发送数据帧链表节点定义 */
struct MsgSendLinkNode_t
{
    struct MsgSendLinkNode_t    *next;
    
    /* 发送时间 */
    tick_t                      sendTick;
    
    /* 发送回调函数：成功或失败 */
    void                        (*pMacSendOutCallBack)(struct MsgSendLinkNode_t *, bool_t);
    
    uint8_t                     retry;
    
    struct Msg_t                msg;
};


/* 接收数据帧链表节点定义 */
struct MsgRecvLinkNode_t
{
    struct MsgRecvLinkNode_t   *next;
    
    /* 接收时间 */
    tick_t                      rcvTick;
    
    uint8_t                     rssi;
    
    struct Msg_t               msg;
};




#endif