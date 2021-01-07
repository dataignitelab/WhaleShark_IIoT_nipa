/*
 * File      : common.h
 */
 
#ifndef __COMMON_H__
#define __COMMON_H__

#include <rtdef.h>

#define HTONS(x) ((((x)>>8)&0x00ff)|(((x)<<8)&0xff00))
#define NTOHS(x) HTONS((x))
#define HTONL(x) ((((x)>>24)&0x000000ff)|(((x)>> 8)&0x0000ff00)|(((x)<< 8)&0x00ff0000)|(((x)<<24)&0xff000000))
#define NTOHL(x) HTONL((x))

rt_uint16_t base16_encode(rt_uint8_t *pSrc, rt_uint16_t srcLen, rt_uint8_t *pDst);
rt_uint16_t base16_decode(rt_uint8_t *pSrc, rt_uint16_t srcLen, rt_uint8_t *pDst);
rt_uint8_t chechsum_xor(rt_uint8_t *pData, rt_uint16_t len);
rt_uint16_t CalCrc16(rt_uint8_t *p_buff, rt_uint32_t length);
int _strncasecmp (const char* s1, const char* s2, size_t len);
void BufferShow(rt_uint8_t *pData, rt_uint32_t len);

#endif  // #ifndef __COMMON_H__