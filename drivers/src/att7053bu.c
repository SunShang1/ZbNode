/*
 * 文件名称：att7053bu.c
 *
 * 文件说明：功率计模组驱动
 *
 */

#include "pub_def.h"
#include "error.h"
#include "gpioSpi.h"
#include "att7053bu.h"
/********************************************
* Defines *
********************************************/ 
#define KPQS            0.01083642



/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/


/********************************************
* Function *
********************************************/
static uint32_t att7053buRegRead(uint8_t reg);
static void att7053buRegWrite(uint8_t reg, uint32_t val);

error_t att7053buInit(void)
{
    uint32_t chipid;
    
    halSpiInit();
    
    chipid = att7053buRegRead(ATT_CHIPID);

    if (chipid != 0x7053b0)
        return - BOS_ERROR;        
        
    //初始化ATT7053BU芯片，开启采集数据
    att7053buRegWrite(WPREG, 0xBC);
    att7053buRegWrite(EMUCFG, 0x2004);
    att7053buRegWrite(FREQCFG, 0x0088);
    att7053buRegWrite(ANAEN, 0x05);
        
    att7053buRegWrite(WPREG, 0x99);
        
    return BOS_EOK;
}

uint32_t att7053buPowerGet(void)
{
    uint32_t u32tmp[4] = {0};
    
    float   powerP2;
    uint32_t reedPV;
    
    u32tmp[0] = att7053buRegRead(RMSI2);
    u32tmp[1] = att7053buRegRead(RMSU);
    u32tmp[2] = att7053buRegRead(FREQU);
    u32tmp[3] = att7053buRegRead(POWERP2);

    if(u32tmp[3] < (1UL<<23))
        powerP2 = u32tmp[3];
    else
        powerP2 = ((int32_t)u32tmp[3]-(int32_t)(1UL<<24));
    
    if(powerP2 < 0)
        powerP2 = 0 - powerP2;
    
    powerP2 *= KPQS;
    
    powerP2 += 0.5;  //4四舍五入
    
    reedPV = (uint32_t)powerP2;
    
    return reedPV;
}

static uint32_t att7053buRegRead(uint8_t reg)
{
    uint8_t buf[3];
    uint32_t val = 0;
    
    halSpiBufRead(reg&0x7F, buf, sizeof(buf));
    
    val = buf[0];
    val <<= 8;
    val += buf[1];
    val <<= 8;
    val += buf[2];
    
    return val;
}

static void att7053buRegWrite(uint8_t reg, uint32_t val)
{
    uint8_t buf[3];
    
    buf[0] = (uint8_t)(val>>16);
    buf[1] = (uint8_t)(val>>8);
    buf[2] = (uint8_t)(val);
    
    halSpiBufWrite(reg|0x80, buf, sizeof(buf));
    
    return;
}
        