//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stddef.h>

// pseudo-SAL definitiona for targets that don't support them
#if defined(_Ret_opt_bytecap_)
#define SAL_BCOUNT _Ret_bytecap_
#elif defined(__bcount)
#define SAL_BCOUNT __bcount
#else
#define SAL_BCOUNT(x)
#endif

#ifdef DAGOR_PREFER_HEAP_ALLOCATION
#elif defined(__clang__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define DAGOR_PREFER_HEAP_ALLOCATION 1
#endif
#elif defined(__GNUC__) && defined(__SANITIZE_ADDRESS__)
#define DAGOR_PREFER_HEAP_ALLOCATION 1
#endif

#ifndef __B_NO_MEM_BASE_INCLUDE
#ifdef __cplusplus

//
/// general memory allocation interface
//
class IMemAlloc
{
public:
  /// destroy this memory allocator
  virtual void destroy() = 0;

  /// returns true if allocator is empty (no more memory to allocate)
  virtual bool isEmpty() = 0;

  /// returns size of block
  /// @param p pointer returned from alloc/tryAlloc/realloc
  /// @note NULL is valid input; size of NULL = 0
  virtual size_t getSize(void *p) = 0;

  /**
    allocate memory.
    calls fatal on allocation error.
    @param sz size of requested block, in bytes
    @return pointer to allocated block or NULL on error
  */
  virtual void *alloc(size_t sz) = 0;

  /**
    fault-tolerant allocate memory.
    returns NULL on allocation error;
    @param sz size of requested block, in bytes
    @return pointer to allocated block or NULL on error
  */
  virtual void *tryAlloc(size_t sz) = 0;

  /**
    allocate aligned memory.
    calls fatal on allocation error.
    @param sz size of requested block, in bytes
    @param alignment of requested block, in bytes. If 0 - page size used. 16 - is default alloc alignment, don't bother to call with 16
    @return pointer to aligned block of memory
  */
  virtual void *allocAligned(size_t sz, size_t alignment) = 0;

  /**
    tries to resize block without moving it.
    @param sz new (expanded/shrinked) block size, in bytes
    @return returns true if succesfully expanded/shrinked or false when cannot expand
  */
  virtual bool resizeInplace(void *p, size_t sz) = 0;

  /**
    resize if possible, if not, alloc new, copy old to new, and free old block.
    Uses getsize().
    @return on error, returns NULL, but original block is preserved
  */
  virtual void *realloc(void *p, size_t sz) = 0;

  /// free previously allocated block.
  /// @param p pointer returned from alloc/tryAlloc/realloc
  /// @note NULL is valid input (it just does nothing)
  virtual void free(void *p) = 0;

  /// free block previously allocated with allocAligned()
  /// @param p pointer returned from allocAligned
  /// @note NULL is valid input (it just does nothing)
  virtual void freeAligned(void *p) = 0;

  //
  // inline versions of allocation functions that return size of allocated block
  //
  inline void *alloc(size_t sz, size_t *asize)
  {
    void *p = alloc(sz);
    *asize = sz;
    return p;
  }

  inline void *tryAlloc(size_t sz, size_t *asize)
  {
    void *p = tryAlloc(sz);
    *asize = sz;
    return p;
  }

  inline void *allocAligned(size_t sz, size_t alignment, size_t *asize)
  {
    void *p = allocAligned(sz, alignment);
    *asize = sz;
    return p;
  }

  inline bool resizeInplace(void *p, size_t sz, size_t *asize)
  {
    bool ret = resizeInplace(p, sz);
    *asize = getSize(p);
    if (!*asize)
      *asize = sz;
    return ret;
  }

  inline void *realloc(void *p, size_t sz, size_t *asize)
  {
    p = realloc(p, sz);
    *asize = sz;
    return p;
  }
};


#include <supp/dag_define_COREIMP.h>

#define DAG_MEMBASE_CONCAT(a, b) a##b
#define DECLARE_DELETER(alloc)              \
  struct DAG_MEMBASE_CONCAT(alloc, Deleter) \
  {                                         \
    template <typename T>                   \
    void operator()(T *ptr) const           \
    {                                       \
      if (ptr)                              \
        ptr->~T();                          \
      alloc##_ptr()->free(ptr);             \
    }                                       \
  }

//
/// Global allocators
//
#define DECL_MEM(name)            \
  extern KRNLIMP IMemAlloc *name; \
  inline IMemAlloc *name##_ptr()  \
  {                               \
    return name;                  \
  }                               \
  DECLARE_DELETER(name)

DECL_MEM(stdmem);    //< standard c/c++ memory allocator
DECL_MEM(scriptmem); //< memory for scripts objects
DECL_MEM(strmem);    //< memory for strings objects
DECL_MEM(tmpmem);    //< memory for small temporary objects
DECL_MEM(inimem);    //< memory for objects allocated on init and released on shutdown
DECL_MEM(midmem);    //< memory for objects that remain in memory for significant time
DECL_MEM(uimem);     //< memory for UI-related objects
DECL_MEM(globmem);
DECL_MEM(defaultmem); //< memory used by default (in operator new)

#undef DECL_MEM
#undef DECLARE_DELETER
#undef DAG_MEMBASE_CONCAT


/// C-style functions to provide allocation/deallocation
__forceinline void *memalloc(size_t sz, IMemAlloc *a) { return a->alloc(sz); }
__forceinline void *memalloc(size_t sz, IMemAlloc *a, size_t *asz) { return a->alloc(sz, asz); }

/// special helper to allocate memory for typed array of 'cnt' items
template <class T>
inline T *memalloc_typed(size_t cnt, IMemAlloc *a)
{
  return (T *)a->alloc(cnt * sizeof(T));
}

__forceinline void memfree(void *p, IMemAlloc *a) { a->free(p); }
__forceinline void memfree_aligned(void *p, IMemAlloc *a) { a->freeAligned(p); }
extern "C" KRNLIMP void memfree_anywhere(void *p);

/// special helper to delete C++ object using specified allocator
template <class T>
inline void memdelete(T *p, IMemAlloc *a)
{
  p->~T();
  a->free(p);
}
template <class T>
inline void memdelete_aligned(T *p, IMemAlloc *a)
{
  p->~T();
  a->freeAligned(p);
}

/// overloaded operators new/delete
#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | defined(__clang__)
#include <new>
extern "C" KRNLIMP void *memalloc(size_t sz);
#if defined(__clang__) && ((((__clang_major__ << 8) | __clang_minor__) >= 0x501) && _TARGET_APPLE || \
                            (((__clang_major__ << 8) | __clang_minor__) >= 0x303)) // Apple clang 5.1 based on LLVM clang 3.3
// from clang 5.1.0 these overrides must be in CPP
#else
#if (__cplusplus >= 201703L) // c++17
__forceinline void *operator new(std::size_t s) noexcept(false) { return memalloc(s); }
__forceinline void *operator new[](std::size_t s) noexcept(false) { return memalloc(s); }
__forceinline void operator delete(void *p) noexcept { memfree_anywhere(p); }
__forceinline void operator delete[](void *p) noexcept { memfree_anywhere(p); }
__forceinline void operator delete(void *p, size_t) noexcept { memfree_anywhere(p); }
__forceinline void operator delete[](void *p, size_t) noexcept { memfree_anywhere(p); }
#else
__forceinline void *operator new(std::size_t s) throw(std::bad_alloc) { return memalloc(s); }
__forceinline void *operator new[](std::size_t s) throw(std::bad_alloc) { return memalloc(s); }
__forceinline void operator delete(void *p) throw() { memfree_anywhere(p); }
__forceinline void operator delete[](void *p) throw() { memfree_anywhere(p); }
__forceinline void operator delete(void *p, size_t) throw() { memfree_anywhere(p); }
__forceinline void operator delete[](void *p, size_t) throw() { memfree_anywhere(p); }
#endif // c++17
#endif
#else
__forceinline void *memalloc(size_t sz) { return defaultmem->alloc(sz); }
__forceinline void *__cdecl operator new(size_t s) { return memalloc(s); }
SAL_BCOUNT(_Size) __forceinline void *__cdecl operator new[](size_t _Size) { return memalloc(_Size); }

__forceinline void __cdecl operator delete(void *p) { memfree_anywhere(p); }
__forceinline void __cdecl operator delete[](void *p) { memfree_anywhere(p); }

__forceinline void __cdecl operator delete(void *p, size_t) { memfree_anywhere(p); }
__forceinline void __cdecl operator delete[](void *p, size_t) { memfree_anywhere(p); }
#endif

#if !defined(__clang__) && (__cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703))
#include <new>
__forceinline void *__cdecl operator new(std::size_t sz, std::align_val_t al) { return defaultmem->allocAligned(sz, (size_t)al); }
__forceinline void *__cdecl operator new[](std::size_t sz, std::align_val_t al) { return defaultmem->allocAligned(sz, (size_t)al); }
__forceinline void __cdecl operator delete(void *ptr, std::align_val_t) noexcept { return defaultmem->freeAligned(ptr); }
__forceinline void __cdecl operator delete[](void *ptr, std::align_val_t) noexcept { return defaultmem->freeAligned(ptr); }
#endif

__forceinline void *__cdecl operator new(size_t sz, IMemAlloc *a) { return memalloc(sz, a); }
__forceinline void *__cdecl operator new[](size_t sz, IMemAlloc *a) { return memalloc(sz, a); }

/// special inplace new/delete operators (used in dyntab)
static constexpr int _NEW_INPLACE = 1;
__forceinline void *__cdecl operator new(size_t /*sz*/, void *ptr, int /*dummy*/) { return (ptr); }

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_EXCEPTIONS_ENABLED)
__forceinline void __cdecl operator delete(void *p, IMemAlloc *a) { a->free(p); }
__forceinline void __cdecl operator delete[](void *p, IMemAlloc *a) { a->free(p); }

// in-place delete is needed only when exception handling is on
__forceinline void __cdecl operator delete(void * /*d*/, void * /*ptr*/, int /*dummy*/) {}
#endif

// default C style pool operating
extern "C" KRNLIMP void *memalloc_default(size_t sz);
extern "C" KRNLIMP void memfree_default(void *);
extern "C" KRNLIMP void *memrealloc_default(void *, size_t sz);
extern "C" KRNLIMP void *memcalloc_default(size_t, size_t);
extern "C" KRNLIMP int memresizeinplace_default(void *, size_t sz);

#include <supp/dag_undef_COREIMP.h>

#else //__cplusplus

#include <supp/dag_define_COREIMP.h>
// default C pool operating
KRNLIMP void *memalloc_default(size_t sz);
KRNLIMP void memfree_default(void *);
KRNLIMP void *memrealloc_default(void *, size_t sz);
KRNLIMP void *memcalloc_default(size_t, size_t);
KRNLIMP int memresizeinplace_default(void *, size_t sz);
#include <supp/dag_undef_COREIMP.h>

#endif //__cplusplus

#else
// disabling compilation of new/delete code for C++
#ifndef __cplusplus
#define new :.:
#define delete :.:
#endif
#endif //__B_NO_MEM_BASE_INCLUDE
