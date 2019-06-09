/*! \file crc.h
    \brief CRC implementation header file
*/

#ifndef __CRC_H__
#define __CRC_H__

#include "pub_def.h"

/**************************************************************************/
/*! \fn uint16_t crc16(uint16_t crc, const unsigned char *buf, int len)
**************************************************************************
 *  \brief  16 bit CRC with polynomial x^16+x^12+x^5+1
 *  \param[in] crc: Initial CRC
 *  \param[in] buf: Pointer to data buffer
 *  \param[in] len: Buffer length
 *  \return CRC16 value
  **************************************************************************/
uint16_t crc16(uint16_t crc, const unsigned char *buf, int len);

#endif

