/*
 * 文件名称：error.h
 *
 * 文件说明：BOS error description 
 * 
 */
 

#ifndef     ERROE_H
#define     ERROE_H


#define BOS_EOK         0   /* 没有错误 */
#define BOS_ERROR       1   /* 通用错误 */
#define BOS_ETIMEOUT    2   /* 超时 */
#define BOS_EFULL       3   /*  */
#define BOS_EEMPTY      4   /*  */
#define BOS_ENOMEM      5   /* 内存不足 */
#define BOS_EBUSY       6   /* 系统忙 */
#define BOS_EOVERLEN    7   /* 超长 */


typedef signed int  error_t;

#endif                                                            
    

