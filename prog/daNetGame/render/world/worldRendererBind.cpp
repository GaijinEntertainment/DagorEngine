// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "private_worldRenderer.h"

namespace bind_dascript
{

void toggleFeature(FeatureRenderFlags flag, bool enable)
{
  WorldRenderer *wr = (WorldRenderer *)get_world_renderer();
  if (!wr)
    return;

  wr->toggleFeatures(FeatureRenderFlagMask().set(flag), enable);
}

bool worldRenderer_getWorldBBox3(BBox3 &bbox)
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
  {
    bbox = wr->getWorldBBox3();
    return true;
  }

  return false;
}

void worldRenderer_shadowsInvalidate(const BBox3 &bbox)
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
    wr->shadowsInvalidate(bbox);
}

void worldRenderer_invalidateAllShadows()
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
    wr->invalidateAllShadows();
}

int worldRenderer_getDynamicResolutionTargetFps()
{
  if (auto wr = static_cast<WorldRenderer *>(get_world_renderer()))
    return wr->getDynamicResolutionTargetFps();
  return 0;
}

void worldRenderer_setDaGdpRangeScale(float scale)
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
    wr->daGdpRangeScale = scale;
}

bool does_world_renderer_exist() { return static_cast<bool>(get_world_renderer()); }

} // namespace bind_dascript
