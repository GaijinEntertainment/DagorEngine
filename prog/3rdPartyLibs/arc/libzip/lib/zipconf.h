#ifndef _HAD_ZIPCONF_H
#define _HAD_ZIPCONF_H

/*
   zipconf.h -- platform specific include file

   This file was generated automatically by ./make_zipconf.sh
   based on ../config.h.
 */

#include <limits.h>

#ifndef INT32_MAX
#define INT32_MAX INT_MAX
#endif //INT32_MAX

#define LIBZIP_VERSION "0.10.1"
#define LIBZIP_VERSION_MAJOR 0
#define LIBZIP_VERSION_MINOR 10
#define LIBZIP_VERSION_MICRO 1

typedef signed char zip_int8_t;
#define ZIP_INT8_MIN -128
#define ZIP_INT8_MAX 127

typedef unsigned char zip_uint8_t;
#define ZIP_UINT8_MAX 0xffU

typedef short zip_int16_t;
#define ZIP_INT16_MIN -32768
#define ZIP_INT16_MAX 32767

typedef unsigned short zip_uint16_t;
#define ZIP_UINT16_MAX 0xffffU

typedef long zip_int32_t;
#define ZIP_INT32_MIN (-INT32_MAX - 1)
#define ZIP_INT32_MAX 2147483647

typedef unsigned long zip_uint32_t;
#define ZIP_UINT32_MAX 0xffffffffUL

typedef long long zip_int64_t;
#define ZIP_INT64_MIN (-INT64_MAX - 1)
#define ZIP_INT64_MAX 9223372036854775807LL

typedef unsigned long long zip_uint64_t;
#define ZIP_UINT64_MAX 0xffffffffffffffffULL


#endif /* zipconf.h */
