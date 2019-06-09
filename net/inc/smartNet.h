/*
 * Copyrite (C) 2010, BeeLinker
 *
 * 文件名称：smartNet.h
 * 文件说明：smartNet 网络协议头文件
 * 
 * 版本信息：
 * v0.1      wzy        2011/07/16
 */
 
#ifndef     __SMARTNET_H_
#define     __SMARTNET_H_

#include "sNwk.h"
#include "sMac.h"

/********************************************
* Defines *
********************************************/
#define I_AM_COORDINATOR                            (sMacDeviceTypeGet() == COORDINATOR)
#define I_AM_ROUTER                                 (sMacDeviceTypeGet() == ROUTER)
#define I_AM_ENDDEVICE                              (sMacDeviceTypeGet() == ENDDEVICE)
                       


/********************************************
* Typedefs *
********************************************/





/********************************************
* Globals * 
********************************************/



/********************************************
* Function defines *
********************************************/
/***************************网络层 & 媒体接入层接口定义 *****************************/
/* 左边为对应层所需要提供的接口 */
/* NWK 必须要实现的函数 */
#define smartNetInit()                              sNwkInit()
#define send(dstAddr, buf, len, radius)             sNwkSend(dstAddr, buf, len, radius)
#define receive(dstAddr,buf,bufSize,pHead,pLiq)     sNwkRecv(dstAddr,buf,bufSize,pHead,pLiq)
#define DataIndicationCallBackSet(p)                sNwkDataIndicationCallBackSet(p)






#endif 