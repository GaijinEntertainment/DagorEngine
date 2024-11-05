// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "front_framebuffer_state.h"
#include "graphics_state2.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(FrontFramebufferState, framebuffer, FrontGraphicsStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void FrontFramebufferStateStorage::makeDirty() { attachments.makeDirty(); }

void FrontFramebufferStateStorage::clearDirty() { attachments.clearDirty(); }

bool FrontFramebufferState::isReferenced(const Image *object) const
{
  using Bind = StateFieldFramebufferAttachment;

  const StateFieldFramebufferAttachment *fbAttachments = &getRO<StateFieldFramebufferAttachments, StateFieldFramebufferAttachment>();

  for (int i = 0; i < StateFieldFramebufferAttachments::size(); ++i)
    if (fbAttachments[i].image == object)
      return true;

  return false;
}

bool FrontFramebufferState::handleObjectRemoval(const Image *object)
{
  bool ret = false;
  using Bind = StateFieldFramebufferAttachment;

  StateFieldFramebufferAttachment *fbAttachments = &get<StateFieldFramebufferAttachments, StateFieldFramebufferAttachment>();

  for (uint32_t i = 0; i < StateFieldFramebufferAttachments::size(); ++i)
  {
    Bind &rt = fbAttachments[i];
    if (rt.image == object)
    {
      // this will prevent crash, but must be fixed in game code!
      // leave this D3D_ERROR to actual use place
      // D3D_ERROR("vulkan: removing image %p<%s> bound as fb attachment %u", object, object->getDebugName(), i);
      set<StateFieldFramebufferAttachments, uint32_t>(i);
      ret |= true;
    }
  }

  return ret;
}

Driver3dRenderTarget &FrontFramebufferState::asDriverRT()
{
  // slow, buggy & inefficient reconstruction that we keeping intact for d3d:: getters sake
  Driver3dRenderTarget &rt = getData().rtSetCopy;

  StateFieldFramebufferAttachment *fbAttachments = &get<StateFieldFramebufferAttachments, StateFieldFramebufferAttachment>();
  rt.reset();

  StateFieldFramebufferAttachment &ds = fbAttachments[MRT_INDEX_DEPTH_STENCIL];
  if (ds.image)
    rt.setDepth(ds.tex, getData().readOnlyDepth.data);

  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    StateFieldFramebufferAttachment &col = fbAttachments[i];
    if (col.image)
      rt.setColor(i, col.tex, col.view.getMipBase(), col.view.getArrayBase());
    else if (col.useSwapchain)
    {
      if (i == 0)
        rt.setBackbufColor();
      else
        rt.setColor(i, col.tex, col.view.getMipBase(), col.view.getArrayBase());
    }
  }

  return rt;
}