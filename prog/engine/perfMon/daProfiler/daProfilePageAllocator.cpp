// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerDefines.h"
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#if _TARGET_XBOX
#include "daProfilePageAllocator_xbox.h"
#endif
namespace da_profiler
{
void *allocate_page(size_t bytes, size_t)
{
#if _TARGET_XBOX
  if (void *p = xbox_allocate_page(bytes))
    return p;
#endif
  return VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
void free_page(void *p, size_t) { VirtualFree(p, 0, MEM_RELEASE); }
} // namespace da_profiler
#else
namespace da_profiler
{
void *allocate_page(size_t bytes, size_t) { return new char[bytes]; }
void free_page(void *p, size_t) { delete[] ((char *)p); }
} // namespace da_profiler
#endif
