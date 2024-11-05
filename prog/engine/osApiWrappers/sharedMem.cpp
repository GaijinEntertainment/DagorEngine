// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sharedMem.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#include <malloc.h>

#define CVT_TO_U16(NM)                                                      \
  int NM##_slen = i_strlen(NM);                                             \
  wchar_t *NM##_u16 = (wchar_t *)alloca((NM##_slen + 1) * sizeof(wchar_t)); \
  NM##_u16[0] = '\0';                                                       \
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, NM, NM##_slen + 1, NM##_u16, NM##_slen + 1)

#elif _TARGET_APPLE | _TARGET_PC_LINUX
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#endif

void *create_global_map_shared_mem(const char *shared_mem_fname, void *base_addr, size_t sz, intptr_t &out_fd)
{
#if _TARGET_PC_WIN
  CVT_TO_U16(shared_mem_fname);
  HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (int)sz, shared_mem_fname_u16);
  out_fd = (intptr_t)hMapFile;
  if (!hMapFile)
    return NULL;

  void *resv = base_addr ? NULL : VirtualAlloc(NULL, 8 << 20, MEM_RESERVE, PAGE_READWRITE);
  void *p = MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, sz, base_addr);
  if (resv)
    VirtualFree(resv, 0, MEM_RELEASE);
  return p;
#elif _TARGET_APPLE | _TARGET_PC_LINUX
  char name[255];
  G_ASSERT(strchr(shared_mem_fname, '/') == NULL);
  SNPRINTF(name, sizeof(name), "/%s", shared_mem_fname);
  out_fd = shm_open(name, O_CREAT | O_RDWR, 0644);
  if (out_fd < 0)
    return NULL;
  if (ftruncate(out_fd, sz) < 0)
    return NULL;
  void *resv = mmap(0, 16 << 20, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  void *p = mmap(base_addr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, out_fd, 0);
  if (resv != MAP_FAILED)
    munmap(resv, 16 << 20);
  return p;
#else
  (void)(shared_mem_fname);
  (void)(base_addr);
  (void)(sz);
  (void)(out_fd);
  return NULL;
#endif
}

void *open_global_map_shared_mem(const char *shared_mem_fname, void *base_addr, size_t sz, intptr_t &out_fd)
{
#if _TARGET_PC_WIN
  CVT_TO_U16(shared_mem_fname);
  HANDLE hMapFile = OpenFileMappingW(FILE_MAP_WRITE, FALSE, shared_mem_fname_u16);
  out_fd = (intptr_t)hMapFile;
  if (!hMapFile)
    return NULL;
  return MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, sz, base_addr);
#elif _TARGET_APPLE | _TARGET_PC_LINUX
  char name[255];
  G_ASSERT(strchr(shared_mem_fname, '/') == NULL);
  SNPRINTF(name, sizeof(name), "/%s", shared_mem_fname);
  out_fd = shm_open(name, O_RDWR, 0644);
  if (out_fd < 0)
    return NULL;
  return mmap(base_addr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, out_fd, 0);
#else
  (void)(shared_mem_fname);
  (void)(base_addr);
  (void)(sz);
  (void)(out_fd);
  return NULL;
#endif
}

void close_global_map_shared_mem(intptr_t fd, void *addr, size_t sz)
{
#if _TARGET_PC_WIN
  if (addr)
    UnmapViewOfFile(addr);
  if (fd)
    CloseHandle((HANDLE)fd);
  (void)(sz);
#elif _TARGET_APPLE | _TARGET_PC_LINUX
  if (fd >= 0)
    close(fd);
  if (addr)
    munmap(addr, sz);
#else
  (void)(fd);
  (void)(addr);
  (void)(sz);
#endif
}

void mark_global_shared_mem_readonly(void *addr, size_t sz, bool read_only)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  DWORD old_prot;
  VirtualProtect(addr, sz, read_only ? PAGE_READONLY : PAGE_READWRITE, &old_prot);
#elif _TARGET_APPLE | _TARGET_PC_LINUX
  size_t pageMask = getpagesize() - 1;
  if (sz > pageMask)
  {
    void *alignedAddr = (void *)(((uintptr_t)addr + pageMask) & ~pageMask);
    size_t alignedSz = ((uintptr_t)addr + sz - (uintptr_t)alignedAddr) & ~pageMask;
    if (alignedSz > 0)
      mprotect(alignedAddr, alignedSz, read_only ? PROT_READ : PROT_READ | PROT_WRITE);
  }
#else
  (void)(addr);
  (void)(sz);
  (void)(read_only);
#endif
}

void unlink_global_shared_mem(const char *shared_mem_fname)
{
#if _TARGET_APPLE | _TARGET_PC_LINUX
  shm_unlink(shared_mem_fname);
#else
  (void)(shared_mem_fname);
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_sharedMem
#include <supp/exportPull.h>
