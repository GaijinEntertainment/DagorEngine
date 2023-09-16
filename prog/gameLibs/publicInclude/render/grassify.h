//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

#pragma once

#include <math/dag_Point3.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaderBlock.h>
#include <render/gpuGrass.h>

class BaseTexture;
struct GrassGenerateHelper;
struct GrassMaskSliceHelper;
class GPUGrassBase;
struct IRandomGrassRenderHelper;

class Grassify
{
public:
  typedef eastl::function<bool(const BBox2 &, float &min_ht, float &max_ht)> frustum_heights_cb_t;

public:
  Grassify(const DataBlock &settings, int grassMaskResolution, float grassDistance);
  ~Grassify();

  void generate(const Point3 &pos, const TMatrix &view_itm, BaseTexture *grass_mask, IRandomGrassRenderHelper &grassRenderHelper,
    MaskRenderCallback::PreRenderCallback pre_render_cb, const GPUGrassBase &gpuGrassBase);
  void generateGrassMask(IRandomGrassRenderHelper &grassRenderHelper, MaskRenderCallback::PreRenderCallback pre_render_cb);

private:
  eastl::unique_ptr<GrassMaskSliceHelper> grassMaskHelper;
  eastl::unique_ptr<GrassGenerateHelper> grassGenHelper;
  float texelSize = 0.0f;
};