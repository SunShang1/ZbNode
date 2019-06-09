/******************************************************************************
    Filename: uart.c

    This file defines uart related functions for the CC253x family
    of RF system-on-chips from Texas Instruments.

******************************************************************************/


/*******************************************************************************
 * INCLUDES
 */

#include "hal_uart.h"
#include "BosMem.h"
#include "string.h"

/*******************************************************************************
 * TYPEDEF
 */


/*******************************************************************************
 * GLOBALE
 */
#if ENABLE_UART0
struct uartInfo_t Uart0Info;
#endif
#if ENABLE_UART1
struct uartInfo_t Uart1Info;
#endif

#if (ENABLE_UART0) || (ENABLE_UART1)
__code static const uint8_t baudRate[12][2] =
{
    {59, 6},    //2400
    {59, 7},
    {59, 8},    //9600
    {216, 8},
    {59, 9},    //19200
    {216, 9},
    {59, 10},
    {216, 10},  //57600
    {59, 11},
    {216, 11},  //115200
    {216, 12}   //230400
};
#endif

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
error_t halUartInit(uint8_t port, struct uartOpt_t *opt)
{
    struct uartInfo_t   *pUartInfo;

#if ENABLE_UART0
    if (port == 0)
    {
        pUartInfo = &Uart0Info;
    }
    else
#endif
#if ENABLE_UART1
    if (port == 1)
    {
        pUartInfo = &Uart1Info;
    }
    else
#endif
        return  -BOS_ERROR;

#if (UART_TX_MODE != UART_POLLING_TX)    
    pUartInfo->TxBufWrite = pUartInfo->TxBufRead = 0;
#endif 
    pUartInfo->RxBufWrite = pUartInfo->RxBufRead = 0;
    pUartInfo->pRxCallBack = NULL;
    pUartInfo->RxCnt = 0;

#if ENABLE_UART0
    if (port == 0)
    {
        MCU_IO_DIR_OUTPUT_PREP(0, 3);
        MCU_IO_PERIPHERAL_PREP(0, 3);
        
        MCU_IO_DIR_INPUT_PREP(0, 2);
        MCU_IO_PERIPHERAL_PREP(0, 2);
        
        PERCFG &= ~(1 << 0);
        
        if (opt->parity == 'E')
        {
            U0UCR = (1<<3);
        }
        else if (opt->parity == 'O')
        {
            U0UCR = (1<<5)|(1<<3);
        }
        else // if (opt->parity == 'N')
            U0UCR = 0x0;
        
        if (opt->dataBits == UART_DATA_BIT8)
        {
            U0UCR &= ~(1<<4);
        }
        else // if (opt->dataBits == UART_DATA_BIT9)
        {
            U0UCR |= (1<<4);
        }
        
        if (opt->stopBits == UART_STOP_BIT1)
        {
            U0UCR &= ~(1<<2);
        }
        else // if (opt->stopBits == UART_STOP_BIT2)
        {
            U0UCR |= (1<<2);
        }
        
        U0UCR |= (0x2<<0);
        
        U0GCR = baudRate[opt->baudRate][1];
        U0BAUD = baudRate[opt->baudRate][0];
        
        URX0IE = 1; //enable Rx interrupt
        U0CSR = (1<<7) | (1<<6); // UART mode, start rx
    }
#endif

#if ENABLE_UART1
    if (port == 1)
    {
        MCU_IO_DIR_OUTPUT_PREP(1, 6);
        MCU_IO_PERIPHERAL_PREP(1, 6);
        
        MCU_IO_DIR_INPUT_PREP(1, 7);
        MCU_IO_PERIPHERAL_PREP(1, 7);
        
        PERCFG |= (1 << 1);
        
        if (opt->parity == 'E')
        {
            U1UCR = (1<<3);
        }
        else if (opt->parity == 'O')
        {
            U1UCR = (1<<5)|(1<<3);
        }
        else // if (opt->parity == 'N')
            U1UCR = 0x0;
        
        if (opt->dataBits == UART_DATA_BIT8)
        {
            U1UCR &= ~(1<<4);
        }
        else // if (opt->dataBits == UART_DATA_BIT9)
        {
            U1UCR |= (1<<4);
        }
        
        if (opt->stopBits == UART_STOP_BIT1)
        {
            U1UCR &= ~(1<<2);
        }
        else // if (opt->stopBits == UART_STOP_BIT2)
        {
            U1UCR |= (1<<2);
        }
        
        U1UCR |= (0x2<<0);
        
        U1GCR = baudRate[opt->baudRate][1];
        U1BAUD = baudRate[opt->baudRate][0];
        
        URX1IE = 1; //enable Rx interrupt
        U1CSR = (1<<7) | (1<<6); // UART mode, start rx
    }
#endif

    return BOS_EOK;
}

error_t halUartRxCbSet(uint8_t port, pInterruptHandler_t pCallBack)
{
#if ENABLE_UART0
    if (port == 0)
        Uart0Info.pRxCallBack = pCallBack;
    else
#endif
#if ENABLE_UART1
    if (port == 1)
        Uart1Info.pRxCallBack = pCallBack;
    else
#endif
        return  -BOS_ERROR;
    
    return BOS_EOK;
}

int halUartSend(uint8_t port, uint8_t *buf, int size)
{
    int len = -1;
    
    if (buf == NULL)
        return -1;
#if (UART_TX_MODE == UART_POLLING_TX)
#if ENABLE_UART0    
    if (port == 0)
    {
        int i;
        
        U0CSR &= ~(1<<1);
    
        for (i = 0; i < size; i ++)
        {
            U0DBUF = *buf++;
            
            while(!(U0CSR&(1<<1)));
            U0CSR &= ~(1<<1);
        }
        
        len = size;
    }
#endif
    
#if ENABLE_UART1    
    if (port == 1)
    {
        int i;
        
        U1CSR &= ~(1<<1);
    
        for (i = 0; i < size; i ++)
        {
            U1DBUF = *buf++;
            
            while(!(U1CSR&(1<<1)));
            U1CSR &= ~(1<<1);
        }
        
        len = size;
    }
#endif
    
#endif
     
    return len;
}
                 
int halUartRecv(uint8_t port, uint8_t *buf, int bufSize)
{
    uint8_t hwlock;
    int len=0;
    struct uartInfo_t   *pUartInfo;
    
#if ENABLE_UART0
    if (port == 0)
    {
        pUartInfo = &Uart0Info;
    }
    else
#endif
#if ENABLE_UART1
    if (port == 1)
    {
        pUartInfo = &Uart1Info;
    }
    else
#endif
        return  -BOS_ERROR;

    EnterCritical(hwlock);
    
    while((pUartInfo->RxCnt>0)&&(len<bufSize))
    {
        *buf ++ = pUartInfo->RxBuf[pUartInfo->RxBufRead++];
        if (pUartInfo->RxBufRead == UART_BUF_SIZE)
            pUartInfo->RxBufRead = 0;
        
        pUartInfo->RxCnt--;
        len ++;
    }
    
    ExitCritical(hwlock);
    
    return len;
}

int halUartPutChar(uint8_t port, char ch)
{
#if ENABLE_UART0    
    if (port == 0)
    {
        U0CSR &= ~(1<<1);
    
        U0DBUF = ch;
            
        while(!(U0CSR&(1<<1)));
        U0CSR &= ~(1<<1);
        
        return BOS_EOK;
    }
#endif
     
#if ENABLE_UART1   
    if (port == 1)
    {
        U1CSR &= ~(1<<1);
    
        U1DBUF = ch;
            
        while(!(U1CSR&(1<<1)));
        U1CSR &= ~(1<<1);
        
        return BOS_EOK;
    }
#endif
     
    return -BOS_ERROR;
}

int halUartGetChar(uint8_t port, char *ch)
{
    uint8_t hwlock;
    struct uartInfo_t   *pUartInfo;
    
    if (ch == NULL)
        return -BOS_ERROR;
    
#if ENABLE_UART0
    if (port == 0)
    {
        pUartInfo = &Uart0Info;
    }
    else
#endif
#if ENABLE_UART1
    if (port == 1)
    {
        pUartInfo = &Uart1Info;
    }
    else
#endif
        return  -BOS_ERROR;

    EnterCritical(hwlock);
    
    if (pUartInfo->RxCnt>0)
    {
        *ch = pUartInfo->RxBuf[pUartInfo->RxBufRead++];
        if (pUartInfo->RxBufRead == UART_BUF_SIZE)
            pUartInfo->RxBufRead = 0;
        pUartInfo->RxCnt--;
        
        ExitCritical(hwlock);
        
        return BOS_EOK;
    }
    else
    {
        ExitCritical(hwlock);
        
        return -BOS_ERROR;
    }
}

#if ENABLE_UART0
#pragma vector = URX0_VECTOR
__interrupt void UART0_RX_ISR(void)
{
    static uint8_t lastByte = 0;
    uint8_t  tmp;
    
    // Clear UART0 RX Interrupt Flag (TCON.URX0IF = 0).
    URX0IF = 0;

    // Read UART0 RX buffer.
    tmp = U0DBUF;
    
    if (Uart0Info.RxCnt < UART_BUF_SIZE)
    {
        Uart0Info.RxBuf[Uart0Info.RxBufWrite++] = tmp;
        if (Uart0Info.RxBufWrite == UART_BUF_SIZE)
            Uart0Info.RxBufWrite = 0;
        Uart0Info.RxCnt ++;
    }

    if (Uart0Info.pRxCallBack)
    {
        if ( (Uart0Info.RxCnt > (UART_BUF_SIZE>>1)) ||
             ((lastByte == 0x0D) && (tmp == 0x0A)) )
        {
            Uart0Info.pRxCallBack(NULL);
        }
    }
    
    lastByte = tmp;
}
#endif

#if ENABLE_UART1
#pragma vector = URX1_VECTOR
__interrupt void UART1_RX_ISR(void)
{
    uint8_t  tmp;
    
    // Clear UART1 RX Interrupt Flag (TCON.URX1IF = 0).
    URX1IF = 0;

    // Read UART0 RX buffer.
    tmp = U1DBUF;
    
    if (Uart1Info.RxCnt < UART_BUF_SIZE)
    {
        Uart1Info.RxBuf[Uart1Info.RxBufWrite++] = tmp;
        if (Uart1Info.RxBufWrite == UART_BUF_SIZE)
            Uart1Info.RxBufWrite = 0;
        Uart1Info.RxCnt ++;
    }

    if (Uart1Info.pRxCallBack)
    {
        if (Uart1Info.RxCnt > (UART_BUF_SIZE>>1))
        {
            Uart1Info.pRxCallBack(NULL);
        }
    }
}
#endif

#if (__CODE_MODEL__ == __CM_BANKED__)
__near_func int  putchar(int ch)
#else
MEMORY_ATTRIBUTE int  putchar(int ch)
#endif
{
#if (CONSOLE_UART==0)
  
    U0CSR &= ~(1<<1);
    
    U0DBUF = (uint8_t)ch;
            
    while(!(U0CSR&(1<<1)));
    U0CSR &= ~(1<<1);
    
#else
    
    U1CSR &= ~(1<<1);
    
    U1DBUF = (uint8_t)ch;
            
    while(!(U1CSR&(1<<1)));
    U1CSR &= ~(1<<1);
    
#endif
    return ch;
}

