//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC && !_TARGET_STATIC_LIB
#define HMAP_FOR_TOOLS 1
#else
#define HMAP_FOR_TOOLS 0
#endif

#include <math/dag_mathBase.h>
#include <util/dag_stdint.h>

class BaseTexture;
typedef BaseTexture Texture;

#if HMAP_FOR_TOOLS
class IHeightmapLandProvider
{
public:
  static constexpr int MAX_DET_TEX_NUM = 7;

  virtual void requestHeightmapData(int x0, int y0, int step, int x_size, int y_size) = 0;

  virtual void getHeightmapData(int x0, int y0, int step, int x_size, int y_size, real *data, int stride_bytes) = 0;

  virtual bool isLandPreloadRequestComplete() = 0;
};
#endif
