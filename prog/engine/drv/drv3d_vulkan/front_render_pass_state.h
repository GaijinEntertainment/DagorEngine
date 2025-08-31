// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "util/tracked_state.h"
#include "state_field_render_pass.h"

namespace drv3d_vulkan
{

struct FrontRenderPassStateStorage
{
  StateFieldRenderPassResource resource;
  StateFieldRenderPassTargets targets;
  StateFieldRenderPassSubpassIdx subpassIndex;
  StateFieldRenderPassArea area;
  StateFieldRenderPassIndex index;

  void reset() {}
  void dumpLog() const { debug("RenderPassStateStorage end"); }
  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class FrontRenderPassState : public TrackedState<FrontRenderPassStateStorage, StateFieldRenderPassTargets, StateFieldRenderPassArea,
                               StateFieldRenderPassResource, StateFieldRenderPassSubpassIdx, StateFieldRenderPassIndex>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();

  bool handleObjectRemoval(const Image *object);
  bool isReferenced(const Image *object) const;

  bool handleObjectRemoval(const RenderPassResource *object);
  bool isReferenced(const RenderPassResource *object) const;
  bool replaceImage(const Image *src, Image *dst);

  FrontRenderPassState &getValue() { return *this; }
  const FrontRenderPassState &getValueRO() const { return *this; }
};

} // namespace drv3d_vulkan