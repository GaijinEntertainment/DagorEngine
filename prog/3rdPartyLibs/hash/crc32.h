/*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
/*  code or tables extracted from it, as desired without restriction.     */
#ifndef _CRC32_H_
#define _CRC32_H_
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

extern unsigned calc_crc32(const unsigned char* data, int len, unsigned cur_sum);
extern unsigned calc_crc32_continuous(const unsigned char* data, int len, unsigned cur_sum);

#ifdef __cplusplus
}
#endif

#endif
