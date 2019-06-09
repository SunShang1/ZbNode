/*
 * nvram.c
 * Description: nvram managerment
 */
#include "layout.h"
#include "board.h"
#include "nvram.h"
#include "BosMem.h"
#include "crc.h"
#include "smartNet.cfg"
#include "attribute.h"
#include "hal_rf.h"

static int nvramBlockCheck(int);
static int nvramBlockWrite(int index, void *buf, uint16_t size);
static int nvramBlockInit(int);
/*
 * Init NVRAM
 */
int nvramInit(void)
{
    if (nvramBlockCheck(NV_BLK_SWAP) == 0)
    {
        uint8_t         dstIndex;
        uint8_t         *ptr;
        
        ptr = BosMalloc(NV_BLK_SIZE);
        if (ptr == NULL)
            return -1;
        
        HalFlashRead(ptr, CFG_NVRAM_ADDR + NV_BLK_SWAP*NV_BLK_OFFSET, NV_BLK_SIZE);
        dstIndex = ptr[sizeof(uint32_t)];
        
        if ((dstIndex != NV_BLK_SWAP) && (dstIndex < NV_BLK_NUM))
        {
            uint8_t x;
            
            HAL_INT_LOCK(x);

            HalFlashErase(CFG_NVRAM_ADDR + dstIndex*NV_BLK_OFFSET);
            HalFlashWrite(CFG_NVRAM_ADDR + dstIndex*NV_BLK_OFFSET, ptr, NV_BLK_SIZE);

            HalFlashErase(CFG_NVRAM_ADDR + NV_BLK_SWAP*NV_BLK_OFFSET);
            
            HAL_INT_UNLOCK(x);
        }
        
        BosFree(ptr);        
    }
    
    if (nvramBlockCheck(NV_BLK_SYS) != 0)
    {
        nvramBlockInit(NV_BLK_SYS);
    }
    return 0;
}

int nvramFactoryReset(void)
{    
    uint8_t     *ptr;

    ptr = BosMalloc(NV_BLK_SIZE);
    if (ptr == NULL)
        return - 1;
    
    memset(ptr, 0, NV_BLK_SIZE); 
    
    //sys blk
    {
        saddr_t  addr;
    
        nvramSysBlkParamGet(devId, &addr);
        ((nvSysBlk_t *)ptr)->devId = addr;
        
        ((nvSysBlk_t *)ptr)->hwVer = 0x00;
        ((nvSysBlk_t *)ptr)->heartBeat = 180;
        
        nvramBlockWrite(NV_BLK_SYS, ptr, NV_BLK_SIZE);
    }
    
    memset(ptr, 0, NV_BLK_SIZE); 
    
    
    BosFree(ptr);

    return 0;
}

static int nvramBlockRead(int index, void *buf, uint16_t bufSize)
{
    if (bufSize < NV_BLK_SIZE)
        return -1;

    if ( (index == NV_BLK_SWAP) || (index >= NV_BLK_NUM) )
        return -1;

    HalFlashRead(buf, CFG_NVRAM_ADDR + index*NV_BLK_OFFSET, NV_BLK_SIZE);

    return NV_BLK_SIZE;
}

static int nvramBlockWrite(int index, void *buf, uint16_t size)
{
    uint8_t         *ptr = (uint8_t *)buf;
    uint8_t x;

    if (size != NV_BLK_SIZE)
        return -1;

    if ( (index == NV_BLK_SWAP) || (index >= NV_BLK_NUM) )
        return -1;

    /* 计算CRC */
    /* 这里虽然block 不一定是 sys blk，但每个 block 的头都是一样的 */
    ptr[sizeof(uint32_t)] = index; 
    ((nvSysBlk_t *)ptr)->dcrc = crc32(0, ptr + sizeof(uint32_t), NV_BLK_SIZE-sizeof(uint32_t));

    HAL_INT_LOCK(x);
    
    HalFlashWrite(CFG_NVRAM_ADDR+NV_BLK_SWAP*NV_BLK_OFFSET, buf, NV_BLK_SIZE);

    HalFlashErase(CFG_NVRAM_ADDR+index*NV_BLK_OFFSET);

    HalFlashWrite(CFG_NVRAM_ADDR+index*NV_BLK_OFFSET, buf, NV_BLK_SIZE);
    
    HalFlashErase(CFG_NVRAM_ADDR + NV_BLK_SWAP*NV_BLK_OFFSET);
    
    HAL_INT_UNLOCK(x);

    return 0;
}

static int nvramBlockCheck(int index)
{
    uint8_t *ptr;
    uint32_t crc;
    int err = 0;
    
    if (index >= NV_BLK_NUM)
        return -1;
    
    ptr = BosMalloc(NV_BLK_SIZE);
    if (ptr == NULL)
        return -1;
    
    HalFlashRead(ptr, CFG_NVRAM_ADDR + index*NV_BLK_OFFSET, NV_BLK_SIZE);

    crc = *(uint32_t *)ptr;
    
    if (crc != crc32(0, ptr+sizeof(crc), NV_BLK_SIZE - sizeof(crc)))
        err = -1;
    
    BosFree(ptr); 
    return err;
}

static int nvramBlockInit(int index)
{
    uint8_t     *ptr;

    if ( (index == NV_BLK_SWAP) || (index >= NV_BLK_NUM) )
        return -1;
    
    ptr = BosMalloc(NV_BLK_SIZE);
    if (ptr == NULL)
        return - 1;
    
    memset(ptr, 0, NV_BLK_SIZE); 
    
    if (index == NV_BLK_SYS)
    {
        
        ((nvSysBlk_t *)ptr)->hwVer = 0x00;
        ((nvSysBlk_t *)ptr)->heartBeat = 180;
    }
    
    nvramBlockWrite(index, ptr, NV_BLK_SIZE);
    
    BosFree(ptr);

    return 0;
}

int nvramBlkParamSetInit(int index, uint8_t **ptr)
{
    if ( (index == NV_BLK_SWAP) || (index >= NV_BLK_NUM) )
        return -1;

    *ptr = (uint8_t *)BosMalloc(NV_BLK_SIZE);
    
    if (*ptr == NULL)
        return -1;
    
    nvramBlockRead(index, *ptr, NV_BLK_SIZE);
    
    return 0;
}

int nvramBlkParamSetFinal(int index, uint8_t *ptr, uint8_t write)
{
    if (write)
        nvramBlockWrite(index, ptr, NV_BLK_SIZE);
    
    BosFree(ptr);
    
    return 0;
}
