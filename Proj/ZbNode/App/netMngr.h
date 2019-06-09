/*
 * Copyrite (C) 2018, Lamp
 * 
 * 文件名称: Netmngr.h
 * 文件说明：
 *
 *
 * 版本信息：
 * v0.1       wcc         2018/05/23
 *
 */
#ifndef __NETMNGR_H__
#define __NETMNGR_H__


#include "pub_def.h"

/********************************************
* Defines *
********************************************/
#define MAX_SEND_LIST       2

#define MAX_RECV_LIST       4


#define MAX_RXPKT_LEN       80


/********************************************
* Typedefs *
********************************************/
/*
 * define the at cmd
 */
typedef struct
{
    char *cmd;
    char *resp;
    uint32_t timeout;
    uint8_t retry;
}at_cmd_t;

/*
 * define the module status
 */
enum
{
    //关机
    STA_OFF     = 0x0,
    
    //开机，开始初始化
    STA_INIT,
    
    //连接成功
    STA_CONNECTED,
};

/*
 * define the AT command send step
 */
typedef enum
{
    AT_IDLE     = 0x0,
    AT_WAIT_RESP,
    AT_RESP_OK,
}at_status_t;

typedef struct
{
    char        *cmd;
    char        *resp_format;
    uint8_t      format_cnt;

#define MAX_RESP_SIZE   256       
    char        *resp;    
    uint16_t    param_offset;
    uint16_t    param_len;
      
    uint32_t     timeout;
    
    at_status_t  status;
}at_machine_t;

typedef struct
{
    list_t       link;
    
    uint16_t     len;
    uint8_t      *buf;
}send_buf_info_t;

typedef struct
{
    list_t       link;
    
    uint32_t     id;
    uint8_t      acked;
    
    uint16_t     len;
    uint8_t      *buf;
}recv_buf_info_t;


/********************************************
* Functions * 
********************************************/
void nb_power_on(void);

void nb_power_off(void);

int netMngrInit(void);

int netSend(uint8_t *buf, int lens);

int netRecv(uint8_t **pbuf);

void netDataIndicationCallBackSet(void (*p)(void *));

uint8_t isNetConnected(void);

uint8_t isAtMachineIdle(void);

int atCmdStart(at_machine_t * atm, char *cmd, const char *resp_format, uint32_t timeout);

int atCmdStop(at_machine_t *atm);

void BC95InitAT(void);

void BC95PollingAT(void);

void BC95RxIndicationSearch(at_machine_t *atm, char ch);

int BC95IoTHeaderAdd(uint8_t *in, int lens, uint8_t **out);

#endif