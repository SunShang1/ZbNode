/***********************************************************************************

  Filename:     hal_rf.h

  Description:  HAL radio interface header file

***********************************************************************************/


#ifndef HAL_RF_H
#define HAL_RF_H

/***********************************************************************************
* INCLUDES
*/
#include "pub_def.h"


/***********************************************************************************
* TYPEDEFS
*/

/***********************************************************************************
* CONSTANTS AND DEFINES
*/

// Chip ID's
#define HAL_RF_CHIP_ID_CC1100               0x00
#define HAL_RF_CHIP_ID_CC1110               0x01
#define HAL_RF_CHIP_ID_CC1111               0x11
#define HAL_RF_CHIP_ID_CC2420               0x02
#define HAL_RF_CHIP_ID_CC2500               0x80
#define HAL_RF_CHIP_ID_CC2510               0x81
#define HAL_RF_CHIP_ID_CC2511               0x91
#define HAL_RF_CHIP_ID_CC2550               0x82
#define HAL_RF_CHIP_ID_CC2520               0x84
#define HAL_RF_CHIP_ID_CC2430               0x85
#define HAL_RF_CHIP_ID_CC2431               0x89
#define HAL_RF_CHIP_ID_CC2530               0xA5
#define HAL_RF_CHIP_ID_CC2531               0xB5
#define HAL_RF_CHIP_ID_CC2540               0x8D

// CC2590/91 gain modes
#define HAL_RF_GAIN_LOW                     0
#define HAL_RF_GAIN_HIGH                    1

// TX power lookup index
#if INCLUDE_PA==2592
    #define HAL_RF_TXPOWER_0_DBM                0
    #define HAL_RF_TXPOWER_13_DBM               1
    #define HAL_RF_TXPOWER_16_DBM               2
    #define HAL_RF_TXPOWER_18_DBM               3
    #define HAL_RF_TXPOWER_20_DBM               4
#else
    #define HAL_RF_TXPOWER_MIN_3_DBM            0
    #define HAL_RF_TXPOWER_0_DBM                1
    #define HAL_RF_TXPOWER_4_DBM	            2
#endif

// IEEE 802.15.4 defined constants (2.4 GHz logical channels)
#define MIN_CHANNEL 				        11    // 2405 MHz
#define MAX_CHANNEL                         26    // 2480 MHz
#define CHANNEL_SPACING                     5     // MHz

/* FLASH */
#define HAL_INFOP_IEEE_OSET                 0x0C

/***********************************************************************************
* GLOBAL FUNCTIONS
*/

// Selected strobes
#define ISRXON()                st(RFST = 0xE3;)
#define ISTXON()                st(RFST = 0xE9;)
#define ISTXONCCA()             st(RFST = 0xEA;)
#define ISRFOFF()               st(RFST = 0xEF;)
#define ISFLUSHRX()             st(RFST = 0xED;)
#define ISFLUSHTX()             st(RFST = 0xEE;)
#define ISSAMPLECCA()           st(RFST = 0xEB;)

/* ------------------------------------------------------------------------------------------------
 *                                      Target Specific Defines
 * ------------------------------------------------------------------------------------------------
 */
/* IP0, IP1 */
#define IPX_0                 BV(0)
#define IPX_1                 BV(1)
#define IPX_2                 BV(2)
#define IP_RFERR_RF_DMA_BV    IPX_0
#define IP_RXTX0_T2_BV        IPX_2

/* IEN0 */
#define RFERRIE                       BV(0)

/* IEN2 */
#define RFIE                          BV(0)

/* MDMCTRL1 */
#define CORR_THR_SFD                  BV(5)
#define CORR_THR                      0x14

/* CCACTRL0 */
#define CCA_THR                       0xFC   /* -4-76=-80dBm when CC2530 operated alone or with CC2591 in LGM */
#define CCA_THR_HGM                   0x06   /* 6-76=-70dBm when CC2530 operated with CC2591 in HGM */

/* ADCCON1 */
#define RCTRL1                        BV(3)
#define RCTRL0                        BV(2)
#define RCTRL_BITS                    (RCTRL1 | RCTRL0)
#define RCTRL_CLOCK_LFSR              RCTRL0


/* TXFILTCFG */
#define TXFILTCFG                     XREG( 0x61FA )
#define TXFILTCFG_RESET_VALUE         0x09

// Generic RF interface
uint8_t halRfInit(void);
uint8_t halRfSetTxPower(uint8_t power);
uint8_t halRfTransmit(void);
void  halRfSetGain(uint8_t gainMode);     // With CC2590/91 only

uint8_t halRfGetChipId(void);
uint8_t halRfGetChipVer(void);
uint8_t *halRgGetMACAddress(uint8_t *mac);
uint8_t halRfGetRandomByte(void);
uint16_t halRfGetRandomWord(void);
uint8_t halRfGetRssiOffset(void);

void  halRfWriteTxBuf(uint8_t* pData, uint8_t length);
void  halRfReadRxBuf(uint8_t* pData, uint8_t length);
void  halRfWaitTransceiverReady(void);
uint8_t halRfReadMemory(uint16_t addr, uint8_t* pData, uint8_t length);
uint8_t halRfWriteMemory(uint16_t addr, uint8_t* pData, uint8_t length);

void  halRfReceiveOn(void);
void  halRfReceiveOff(void);
void  halRfDisableRxInterrupt(void);
void  halRfEnableRxInterrupt(void);
void  halRfRxInterruptConfig(ISR_FUNC_PTR pfISR);



// IEEE 802.15.4 specific interface
void  halRfSetChannel(uint8_t channel);
void  halRfSetShortAddr(uint16_t shortAddr);
void  halRfSetPanId(uint16_t PanId);


#endif

/***********************************************************************************
  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
***********************************************************************************/
