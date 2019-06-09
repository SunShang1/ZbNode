/*
 * 文件名称：gpioSpi.c
 *
 * 文件说明：gpio 模拟 SPI 驱动
 *
 */

#include "pub_def.h"
#include "board.h"
#include "error.h"
/********************************************
* Defines *
********************************************/
#define SPI_OUT_REVERSE


#ifdef SPI_OUT_REVERSE

#define halSpiAssertCSN()   MCU_IO_OUTPUT(0, 4, 1)
#define halSpiDeAssertCSN() MCU_IO_OUTPUT(0, 4, 0)

#define halSpiHighSCK()     MCU_IO_OUTPUT(0, 5, 0)
#define halSpiLowSCK()      MCU_IO_OUTPUT(0, 5, 1)

#define halSpiHighMOSI()    MCU_IO_OUTPUT(0, 3, 0)
#define halSpiLowMOSI()     MCU_IO_OUTPUT(0, 3, 1)

#define halSpiGetMISO()     MCU_IO_GET(0, 2)

#endif

#define UDELAY(n)           for(volatile int delay = n; delay > 0; delay --)
/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/


/********************************************
* Function *
********************************************/
error_t halSpiInit(void)
{
#ifdef SPI_OUT_REVERSE
    //sck
    MCU_IO_OUTPUT(0, 5, 1);
    
    //miso
    MCU_IO_INPUT(0, 2, MCU_IO_PULLUP);
    
    //mosi
    MCU_IO_OUTPUT(0, 3, 1);
    
    //csn
    MCU_IO_OUTPUT(0, 4, 0);
#endif
    
    return BOS_EOK;
}

static uint8_t halSpiTranceive(uint8_t u8tmp)
{
    uint8_t u8data = 0;
    uint8_t index;     
    
    for (index = 8; index >0; index --)
    {
        u8data <<= 1;
        
        UDELAY(4);
        if (u8tmp & 0x80)
            halSpiHighMOSI();
        else
            halSpiLowMOSI();
        
        UDELAY(4);
        
         halSpiHighSCK();
         
         UDELAY(4);
        
        if (halSpiGetMISO())
            u8data ++;
        
        u8tmp <<= 1;
        
        UDELAY(4);
        
        halSpiLowSCK();
    }
    
    UDELAY(4);
    
    return u8data;
}

uint8_t halSpiByteRead(uint8_t reg)
{
    uint8_t     u8tmp;
    
    halSpiAssertCSN();
	
	u8tmp = halSpiTranceive(reg);
	u8tmp = halSpiTranceive(0x00);				

	halSpiDeAssertCSN();

	return u8tmp;
}

void halSpiByteWrite(uint8_t reg, uint8_t val)
{
    halSpiAssertCSN();
	
	halSpiTranceive(reg);

    halSpiTranceive(val);
  
	halSpiDeAssertCSN();
    
    return;
}

uint8_t *halSpiBufRead(uint8_t reg, uint8_t *buf, uint8_t size)
{
    halSpiAssertCSN();
	
	halSpiTranceive(reg);
    
    for (int i = 0; i < size; i ++)
    {
        buf[i] = halSpiTranceive(0x00);
    }

	halSpiDeAssertCSN();
    
    
    return buf;
}

void halSpiBufWrite(uint8_t reg, uint8_t *buf, uint8_t size)
{
    halSpiAssertCSN();
	
	halSpiTranceive(reg);

    for (int i = 0; i < size; i ++)
    {
        halSpiTranceive(buf[i]);
    }
  
	halSpiDeAssertCSN();
    
    return;
}
        