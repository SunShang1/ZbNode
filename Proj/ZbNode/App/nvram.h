/*
 * nvram.h
 */

#ifndef __NVRAM_H__
#define __NVRAM_H__

#include <stdio.h>
#include <string.h>
#include "pub_def.h"
#include "saddr.h"
#include "layout.h"

/********************************************
* Defines *
********************************************/
#define NV_BLK_SIZE             0x100
#define NV_BLK_OFFSET           HAL_FLASH_PAGE_SIZE


enum
{
    NV_BLK_SWAP                     = 0,    /* 数据备份空间 */
    NV_BLK_SYS                      = 1,    /* 系统参数 */

    
    NV_BLK_NUM                      = 2,
};


/********************************************
* Typedefs *
********************************************/
typedef struct
{
    uint32_t    dcrc;
    uint8_t     index;
    
/******************************** data *******************/
    uint8_t     hwVer;    
    uint16_t    heartBeat;
    
    saddr_t     devId;
    
    uint8_t     RadioChannel;
    uint8_t     RadioPower;
  
    

}nvSysBlk_t;



/********************************************
* Function *
********************************************/
/* Init nvram */
int nvramInit(void);
int nvramFactoryReset(void);
int nvramBlkParamSetInit(int index, uint8_t **ptr);
int nvramBlkParamSetFinal(int index, uint8_t *ptr, uint8_t write);

#define nvramSysBlkParamGet(name, pVal)    \
do  \
{   \
    HalFlashRead((void *)(pVal), CFG_NVRAM_ADDR + NV_BLK_SYS*NV_BLK_OFFSET + (unsigned long)(&(((nvSysBlk_t *)0)->name)), sizeof(((nvSysBlk_t *)0)->name));  \
}while(0)

#define nvramSysBlkParamSet(name, buf, pVal)     \
do  \
{   \
    memcpy(&(((nvSysBlk_t *)buf)->name), (uint8_t *)(pVal), sizeof(((nvSysBlk_t *)0)->name));   \
}while(0)


#endif


