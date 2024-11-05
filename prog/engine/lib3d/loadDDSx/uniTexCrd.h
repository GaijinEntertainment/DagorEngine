// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <3d/ddsxTex.h>

struct UnifiedTexGenLoad
{
  IGenLoad *ucrd;
  alignas(16) union
  {
    void *vptr = nullptr;
    char zlib[sizeof(BufferedZlibLoadCB)];
    char lzma[sizeof(BufferedLzmaLoadCB)];
    char zstd[sizeof(ZstdLoadCB)];
    char oodle[sizeof(OodleLoadCB)];
  } crdStorage;

  UnifiedTexGenLoad(IGenLoad &crd, const ddsx::Header &hdr) : ucrd(&crd) //-V730
  {
    if (hdr.isCompressionZLIB())
      ucrd = new (&crdStorage, _NEW_INPLACE) BufferedZlibLoadCB(crd, hdr.packedSz);
    else if (hdr.isCompression7ZIP())
      ucrd = new (&crdStorage, _NEW_INPLACE) BufferedLzmaLoadCB(crd, hdr.packedSz);
    else if (hdr.isCompressionZSTD())
      ucrd = new (&crdStorage, _NEW_INPLACE) ZstdLoadCB(crd, hdr.packedSz);
    else if (hdr.isCompressionOODLE())
      ucrd = new (&crdStorage, _NEW_INPLACE) OodleLoadCB(crd, hdr.packedSz, hdr.memSz);
  }
  ~UnifiedTexGenLoad() { term(); }

  void term()
  {
    if ((void *)ucrd == (void *)&crdStorage)
    {
      ucrd->ceaseReading();
      ucrd->~IGenLoad();
      ucrd = NULL;
    }
  }

  operator IGenLoad *() const { return ucrd; }
  IGenLoad *operator->() const { return ucrd; }
};

#define DDSX_LD_ITERATE_MIPS_START(CRD, HDR, SKIP_LEV, START_LEV, LEVELS, DEPTH, SLICES)                          \
  for (int mip_ord = 0; mip_ord < (HDR).levels; mip_ord++)                                                        \
  {                                                                                                               \
    int src_level = ((HDR).flags & (HDR).FLG_REV_MIP_ORDER) ? ((HDR).levels - 1 - mip_ord) : mip_ord;             \
    int tex_level = (START_LEV) + src_level - (SKIP_LEV);                                                         \
    const uint32_t mip_data_w = max(hdr.w >> src_level, 1);                                                       \
    const uint32_t mip_data_h = max(hdr.h >> src_level, 1);                                                       \
    const uint32_t mip_data_depth = max((DEPTH) >> src_level, 1) * (SLICES);                                      \
    const uint32_t mip_data_lines = (HDR).getSurfaceScanlines(src_level);                                         \
    const uint32_t mip_data_pitch = (HDR).getSurfacePitch(src_level);                                             \
    const uint32_t mip_data_slice_pitch = (HDR).getSurfaceSz(src_level);                                          \
    const uint32_t mip_data_sz = mip_data_slice_pitch * mip_data_depth;                                           \
    const bool last_iteration =                                                                                   \
      ((HDR).flags & (HDR).FLG_REV_MIP_ORDER) ? tex_level == (START_LEV) : tex_level == (START_LEV) + (LEVELS)-1; \
    if (tex_level < 0)                                                                                            \
    {                                                                                                             \
      if ((HDR).flags & (HDR).FLG_REV_MIP_ORDER)                                                                  \
        break;                                                                                                    \
      (CRD).seekrel(mip_data_sz);                                                                                 \
      continue;                                                                                                   \
    }                                                                                                             \
    else if (tex_level >= (START_LEV) + (LEVELS))                                                                 \
    {                                                                                                             \
      if (!((HDR).flags & (HDR).FLG_REV_MIP_ORDER))                                                               \
        break;                                                                                                    \
      (CRD).seekrel(mip_data_sz);                                                                                 \
      continue;                                                                                                   \
    }                                                                                                             \
    (void)last_iteration;                                                                                         \
    (void)mip_data_w;                                                                                             \
    (void)mip_data_h;                                                                                             \
    (void)mip_data_lines;

#define DDSX_LD_ITERATE_MIPS_END() }
