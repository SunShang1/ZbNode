/*
 * Copyrite (C) 2010, BeeLinker
 *
 * �ļ����ƣ�smartNet.h
 * �ļ�˵����smartNet ����Э��ͷ�ļ�
 * 
 * �汾��Ϣ��
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
/***************************����� & ý������ӿڶ��� *****************************/
/* ���Ϊ��Ӧ������Ҫ�ṩ�Ľӿ� */
/* NWK ����Ҫʵ�ֵĺ��� */
#define smartNetInit()                              sNwkInit()
#define send(dstAddr, buf, len, radius)             sNwkSend(dstAddr, buf, len, radius)
#define receive(dstAddr,buf,bufSize,pHead,pLiq)     sNwkRecv(dstAddr,buf,bufSize,pHead,pLiq)
#define DataIndicationCallBackSet(p)                sNwkDataIndicationCallBackSet(p)






#endif 