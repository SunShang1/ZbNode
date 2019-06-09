/*
 * Copyrite (C) 2019, Lamp
 * 
 * 文件名称: netMngr.c
 * 文件说明：通信模组运行维护程序
 *
 */

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "sysTimer.h"
#include "kernel.h"
#include "netMngr.h"
#include "BosMem.h"
#include "list.h"
#include "syslib.h"
#include "sysTask.h"
/********************************************
* Defines *
********************************************/
#define NET_MNGR_EVENT           0x01



/********************************************
* Typedefs *
********************************************/


/********************************************
* Globals * 
********************************************/
/* 模组当前状态 */
uint8_t  module_status = STA_OFF;

at_machine_t *atm;

/* net send list */
list_t           send_list_head;
uint8_t          send_list_cnt;

/* net receive list */
list_t           recv_list_head;
uint8_t          recv_list_cnt;

char    imei[15];
char    iccid[20];

uint8_t   rssi;

void (*netDataIndication)(void *);

/********************************************
* Function defines *
********************************************/
static void netUartRxIndicat(void* p);
static void netConnect(void);
static void netMngr(void);

static void netRxTask(void);
static void atcmd_resp_search(at_machine_t *atm, char ch);
static void close_format_search(at_machine_t *atm, char ch);
static void qird_format_search(at_machine_t *atm, char ch);


uint8_t isNetConnected(void)
{
    return (module_status == STA_CONNECTED);
}

uint8_t isAtMachineIdle(void)
{
    return (atm->status == AT_IDLE);
}

/* 
 * name:    atMachineCreate
 * brief:   crreate a at machine.
 * param:   
 * return:   result, NULL - create failed
                     else - the ptr of a new at machine.
**/
static at_machine_t *atMachineCreate(void)
{
    at_machine_t *atm = BosMalloc(sizeof(at_machine_t));
                              
    if (atm != NULL)
    {
        atm->cmd = NULL;
        atm->resp_format = NULL;
        atm->format_cnt = 0;
        
        atm->resp = NULL;        
        atm->param_offset = 0;
        atm->param_len = 0;
        
        atm->status = AT_IDLE;
        atm->timeout = 0;
    }
    
    return atm;
}
                              
/* 
 * name:    atMachineInit
 * brief:   init the at machine.
 * param:   ptr of the at machine.
 * return:   result, 0 - success
**/
static int atMachineInit(at_machine_t *atm)
{
    if (atm == NULL)
        return -1;
    
    if (atm->cmd != NULL)
        BosFree(atm->cmd);

    atm->cmd = NULL;
    atm->resp_format = NULL;
    atm->format_cnt = 0;
        
    atm->resp = NULL;    
    atm->param_offset = 0;
    atm->param_len = 0;
        
    atm->status = AT_IDLE;
    atm->timeout = 0;

    return 0;
}

/* 
 * name:    netMngrInit
 * brief:   init the net manager system.
 * param:   
 * return:   result, 0 - success
**/
int netMngrInit(void)
{
    struct uartOpt_t opt;
    
    /* Init uart */
    opt.baudRate=UART_BAUD_9600;
    opt.dataBits=UART_DATA_BIT8;
    opt.stopBits=UART_STOP_BIT1;
    opt.parity='N';
    halUartInit(NBIOT_UART, &opt);
    halUartRxCbSet(NBIOT_UART, netUartRxIndicat);    
    
    /* init the tx list */
    INIT_LIST_HEAD(&send_list_head);
    send_list_cnt = 0;
    
    /* init the rx list */
    INIT_LIST_HEAD(&recv_list_head);
    recv_list_cnt = 0;

    atm = atMachineCreate();
    
    BosEventSend(TASK_NETMNGR_ID,NET_MNGR_EVENT);
    
    netDataIndication = NULL;

    return 0;
}

/* 
 * name:    netMngrTask
 * brief:   the net manager polling task.
 * param:   
 * return:  
**/
__task void netMngrTask(event_t event)
{
    if (event == NET_MNGR_EVENT)
    {
        /* module uart rx check */
        netRxTask();
            
        if (module_status < STA_CONNECTED)
        {
            netConnect();
        }
        else
        {
            netMngr();
        }
        
        BosEventSend(TASK_NETMNGR_ID,NET_MNGR_EVENT);
    }
}

/* 
 * name:    netSend()
 * brief:   send the data to net.
 * param:   buf - the data to be send
            lens - the length of the data
 * return:  the length the data that have been send
**/
int netSend(uint8_t *buf, int lens)
{
    send_buf_info_t   *sendbuf;
    
    sendbuf = BosMalloc(sizeof(send_buf_info_t));
    if (sendbuf == NULL)
        return - 1;
    
    memset(sendbuf, 0, sizeof(send_buf_info_t));
    
    sendbuf->len = BC95IoTHeaderAdd(buf, lens, &(sendbuf->buf));

    if (sendbuf->len == 0)
    {
        BosFree(sendbuf);
        return - 1;
    }
    
    list_add_tail(&sendbuf->link, &send_list_head);
    if (send_list_cnt >= MAX_SEND_LIST)
    {
        //溢出，删除最早的一个数据
        list_t       *entry;
        
        entry = send_list_head.next;
        
        sendbuf = list_entry(entry, send_buf_info_t, link);
        
        list_del(entry);
        BosFree(sendbuf);
    }
    else
        send_list_cnt ++;
    
    return lens;
}

/* 
 * name:    netRecv()
 * brief:   read the data that received from net.
 * param:   buf - the ptr of the data buf, the buf must be free by user!
 * return:  the length the received
**/
int netRecv(uint8_t **pbuf)
{
    recv_buf_info_t   *recv;
    list_t            *entry;
    int              lens = 0;
    
    if (recv_list_cnt == 0)
        return 0;
    
    if (pbuf == NULL)
        return -1;
    
    list_for_each(entry, &recv_list_head)
    {
        recv = list_entry(entry, recv_buf_info_t, link);
        
        if (recv->buf != NULL)
        {
            *pbuf = recv->buf;
            lens = recv->len;
        
            recv->buf = NULL;
            recv->len = 0;
            
            //the link ptr will be freed in netMngrTask
            BosFree(recv);
            
            recv_list_cnt --;
            
            break;
        }
    }
    
    return lens;      
}

/* */
void netDataIndicationCallBackSet(void (*p)(void *))
{
    netDataIndication = p;
}

/* 
 * name:    netConnect
 * brief:   check if the net is connected and if is not, created it.
 * param:   
 * return:  
**/
static void netConnect(void)
{
    BC95InitAT();
}

/* 
 * name:    netMngr
 * brief:   polling task after connected.
 * param:   
 * return:  
**/
static void netMngr(void)
{
    BC95PollingAT();
}

/* 
 * name:    atCmdStart()
 * brief:   start send a at cmd.
 * param:   at - at machine
            cmd - at cmd
            resp - response format
            timeout - timeout for response
 * return:  the status of the operate
             0 - cmd send out and received the right response
             1 - cmd send out and wait for response
             2 - waitting for response
            -1 - cmd send failed.
            -2 - timeout and no response
            
**/
int atCmdStart(at_machine_t *atm, char *cmd, const char *resp_format, uint32_t timeout)
{
    if (atm->status == AT_IDLE)
    {
        atm->cmd = BosMalloc(strlen(cmd)+1 + strlen(resp_format)+1 + 256);
        if (atm->cmd == NULL)
            return -1;
        
        memset(atm->cmd, 0, strlen(cmd) + strlen(resp_format) + 256);
        
        strcpy(atm->cmd, cmd);
        atm->resp_format = atm->cmd + strlen(cmd)+1;
        strcpy(atm->resp_format, resp_format);
        atm->format_cnt = 0;
        
        atm->resp = atm->resp_format + strlen(resp_format)+1;
        atm->param_offset = 0;
        atm->param_len = 0;
        
        halUartSend(NBIOT_UART, (uint8_t *)atm->cmd, strlen(atm->cmd));
        
        atm->status ++;
        atm->timeout = TickGet() + timeout;
      
        return 1;
    }
    else
    {
        if (atm->status == AT_RESP_OK)
        {
            atm->status = AT_IDLE;
            
            return 0;
        }
        else if ((TickGet() - atm->timeout) < (uint32_t)(-1)>>1)
        {
            //timeout
            atm->status = AT_IDLE;
            
            return -2;
        }
    }
    
    return 2;
}

int atCmdStop(at_machine_t *atm)
{
    return atMachineInit(atm);
}

static void netRxTask(void)
{
    uint8_t ch;
    
    while(1 == halUartRecv(NBIOT_UART, &ch, 1))
    {
        atcmd_resp_search(atm, ch);
        close_format_search(atm,ch);
        qird_format_search(atm, ch);
    }
}

/* search the at response */
static void atcmd_resp_search(at_machine_t *atm, char ch)
{
    if (atm->status != AT_WAIT_RESP)
        return;

RESP_FORMAT_SEARCH:
    if (atm->resp_format[atm->format_cnt] == '%')
        atm->format_cnt ++;
        
    if ((atm->format_cnt > 0) && (atm->resp_format[atm->format_cnt - 1] == '%'))
    {
        if (atm->resp_format[atm->format_cnt] == 's')
        {
            if (ch == atm->resp_format[atm->format_cnt+1])
            {
                atm->format_cnt += 2;
                atm->resp[atm->param_offset + atm->param_len] = '\0';
                atm->param_len ++;
                atm->param_offset +=atm->param_len;
                atm->param_len = 0;
            }
            else
            {
                atm->resp[atm->param_offset + atm->param_len] = ch;
                atm->param_len ++;
            }
        }
        else if (atm->resp_format[atm->format_cnt] == 'd')
        {
            if (ch == atm->resp_format[atm->format_cnt+1])
            {
                atm->format_cnt += 2;
                atm->param_offset += sizeof(int);
                atm->param_len = 0;
            }
            else if (ch < '0' || ch > '9')
            {
                atm->param_len = 0;
                atm->param_offset = 0;
                    
                if ( ((atm->format_cnt == 1) && (atm->resp_format[0] != '%')) ||
                     (atm->format_cnt > 1) )
                {
                    atm->format_cnt = 0;
                        
                    memset(atm->resp, 0, MAX_RESP_SIZE);
                    goto RESP_FORMAT_SEARCH;
                }
            }
            else
            {
                int d;
                
                memcpy(&d, atm->resp+atm->param_offset, sizeof(int));
                
                if (atm->param_len == 0)
                    d = 0;
                else
                    d *= 10;
                    
                d += (ch - '0');
                
                memcpy(atm->resp+atm->param_offset, &d, sizeof(int));
                    
                atm->param_len ++;          
           }
        }
        else if (atm->resp_format[atm->format_cnt] == 'x')
        {
            if (ch == atm->resp_format[atm->format_cnt+1])
            {
                atm->format_cnt += 2;
                atm->param_offset +=sizeof(int);
                atm->param_len = 0;
            }
            else if ( ((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'f')) || ((ch >= 'A') && (ch <= 'F')) )
            {
                if (atm->param_len == 0)
                    *(int *)atm->resp[atm->param_offset] = 0;
                else
                    *(int *)atm->resp[atm->param_offset] <<= 4;
                    
                if ((ch >= '0') && (ch <= '9'))
                    *(int *)atm->resp[atm->param_offset] += (ch - '0');
                else if ((ch >= 'a') && (ch <= 'f'))
                    *(int *)atm->resp[atm->param_offset] += (ch - 'a' + 10);
                else
                    *(int *)atm->resp[atm->param_offset] += (ch - 'A' + 10);
                    
                atm->param_len ++;  
            }
            else
            {
                atm->param_len = 0;
                atm->param_offset = 0;
                    
                if ( ((atm->format_cnt == 1) && (atm->resp_format[0] != '%')) ||
                     (atm->format_cnt > 1) )
                {
                    atm->format_cnt = 0;
                        
                    memset(atm->resp, 0, MAX_RESP_SIZE);
                    goto RESP_FORMAT_SEARCH;
                }
            }
        }
    }
    else if (atm->resp_format[atm->format_cnt] == ch)
        atm->format_cnt ++;
    else
    {
        atm->param_len = 0;
        atm->param_offset = 0;
            
        if ( ((atm->format_cnt == 1) && (atm->resp_format[0] != '%')) ||
             (atm->format_cnt > 1) )
        {
            atm->format_cnt = 0;
                
            memset(atm->resp, 0, MAX_RESP_SIZE);
            goto RESP_FORMAT_SEARCH;
        }
    }
    
    if (atm->format_cnt == strlen(atm->resp_format))
    {
        atm->status = AT_RESP_OK;
    }

    return;
}

/* search the close notice packet*/
static void close_format_search(at_machine_t *atm, char ch)
{
    
}

/* search the rx notice packet*/
static void qird_format_search(at_machine_t *atm, char ch)
{
    BC95RxIndicationSearch(atm, ch);
}

/* 串口接收回调函数 */
static void netUartRxIndicat(void * p)
{
    (void)p;
}
