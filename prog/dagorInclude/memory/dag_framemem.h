//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_assert.h>
#include <supp/dag_define_COREIMP.h>

#if _TARGET_STATIC_LIB && !DAGOR_PREFER_HEAP_ALLOCATION
extern thread_local class IMemAlloc *thread_framemem;
inline IMemAlloc *framemem_ptr() { return thread_framemem; }
#else
KRNLIMP IMemAlloc *framemem_ptr();
#endif

KRNLIMP void reset_framemem(); // resets for current thread

KRNLIMP IMemAlloc *alloc_thread_framemem(size_t area_size = 0); // 0 - means default
KRNLIMP void free_thread_framemem();
struct RAIIThreadFramememAllocator
{
  RAIIThreadFramememAllocator(size_t area_size = 0) { alloc_thread_framemem(area_size); }
  ~RAIIThreadFramememAllocator() { free_thread_framemem(); }
};

struct FramememState
{
  char *cur_ptr = nullptr, *lastBlock = nullptr;
#if DAGOR_DBGLEVEL > 0
  char *lastLiberatorPtr = nullptr;
#endif
};
class FramememScopedRegion
{
public:
  KRNLIMP FramememScopedRegion();
  KRNLIMP ~FramememScopedRegion();

private:
  FramememState p;
  FramememScopedRegion(const FramememScopedRegion &) = delete;
  FramememScopedRegion &operator=(const FramememScopedRegion &) = delete;
};

#ifndef __CONCAT_HELPER__
#define __CONCAT_HELPER__(x, y) x##y
#define __CONCAT__(x, y)        __CONCAT_HELPER__(x, y)
#endif
#define FRAMEMEM_REGION FramememScopedRegion __CONCAT__(scope_framemem_lib, __COUNTER__)

#if DAGOR_DBGLEVEL > 0
class FramememScopedValidator
{
public:
  KRNLIMP FramememScopedValidator(const char *in_file, uint32_t in_line);
  KRNLIMP ~FramememScopedValidator();

private:
  const char *savedLastBlock;
  const char *file;
  uint32_t line;
  FramememScopedValidator(const FramememScopedValidator &) = delete;
  FramememScopedValidator &operator=(const FramememScopedValidator &) = delete;
};

// Validates whether all lifetimes nest within a scope and that the framemem stack is cleaned up gracefully
#define FRAMEMEM_VALIDATE FramememScopedValidator __CONCAT__(scope_framemem_validator, __COUNTER__)(__FILE__, __LINE__)
#else
struct FramememScopedValidator
{};
#define FRAMEMEM_VALIDATE ((void)0)
#endif

struct framememDeleter
{
  template <typename T>
  void operator()(T *ptr) const
  {
    if (ptr)
      ptr->~T();
    framemem_ptr()->free(ptr);
  }
};

// EASTL-compatible allocator interface.
// Intentionally doesn't have state to avoid occupy space in container's instances.
// WARNING: transferring objects that use this allocator between threads is
//          COMPLETELY UNSUPPORTED. Don't do it, ever.
class framemem_allocator
{
public:
  explicit framemem_allocator(const char * = NULL) {}
  framemem_allocator(const framemem_allocator &) {}
  framemem_allocator(const framemem_allocator &, const char *) {}

  friend __forceinline bool operator==(framemem_allocator, framemem_allocator) { return true; }
  friend __forceinline bool operator!=(framemem_allocator, framemem_allocator) { return false; }

  static inline void *allocate(size_t n, int /*flags*/ = 0) { return framemem_ptr()->alloc(n); }
  static inline void *allocate(size_t n, size_t al, size_t, int = 0)
  {
    G_FAST_ASSERT(al <= 16);
    return framemem_ptr()->allocAligned(n, al);
  }
  static inline void deallocate(void *p, size_t) { framemem_ptr()->free(p); }
  static inline bool resizeInplace(void *p, size_t sz) { return framemem_ptr()->resizeInplace(p, sz); }
  static inline void *realloc(void *p, size_t sz) { return framemem_ptr()->realloc(p, sz); }
  static constexpr size_t min_expand_size = 0;

#if DAGOR_DBGLEVEL > 0
  const char *get_name() const { return "framemem_allocator"; }
#else
  const char *get_name() const { return ""; }
#endif
  void set_name(const char *) {}

  static inline IMemAlloc *getMem() { return framemem_ptr(); }
  static inline void *alloc(int sz) { return framemem_ptr()->alloc(sz); }
  static inline void free(void *p) { return framemem_ptr()->free(p); }
};

#include <supp/dag_undef_COREIMP.h>
