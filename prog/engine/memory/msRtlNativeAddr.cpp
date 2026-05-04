// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define DAGOR_MSRTL_DECL_CHUNK_HEADER_ONLY 1
#include <util/dag_compilerDefs.h>
#include "msRtlAlignedHelper.h"
#include <windows.h>

static inline bool is_address_readable(const void *addr)
{
  MEMORY_BASIC_INFORMATION mi;
  if (VirtualQuery(addr, &mi, sizeof(mi)))
    return mi.State == MEM_COMMIT && (mi.Protect & (PAGE_READONLY | PAGE_READWRITE)) && !(mi.Protect & (PAGE_GUARD | PAGE_NOACCESS));
  return false;
}

#if DAGOR_ADDRESS_SANITIZER
#if defined(_MSC_VER) && !defined(__clang__)
__declspec(no_sanitize_address)
#else
__attribute__((no_sanitize("address")))
#endif
#endif
bool is_address_natively_allocated(const void *addr)
{
  if (!addr)
    return true; // treat nullptr as native pointer
  auto *chunk_hdr = ((const MsRtlAllocatorAlignedChunkHdr *)addr) - 1;
  unsigned __int64 orig_addr = 0;
  if (!is_address_readable(chunk_hdr))
    return true;

  __try
  {
    orig_addr = chunk_hdr->origAddr;
    if (orig_addr != ~chunk_hdr->origAddrInv)
      return true;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return true;
  }
  return orig_addr > uintptr_t(chunk_hdr) || uintptr_t(chunk_hdr) >= orig_addr + 4096 || (orig_addr & (MALLOC_MIN_ALIGNMENT - 1));
}
