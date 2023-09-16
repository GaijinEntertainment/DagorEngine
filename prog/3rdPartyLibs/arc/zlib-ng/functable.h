/* functable.h -- Struct containing function pointers to optimized functions
 * Copyright (C) 2017 Hans Kristian Rosbach
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef FUNCTABLE_H_
#define FUNCTABLE_H_

#include "deflate.h"
#include "crc32_fold.h"

struct functable_s {
    uint32_t (* adler32)            (uint32_t adler, const unsigned char *buf, size_t len);
    uint32_t (* crc32)              (uint32_t crc, const unsigned char *buf, uint64_t len);
    uint32_t (* crc32_fold_reset)   (crc32_fold *crc);
    void     (* crc32_fold_copy)    (crc32_fold *crc, uint8_t *dst, const uint8_t *src, size_t len);
    void     (* crc32_fold)         (crc32_fold *crc, const uint8_t *src, size_t len, uint32_t init_crc);
    uint32_t (* crc32_fold_final)   (crc32_fold *crc);
    uint32_t (* compare256)         (const uint8_t *src0, const uint8_t *src1);
    uint32_t (* chunksize)          (void);
    uint8_t* (* chunkcopy)          (uint8_t *out, uint8_t const *from, unsigned len);
    uint8_t* (* chunkunroll)        (uint8_t *out, unsigned *dist, unsigned *len);
    uint8_t* (* chunkmemset)        (uint8_t *out, unsigned dist, unsigned len);
    uint8_t* (* chunkmemset_safe)   (uint8_t *out, unsigned dist, unsigned len, unsigned left);
    void     (* insert_string)      (deflate_state *const s, uint32_t str, uint32_t count);
    uint32_t (* longest_match)      (deflate_state *const s, Pos cur_match);
    uint32_t (* longest_match_slow) (deflate_state *const s, Pos cur_match);
    Pos      (* quick_insert_string)(deflate_state *const s, uint32_t str);
    void     (* slide_hash)         (deflate_state *s);
    uint32_t (* update_hash)        (deflate_state *const s, uint32_t h, uint32_t val);
};

Z_INTERNAL extern Z_TLS struct functable_s functable;

#endif
