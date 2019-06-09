/*
 * 文件名称：pub_def.h
 * 文件说明：public typedef
 * 
 */

#ifndef		PUB_DEF_H
#define		PUB_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char           uint8_t;
typedef unsigned short          uint16_t;
typedef unsigned long           uint32_t;

typedef signed char             int8_t;
typedef signed short            int16_t;
typedef signed long             int32_t;

typedef void                    (*ISR_FUNC_PTR)(void);
typedef void                    (*VFPTR)(void);


typedef enum
{
    FALSE = 0,
    TRUE
}bool_t;

struct list_node
{
    struct list_node *next;                          /**< point to next node. */
    struct list_node *prev;                          /**< point to prev node. */
};
typedef struct list_node list_t;                    /**< Type for lists. */


#ifndef VALID
    #define VALID               1
#endif

#ifndef NULL
    #define NULL                (void *)0
#endif

#ifndef INVALID
    #define INVALID             0
#endif

#include "hal_defs.h"

/***********************************************************************************
 * Compiler abstraction
 */

/*****************************************************
 * IAR MSP430
 */
#ifdef __IAR_SYSTEMS_ICC__

#define _PRAGMA(x) _Pragma(#x)

#if defined __ICC430__

#ifndef CODE
#define CODE
#endif
#ifndef XDATA
#define XDATA
#endif
#define FAR
#define NOP()  asm("NOP")

#define HAL_ISR_FUNC_DECLARATION(f,v)   \
    _PRAGMA(vector=v##_VECTOR) __interrupt void f(void)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     \
    _PRAGMA(vector=v##_VECTOR) __interrupt void f(void)
#define HAL_ISR_FUNCTION(f,v)           \
    HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)


/*****************************************************
 * IAR 8051
 */
#elif defined __ICC8051__

#ifndef BSP_H
#define CODE   __code
#define XDATA  __xdata
#endif

#define FAR
#define NOP()  asm("NOP")

#define HAL_MCU_LITTLE_ENDIAN()   __LITTLE_ENDIAN__
#define HAL_ISR_FUNC_DECLARATION(f,v)   \
    _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     \
    _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNCTION(f,v)           \
    HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

#else
#error "Unsupported architecture"
#endif


/*****************************************************
 * KEIL 8051
 */
#elif defined __KEIL__
#include <intrins.h>
#define BIG_ENDIAN

#define CODE   code
#define XDATA  xdata
#define FAR
#define NOP()  _nop_()

#define HAL_ISR_FUNC_DECLARATION(f,v)   \
    void f(void) interrupt v
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     \
    void f(void)
#define HAL_ISR_FUNCTION(f,v)           \
    HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

typedef unsigned short istate_t;

// Keil workaround
#define __code  code
#define __xdata xdata


/*****************************************************
 * WIN32
 */
#elif defined WIN32

#define CODE
#define XDATA
#include "windows.h"
#define FAR far
#pragma warning (disable :4761)


/*****************************************************
 * Code Composer Essential
 */

#elif __TI_COMPILER_VERSION__
#define CODE
#define XDATA
#define FAR

typedef unsigned short istate_t;


/*****************************************************
 * Other compilers
 */
#else
#error "Unsupported compiler"
#endif


#ifdef __cplusplus
}
#endif

#endif