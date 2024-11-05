// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

#if defined(_STD_RTL_MEMORY) && defined(_DLMALLOC_MSPACE)
#define ONLY_MSPACES        1 //(true)
#define HAVE_MORECORE       0
#define MORECORE_CONTIGUOUS 0
#else
#define ONLY_MSPACES        0 //(false)
#define HAVE_MORECORE       1
#define MORECORE(x)         win32_more_core(x)
#define MORECORE_CONTIGUOUS 1 //(true)
#endif

// level of memmory debugging
//  -1= debug statistics and pattern fill also disabled (matters only for non rel configurations)
//   0= no debugging, fast native allocator
//   1= debug allocator, leak detector
//   2= debug allocator, leak detector with guards
//   3= debug allocator, leak detector with guards and fatal on 1st error
//   4= debug allocator, leak detector with guards and ud2 on 1st error
//   5= debug allocator, leak detector with guards and ud2 on 1st error; logging
#ifndef MEM_DEBUGALLOC
#define MEM_DEBUGALLOC 0
#endif

#if defined(_STD_RTL_MEMORY) && MEM_DEBUGALLOC > 0
#error cannot use STD RTL allocator with MEM_DEBUGALLOC > 0
#endif

#ifndef DEBUG_DLMALLOC_MODE
#define DEBUG_DLMALLOC_MODE (MEM_DEBUGALLOC > 0)
#endif

#define FOOTERS  DEBUG_DLMALLOC_MODE
#define INSECURE (!DEBUG_DLMALLOC_MODE)
#if MEM_DEBUGALLOC > 0
#define DEBUG 1
#endif

#define MSPACES       1 //(true)
#define USE_DL_PREFIX 1 //(true)
#if !(_TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3)
#define WIN32 1 //(true)
#endif

#define MALLOC_ALIGNMENT        16
#define USE_LOCKS               0 //(false)
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
{
#endif

  extern void *win32_more_core(intptr_t size);

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

#ifdef __cplusplus
}
#endif
