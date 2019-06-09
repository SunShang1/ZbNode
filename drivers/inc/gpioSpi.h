/*
 * 文件名称：gpioSpi.h
 *
 * 文件说明：gpio 模拟 SPI 驱动头文件
 *
 */

#ifndef __GPIO_SPI_H__
#define __GPIO_SPI_H__

#include "pub_def.h"
#include "error.h"
/********************************************
* Defines *
********************************************/ 


/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/


/********************************************
* Function *
********************************************/
error_t halSpiInit(void);

uint8_t halSpiByteRead(uint8_t reg);

void halSpiByteWrite(uint8_t reg, uint8_t val);

uint8_t *halSpiBufRead(uint8_t reg, uint8_t *buf, uint8_t size);

void halSpiBufWrite(uint8_t reg, uint8_t *buf, uint8_t size);
    
#endif    