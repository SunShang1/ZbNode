/*! \file crc.h
    \brief CRC implementation header file
*/

#ifndef __CRC_H__
#define __CRC_H__

#include "pub_def.h"

/**************************************************************************/
/*! \fn Uint32 crc32( Uint32 crc, const Uint8 *buf, Uint32 len )
**************************************************************************
 *  \brief  CRC32 implementation, x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+1
 *  \param[in] crc: Initial CRC
 *  \param[in] buf: Pointer to data buffer
 *  \param[in] len: Buffer length
 *  \return CRC32 value
  **************************************************************************/
unsigned int crc32( unsigned int crc, const unsigned char *buf, unsigned int len );

uint16_t crc16(uint16_t crc, const unsigned char *buf, int len);

uint8_t sum8(uint8_t sum, const unsigned char *buf, int len);

#endif

