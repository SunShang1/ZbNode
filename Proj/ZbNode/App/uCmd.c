/*
 * 
 * 文件名称: uCmd.c
 * 文件说明：串口通信命令解析
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
#include "attribute.h"

 /********************************************
* Defines *
********************************************/



/********************************************
* Typedefs *
********************************************/




/********************************************
* Globals * 
********************************************/
extern __code const uint8_t preFix[2];
extern __code const uint8_t sufFix[2];

/********************************************
* Function defines *
********************************************/
static void uCmdSend(uint8_t type, uint8_t *payload, uint8_t len);


void uartCmdExe(uint8_t type, uint8_t *payLoad, uint8_t payLen)
{    
    uint8_t *ptxBuf;
    uint8_t plen = 0;
    
    uint8_t rt = 0;
    
    ptxBuf = BosMalloc(255);
    if (ptxBuf == NULL)
        return;
    
    switch(type)
    {
        case APPCMD_DATA_SEND_REQ:
        {
            saddr_t dstAddr;            
            
            if (payLen <= sizeof(dstAddr))
                break;
            
            if (payLen > sizeof(dstAddr) + 96)
            {
                rt = 0x03;
                goto APPCMD_DATA_SEND_RESP;
            }
            
            if (sMacLinkStatusGet() != STATUS_LINKED)
            {
                rt = 0x01;
                goto APPCMD_DATA_SEND_RESP;
            }
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);            
            
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
                
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf+plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
APPCMD_DATA_SEND_RESP:
  
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);
            payLoad += sizeof(dstAddr);
            payLen -= sizeof(dstAddr);
  
            ptxBuf[plen ++] = rt;
            
            uCmdSend(APPCMD_DATA_SEND_RESP, ptxBuf, plen);
  
            break;
        }
        case APPCMD_NODELIST_SET_REQ:
        {
            uint16_t num;
            saddr_t  *paddr;
            nvNodesListBlk_t    *pNListBlk;
            
            if (payLen < sizeof(num))
            {
                rt = 0x01;
                goto APPCMD_NODELIST_SET_RESP;
            }
            
            memcpy(&num, payLoad, sizeof(num));
            num = ntohs(num);
            payLoad += sizeof(num);
            payLen -= sizeof(num);
            
            if (num > MAX_NODES_IN_BLK)
            {
                rt = 0x02;
                goto APPCMD_NODELIST_SET_RESP;
            }
            
            if (payLen != sizeof(saddr_t)*num)
            {
                rt = 0x01;
                goto APPCMD_NODELIST_SET_RESP;
            }
            
            /* save to nvram */
            if (nvramBlkParamSetInit(NV_BLK_NODESLIST, (uint8_t **)&pNListBlk) < 0)
            {
                rt = 0x03;
                goto APPCMD_NODELIST_SET_RESP;
            }
            
            pNListBlk->num = (uint8_t)num;
            paddr = (saddr_t *)payLoad;
            for (uint8_t i = 0; i < pNListBlk->num; i ++)
            {
                pNListBlk->addr[i] = ntohl(*paddr);
                paddr ++;
            }
            
            nvramBlkParamSetFinal(NV_BLK_NODESLIST, (uint8_t *)pNListBlk, 1);
            
APPCMD_NODELIST_SET_RESP:
  
            uCmdSend(APPCMD_NODELIST_SET_RESP, &rt, sizeof(rt));
  
            break;
        }
        case APPCMD_NODELIST_GET_REQ:
        {
            nvNodesListBlk_t    *pNListBlk;
            uint8_t *pbuf;
            uint16_t    num;
            saddr_t  *paddr;
            
            if (nvramBlkParamSetInit(NV_BLK_NODESLIST, (uint8_t **)&pNListBlk) < 0)
            {
                rt = 0x03;
                goto APPCMD_NODELIST_GET_RESP;
            }
            
            num = pNListBlk->num;
            pbuf = BosMalloc(num*sizeof(saddr_t) + sizeof(num));
            if (pbuf == NULL)
            {
                nvramBlkParamSetFinal(NV_BLK_NODESLIST, (uint8_t *)pNListBlk, 0);
                
                rt = 0x03;
                goto APPCMD_NODELIST_GET_RESP;
            }
            
            num = htons(num);
            memcpy(pbuf, &num, sizeof(num));
            paddr = (saddr_t *)(pbuf + sizeof(num));
            num = pNListBlk->num;
            
            for (uint8_t i = 0; i < num; i ++)
            {
                *paddr ++ = htonl(pNListBlk->addr[i]);
            }
            
            nvramBlkParamSetFinal(NV_BLK_NODESLIST, (uint8_t *)pNListBlk, 0);
            
            uCmdSend(APPCMD_NODELIST_GET_RESP, pbuf, num*sizeof(saddr_t) + sizeof(num));
            BosFree(pbuf);
            
APPCMD_NODELIST_GET_RESP:
  
            break;
        }
        case APPCMD_ATTR_SET_REQ:
        {
            saddr_t dstAddr;
            
            if (payLen <= sizeof(dstAddr) + sizeof(uint8_t) + sizeof(uint8_t))
            {
                rt = 0x0F;
                goto APPCMD_ATTR_SET_RESP;
            }
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
            
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf+plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
            //for me 
APPCMD_ATTR_SET_RESP:
  
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);
            
            if (rt == 0)
                attributeSet(payLoad, payLen, 0);
  
            ptxBuf[plen ++] = rt;
            
            uCmdSend(APPCMD_ATTR_SET_RESP, ptxBuf, plen);
            
            break;
        }
        case APPCMD_ATTR_GET_REQ:
        {
            saddr_t dstAddr;
            
            uint8_t num;
            uint8_t maxSize = 255;

            if (payLen <= sizeof(dstAddr) + sizeof(uint8_t) + sizeof(uint8_t))
            {
                rt = 0x0F;
                goto APPCMD_ATTR_GET_RESP;
            }
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
            
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf + plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
                
            //for me
APPCMD_ATTR_GET_RESP:
  
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);
            
            payLoad += sizeof(dstAddr);
            payLen -= sizeof(dstAddr);
            
            ptxBuf[plen ++] = rt;//result            
            if (rt == 0x00)
            {
                ptxBuf[plen ++] = 0; //ptxBuf[5] for attr number
                    
                num = *payLoad ++;
                payLen --;
                    
                while (num --)
                {
                    uint8_t attrLen = attributeGet(*payLoad ++, ptxBuf+plen, maxSize-plen);
                        
                    payLen --;
                        
                    if (attrLen != 0)
                    {
                        plen += attrLen;
                                    
                        ptxBuf[5] ++;
                    } 
                }
            }

            uCmdSend(APPCMD_ATTR_GET_RESP, ptxBuf, plen);

            break;
        }
        case APPCMD_NODE_CTRL_REQ:
        {
            saddr_t dstAddr;
            uint8_t ctrl;
            
            if (payLen <= sizeof(dstAddr))
                break;
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
            
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf + plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);
            payLoad += sizeof(dstAddr);
            payLen -= sizeof(dstAddr);
            
            ctrl = *payLoad ++;            
            payLen --;
            
            switch(ctrl)
            {
                case CTRL_FACTORYRESET:
                    nvramFactoryReset();
                case CTRL_REBOOT:
                    sysReboot();
                    
                    break;
                
                case CTRL_SLEEP:
                  
                    break;
                
                default:
                    break;
            }
            
            ptxBuf[plen ++] = ctrl;
            ptxBuf[plen ++] = 0;//result
            
            uCmdSend(APPCMD_NODE_CTRL_RESP, ptxBuf, plen);
            
            break;
        }
        case APPCMD_NEWIMG_RELEASE_NTF:
        {
            saddr_t dstAddr;
            
            if (payLen <= sizeof(dstAddr) + sizeof(uint8_t)*2 + sizeof(uint16_t))
            {
                rt = 0x0F;
                goto APPCMD_NEWIMG_RELEASE_RESP;
            }
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
            
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf + plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
            //for me
APPCMD_NEWIMG_RELEASE_RESP:
   
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);            
            payLoad += sizeof(dstAddr);
            payLen -= sizeof(dstAddr);
            
            if (rt == 0)
                rt = newImgReleaseNotify(payLoad + sizeof(dstAddr), payLen - sizeof(dstAddr));
   
            ptxBuf[plen ++] = rt;
  
            uCmdSend(APPCMD_NEWIMG_RELEASE_RESP, ptxBuf, plen);

            break;
        }
        case APPCMD_NEWIMG_GET_RESP:
        {
            saddr_t dstAddr;

            if (payLen <= sizeof(dstAddr) + sizeof(uint8_t))
                break;
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                payLoad += sizeof(dstAddr);
                payLen -= sizeof(dstAddr);
            
                ptxBuf[plen ++] = type;
                memcpy(ptxBuf + plen, payLoad, payLen);
                plen += payLen;
                
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
            //for me
            payLoad += sizeof(dstAddr);
            payLen -= sizeof(dstAddr);
                
            newImgGetResp(payLoad, payLen);
            
            break;
            
        }
        case APPCMD_ROUTE_TBL_GET_REQ:
        {
            saddr_t dstAddr;

            if (payLen != sizeof(dstAddr))
                break;
            
            memcpy(&dstAddr, payLoad, sizeof(dstAddr));
            dstAddr = ntohl(dstAddr);
            
            if ( (dstAddr != 0x00000000) && dstAddr != sMacAddressGet() )
            {
                ptxBuf[plen ++] = type;
                send(dstAddr, ptxBuf, plen, 0);
                
                break;
            }
            
            if (sMacLinkStatusGet() != STATUS_LINKED)
                rt = 1;
            
            memcpy(ptxBuf, payLoad, sizeof(dstAddr));
            plen += sizeof(dstAddr);
            
            ptxBuf[plen ++] = rt;
            
            if (rt == 0)
            {
                rt = sMacSonAddrGet(ptxBuf, sizeof(ptxBuf) - (plen+1));
                
                ptxBuf[plen ++] = rt;
                plen += sizeof(saddr_t) * rt;
            }
            
            uCmdSend(APPCMD_ROUTE_TBL_GET_RESP, ptxBuf, plen);
            
            break;
        }
          
        default:
            break;
    }
    
    BosFree(ptxBuf);
}

static void uCmdSend(uint8_t type, uint8_t *payload, uint8_t len)
{
    uint8_t sum = 0;
    
    halUartSend(APP_UART, (uint8_t *)preFix, sizeof(preFix));
    sum = sum8(0, (uint8_t *)preFix, sizeof(preFix));
    
    halUartSend(APP_UART, &type, sizeof(type));
    sum = sum8(sum, &type, sizeof(type));
    
    halUartSend(APP_UART, &len, sizeof(len));
    sum = sum8(sum, &len, sizeof(len));
    
    if (len != 0)
    {
        halUartSend(APP_UART, payload, len);
        sum = sum8(sum, payload, len);
    }
    
    halUartSend(APP_UART, &sum, sizeof(sum));
    
    halUartSend(APP_UART, (uint8_t *)sufFix, sizeof(sufFix));    
}


