// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>
#include <EASTL/tuple.h>
#include <generic/dag_smallTab.h>
#include <resourceName.h>


class BaseTextureImpl : public D3dResourceNameImpl<BaseTexture>
{
public:
  uint8_t minMipLevel;
  uint8_t maxMipLevel;

  uint32_t base_format;
  uint32_t cflg;
  uint32_t lockFlags = 0;
  uint16_t width = 1, height = 1, depth = 1;
  uint8_t mipLevels;
  D3DResourceType type;
  uint8_t lockedLevel = 0;


  static bool isSameFormat(int cflg1, int cflg2);

  BaseTextureImpl(uint32_t cflg, D3DResourceType type);

  D3DResourceType getType() const override { return type; }

  virtual int update(BaseTexture *src) = 0;
  virtual int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) = 0;

  int level_count() const override;
  virtual int texmiplevel(int minlev, int maxlev) = 0;

  void setTexName(const char *name);

  virtual int lockimg(void **, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) = 0;
  virtual int unlockimg() = 0;

  virtual int generateMips() = 0;

  int getinfo(TextureInfo &ti, int level) const override;
};

int count_mips_if_needed(int w, int h, int32_t flg, int levels);

// common helpers to convert DDSx format to TEXFMT_ one
uint32_t d3dformat_to_texfmt(/*D3DFORMAT*/ uint32_t fmt);
uint32_t texfmt_to_d3dformat(/*D3DFORMAT*/ uint32_t fmt);
static inline uint32_t implant_d3dformat(uint32_t cflg, uint32_t fmt) { return (cflg & ~TEXFMT_MASK) | d3dformat_to_texfmt(fmt); }
