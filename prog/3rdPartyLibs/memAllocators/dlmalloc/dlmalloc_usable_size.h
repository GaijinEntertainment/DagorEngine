#pragma once

#include "dlmalloc-setup.h"
// extracts from dlmalloc code, for fastest dagmem_malloc_usable_size()
typedef struct malloc_chunk
{
  size_t prev_foot; /* Size of previous chunk (if free).  */
  size_t head;      /* Size and inuse bits. */
  malloc_chunk *fd; /* double links -- used only if free. */
  malloc_chunk *bk;
} *mchunkptr;

#define SIZE_T_SIZE (sizeof(size_t))

#define SIZE_T_ONE       ((size_t)1)
#define SIZE_T_TWO       ((size_t)2)
#define SIZE_T_FOUR      ((size_t)4)
#define TWO_SIZE_T_SIZES (SIZE_T_SIZE << 1)

#if FOOTERS
#define CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
#else
#define CHUNK_OVERHEAD (SIZE_T_SIZE)
#endif

#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)

#define PINUSE_BIT (SIZE_T_ONE)
#define CINUSE_BIT (SIZE_T_TWO)
#define FLAG4_BIT  (SIZE_T_FOUR)
#define INUSE_BITS (PINUSE_BIT | CINUSE_BIT)
#define FLAG_BITS  (PINUSE_BIT | CINUSE_BIT | FLAG4_BIT)

#define CHUNKSIZE(p)    ((p)->head & ~(FLAG_BITS))
#define IS_INUSE(p)     (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define IS_MMAPPED(p)   (((p)->head & INUSE_BITS) == 0)
#define OVERHEAD_FOR(p) (IS_MMAPPED(p) ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

#define MEM2CHUNK(mem) ((mchunkptr)((char *)(mem) - TWO_SIZE_T_SIZES))

static inline size_t dagmem_malloc_usable_size(void *mem)
{
  if (mem != 0)
  {
    mchunkptr p = MEM2CHUNK(mem);
    if (IS_INUSE(p))
      return CHUNKSIZE(p) - OVERHEAD_FOR(p);
  }
  return 0;
}
