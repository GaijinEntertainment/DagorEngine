/* chunkset.c -- inline functions to copy small data chunks.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zbuild.h"

typedef uint64_t chunk_t;

#define CHUNK_SIZE 8

#define HAVE_CHUNKMEMSET_1
#define HAVE_CHUNKMEMSET_4
#define HAVE_CHUNKMEMSET_8

static inline void chunkmemset_1(uint8_t *from, chunk_t *chunk) {
    memset(chunk, *from, sizeof(chunk_t));
}

static inline void chunkmemset_4(uint8_t *from, chunk_t *chunk) {
    uint8_t *dest = (uint8_t *)chunk;
    zmemcpy_4(dest, from);
    zmemcpy_4(dest+4, from);
}

static inline void chunkmemset_8(uint8_t *from, chunk_t *chunk) {
    zmemcpy_8(chunk, from);
}

static inline void loadchunk(uint8_t const *s, chunk_t *chunk) {
    zmemcpy_8(chunk, (uint8_t *)s);
}

static inline void storechunk(uint8_t *out, chunk_t *chunk) {
    zmemcpy_8(out, chunk);
}

#define CHUNKSIZE        chunksize_c
#define CHUNKCOPY        chunkcopy_c
#define CHUNKCOPY_SAFE   chunkcopy_safe_c
#define CHUNKUNROLL      chunkunroll_c
#define CHUNKMEMSET      chunkmemset_c
#define CHUNKMEMSET_SAFE chunkmemset_safe_c

#include "chunkset_tpl.h"
