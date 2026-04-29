// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if (_TARGET_PC_WIN | _TARGET_XBOX) && USE_RTL_ALLOC
#include <windows.h>
#undef WIN32
#include "mallocMinAlignment.h"
#if !DAGOR_ADDRESS_SANITIZER
extern "C" __declspec(noinline) void *__cdecl _expand_base(void *const block, size_t const size);
#define MSIZE_BASE  _msize_base
#define EXPAND_BASE _expand_base
#else
#define MSIZE_BASE  _msize
#define EXPAND_BASE _expand
#endif

struct MsRtlAllocatorAlignedChunkHdr
{
  uintptr_t origAddr;
  uintptr_t origAddrInv;
};

static inline void *dagmem_mark_aligned_chunk(void *aligned_ptr, void *orig_ptr)
{
  auto *chunk_hdr = ((MsRtlAllocatorAlignedChunkHdr *)aligned_ptr) - 1;
  chunk_hdr->origAddr = uintptr_t(orig_ptr);
  chunk_hdr->origAddrInv = ~uintptr_t(orig_ptr);
  return aligned_ptr;
}
#if DAGOR_ADDRESS_SANITIZER
#if defined(_MSC_VER) && !defined(__clang__)
__declspec(no_sanitize_address)
#else
__attribute__((no_sanitize("address")))
#endif
#endif
static inline bool
dagmem_is_marked_aligned_chunk(const MsRtlAllocatorAlignedChunkHdr &ch)
{
  if (ch.origAddr != ~ch.origAddrInv)
    return false;
  return ch.origAddr <= uintptr_t(&ch) && uintptr_t(&ch) < ch.origAddr + 4096 && !(ch.origAddr & (MALLOC_MIN_ALIGNMENT - 1));
}
static inline void dagmem_clear_aligned_chunk_mark(MsRtlAllocatorAlignedChunkHdr *chunk_hdr)
{
  chunk_hdr->origAddr = 0;
  chunk_hdr->origAddrInv = 0;
}

extern bool is_address_natively_allocated(const void *addr);

#if DAGOR_ADDRESS_SANITIZER
#if defined(_MSC_VER) && !defined(__clang__)
__declspec(no_sanitize_address)
#else
__attribute__((no_sanitize("address")))
#endif
#endif
static inline const MsRtlAllocatorAlignedChunkHdr *
dagmem_is_aligned_chunk_min_guarded(const void *p)
{
  if (!p)
    return nullptr;
  auto *chunk_hdr = ((const MsRtlAllocatorAlignedChunkHdr *)p) - 1;
  return dagmem_is_marked_aligned_chunk(*chunk_hdr) ? chunk_hdr : nullptr;
}
static inline const MsRtlAllocatorAlignedChunkHdr *dagmem_is_aligned_chunk_full_guarded(const void *p)
{
  if ((uintptr_t(p) & 0xFFF) == 0 && is_address_natively_allocated(p)) // 4K boundary
    return nullptr;
  return dagmem_is_aligned_chunk_min_guarded(p);
}

#if DAGOR_DBGLEVEL <= 0 && MALLOC_MIN_ALIGNMENT >= 16
// fast path for release where we allow only allocAligned()/freeAligned()/getSize() for aligned pointers
#define DAGMEM_IS_ALIGNED_CHUNK_MIN_GUARDED(P) dagmem_is_aligned_chunk_min_guarded(P)
static inline const MsRtlAllocatorAlignedChunkHdr *dagmem_is_aligned_chunk(const void *) { return nullptr; }
#else
#define DAGMEM_IS_ALIGNED_CHUNK_MIN_GUARDED(P) dagmem_is_aligned_chunk_full_guarded(P)
static inline const auto *dagmem_is_aligned_chunk(const void *p) { return dagmem_is_aligned_chunk_full_guarded(p); }
#endif

#ifndef DAGOR_MSRTL_DECL_CHUNK_HEADER_ONLY
static inline void *dagmem_get_aligned_chunk_base_ptr(const MsRtlAllocatorAlignedChunkHdr &ch) { return (void *)ch.origAddr; }
static inline uintptr_t dagmem_get_aligned_chunk_base_ofs(const MsRtlAllocatorAlignedChunkHdr &ch)
{
  return uintptr_t(&ch + 1) - ch.origAddr;
}
static inline size_t dagmem_get_aligned_chunk_size(const MsRtlAllocatorAlignedChunkHdr &ch)
{
  return MSIZE_BASE((void *)ch.origAddr) - (uintptr_t(&ch + 1) - ch.origAddr);
}

static inline size_t dagmem_get_aligned_chunk_size(void *p)
{
  auto *chunk_hdr = dagmem_is_aligned_chunk_full_guarded(p);
  return !chunk_hdr ? MSIZE_BASE(p) : dagmem_get_aligned_chunk_size(*chunk_hdr);
}

static inline size_t dagmem_malloc_usable_size(void *p) { return p ? dagmem_get_aligned_chunk_size(p) : 0; }
static inline void dagmem_free_base(void *p)
{
  if (!p)
    return;
  if (auto *chunk_hdr = dagmem_is_aligned_chunk(p)) // -V547
  {
    p = dagmem_get_aligned_chunk_base_ptr(*chunk_hdr);
    dagmem_clear_aligned_chunk_mark(const_cast<MsRtlAllocatorAlignedChunkHdr *>(chunk_hdr));
  }
  _free_base(p);
}

static inline void *dagmem_memalign_base(size_t alignment, size_t sz)
{
#if DAGOR_DBGLEVEL > 0 // to avoid mixing native and aligned pointers in release
  if (alignment <= MALLOC_MIN_ALIGNMENT)
    return _malloc_base(sz);
#endif
  if (alignment > 4096)
    return nullptr;
  size_t alignment_m = ~(alignment - 1);
#if DAGOR_DBGLEVEL > 0 && MALLOC_MIN_ALIGNMENT < 16
  size_t raw_size = sz + alignment - MALLOC_MIN_ALIGNMENT;
  void *p = _malloc_base(raw_size);
  if ((uintptr_t(p) & ~alignment_m) == 0)
    return p;
  return dagmem_mark_aligned_chunk((void *)((uintptr_t(p) + alignment - 1) & alignment_m), p);
#else
  size_t raw_size = sz + alignment - MALLOC_MIN_ALIGNMENT + sizeof(MsRtlAllocatorAlignedChunkHdr);
  void *p = _malloc_base(raw_size);
  if (!p)
    return nullptr;
  return dagmem_mark_aligned_chunk((void *)((uintptr_t(p) + sizeof(MsRtlAllocatorAlignedChunkHdr) + alignment - 1) & alignment_m), p);
#endif
}

#if MALLOC_MIN_ALIGNMENT >= 16
static inline void *dagmem_malloc_base(size_t sz) { return _malloc_base(sz); }
#else
static inline void *dagmem_malloc_base(size_t sz) { return dagmem_memalign_base(16, sz); }
#endif

inline void *dagmem_calloc_base(size_t elem_count, size_t elem_sz) { return _calloc_base(elem_count, elem_sz); }

static inline void *dagmem_realloc_base(void *p, size_t sz)
{
  if (!p)
    return dagmem_malloc_base(sz);
  auto *chunk_hdr = dagmem_is_aligned_chunk(p);
  if (!chunk_hdr) // -V547
    if ((uintptr_t(p = _realloc_base(p, sz)) & (MALLOC_MIN_ALIGNMENT - 1)) == 0 /* check native alignment */)
      return p;

  // for non-aligned native pointer to reallocation via malloc-copy-free
  if (void *r = dagmem_malloc_base(sz))
  {
    size_t psz = dagmem_malloc_usable_size(p);
    memcpy(r, p, (sz < psz) ? sz : psz);
    dagmem_free_base(p);
    return r;
  }
  return nullptr;
}

static inline void *dagmem_expand_base(void *p, size_t sz)
{
  auto *chunk_hdr = dagmem_is_aligned_chunk(p);
  if (!chunk_hdr) // -V547
    return EXPAND_BASE(p, sz);
  return EXPAND_BASE(dagmem_get_aligned_chunk_base_ptr(*chunk_hdr), sz + dagmem_get_aligned_chunk_base_ofs(*chunk_hdr));
}

static inline void *dagmem_get_effective_ptr(void *p, bool surely_native_ptr, bool clr_chunk_hdr = true)
{
  if (DAGOR_UNLIKELY(!surely_native_ptr))
    if (auto *chunk_hdr = DAGMEM_IS_ALIGNED_CHUNK_MIN_GUARDED(p); DAGOR_UNLIKELY(chunk_hdr))
    {
      void *eff_p = dagmem_get_aligned_chunk_base_ptr(*chunk_hdr);
      if (clr_chunk_hdr)
        dagmem_clear_aligned_chunk_mark(const_cast<MsRtlAllocatorAlignedChunkHdr *>(chunk_hdr));
      return eff_p;
    }
  return p;
}
#if DAGOR_DBGLEVEL > 0
static inline void *dagmem_get_effective_ptr_and_size(void *p, bool surely_native_ptr, size_t &asz, bool clr_chunk_hdr = true)
{
  if (DAGOR_UNLIKELY(!surely_native_ptr))
    if (auto *chunk_hdr = dagmem_is_aligned_chunk(p); DAGOR_UNLIKELY(chunk_hdr))
    {
      asz = dagmem_get_aligned_chunk_size(*chunk_hdr);
      void *eff_p = dagmem_get_aligned_chunk_base_ptr(*chunk_hdr);
      if (clr_chunk_hdr)
        dagmem_clear_aligned_chunk_mark(const_cast<MsRtlAllocatorAlignedChunkHdr *>(chunk_hdr));
      return eff_p;
    }
  asz = p ? MSIZE_BASE(p) : 0;
  return p;
}
#endif

static inline size_t dagmem_msize_base(void *p) { return MSIZE_BASE(p); }

#define DAGMEM_GET_ALLOCATED_USABLE_SIZE(PTR, AL, SZ) \
  ((PTR) ? ((AL) <= MALLOC_MIN_ALIGNMENT ? max<size_t>(SZ, 1u) : dagmem_get_aligned_chunk_size(PTR)) : 0)
#define DAGMEM_GET_USABLE_SIZE(PTR, AL) \
  ((PTR) ? ((AL) <= MALLOC_MIN_ALIGNMENT ? dagmem_msize_base(PTR) : dagmem_get_aligned_chunk_size(PTR)) : 0)

#define DAGMEM_DECL_EFF_PTR_AND_CLRHDR(VAR, PTR, MAX_AL) void *VAR = dagmem_get_effective_ptr(PTR, (MAX_AL) <= MALLOC_MIN_ALIGNMENT)
#define DAGMEM_DECL_EFF_PTR_WITH_SIZE_AND_CLRHDR(VAR, PTR, MAX_AL, OUT_SZ) \
  void *VAR = dagmem_get_effective_ptr_and_size(PTR, (MAX_AL) <= MALLOC_MIN_ALIGNMENT, OUT_SZ)
#define DAGMEM_FREE_NATIVE(PTR) (PTR ? _free_base(PTR) : (void)0)

#define DAGMEM_SHOULD_USE_TOTAL_ALLOC_FROM_CRT 1

#undef MSIZE_BASE
#undef EXPAND_BASE
#endif

#endif
