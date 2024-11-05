// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>

// TODO: mmap is deprecated on PS5 and it is suggested to use APR instead
#if _TARGET_C3 | _TARGET_C2

#elif _TARGET_C1

#else
#define DAG_MMAP_THRESHOLD -1 // Always
#endif

#if DAG_MMAP_THRESHOLD
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#include <io.h>
#include <intsafe.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#else
#include <stdlib.h> // malloc, free
#endif
#include <osApiWrappers/dag_vromfs.h>
#include "romFileReader.h"
#if _TARGET_C1

#endif

#if (_TARGET_PC_WIN | _TARGET_XBOX) & DAG_MMAP_THRESHOLD
static int getpagesize()
{
  SYSTEM_INFO sysi;
  GetSystemInfo(&sysi);
  return sysi.dwAllocationGranularity;
}
#endif
#define ALLOC_MASK (~(getpagesize() - 1))

const void *df_mmap(file_ptr_t fp, int *flen, int length, int offset)
{
  if (!fp)
    return NULL;
  int base = 0, len = 0;
  flen = flen ? flen : &len;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
  {
    FILE *f = NULL;
    void *ret = rr->mmap(&base, offset, flen, &f);
    if (!f)
      return ret;
    else
      fp = (file_ptr_t)f;
  }
  else
    *flen = df_length(fp);
  if (*flen <= 0 || offset >= *flen) // empty files do not mmaps
    return NULL;
  if (length == 0)
    length = *flen;
  if ((offset + length) > *flen)
    length = *flen - offset;
#if DAG_MMAP_THRESHOLD
  uint64_t pa_offs = (base + offset) & ALLOC_MASK;
  int pa_diff = (base + offset) - pa_offs;
#if _TARGET_C1

#else
  int fd = fileno((FILE *)fp);
#endif
#if _TARGET_PC_WIN | _TARGET_XBOX
  HANDLE hmap = CreateFileMappingW((HANDLE)_get_osfhandle(fd), NULL, PAGE_READONLY, 0, 0, NULL);
  if (!hmap)
    return NULL;
  void *ret = MapViewOfFileEx(hmap, FILE_MAP_READ, HIDWORD(pa_offs), LODWORD(pa_offs), length + pa_diff, NULL);
  CloseHandle(hmap);
  return ret ? (char *)ret + pa_diff : NULL;
#else
  if (DAG_MMAP_THRESHOLD < 0 || length < DAG_MMAP_THRESHOLD)
  {
    void *ret = mmap(NULL, (size_t)(length + pa_diff), PROT_READ, MAP_SHARED, fd, (off_t)pa_offs);
    if (ret == MAP_FAILED)
      ret = NULL;
    return ret ? (char *)ret + pa_diff : NULL;
  }
#endif
#endif
  void *mem = malloc(length); //-V779
  if (!mem)
    return NULL;
  if (offset)
    df_seek_to(fp, offset);
  if (df_read(fp, mem, length) != length)
  {
    free(mem);
    return NULL;
  }
  return mem;
}

void df_unmap(const void *start, int length)
{
  (void)length;
  if (!start || RomFileReader::munmap(start))
    return;
#if DAG_MMAP_THRESHOLD
  void *pa_start = (void *)(((uintptr_t)(void *)start) & ALLOC_MASK);
#if _TARGET_PC_WIN | _TARGET_XBOX
  UnmapViewOfFile(pa_start);
  return;
#else
  if (DAG_MMAP_THRESHOLD < 0 || length < DAG_MMAP_THRESHOLD)
    munmap(pa_start, ((char *)start + length) - (char *)pa_start);
  else
#endif
#endif
  {
    free((void *)start); //-V779
  }
}

#define EXPORT_PULL dll_pull_osapiwrappers_mmap
#include <supp/exportPull.h>
