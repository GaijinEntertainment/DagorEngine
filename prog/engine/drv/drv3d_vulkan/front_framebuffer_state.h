#pragma once
#include "util/tracked_state.h"
#include "state_field_framebuffer.h"
#include "render_pass.h"

namespace drv3d_vulkan
{

struct FrontFramebufferStateStorage
{
  StateFieldFramebufferSwapchainSrgbWrite swapchainSrgbWrite;
  StateFieldFramebufferAttachments attachments;
  StateFieldFramebufferClearColor clearColor;
  StateFieldFramebufferClearDepth clearDepth;
  StateFieldFramebufferClearStencil clearStencil;
  StateFieldFramebufferReadOnlyDepth readOnlyDepth;

  // remove this copy state when we no longer use getters for RT in d3d::
  Driver3dRenderTarget rtSetCopy;

  void reset() {}
  void dumpLog() const { debug("FramebufferStateStorage end"); }
  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class FrontFramebufferState : public TrackedState<FrontFramebufferStateStorage, StateFieldFramebufferSwapchainSrgbWrite,
                                StateFieldFramebufferAttachments, StateFieldFramebufferReadOnlyDepth, StateFieldFramebufferClearColor,
                                StateFieldFramebufferClearDepth, StateFieldFramebufferClearStencil>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();

  Driver3dRenderTarget &asDriverRT();
  bool handleObjectRemoval(const Image *object);
  bool isReferenced(const Image *object) const;

  FrontFramebufferState &getValue() { return *this; }
  const FrontFramebufferState &getValueRO() const { return *this; }
};

} // namespace drv3d_vulkan