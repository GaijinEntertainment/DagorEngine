//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_io.h>

//! unpacks data through intermediate buffer to increase performance when reading to uncached memory
static inline void read_to_uncached_mem(IGenLoad &crd, void *ptr, int size)
{
  static constexpr int OUTBUF_SZ = 8192;
  char outBuf[OUTBUF_SZ];
  char *p = (char *)ptr;

  while (size > OUTBUF_SZ)
  {
    crd.read(outBuf, OUTBUF_SZ);
    memcpy(p, outBuf, OUTBUF_SZ);
    p += OUTBUF_SZ;
    size -= OUTBUF_SZ;
  }

  crd.read(outBuf, size);
  memcpy(p, outBuf, size);
}

//! helper to reduce reading to single line
static inline void read_to_device_mem(IGenLoad &crd, void *ptr, int size, bool cached_mem)
{
  if (cached_mem)
    crd.read(ptr, size);
  else
    read_to_uncached_mem(crd, ptr, size);
}
