/*
 * Copyrite (C) 2019, Lamp
 * 
 * 文件名称: BC95.c
 * 文件说明：电信NB-IoT通信模组
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "netMngr.h"
#include "sysTimer.h"
#include "syslib.h"
#include "BosMem.h"
#include "list.h"
/********************************************
* Defines *
********************************************/
#define TELECOM_IOT_HEAD_SIZE       3



/********************************************
* Typedefs *
********************************************/




/********************************************
* Globals * 
********************************************/
const at_cmd_t init_at[] = 
{
    /* 开机是否成功检查 */
    {
        "AT\r\n",
        "\r\nOK\r\n",
        5*ONE_SECOND,
        3
    },
    /* 设置NB-IOT模组基带信息 */
    {
        "AT+NBAND=?\r\n",
        "+NBAND:(5)\r\n\r\nOK\r\n",
        ONE_SECOND,
        3
    },
    
    /* 设置SIM卡功能数 */
    {
        "AT+CFUN=0\r\n",
        "\r\nOK\r\n",
        2*ONE_SECOND,
        3 
    },
    
    /* 设置NB模块扰码功能*/
    {
        "AT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3
    },
    
    {
        "AT+NCONFIG=CR_0859_SI_AVOID,TRUE\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3
    },
    
    {
        "AT+NCONFIG=AUTOCONNECT,TRUE\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3
    },
    
    /* 重启NB模块 */
    {
        "AT+NRB\r\n",
        "\r\nREBOOTING\r\n",
        ONE_SECOND,
        3,  
    },
    
    /* IMEI */
    {
        "AT+CGSN=1\r\n",
        "+CGSN:%s\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        10,
    },
    
    /* 设置SIM卡功能数 */
    {
        
        "AT+CFUN=1\r\n",
        "\r\nOK\r\n",
        2*ONE_SECOND,
        3,
    },
    
    /* 获取SIM卡ICCID标识码 */
    {
        "AT+NCCID\r\n",
        "+NCCID:%s\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        3,
    },
    
    /* 禁止模块进入PSM模式 */
    {
        "AT+CPSMS=0\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,        
    },
    
    /* 使能模块附着网络 */
    {
        "AT+CGATT=1\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* 查询NB-IOT模组附着网络情况 */
    {
        "AT+CGATT?\r\n",
        "+CGATT:%d\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        30
    },
    
    /* 使能EPS网络注册上网 */
    {
        "AT+CEREG=1\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* 读取时间 */
    {
        "AT+CCLK?\r\n",
        "+CCLK:%d/%d/%d,%d:%d:%s\r\n\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* RSSI */
    {
        "AT+CSQ\r\n",
        "+CSQ:%d,%d\r\n\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* 设置NB-IOT模组CDP服务器 */
    {
        "AT+NCDP=117.60.157.137\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    {
        "AT+NCDP?\r\n",
        "+NCDP:%s,%d\r\n\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    {
        "AT+NSMI=1\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    //indications and message
    {
        "AT+NNMI=1\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
};


const at_cmd_t polling_at[] = 
{
//    /* get rx data cmd */
//    {
//        "AT+NMGR\r\n",
//        "%d,%s\r\n\r\nOK\r\n",
//        ONE_SECOND,
//        3,
//    },
    
    /* RSSI */
    {
        "AT+CSQ\r\n",
        "+CSQ:%d,%d\r\n\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* data send */
    {
        "AT+NMGS=%d,%s\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
    
    /* check if the message is send successfully */
    {
        "AT+NQMGS\r\n",
        "\r\nOK\r\n",
        ONE_SECOND,
        3,
    },
};

extern uint8_t  module_status;

extern at_machine_t *atm;

extern char    imei[15];
extern char    iccid[20];

extern uint8_t  rssi;

extern list_t           send_list_head;
extern uint8_t          send_list_cnt;

extern list_t           recv_list_head;
extern uint8_t          recv_list_cnt;

static tick_t   atdelay;
static int retry;

extern void (*netDataIndication)(void *);
/********************************************
* Function defines *
********************************************/
static void BC95_reset_high(void)
{
    MCU_IO_OUTPUT(0, 0, 1);
}

static void BC95_reset_low(void)
{
    MCU_IO_OUTPUT(0, 0, 0);
}

/* 
 * name:    M5310InitAT
 * brief:   run M5310 init AT command.
 * param:   
 * return:  
**/
void BC95InitAT(void)
{
    /* 模组开机初始化指令操作步骤 */
    static uint8_t  init_step;
    
    int rc;
    
    //硬件复位
    if (module_status == STA_OFF)
    {
        BC95_reset_high();
        
        atdelay = TickGet()+ ONE_MILLISECOND*110;
        while(!timeout(atdelay));
        
        BC95_reset_low();
        
        module_status ++;
        
        init_step = 0;
        retry = 0;
        
        atdelay = TickGet()+ ONE_SECOND*10;
    }
    else if (module_status == STA_INIT)
    {
        if (!timeout(atdelay))
            return;
        
        rc = atCmdStart(atm, init_at[init_step].cmd, init_at[init_step].resp, init_at[init_step].timeout);
        
        if (rc == 0)  //receive resp
        {
            if (init_step == 7)    //IMEI
            {
                memcpy(imei, atm->resp, sizeof(imei));
            }
            else if (init_step == 9)    //ICCID
            {
                memcpy(iccid, atm->resp, sizeof(iccid));
            }
            else if (init_step == 12)   //CGATT
            {
                int status;
                
                memcpy(&status, atm->resp, sizeof(int));
            
                if (status != 1)
                {
                    //delay and try again
                    atdelay = TickGet()+ ONE_SECOND*3;
                    
                    atCmdStop(atm);
                    
                    return;
                }
            }
            else if (init_step == 14)    //TIME
            {
                
            }
            else if (init_step == 15)    //RSSI
            {
                int tmp;
                
                memcpy(&tmp, atm->resp, sizeof(int));
                
                rssi = (uint8_t)tmp;
            }
            
            init_step ++;
            retry = 0;

            if (init_step == sizeof(init_at)/sizeof(at_cmd_t))
                module_status ++;
            
            atCmdStop(atm);
        }
        else if (rc < 0)  //timeout
        {
            retry ++;
            
            if (retry >= init_at[init_step].retry)
                module_status = STA_OFF;
            else
            {
                //delay and try again
                atdelay = TickGet()+ ONE_SECOND*3;
            }
            
            atCmdStop(atm);
        }
    }
}


/* 
 * name:    BC95PollingAT
 * brief:   polling run the AT command after connected.
 * param:   
 * return:  
**/
void BC95PollingAT(void)
{
#define POLLING_AT_NUM        3
    static uint8_t  polling_step = 0;
    
    int rc;
    
    if (!timeout(atdelay))
        return;
    
    if (polling_step == POLLING_AT_NUM)
        polling_step = 0;
    
    //RSSI
    if (polling_step == 0)
    {
        if (!timeout(atdelay))
        {
            polling_step ++;
            return;
        }
          
        rc = atCmdStart(atm, polling_at[polling_step].cmd, polling_at[polling_step].resp, polling_at[polling_step].timeout);
        
        if (rc == 0)
        {
            int tmp;
                
            memcpy(&tmp, atm->resp, sizeof(int));
                
            rssi = (uint8_t)tmp;
            
            polling_step ++;
            
            atCmdStop(atm);
            
            retry = 0;
            
            atdelay = TickGet()+ ONE_SECOND*30;
        }
        else if (rc < 0)
        {
            retry ++;
                        
            if (retry > 3)
                module_status = STA_OFF;
               
            atCmdStop(atm);
        }
    }
    
    //MSG send
    if (polling_step == 1)
    {
        if (send_list_cnt != 0)
        {
            send_buf_info_t      *send;
            list_t          *entry, *n;
            
            uint16_t    cmdlen;
            char *cmd;
           
            list_for_each_safe(entry, n, &send_list_head)
            {
                send = list_entry(entry, send_buf_info_t, link);
                break;
            }
            
            cmd = BosMalloc(20+(send->len)*2);
            memset(cmd, 0, 20 + (send->len)*2);
            sprintf(cmd, "AT+NMGS=%d,", send->len);
            cmdlen = strlen(cmd);
            
            for (int i = 0; i < send->len; i ++)
            {
                cmd[cmdlen ++] = I2A((send->buf[i]>>4)&0x0F);
                cmd[cmdlen ++] = I2A(send->buf[i]&0x0F);
            }
            
            cmd[cmdlen ++] = '\r';
            cmd[cmdlen ++] = '\n';
                    
            rc = atCmdStart(atm, cmd, "\r\nOK\r\n", 2*ONE_SECOND);
            BosFree(cmd);
            
            if (rc == 0)
            {
                list_del(entry);
                BosFree(send->buf);
                BosFree(send);
                        
                send_list_cnt --;
                        
                atCmdStop(atm);
                
                atdelay = TickGet()+ ONE_SECOND>>2;
                polling_step ++;
                retry = 0;
            }
            else if (rc < 0)
            {
                retry ++;
                        
                if (retry > 3)
                {
                    list_del(entry);
                    BosFree(send);
                        
                    send_list_cnt --;

                    module_status = STA_OFF;
                }
                        
                atCmdStop(atm);
            }
        }
        else
        {
            polling_step = 0;
        }
    }
    
    //check send result
    if (polling_step == 2)
    {
        rc = atCmdStart(atm, polling_at[polling_step].cmd, polling_at[polling_step].resp, polling_at[polling_step].timeout);
        
        if (rc <= 0)
        {
            polling_step ++;
            
            atCmdStop(atm);
            
            retry = 0;
        }
    }
}

int BC95IoTHeaderAdd(uint8_t *in, int lens, uint8_t **out)
{
    uint8_t *ptr;
    
    if (!in)
        return 0;

    ptr = BosMalloc(lens+TELECOM_IOT_HEAD_SIZE);
    if (ptr == NULL)
        return 0;
    
    ptr[0] = 0x02; //type
    ptr[1] = (uint8_t)(lens>>8); //len
    ptr[2] = (uint8_t)lens; 
    
    memcpy(ptr+3, in, lens);
    
    *out = ptr;
    
    return lens + TELECOM_IOT_HEAD_SIZE;
}

/* search the rx notice packet*/
void BC95RxIndicationSearch(at_machine_t *atm, char ch)
{
    const char *qird_format="+NNMI:%d,%s\r\n";
    
    static uint16_t qird_cnt = 0;
    static uint8_t *prx = NULL;
    static uint16_t rxLen;
    static uint16_t rx_ascii_len;

QIRD_FORMAT_SEARCH:
    if (qird_cnt == 0)
    {
        rxLen = 0;
        
        if (rx_ascii_len != 0)
        {
            BosFree(prx);
            rx_ascii_len = 0;
        }
    }
      
    if (qird_format[qird_cnt] == '%')
        qird_cnt ++;
        
    if ((qird_cnt > 0) && (qird_format[qird_cnt - 1] == '%'))
    {
        if (qird_format[qird_cnt] == 'd')
        {
            if (ch == qird_format[qird_cnt+1])
            {
                qird_cnt += 2;
                
                if (rxLen <= TELECOM_IOT_HEAD_SIZE) //IOT header size is 3
                {
                    qird_cnt = 0;
                    goto QIRD_FORMAT_SEARCH;
                }
            }
            else if (ch < '0' || ch > '9')
            {
                if ( ((qird_cnt == 1) && (qird_format[0] != '%')) ||
                     (qird_cnt > 1) )
                {
                    qird_cnt = 0;
                    
                    goto QIRD_FORMAT_SEARCH;
                }
            }
            else
            {
                rxLen *= 10;
                rxLen += (ch-'0');
            }
        }        
        else if (qird_format[qird_cnt] == 's')
        {
            if (ch == qird_format[qird_cnt+1])
            {
                qird_cnt += 2;
            }
            else
            {
                if (rx_ascii_len == 0)
                {
                    if ((prx = BosMalloc(rxLen - TELECOM_IOT_HEAD_SIZE)) == NULL)
                    {
                        qird_cnt = 0;
                        
                        goto QIRD_FORMAT_SEARCH;
                    }
                    
                    memset(prx, 0, rxLen - TELECOM_IOT_HEAD_SIZE);
                }
                
                if (rx_ascii_len >= TELECOM_IOT_HEAD_SIZE*2)
                {
                    prx[(rx_ascii_len>>1) - TELECOM_IOT_HEAD_SIZE] <<= 4;
                    prx[(rx_ascii_len>>1) - TELECOM_IOT_HEAD_SIZE] += A2I(ch);
                    
                }
                
                rx_ascii_len ++;
                
                if (rx_ascii_len > rxLen*2)
                {
                    qird_cnt = 0;
                    
                    goto QIRD_FORMAT_SEARCH;
                }
            }
        }

    }
    else if (qird_format[qird_cnt] == ch)
        qird_cnt ++;
    else
    {
        if ( ((qird_cnt == 1) && (qird_format[0] != '%')) ||
             (qird_cnt > 1) )
        {
            qird_cnt = 0;
            
            goto QIRD_FORMAT_SEARCH;
        }
    }
    
    if (qird_cnt == strlen(qird_format))
    {
        recv_buf_info_t   *recv;
        
        recv = BosMalloc(sizeof(recv_buf_info_t));
        if (recv != 0)
        {
            memset(recv, 0, sizeof(recv_buf_info_t));
                       
            recv->buf = prx;            
            recv->len = (rx_ascii_len>>1)-TELECOM_IOT_HEAD_SIZE;
            
            list_add_tail(&recv->link, &recv_list_head);
            recv_list_cnt ++;
            /* prx will be free by user, so must clean rx_ascii_len here. */
            rx_ascii_len = 0;
            prx = NULL;
            
            if (netDataIndication != NULL)
                netDataIndication(NULL);
        }

        qird_cnt = 0;        
    }

    return;
}




