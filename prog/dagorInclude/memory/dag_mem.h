//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IMemAlloc;

/// macro for defining member operators new and delete
#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_EXCEPTIONS_ENABLED)
#define DAG_DECLARE_NEW(mem)                                           \
  inline void *operator new(size_t s)                                  \
  {                                                                    \
    return memalloc(s, mem);                                           \
  }                                                                    \
  inline void *operator new[](size_t s)                                \
  {                                                                    \
    return memalloc(s, mem);                                           \
  }                                                                    \
  inline void *operator new(size_t s, IMemAlloc *a)                    \
  {                                                                    \
    return memalloc(s, a);                                             \
  }                                                                    \
  inline void *operator new[](size_t s, IMemAlloc *a)                  \
  {                                                                    \
    return memalloc(s, a);                                             \
  }                                                                    \
  inline void *__cdecl operator new(size_t /*s*/, void *p, int)        \
  {                                                                    \
    return (p);                                                        \
  }                                                                    \
  inline void operator delete(void *p)                                 \
  {                                                                    \
    memfree_anywhere(p);                                               \
  }                                                                    \
  inline void operator delete[](void *p)                               \
  {                                                                    \
    memfree_anywhere(p);                                               \
  }                                                                    \
  inline void operator delete(void *p, IMemAlloc * /*a*/)              \
  {                                                                    \
    memfree_anywhere(p);                                               \
  }                                                                    \
  inline void operator delete[](void *p, IMemAlloc * /*a*/)            \
  {                                                                    \
    memfree_anywhere(p);                                               \
  }                                                                    \
  inline void __cdecl operator delete(void * /*d*/, void * /*p*/, int) \
  {}
#else
#define DAG_DECLARE_NEW(mem)                                    \
  inline void *operator new(size_t s)                           \
  {                                                             \
    return memalloc(s, mem);                                    \
  }                                                             \
  inline void *operator new[](size_t s)                         \
  {                                                             \
    return memalloc(s, mem);                                    \
  }                                                             \
  inline void *operator new(size_t s, IMemAlloc *a)             \
  {                                                             \
    return memalloc(s, a);                                      \
  }                                                             \
  inline void *operator new[](size_t s, IMemAlloc *a)           \
  {                                                             \
    return memalloc(s, a);                                      \
  }                                                             \
  inline void *__cdecl operator new(size_t /*s*/, void *p, int) \
  {                                                             \
    return (p);                                                 \
  }                                                             \
  inline void operator delete(void *p)                          \
  {                                                             \
    memfree_anywhere(p);                                        \
  }                                                             \
  inline void operator delete[](void *p)                        \
  {                                                             \
    memfree_anywhere(p);                                        \
  }
#endif


#include <supp/dag_define_COREIMP.h>

/// analog of standard strdup()
KRNLIMP char *str_dup(const char *s, IMemAlloc *a);

/// fast memory fill with 4-byte pattern
KRNLIMP void memset_dw(void *p, int dw, int dword_num);

/// return max memory used thus far
KRNLIMP size_t get_max_mem_used();

/// fills buffer with statistic on memory usage
KRNLIMP void get_memory_stats(char *buf, int buflen);

/// secure_zero_memory guaranteed to zero memory location
/// (the compiler can optimize away a call to memset(dst, 0, size),
/// if it determines that the caller does not access that memory range again)
__forceinline void *secure_zero_memory(void *ptr, size_t cnt)
{
  volatile char *vptr = (volatile char *)ptr;
  while (cnt)
  {
    *vptr = 0;
    vptr++;
    cnt--;
  }
  return ptr;
}


#include <supp/dag_undef_COREIMP.h>
