// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

#define ONLY_MSPACES        0 //(false)
#define HAVE_MORECORE       1
#define MORECORE(x)         winapi_more_core(x)
#define MORECORE_CONTIGUOUS 1 //(true)

#ifdef MEM_DEBUGALLOC
#define DEBUG_DLMALLOC_MODE (MEM_DEBUGALLOC > 0)
#else
#define DEBUG_DLMALLOC_MODE 0
#endif

#define FOOTERS  DEBUG_DLMALLOC_MODE
#define INSECURE (!DEBUG_DLMALLOC_MODE)
#if DEBUG_DLMALLOC_MODE
#define DEBUG 1
#endif

#define MSPACES       1 //(true)
#define USE_DL_PREFIX 1 //(true)
#if _TARGET_PC_WIN | _TARGET_XBOX
#define WIN32 1 //(true)
#endif

#define MALLOC_ALIGNMENT        16
#define USE_LOCKS               1 //(true)
#define ABORT                   instant_abort()
#define PROCEED_ON_ERROR        0 //(false)
#define ABORT_ON_ASSERT_FAILURE 1 //(true)
#define MALLOC_FAILURE_ACTION

// #define MORECORE_CANNOT_TRIM
#define HAVE_MMAP              1 //(true)
#define MAX_RELEASE_CHECK_RATE 4095
#define HAVE_MREMAP            0 //(false)
#define MMAP_CLEARS            1 //(false)
#define USE_BUILTIN_FFS        0 //(false)
#define malloc_getpagesize     4096
#define USE_DEV_RANDOM         0 //(false)
#define NO_MALLINFO            0 //(false)
#define MALLINFO_FIELD_TYPE    size_t
#define REALLOC_ZERO_BYTES_FREES

#ifndef __GNUC__
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#endif
// #define LACKS_STRING_H
// #define LACKS_STRINGS_H
// #define LACKS_SYS_TYPES_H
// #define LACKS_ERRNO_H

#define DEFAULT_GRANULARITY    (16 * 4096)
#define DEFAULT_TRIM_THRESHOLD (2 << 20)
#define NO_SEGMENT_TRAVERSAL   0

#define DEFAULT_MMAP_THRESHOLD (4 << 20)

#ifdef __cplusplus
extern "C"
#endif
  void *winapi_more_core(intptr_t size);

static __inline void instant_abort(void)
{
#if _MSC_VER
  __debugbreak();
#elif __GNUC__
  __builtin_trap();
#else
  *(volatile int *)0 = 0xBADC0DE;
#endif
}
