// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_virtualMem.h>
#include <util/dag_globDef.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#define DAG_HAVE_MMAP 1

void *os_virtual_mem_alloc(size_t sz)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  return VirtualAlloc(NULL, sz, MEM_RESERVE, PAGE_READWRITE);
#elif DAG_HAVE_MMAP
  return mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
  G_UNREFERENCED(sz);
  return NULL; // not implemented
#endif
}

bool os_virtual_mem_free(void *ptr, size_t sz)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  G_UNREFERENCED(sz);
  return VirtualFree(ptr, 0, MEM_RELEASE);
#elif DAG_HAVE_MMAP
  return munmap(ptr, sz) == 0;
#else
  G_UNREFERENCED(ptr);
  G_UNREFERENCED(sz);
  return false; // not implemented
#endif
}

bool os_virtual_mem_protect(void *ptr_region_start, size_t region_sz, unsigned prot_flags)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  DWORD old_prot_flags, flg = PAGE_NOACCESS;
  switch (prot_flags)
  {
    case VM_PROT_READ: flg = PAGE_READONLY; break;
    case VM_PROT_READ | VM_PROT_WRITE:
    case VM_PROT_WRITE: flg = PAGE_READWRITE; break;
    case VM_PROT_EXEC: flg = PAGE_EXECUTE; break;
    case VM_PROT_EXEC | VM_PROT_READ: flg = PAGE_EXECUTE_READ; break;
    case VM_PROT_EXEC | VM_PROT_WRITE:
    case VM_PROT_EXEC | VM_PROT_READ | VM_PROT_WRITE: flg = PAGE_EXECUTE_READWRITE; break;
    default: flg = PAGE_NOACCESS;
  }
  return VirtualProtect(ptr_region_start, region_sz, flg, &old_prot_flags);
#elif DAG_HAVE_MMAP
  unsigned flg = 0;
  if (prot_flags & VM_PROT_EXEC)
    flg |= PROT_EXEC;
  if (prot_flags & VM_PROT_READ)
    flg |= PROT_READ;
  if (prot_flags & VM_PROT_WRITE)
    flg |= PROT_WRITE;
  return mprotect(ptr_region_start, region_sz, flg) == 0;
#else
  G_UNREFERENCED(ptr_region_start);
  G_UNREFERENCED(region_sz);
  G_UNREFERENCED(prot_flags);
  return false; // not implemented
#endif
}

bool os_virtual_mem_commit(void *ptr_region_start, size_t region_sz)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  return VirtualAlloc(ptr_region_start, region_sz, MEM_COMMIT, PAGE_READWRITE) == ptr_region_start;
#elif DAG_HAVE_MMAP
  return ptr_region_start && region_sz;
#else
  G_UNREFERENCED(ptr_region_start);
  G_UNREFERENCED(region_sz);
  return false; // not implemented
#endif
}

bool os_virtual_mem_decommit(void *ptr_region_start, size_t region_sz)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  return VirtualFree(ptr_region_start, region_sz, MEM_DECOMMIT);
#elif _TARGET_APPLE
  return madvise(ptr_region_start, region_sz, MADV_FREE) == 0;
#elif DAG_HAVE_MMAP
  return madvise(ptr_region_start, region_sz, MADV_DONTNEED) == 0;
#else
  G_UNREFERENCED(ptr_region_start);
  G_UNREFERENCED(region_sz);
  return false; // not implemented
#endif
}

bool os_virtual_mem_reset(void *ptr_region_start, size_t region_sz)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  return VirtualAlloc(ptr_region_start, region_sz, MEM_RESET, PAGE_READWRITE) == ptr_region_start;
#elif DAG_HAVE_MMAP
  return os_virtual_mem_decommit(ptr_region_start, region_sz);
#else
  G_UNREFERENCED(ptr_region_start);
  G_UNREFERENCED(region_sz);
  return false; // not implemented
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_virtualMem
#include <supp/exportPull.h>