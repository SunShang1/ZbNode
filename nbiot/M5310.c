/*
 * Copyrite (C) 2019, Lamp
 * 
 * 文件名称: M5310.c
 * 文件说明：移动通信模组
 *
 */

#include <string.h>
#include <stdlib.h>

#include "netMngr.h"
#include "sysTimer.h"

/********************************************
* Defines *
********************************************/




/********************************************
* Typedefs *
********************************************/




/********************************************
* Globals * 
********************************************/
char miplcfg[] = "AT+MIPLCONF=71,10000000805101001900636F61703A2F2F3138332E3233302E34302E33393A353638331F003132333435363738393031323334353B313233343536373839303132333435050501,1,1\r\n";

const at_cmd_t init_at[] = 
{
    /* 开机是否成功检查 */
    {
        "AT\r\n",
        "\r\nOK\r\n",
        5*ONE_SECOND,
        3
    },
    /* 基站附着检查 */
    {
        "AT+CGSN=1\r\n",
        "+CGSN:%s\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        "AT+NCCID\r\n",
        "+NCCID:%s\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        "AT+CEREG?\r\n",
        "+CEREG:%d,%d\r\n\r\nOK\r\n",
        1*ONE_SECOND,
        30
    },
    
    {
        "AT+CGATT?\r\n",
        "+CGATT:%d\r\n\r\nOK\r\n",
        1*ONE_SECOND,
        30
    },
    
    {
        "AT+CSQ\r\n",
        "+CSQ:%d,%d\r\n\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        miplcfg,
        "\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        "AT+MIPLADDOBJ=0,3200,0\r\n",
        "\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        "AT+MIPLNOTIFY=0,3200,0,5505,6,\"000102030405\",1\r\n",
        "\r\nOK\r\n",
        2*ONE_SECOND,
        3
    },
    
    {
        "AT+MIPLOPEN=0,30\r\n",
        "\r\n+MIPLOPEN:0,1\r\n",
        20*ONE_SECOND,
        3
    },
};

extern uint8_t  module_status;

extern at_machine_t *atm;

extern char    imei[15];
extern char    iccid[20];

extern uint8_t   rssi;
/********************************************
* Function defines *
********************************************/
static void M5310_reset_high(void)
{
    MCU_IO_OUTPUT(0, 0, 0);
}

static void M5310_reset_low(void)
{
    MCU_IO_OUTPUT(0, 0, 1);
}

/* 
 * name:    M5310InitAT
 * brief:   run M5310 init AT command.
 * param:   
 * return:  
**/
void M5310InitAT(void)
{
    /* 模组开机初始化指令操作步骤 */
    static uint8_t  init_step;
    static int retry;
    static tick_t delay;
    
    int rc;
    
    //硬件复位
    if (module_status == STA_OFF)
    {
        M5310_reset_high();
        
        delay = TickGet()+ ONE_MILLISECOND*110;
        while(!timeout(delay));
        
        M5310_reset_low();
        
        module_status ++;
        
        init_step = 0;
        retry = 0;
        
        delay = TickGet()+ ONE_SECOND*10;
    }
    else if (module_status == STA_INIT)
    {
        if (!timeout(delay))
            return;
        
        rc = atCmdStart(atm, init_at[init_step].cmd, init_at[init_step].resp, init_at[init_step].timeout);
        
        if (rc == 0)  //receive resp
        {
            retry ++;
            
            if (init_step == 3) //reg check
            {
                int status;
                
                memcpy(&status, atm->resp+sizeof(int), sizeof(int));
            
                if ( (status != 1) && (status != 5) )  //not registered
                {
                    //delay and try again
                    delay = TickGet()+ ONE_SECOND*3;
                    
                    atCmdStop(atm);
                    
                    return;
                }
            }
            else if (init_step == 4)
            {
                int status;
                
                memcpy(&status, atm->resp, sizeof(int));
            
                if (status != 1)
                {
                    //delay and try again
                    delay = TickGet()+ ONE_SECOND*3;
                    
                    atCmdStop(atm);
                    
                    return;
                }
            }
            else if (init_step == 1)    //IMEI
            {
                memcpy(imei, atm->resp, sizeof(imei));
                
                //参考移动提供的配置工具
                for (int i = 0; i < 15; i ++)
                {
                    miplcfg[90 + 2*i] = imei[i];
                    miplcfg[122 + 2*i] = imei[i];
                }
            }
            else if (init_step == 2)    //ICCID
            {
                memcpy(iccid, atm->resp, sizeof(iccid));
            }
            else if (init_step == 5)    //CSQ
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
                delay = TickGet()+ ONE_SECOND*3;
            }
            
            atCmdStop(atm);
        }
    }
}