/***********************************************************************************

  Filename:       hal_rf.c

  Description:    CC2530 radio interface.

***********************************************************************************/

/***********************************************************************************
* INCLUDES
*/
#include "board.h"
#include "hal_mcu.h"
#include "hal_rf.h"


/***********************************************************************************
* CONSTANTS AND DEFINES
*/

// Chip revision
#define REV_A                      0x01
#define CHIPREVISION              REV_A

// CC2530 RSSI Offset
#define RSSI_OFFSET                               73
#define RSSI_OFFSET_LNA_HIGHGAIN                  79
#define RSSI_OFFSET_LNA_LOWGAIN                   67

// Various radio settings
#define AUTO_ACK                   0x20
#define AUTO_CRC                   0x40

// TXPOWER values
#if CC2530_PG1
#define CC2530_TXPOWER_MIN_3_DBM   0x88 
#define CC2530_TXPOWER_0_DBM       0x32
#define CC2530_TXPOWER_4_DBM       0xF7
#else
#define CC2530_TXPOWER_MIN_3_DBM   0xB5 
#define CC2530_TXPOWER_0_DBM       0xD5
#define CC2530_TXPOWER_4_DBM       0xF5
#endif

// RF interrupt flags
#define IRQ_TXDONE                 0x02
#define IRQ_RXPKTDONE              0x40
#define RFERR_RXOVERF              BV(2)

// CC2590-CC2591 support
#if INCLUDE_PA==2592

// Support for PA/LNA
#define HAL_PA_LNA_INIT()

// Select CC2591 RX high gain mode 
#define HAL_PA_LNA_RX_HGM() st( uint8_t i; P0_7 = 1; for (i=0; i<8; i++) asm("NOP"); )

// Select CC2591 RX low gain mode
#define HAL_PA_LNA_RX_LGM() st( uint8_t i; P0_7 = 0; for (i=0; i<8; i++) asm("NOP"); )

// TX power values
#define CC2530_92_TXPOWER_0_DBM       0x25
#define CC2530_92_TXPOWER_13_DBM      0x85
#define CC2530_92_TXPOWER_16_DBM      0xA5
#define CC2530_92_TXPOWER_18_DBM      0xC5
#define CC2530_92_TXPOWER_20_DBM      0xE5

#else // dummy macros when not using CC2591

#define HAL_PA_LNA_INIT()
#define HAL_PA_LNA_RX_LGM()
#define HAL_PA_LNA_RX_HGM()


#endif

/***********************************************************************************
* GLOBAL DATA
*/

/***********************************************************************************
* LOCAL DATA
*/
#ifndef MRFI
static ISR_FUNC_PTR pfISR= NULL;
#endif
static uint8_t rssiOffset = RSSI_OFFSET;

/***********************************************************************************
* LOCAL FUNCTIONS
*/
static void halPaLnaInit(void);

/***********************************************************************************
* GLOBAL FUNCTIONS
*/

/***********************************************************************************
* @fn      halRfInit
*
* @brief   Power up, sets default tuning settings, enables autoack, enables random
*          generator.
*
* @param   none
*
* @return  SUCCESS always (for interface compatibility)
*/
uint8_t halRfInit(void)
{
    uint16_t rndSeed;
    uint8_t  i;
    rndSeed = 0;
    
    /*
     * This CORR_THR value should be changed to 0x14 before attempting RX. Testing has shown that
     * too many false frames are received if the reset value is used. Make it more likely to detect
     * sync by removing the requirement that both symbols in the SFD must have a correlation value
     * above the correlation threshold, and make sync word detection less likely by raising the
     * correlation threshold.
     */
    MDMCTRL1 = CORR_THR;
    
#ifdef FEATURE_CC253X_LOW_POWER_RX
    /*
     * Reduce RX power consumption current to 20mA at the cost of some sensitivity
     * Note: This feature can be applied to CC2530 and CC2533 only.
     */
    RXCTRL = 0x00;
    FSCTRL = 0x50;
#else
    /* tuning adjustments for optimal radio performance; details available in datasheet */
    RXCTRL = 0x3F;
    
    /* Adjust current in synthesizer; details available in datasheet. */
    FSCTRL = 0x55;
#endif
    
    CCACTRL0 = CCA_THR;
     
    /*
     * Makes sync word detection less likely by requiring two zero symbols before the sync word.
     * details available in datasheet.
     */
    MDMCTRL0 = 0x85;
    
    /* Adjust current in VCO; details available in datasheet. */
#ifdef FEATURE_VCO_ALTERNATE_SETTING
    FSCAL1 = 0x80;
#else
    FSCAL1 = 0x00;
#endif
    
    /* Adjust target value for AGC control loop; details available in datasheet. */
    AGCCTRL1 = 0x15;
    
    /* Disable source address matching an autopend for now */
    SRCMATCH = 0;

    /* Tune ADC performance, details available in datasheet. */
    ADCTEST0 = 0x10;
    ADCTEST1 = 0x0E;
    ADCTEST2 = 0x03;
    
    /*
     * Sets TX anti-aliasing filter to appropriate bandwidth.
     * Reduces spurious emissions close to signal.
     */
    TXFILTCFG = TXFILTCFG_RESET_VALUE;

    /* disable the CSPT register compare function */
    CSPT = 0xFF;
  
    /* enable general RF interrupts */
    IEN2 |= RFIE;
    
    /* enable general REERR interrupts */
    IEN0 |= RFERRIE;
    
    /* set RF interrupts one notch above lowest priority (four levels available) */
    IP0 |=  IP_RFERR_RF_DMA_BV;
    IP1 &= ~IP_RFERR_RF_DMA_BV;

    // Enable CC2591 with High Gain Mode
    halPaLnaInit();

    /*----------------------------------------------------------------------------------------------
     *  Initialize random seed value.
     */    
    halRfReceiveOn();
    
    /*
     *  Wait for radio to reach infinite reception state by checking RSSI valid flag.
     *  Once it does, the least significant bit of ADTSTH should be pretty random.
     */
    while (!(RSSISTAT & 0x01));
  
    // Enable random generator
    for(i=0; i<16; i++)
    {
        /* use most random bit of analog to digital receive conversion to populate the random seed */
        rndSeed = (rndSeed << 1) | (RFRND & 0x01);
    }

    /*
     *  The seed value must not be zero or 0x0380 (0x8003 in the polynomial).  If it is, the psuedo
     *  random sequence won’t be random.  There is an extremely small chance this seed could randomly
     *  be zero or 0x0380.  The following check makes sure this does not happen.
     */
    if (rndSeed == 0x0000 || rndSeed == 0x0380)
    {
        rndSeed = 0xBABE; /* completely arbitrary "random" value */
    }

    /*
     *  Two writes to RNDL will set the random seed.  A write to RNDL copies current contents
     *  of RNDL to RNDH before writing new the value to RNDL.
     */
    RNDL = rndSeed & 0xFF;
    RNDH = rndSeed >> 8;

    halRfReceiveOff();
    
    //Disable filter
    FRMFILT0 &= ~(1<<0);
    FRMFILT1 = 0xF8;
    // Enable auto crc
    FRMCTRL0 |= AUTO_CRC;
    
    // Enable RX interrupt
    halRfEnableRxInterrupt();

    return SUCCESS;
}



/***********************************************************************************
* @fn      halRfGetChipId
*
* @brief   Get chip id
*
* @param   none
*
* @return  uint8_t - result
*/
uint8_t halRfGetChipId(void)
{
    return CHIPID;
}


/***********************************************************************************
* @fn      halRfGetChipVer
*
* @brief   Get chip version
*
* @param   none
*
* @return  uint8_t - result
*/
uint8_t halRfGetChipVer(void)
{
    // return major revision (4 upper bits)
    return CHVER>>4;
}

/***********************************************************************************
* @fn      halRgGetMACAddress
*
* @brief   Get Rf MAC address
*
* @param   mac buf
*
* @return  mac buf
*/
uint8_t *halRgGetMACAddress(uint8_t *mac)
{
    uint8_t i;
    uint8_t *ptr = (uint8_t *)(P_INFOPAGE + HAL_INFOP_IEEE_OSET);
    
    for (i = 0; i < 8; i ++)
    {
        mac[i] = ptr[7-i];
    }
    
    return mac;
}

/***********************************************************************************
* @fn      halRfGetRandomByte
*
* @brief   Return random byte
*
* @param   none
*
* @return  uint8_t - random byte
*/
uint8_t halRfGetRandomByte(void)
{
    /* clock the random generator to get a new random value */
    ADCCON1 = (ADCCON1 & ~RCTRL_BITS) | RCTRL_CLOCK_LFSR;

    /* return new randomized value from hardware */
    return(RNDH);
}

uint16_t halRfGetRandomWord(void)
{
    uint16_t random_word;

    /* clock the random generator to get a new random value */
    ADCCON1 = (ADCCON1 & ~RCTRL_BITS) | RCTRL_CLOCK_LFSR;

    /* read random word */
    random_word  = (RNDH << 8);
    random_word +=  RNDL;

    /* return new randomized value from hardware */
    return(random_word);
}


/***********************************************************************************
* @fn      halRfGetRssiOffset
*
* @brief   Return RSSI Offset
*
* @param   none
*
* @return  uint8_t - RSSI offset
*/
uint8_t halRfGetRssiOffset(void)
{
    return rssiOffset;
}

/***********************************************************************************
* @fn      halRfSetChannel
*
* @brief   Set RF channel in the 2.4GHz band. The Channel must be in the range 11-26,
*          11= 2005 MHz, channel spacing 5 MHz.
*
* @param   channel - logical channel number
*
* @return  none
*/
void halRfSetChannel(uint8_t channel)
{
    FREQCTRL = (MIN_CHANNEL + (channel - MIN_CHANNEL) * CHANNEL_SPACING);
}


/***********************************************************************************
* @fn      halRfSetShortAddr
*
* @brief   Write short address to chip
*
* @param   none
*
* @return  none
*/
void halRfSetShortAddr(uint16_t shortAddr)
{
    SHORT_ADDR0 = LO_UINT16(shortAddr);
    SHORT_ADDR1 = HI_UINT16(shortAddr);
}


/***********************************************************************************
* @fn      halRfSetPanId
*
* @brief   Write PAN Id to chip
*
* @param   none
*
* @return  none
*/
void halRfSetPanId(uint16_t panId)
{
    PAN_ID0 = LO_UINT16(panId);
    PAN_ID1 = HI_UINT16(panId);
}


/***********************************************************************************
* @fn      halRfSetPower
*
* @brief   Set TX output power
*
* @param   uint8_t power - power level: TXPOWER_MIN_4_DBM, TXPOWER_0_DBM,
*                        TXPOWER_4_DBM
*
* @return  uint8_t - SUCCESS or FAILED
*/
uint8_t halRfSetTxPower(uint8_t power)
{
    uint8_t n;

    switch(power)
    {
#if INCLUDE_PA==2592
    case HAL_RF_TXPOWER_0_DBM: n = CC2530_92_TXPOWER_0_DBM; break;
    case HAL_RF_TXPOWER_13_DBM: n = CC2530_92_TXPOWER_13_DBM; break;
    case HAL_RF_TXPOWER_16_DBM: n = CC2530_92_TXPOWER_16_DBM; break;
    case HAL_RF_TXPOWER_18_DBM: n = CC2530_92_TXPOWER_18_DBM; break;
    case HAL_RF_TXPOWER_20_DBM: n = CC2530_92_TXPOWER_20_DBM; break;
#else
    case HAL_RF_TXPOWER_MIN_3_DBM: n = CC2530_TXPOWER_MIN_3_DBM; break;
    case HAL_RF_TXPOWER_0_DBM: n = CC2530_TXPOWER_0_DBM; break;
    case HAL_RF_TXPOWER_4_DBM: n = CC2530_TXPOWER_4_DBM; break;
#endif
    default:
        return FAILED;
    }

    // Set TX power
    TXPOWER = n;

    return SUCCESS;
}


/***********************************************************************************
* @fn      halRfSetGain
*
* @brief   Set gain mode - only applicable for units with CC2590/91.
*
* @param   uint8_t - gain mode
*
* @return  none
*/
void halRfSetGain(uint8_t gainMode)
{
    if (gainMode==HAL_RF_GAIN_LOW) {
        HAL_PA_LNA_RX_LGM();
        rssiOffset = RSSI_OFFSET_LNA_LOWGAIN;
    } else {
        HAL_PA_LNA_RX_HGM();
        rssiOffset = RSSI_OFFSET_LNA_HIGHGAIN;
    }
}

/***********************************************************************************
* @fn      halRfWriteTxBuf
*
* @brief   Write to TX buffer
*
* @param   uint8_t* pData - buffer to write
*          uint8_t length - number of bytes
*
* @return  none
*/
void halRfWriteTxBuf(uint8_t* pData, uint8_t length)
{
    uint8_t i;

    ISFLUSHTX();          // Making sure that the TX FIFO is empty.

    RFIRQF1 = ~IRQ_TXDONE;   // Clear TX done interrupt

    // Insert data
    RFD = length + 2;
    for(i=0;i<length;i++){
        RFD = pData[i];
    }
}


/***********************************************************************************
* @fn      halRfAppendTxBuf
*
* @brief   Write to TX buffer
*
* @param   uint8_t* pData - buffer to write
*          uint8_t length - number of bytes
*
* @return  none
*/
void halRfAppendTxBuf(uint8_t* pData, uint8_t length)
{
    uint8_t i;

    // Insert data
    for(i=0;i<length;i++){
        RFD = pData[i];
    }
}


/***********************************************************************************
* @fn      halRfReadRxBuf
*
* @brief   Read RX buffer
*
* @param   uint8_t* pData - data buffer. This must be allocated by caller.
*          uint8_t length - number of bytes
*
* @return  none
*/
void halRfReadRxBuf(uint8_t* pData, uint8_t length)
{
    // Read data
    while (length>0) {
        *pData++= RFD;
        length--;
    }
}


/***********************************************************************************
* @fn      halRfReadMemory
*
* @brief   Read RF device memory
*
* @param   uint16_t addr - memory address
*          uint8_t* pData - data buffer. This must be allocated by caller.
*          uint8_t length - number of bytes
*
* @return  Number of bytes read
*/
uint8_t halRfReadMemory(uint16_t addr, uint8_t* pData, uint8_t length)
{
    return 0;
}


/***********************************************************************************
* @fn      halRfWriteMemory
*
* @brief   Write RF device memory
*
* @param   uint16_t addr - memory address
*          uint8_t* pData - data buffer. This must be allocated by caller.
*          uint8_t length - number of bytes
*
* @return  Number of bytes written
*/
uint8_t halRfWriteMemory(uint16_t addr, uint8_t* pData, uint8_t length)
{
    return 0;
}

/***********************************************************************************
* @fn      halRfTransmit
*
* @brief   Transmit frame with Clear Channel Assessment.
*
* @param   none
*
* @return  uint8_t - SUCCESS or FAILED
*/
uint8_t halRfTransmit(void)
{
    uint8_t status;
    volatile uint8_t delay;
    
    ISSAMPLECCA();
    for(delay=0xFF;delay>0;delay--);
    if (!(FSMSTAT1 & BV(3)))
        return FAILED;
    
    ISTXONCCA(); // Sending
    for(delay=0xFF;delay>0;delay--);
    if (!(FSMSTAT1 & BV(3)))
        return FAILED;
    
    // Waiting for transmission to finish
    while(!(RFIRQF1 & IRQ_TXDONE) );

    RFIRQF1 = ~IRQ_TXDONE;
    status= SUCCESS;

    return status;
}



/***********************************************************************************
* @fn      halRfReceiveOn
*
* @brief   Turn receiver on
*
* @param   none
*
* @return  none
*/
void halRfReceiveOn(void)
{
    ISFLUSHRX();     // Making sure that the RX FIFO is empty.
    ISRXON();
}

/***********************************************************************************
* @fn      halRfReceiveOff
*
* @brief   Turn receiver off
*
* @param   none
*
* @return  none
*/
void halRfReceiveOff(void)
{
    ISRFOFF();
    ISFLUSHRX();    // Making sure that the TX FIFO is empty.
}


#ifndef MRFI
/***********************************************************************************
* @fn      halRfDisableRxInterrupt
*
* @brief   Clear and disable RX interrupt.
*
* @param   none
*
* @return  none
*/
void halRfDisableRxInterrupt(void)
{
    // disable RXPKTDONE interrupt
    RFIRQM0 &= ~BV(6);
    // disable RXOVERF interrupt
    RFERRM &= ~BV(2);
    // disable general RF interrupts
    IEN2 &= ~BV(0);
    // disable RF error interrupts
    IEN0 &= ~BV(0);
}


/***********************************************************************************
* @fn      halRfEnableRxInterrupt
*
* @brief   Enable RX interrupt.
*
* @param   none
*
* @return  none
*/
void halRfEnableRxInterrupt(void)
{
    // enable RXPKTDONE interrupt
    RFIRQM0 |= BV(6);
    
    // enable RXOVERF interrupt
    RFERRM |= BV(2);
    
    // enable general RF interrupts
    IEN2 |= BV(0);
    
    // enable RF error interrupts
    IEN0 |= BV(0);
}


/***********************************************************************************
* @fn      halRfRxInterruptConfig
*
* @brief   Configure RX interrupt.
*
* @param   none
*
* @return  none
*/
void halRfRxInterruptConfig(ISR_FUNC_PTR pf)
{
    uint8_t x;
    HAL_INT_LOCK(x);
    pfISR= pf;
    HAL_INT_UNLOCK(x);
}

#endif

/***********************************************************************************
* @fn      halRfWaitTransceiverReady
*
* @brief   Wait until the transciever is ready (SFD inactive).
*
* @param   none
*
* @return  none
*/
void halRfWaitTransceiverReady(void)
{
    // Wait for SFD not active and TX_Active not active
    while (FSMSTAT1 & (BV(1) | BV(5) ));
}

#ifndef MRFI
/************************************************************************************
* @fn          rfIsr
*
* @brief       Interrupt service routine that handles RFPKTDONE interrupt.
*
* @param       none
*
* @return      none
*/
HAL_ISR_FUNCTION( rfIsr, RF_VECTOR )
{
    uint8_t x;
    uint8_t rfim = RFIRQM0;

    HAL_ENTER_ISR(x);

    /*
     *  The CPU level RF interrupt flag must be cleared here (before clearing RFIRQFx).
     *  to allow the interrupts to be nested.
     */
    S1CON = 0x00;
  
    if((RFIRQF0 & IRQ_RXPKTDONE) & rfim)
    {
        if(pfISR)
            (*pfISR)();                 // Execute the custom ISR
        
        ISFLUSHRX();
        RFIRQF0 &= ~IRQ_RXPKTDONE;
    }
    HAL_EXIT_ISR(x);
}

/**************************************************************************************************
 * @fn          macMcuRfErrIsr
 *
 * @brief       Interrupt service routine that handles all RF Error interrupts.  Only the RX FIFO
 *              overflow condition is handled.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
HAL_ISR_FUNCTION( rfErrIsr, RFERR_VECTOR )
{
    uint8_t x;
    uint8_t rferrm;

    HAL_ENTER_ISR(x);

    rferrm = RFERRM;

    if ((RFERRF & RFERR_RXOVERF) & rferrm)
    {
        ISFLUSHRX();
        RFERRF = (RFERR_RXOVERF ^ 0xFF);
    }

    HAL_EXIT_ISR(x);
}
#endif

/***********************************************************************************
* LOCAL FUNCTIONS
*/
/***********************************************************************************
* LOCAL FUNCTIONS
*/
static void halPaLnaInit(void)
{
#if INCLUDE_PA==2591
    // Initialize CC2591 to RX high gain mode
    static uint8_t fFirst= TRUE;

    if(fFirst) {
        AGCCTRL1  = 0x15;
        FSCAL1 = 0x0; 
        RFC_OBS_CTRL0 = 0x68;
        RFC_OBS_CTRL1 = 0x6A;
        OBSSEL1 = 0xFB;
        OBSSEL4 = 0xFC;
        P0DIR |= 0x80;
        halRfSetGain(HAL_RF_GAIN_HIGH);
    }

#else // do nothing
#endif
}


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
