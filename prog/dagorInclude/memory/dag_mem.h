//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h> // memset
#if defined(_MSC_VER) && (defined(_TARGET_ARCH_X86_64) || defined(_TARGET_ARCH_X86))
#ifdef __clang__
#pragma push_macro("__lzcnt32")
#undef __lzcnt32
#include <intrin.h> // __stosd
#pragma pop_macro("__lzcnt32")
#else
extern "C" void __stosd(unsigned long *, unsigned long, size_t);
#pragma intrinsic(__stosd)
#endif
#endif

class IMemAlloc;

/// macro for defining member operators new and delete
#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_EXCEPTIONS_ENABLED)
#define DAG_DECLARE_NEW(mem)                                                         \
  inline void *operator new(size_t s) { return memalloc(s, mem); }                   \
  inline void *operator new[](size_t s) { return memalloc(s, mem); }                 \
  inline void *operator new(size_t s, IMemAlloc *a) { return memalloc(s, a); }       \
  inline void *operator new[](size_t s, IMemAlloc *a) { return memalloc(s, a); }     \
  inline void *__cdecl operator new(size_t /*s*/, void *p, int) { return (p); }      \
  inline void operator delete(void *p) { memfree_anywhere(p); }                      \
  inline void operator delete[](void *p) { memfree_anywhere(p); }                    \
  inline void operator delete(void *p, IMemAlloc * /*a*/) { memfree_anywhere(p); }   \
  inline void operator delete[](void *p, IMemAlloc * /*a*/) { memfree_anywhere(p); } \
  inline void __cdecl operator delete(void * /*d*/, void * /*p*/, int) {}
#else
#define DAG_DECLARE_NEW(mem)                                                     \
  inline void *operator new(size_t s) { return memalloc(s, mem); }               \
  inline void *operator new[](size_t s) { return memalloc(s, mem); }             \
  inline void *operator new(size_t s, IMemAlloc *a) { return memalloc(s, a); }   \
  inline void *operator new[](size_t s, IMemAlloc *a) { return memalloc(s, a); } \
  inline void *__cdecl operator new(size_t /*s*/, void *p, int) { return (p); }  \
  inline void operator delete(void *p) { memfree_anywhere(p); }                  \
  inline void operator delete[](void *p) { memfree_anywhere(p); }
#endif


#include <supp/dag_define_KRNLIMP.h>

/// analog of standard strdup()
KRNLIMP char *str_dup(const char *s, IMemAlloc *a);

/// fast memory fill with 4-byte pattern
__forceinline void memset_dw(void *p, unsigned dw, int dword_num)
{
  if (((unsigned char)dw) * 0x01010101u == dw)
    memset(p, dw, dword_num * sizeof(int));
  else
  {
#if defined(_MSC_VER) && (defined(_TARGET_ARCH_X86_64) || defined(_TARGET_ARCH_X86))
    __stosd((unsigned long *)p, (unsigned long)dw, dword_num);
#elif _TARGET_APPLE
    memset_pattern4(p, &dw, dword_num * sizeof(int));
#else
    for (unsigned *i = (unsigned *)p, *e = i + dword_num; i < e;)
      *i++ = dw;
#endif
  }
}

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


#include <supp/dag_undef_KRNLIMP.h>
