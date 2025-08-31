// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "front_render_pass_state.h"
#include "graphics_state2.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(FrontRenderPassState, renderpass, FrontGraphicsStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void FrontRenderPassStateStorage::makeDirty() { targets.makeDirty(); }

void FrontRenderPassStateStorage::clearDirty() { targets.clearDirty(); }

bool FrontRenderPassState::isReferenced(const Image *object) const
{
  using Bind = StateFieldRenderPassTarget;

  const StateFieldRenderPassTarget *rpTargets = &getRO<StateFieldRenderPassTargets, StateFieldRenderPassTarget>();

  for (int i = 0; i < StateFieldRenderPassTargets::size(); ++i)
    if (rpTargets[i].image == object)
      return true;

  return false;
}

bool FrontRenderPassState::replaceImage(const Image *src, Image *dst)
{
  bool ret = false;
  using Bind = StateFieldRenderPassTarget;
  const Bind *rpTargets = &getRO<StateFieldRenderPassTargets, StateFieldRenderPassTarget>();

  for (uint32_t i = 0; i < StateFieldRenderPassTargets::size(); ++i)
  {
    const Bind &rt = rpTargets[i];
    if (rt.image == src)
    {
      Bind replacement = rt;
      replacement.image = dst;
      set<StateFieldRenderPassTargets, Bind::Indexed>({i, replacement});
      ret |= true;
    }
  }
  return ret;
}

bool FrontRenderPassState::handleObjectRemoval(const Image *object)
{
  if (isReferenced(object))
  {
    // delete image after finishing up render pass / check that cleanup on pass end is correct
    // this check will avoid deletion (and following up crash) but must be fixed if encountered!
    D3D_ERROR("vulkan: removing image %p<%s> that is used in %p<%s> render pass", object, object->getDebugName(),
      getData().resource.ptr, getData().resource.ptr ? getData().resource.ptr->getDebugName() : "<inactive>");
    return true;
  }

  return false;
}

bool FrontRenderPassState::isReferenced(const RenderPassResource *object) const { return object == getDataRO().resource.ptr; }

bool FrontRenderPassState::handleObjectRemoval(const RenderPassResource *object)
{
  if (isReferenced(object))
  {
    // avoid deleting active pass!!!
    // this check will avoid deletion (and following up crash) but must be fixed if encountered!
    D3D_ERROR("vulkan: removing active render pass %p<%s>", object, object->getDebugName());
    return true;
  }
  return false;
}
