/**
  ******************************************************************************
  * @file    layout.h 
  * @author  George
  * @date    
  * @brief   system layout config file
  ******************************************************************************
***/
#ifndef __LAYOUT_H__
#define __LAYOUT_H__


/* flash layout */

#define CFG_FLASH_SIZE          0x40000         //256KB

#define CFG_BOOT_SIZE           0x2000          //8KB

#define CFG_NVRAM_SIZE          0x8000          //32KB

#define CFG_IMG_SIZE \
  ((CFG_FLASH_SIZE - CFG_BOOT_SIZE - CFG_NVRAM_SIZE)>>1)        //108KB
    
   
#define CFG_FLASH_ADDR          0x00

#define CFG_BOOT_ADDR           CFG_FLASH_ADDR
#define CFG_IMG_ADDR            (CFG_BOOT_ADDR + CFG_BOOT_SIZE)     //0x2000
#define CFG_IMG2_ADDR           (CFG_IMG_ADDR + CFG_IMG_SIZE)       //0x1D000
#define CFG_NVRAM_ADDR          (CFG_IMG2_ADDR + CFG_IMG_SIZE)      //0x38000


#endif
