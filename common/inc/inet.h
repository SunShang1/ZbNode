/*
 * 文件名称  inet.h
 *
 * 文件说明：大小端转换 
 * 
 */
 
#ifndef __INET_H__
#define __INET_H__

#include "pub_def.h"

#ifdef __LITTLE_ENDIAN__

static inline uint32_t htonl(uint32_t x)
{
    return ( ((x<<24)&0xFF000000)       |
              ((x<<8)&0x00FF0000)        |
              ((x>>8)&0x0000FF00)        |
              ((x>>24)&0x000000FF) );
}

static inline uint16_t htons(uint16_t x)
{
    return ( ((x<<8)&0xFF00)          |
              ((x>>8)&0x00FF) );
}

#define ntohl(x)    htonl(x)
#define ntohs(x)    htons(x)

#elif defined  (__BIG_ENDIAN__)

#define ntohl(x)    (x)
#define htonl(x)    (x)
#define ntohs(x)    (x)
#define htons(x)    (x)

#else

#error "Please define __BIG_ENDIAN__ or __LITTLE_ENDIAN__"

#endif
      
#endif