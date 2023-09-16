/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2006, 2010, 2011, 2012, 2016, 2018 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
 * CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
 * tables for updating the shift register in one step with three exclusive-ors
 * instead of four steps with four exclusive-ors.  This results in about a
 * factor of two increase in speed on a Power PC G4 (PPC7455) using gcc -O3.
 */

#include "zbuild.h"
#include "zendian.h"
#include "deflate.h"
#include "functable.h"
#include "crc32_tbl.h"

/* ========================================================================= */
const uint32_t * Z_EXPORT PREFIX(get_crc_table)(void) {
    return (const uint32_t *)crc_table;
}

#ifdef ZLIB_COMPAT
unsigned long Z_EXPORT PREFIX(crc32_z)(unsigned long crc, const unsigned char *buf, size_t len) {
    if (buf == NULL) return 0;

    return (unsigned long)functable.crc32((uint32_t)crc, buf, len);
}
#else
uint32_t Z_EXPORT PREFIX(crc32_z)(uint32_t crc, const unsigned char *buf, size_t len) {
    if (buf == NULL) return 0;

    return functable.crc32(crc, buf, len);
}
#endif

#ifdef ZLIB_COMPAT
unsigned long Z_EXPORT PREFIX(crc32)(unsigned long crc, const unsigned char *buf, unsigned int len) {
    return (unsigned long)PREFIX(crc32_z)((uint32_t)crc, buf, len);
}
#else
uint32_t Z_EXPORT PREFIX(crc32)(uint32_t crc, const unsigned char *buf, uint32_t len) {
    return PREFIX(crc32_z)(crc, buf, len);
}
#endif

/* ========================================================================= */

/*
   This BYFOUR code accesses the passed unsigned char * buffer with a 32-bit
   integer pointer type. This violates the strict aliasing rule, where a
   compiler can assume, for optimization purposes, that two pointers to
   fundamentally different types won't ever point to the same memory. This can
   manifest as a problem only if one of the pointers is written to. This code
   only reads from those pointers. So long as this code remains isolated in
   this compilation unit, there won't be a problem. For this reason, this code
   should not be copied and pasted into a compilation unit in which other code
   writes to the buffer that is passed to these routines.
 */

/* ========================================================================= */
#if BYTE_ORDER == LITTLE_ENDIAN
#define DOSWAP(crc) (crc)
#define DO1 \
    c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8)
#define DO4 c ^= *buf4++; \
    c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
        crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#elif BYTE_ORDER == BIG_ENDIAN
#define DOSWAP(crc) ZSWAP32(crc)
#define DO1 \
    c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8)
#define DO4 c ^= *buf4++; \
    c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ \
        crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
#else
#  error "No endian defined"
#endif
#define DO32 DO4; DO4; DO4; DO4; DO4; DO4; DO4; DO4

/* ========================================================================= */
Z_INTERNAL uint32_t crc32_byfour(uint32_t crc, const unsigned char *buf, uint64_t len) {
    Z_REGISTER uint32_t c;
    Z_REGISTER const uint32_t *buf4;

    c = DOSWAP(crc);
    c = ~c;
    while (len && ((ptrdiff_t)buf & 3)) {
        DO1;
        len--;
    }

    buf4 = (const uint32_t *)(const void *)buf;

#ifdef UNROLL_MORE
    while (len >= 32) {
        DO32;
        len -= 32;
    }
#endif

    while (len >= 4) {
        DO4;
        len -= 4;
    }
    buf = (const unsigned char *)buf4;

    if (len) do {
        DO1;
    } while (--len);
    c = ~c;
    return DOSWAP(c);
}
