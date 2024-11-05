// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bsdiffwrap.h"
#include "compression.h"
#include <util/dag_simpleString.h>
#include "vromfsCompressionImpl.h"
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#if _TARGET_PC_WIN
#include <io.h>
#else
#include <unistd.h>
#define O_BINARY 0
#endif

#if defined(BSDIFFWRAP_DIFF_EXECUTABLE) || defined(BSDIFFWRAP_APPLY_EXECUTABLE)
#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      "*"
#include <startup/dag_mainCon.inc.cpp>
#include <perfMon/dag_cpuFreq.h>
size_t dagormem_max_crt_pool_sz = 256 << 20;
#endif

bool unpack_vromfs(const char *fileName, char **ppBuf, off_t *pSize)
{
  char *comprBuf = *ppBuf;
  int comprSize = *pSize;
  VromfsCompression compr;
  int plainSize = compr.getRequiredDecompressionBufferLength(&comprBuf[1], comprSize - 1);
  if (plainSize < 0)
  {
    fprintf(stderr, "Error unpacking vromfs '%s' (unknown decompressed size)\n", fileName);
    return false;
  }
  char *plainBuf = (char *)memalloc(plainSize + 1, defaultmem);
  if (!plainBuf)
  {
    fprintf(stderr, "Error unpacking vromfs '%s' (out of memory)\n", fileName);
    return false;
  }
  const char *plainData = compr.decompress(&comprBuf[1], comprSize - 1, plainBuf, plainSize);
  if (!plainData)
  {
    fprintf(stderr, "Error unpacking vromfs '%s' (decompression failed)\n", fileName);
    return false;
  }
  if (plainData != plainBuf)
  {
    memmove(plainBuf, plainData, plainSize);
  }
  memfree(comprBuf, defaultmem);
  *ppBuf = plainBuf;
  *pSize = plainSize;
  return true;
}


bool pack_vromfs(Tab<char> &storage)
{
  char *plainBuf = storage.data();
  int plainSize = storage.size();
  VromfsCompression compr;
  int comprSize = compr.getRequiredCompressionBufferLength(plainSize, 11);
  char *comprBuf = (char *)memalloc(comprSize + 1, defaultmem);
  const char *comprData = compr.compress(plainBuf, plainSize, &comprBuf[1], comprSize, 11);
  if (!comprData)
  {
    fprintf(stderr, "Error packing resulting vromfs (compression failed)\n");
    return false;
  }
  if (comprData != &comprBuf[1])
  {
    memmove(&comprBuf[1], comprData, comprSize);
  }
  comprBuf[0] = 'V';
  storage.resize(comprSize + 1);
  memcpy(storage.data(), comprBuf, comprSize + 1);
  return true;
}


#if defined(BSDIFFWRAP_DIFF_EXECUTABLE)


int DagorWinMain(bool)
{
  int argc = dgs_argc;
  char **argv = dgs_argv;
  int fd;
  int bz2err;
  char *old, *_new;
  off_t oldsize, newsize;
  FILE *pf;
  Tab<char> diff(tmpmem);
  const Compression::CompressionEntry *comprTypes = Compression::getCompressionTypes();
  int comprCount = Compression::getCompressionTypesCount();

  if (argc < 4 || argc > 5)
  {
    fprintf(stderr, "usage: %s oldfile newfile patchfile [compression]\n", argv[0]);
    fprintf(stderr, "supported compression types:\n");
    const Compression::CompressionEntry *comprTypes = Compression::getCompressionTypes();
    int comprCount = Compression::getCompressionTypesCount();
    for (int i = 0; i < comprCount; i++)
    {
      fprintf(stderr, "  %s\n", comprTypes[i].name);
    }
    return 1;
  }

  /* Allocate oldsize+1 bytes instead of oldsize bytes to ensure
      that we never try to malloc(0) and get a NULL pointer */
  if (((fd = open(argv[1], O_RDONLY | O_BINARY, 0)) < 0) || ((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
      ((old = (char *)memalloc(oldsize + 1, defaultmem)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) ||
      (read(fd, old, oldsize) != oldsize) || (close(fd) == -1))
  {
    fprintf(stderr, "%s", argv[1]);
    return 1;
  }

  /* Allocate newsize+1 bytes instead of newsize bytes to ensure
      that we never try to malloc(0) and get a NULL pointer */
  if (((fd = open(argv[2], O_RDONLY | O_BINARY, 0)) < 0) || ((newsize = lseek(fd, 0, SEEK_END)) == -1) ||
      ((_new = (char *)memalloc(newsize + 1, defaultmem)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) ||
      (read(fd, _new, newsize) != newsize) || (close(fd) == -1))
  {
    fprintf(stderr, "%s", argv[2]);
    return 1;
  }

  int unpack_time_ms = 0, diff_time_ms = 0, t0_ref;
  if (oldsize > 16 && newsize > 16 && old[0] == 'V' && _new[0] == 'V')
  {
    VromfsCompression vromfsCompr;
    VromfsCompression::InputDataType oldType, newType;
    oldType = vromfsCompr.getDataType(&old[1], oldsize - 1, VromfsCompression::CHECK_STRICT);
    newType = vromfsCompr.getDataType(&_new[1], newsize - 1, VromfsCompression::CHECK_STRICT);
    if (oldType != VromfsCompression::IDT_DATA && newType != VromfsCompression::IDT_DATA)
    {
      fprintf(stderr, "Both input files are vromfs and will be unpacked\n");
      t0_ref = get_time_msec();
      if (oldType == VromfsCompression::IDT_VROM_COMPRESSED)
      {
        if (!unpack_vromfs(argv[1], &old, &oldsize))
          return 1;
      }
      if (newType == VromfsCompression::IDT_VROM_COMPRESSED)
      {
        if (!unpack_vromfs(argv[2], &_new, &newsize))
          return 1;
      }
      unpack_time_ms = get_time_msec() - t0_ref;
    }
  }

  /* Create the patch file */
  if ((pf = fopen(argv[3], "wb")) == NULL)
  {
    fprintf(stderr, "%s", argv[3]);
    return 1;
  }

  const char *comprName = ""; // no compression
  if (argc == 5)
    comprName = argv[4];
  else if (comprCount)
    comprName = comprTypes[0].name;
  t0_ref = get_time_msec();
  if (create_bsdiff(old, oldsize, _new, newsize, diff, comprName) != BSDIFF_OK)
  {
    fprintf(stderr, "create_bsdiff");
    return 1;
  }
  diff_time_ms = get_time_msec() - t0_ref;

  if (fwrite(diff.data(), diff.size(), 1, pf) != 1)
  {
    fprintf(stderr, "fwrite");
    return 1;
  }

  if (fclose(pf))
  {
    fprintf(stderr, "fclose");
    return 1;
  }

  /* Free the memory we used */
  memfree(old, defaultmem);
  memfree(_new, defaultmem);

  debug("result diff written: %s  (%d bytes)", argv[3], (unsigned)diff.size());
  debug("unpacked vroms for %dms, diff for %dms", unpack_time_ms, diff_time_ms);

  diff.clear();
  return 0;
}


#elif defined(BSDIFFWRAP_APPLY_EXECUTABLE)


int DagorWinMain(bool)
{
  int argc = dgs_argc;
  char **argv = dgs_argv;
  FILE *f;
  int fd;
  char *old, *diff;
  int64_t oldsize, diffsize;

  if (argc != 4)
  {
    fprintf(stderr, "usage: %s oldfile newfile patchfile\n", argv[0]);
    return 1;
  }

  /* Open patch file */
  if ((f = fopen(argv[3], "rb")) == NULL)
  {
    fprintf(stderr, "fopen(%s)", argv[3]);
    return 1;
  }

  fseek(f, 0, SEEK_END);
  diffsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  if ((diff = (char *)memalloc(diffsize + 1, defaultmem)) == NULL)
  {
    fprintf(stderr, "%s", argv[3]);
    return 1;
  }
  if (fread(diff, 1, diffsize, f) != diffsize)
  {
    fprintf(stderr, "%s", argv[3]);
    return 1;
  }
  fclose(f);

  if (((fd = open(argv[1], O_RDONLY | O_BINARY, 0)) < 0) || ((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
      ((old = (char *)memalloc(oldsize + 1, defaultmem)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) ||
      (read(fd, old, oldsize) != oldsize) || (close(fd) == -1))
  {
    fprintf(stderr, "%s", argv[1]);
    return 1;
  }

  int unpack_time_ms = 0, apply_time_ms = 0, repack_time_ms = 0, t0_ref;
  bool packResultingVrom = false;
  if (oldsize > 16 && old[0] == 'V')
  {
    VromfsCompression vromfsCompr;
    VromfsCompression::InputDataType oldType;
    oldType = vromfsCompr.getDataType(&old[1], oldsize - 1, VromfsCompression::CHECK_STRICT);
    if (oldType != VromfsCompression::IDT_DATA)
    {
      fprintf(stderr, "The input file is vromfs and will be unpacked\n");
      if (oldType == VromfsCompression::IDT_VROM_COMPRESSED)
      {
        off_t tmp = oldsize;
        t0_ref = get_time_msec();
        if (!unpack_vromfs(argv[1], &old, &tmp))
          return 1;
        unpack_time_ms = get_time_msec() - t0_ref;
        oldsize = tmp;
      }
      packResultingVrom = true;
    }
  }

  Tab<char> newStorage(tmpmem);
  t0_ref = get_time_msec();
  if (apply_bsdiff(old, oldsize, diff, diffsize, newStorage) != BSDIFF_OK)
  {
    fprintf(stderr, "bspatch\n");
    return 1;
  }
  apply_time_ms = get_time_msec() - t0_ref;

  if (packResultingVrom)
  {
    fprintf(stderr, "Packing the resulting vrom\n");
    t0_ref = get_time_msec();
    if (!pack_vromfs(newStorage))
      return 1;
    repack_time_ms = get_time_msec() - t0_ref;
  }


  /* Write the new file */
  if (((fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666)) < 0) ||
      (write(fd, newStorage.data(), newStorage.size()) != newStorage.size()) || (close(fd) == -1))
  {
    fprintf(stderr, "%s", argv[2]);
    return 1;
  }

  memfree(diff, defaultmem);
  memfree(old, defaultmem);
  newStorage.clear();

  printf("result file written: %s\nunpacked base for %dms, applied diff for %dms, repacked result for %dms\n", argv[2], unpack_time_ms,
    apply_time_ms, repack_time_ms);

  return 0;
}


#endif
