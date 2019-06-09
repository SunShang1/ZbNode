/******************************************************************************
    Filename: flash.c

    This file defines flash related functions for the CC253x family
    of RF system-on-chips from Texas Instruments.

******************************************************************************/

#include "hal_dma.h"
#include "hal_flash.h"
#include "hal_mcu.h"

// Include chip specific IO definition file
#if (chip == 2530 || chip == 2531)
#include <ioCC2530.h>
#endif

/**************************************************************************************************
 * @fn          HalFlashRead
 *
 * @brief       This function reads 'cnt' bytes from the internal flash.
 *
 * input parameters
 *
 * @param       dst - dest addr in ram.
 * @param       src - src addr in flash.
 * @param       size - A valid number of bytes to read.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void HalFlashRead(void *dst, unsigned long src, uint16_t size)
{
  uint8_t hwlock;
  
  uint8_t *pDstAddr = (uint8_t *)dst;
  uint8_t *pData;
  uint8_t pg;
  uint16_t offset;
  uint8_t bank;
  uint8_t memctr = MEMCTR;  // Save to restore.
  
  pg = src / HAL_FLASH_PAGE_SIZE;
  offset = src % HAL_FLASH_PAGE_SIZE;
  
  pData = (uint8_t *)(offset + HAL_FLASH_PAGE_MAP) +
          ((pg % HAL_FLASH_PAGE_PER_BANK)  * HAL_FLASH_PAGE_SIZE);
  
  bank = pg / HAL_FLASH_PAGE_PER_BANK;
  
  EnterCritical(hwlock);

  // Calculate and map the containing flash bank into XDATA.
  MEMCTR = (MEMCTR & 0xF8) | bank;

  while (size--)
  {
    *pDstAddr++ = *pData++;
  }

  MEMCTR = memctr;

  ExitCritical(hwlock);
}

/**************************************************************************************************
 * @fn          HalFlashWrite
 *
 * @brief       This function writes 'cnt' bytes to the internal flash.
 *
 * input parameters
 *
 * @param       dst - Valid HAL flash write address: actual addr / 4 and quad-aligned.
 * @param       src - Valid buffer space at least as big as 'cnt' X 4.
 * @param       cnt - Number of 4-byte blocks to write.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************/
void HalFlashWrite(unsigned long dst, void *src, uint16_t size)
{
    DMA_DESC dmaConfig0;
    uint16_t addr;
    
    /* Configure DMA channel 0:
     * SRCADDR:   address of the data to be written to flash (increasing).
     * DESTADDR:  the flash controller data register (fixed), so that the
     *            flash controller will write this data to flash.
     * VLEN:      use LEN for transfer count.
     * LEN:       equal to the number of bytes to be transferred.
     * WORDSIZE:  each transfer should transfer one byte.
     * TMODE:     should be set to single mode (see datasheet, DMA Flash Write).
     *            Each flash write complete will re-trigger the DMA channel.
     * TRIG:      let the DMA channel be triggered by flash data write complete
     *            (trigger number 18). That is, the flash controller will trigger
     *            the DMA channel when the Flash Write Data register, FWDATA, is
     *            ready to receive new data.
     * SRCINC:    increment by one byte.
     * DESTINC:   fixed (always write to FWDATA).
     * IRQMASK:   disable interrupts from this channel.
     * M8:        0, irrelevant since we use LEN for transfer count.
     * PRIORITY:  high.
     */
    dmaConfig0.SRCADDRH  = ((uint16_t)src >> 8) & 0x00FF;
    dmaConfig0.SRCADDRL  = (uint16_t)src & 0x00FF;
    dmaConfig0.DESTADDRH = ((uint16_t)&FWDATA >> 8) & 0x00FF;
    dmaConfig0.DESTADDRL = (uint16_t)&FWDATA & 0x00FF;
    dmaConfig0.VLEN      = DMA_VLEN_USE_LEN;
    dmaConfig0.LENH      = (size >> 8) & 0x00FF;
    dmaConfig0.LENL      = size & 0x00FF;
    dmaConfig0.WORDSIZE  = DMA_WORDSIZE_BYTE;
    dmaConfig0.TMODE     = DMA_TMODE_SINGLE;
    dmaConfig0.TRIG      = DMA_TRIG_FLASH;
    dmaConfig0.SRCINC    = DMA_SRCINC_1;
    dmaConfig0.DESTINC   = DMA_DESTINC_0;
    dmaConfig0.IRQMASK   = DMA_IRQMASK_ENABLE;
    dmaConfig0.M8        = DMA_M8_USE_8_BITS;
    dmaConfig0.PRIORITY  = DMA_PRI_HIGH;
    
    /* The DMA configuration data structure may reside at any location in
     * unified memory space, and the address location is passed to the DMA
     * through DMA0CFGH:DMA0CFGL.
     */
    DMA0CFGH = ((uint16_t)&dmaConfig0 >> 8) & 0x00FF;
    DMA0CFGL = (uint16_t)&dmaConfig0 & 0x00FF;
  
    // Waiting for the flash controller to be ready. 
    while (FCTL & 0x80);
    
    addr = (uint16_t)(dst >> 2);    // You address 32-bit words through the flash controller.
    
    FADDRH = (addr >> 8) & 0x00FF;
    FADDRL = addr & 0x00FF;
  
    // Arm the DMA channel, takes 9 system clock cycles.
    DMAARM |= 0x01;
    NOP();NOP();NOP();NOP();NOP();NOP();NOP();NOP();NOP(); // 9 NOPs
  
    // Enable flash write. Generates a DMA trigger.
    FCTL |= 0x02;
  
    // Wait for DMA transfer to complete.
    while (!(DMAIRQ & 0x01));
  
    // Wait until flash controller not busy.
    while (FCTL & (0x80 | 0x40));
  
    /* By now, the transfer is completed, so the transfer count is reached.
     * The DMA channel 0 interrupt flag is then set, so we clear it here.
     */
    DMAIRQ = ~0x01;      // Clear interrupt flag by R/W0, see datasheet.
}

/**************************************************************************************************
 * @fn          HalFlashErase
 *
 * @brief       This function erases the specified page of the internal flash.
 *
 * input parameters
 *
 * @param       addr - any address of one page.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void HalFlashErase(unsigned long addr)
{
  FADDRH = addr / HAL_FLASH_WORD_SIZE / 256;
  FCTL |= 0x01;
}
