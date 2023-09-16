/* zutil_p.h -- Private inline functions used internally in zlib-ng
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef ZUTIL_P_H
#define ZUTIL_P_H

#include <stdlib.h>
#if defined(__APPLE__) || defined(HAVE_POSIX_MEMALIGN)

#elif defined(__GNUC__)
# if __has_include(<malloc.h>)
#  include <malloc.h>
# endif
#else
#  include <memory.h>
#endif

/* Function to allocate 16 or 64-byte aligned memory */
static inline void *zng_alloc(size_t size) {
#ifdef HAVE_POSIX_MEMALIGN
    void *ptr;
    return posix_memalign(&ptr, 64, size) ? NULL : ptr;
#elif defined(_WIN32)
    return (void *)_aligned_malloc(size, 64);
#elif defined(__APPLE__)
    return (void *)malloc(size);     /* MacOS always aligns to 16 bytes */
#else
    return (void *)memalign(64, size);
#endif
}

/* Function that can free aligned memory */
static inline void zng_free(void *ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

#endif
