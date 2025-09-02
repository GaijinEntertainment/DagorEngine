// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state_field_framebuffer.h"
#include "texture.h"
#include "execution_context.h"
#include "device_context.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferAttachments, attachments, FrontFramebufferStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferClearColor, clearColor, FrontFramebufferStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferClearDepth, clearDepth, FrontFramebufferStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferClearStencil, clearStencil, FrontFramebufferStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferReadOnlyDepth, readOnlyDepth, FrontFramebufferStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldFramebufferSwapchainSrgbWrite, swapchainSrgbWrite, FrontFramebufferStateStorage);

template <>
void StateFieldFramebufferAttachment::applyTo(uint32_t index, FrontFramebufferStateStorage &state, ExecutionState &target) const
{
  D3D_CONTRACT_ASSERTF(view.isRenderTarget || !image, "vulkan: trying to use non RT view for FB attachment");
  ImageViewState actualView = view;
  if (image && image->getDescription().memFlags & Image::MEM_SWAPCHAIN)
    actualView.setFormat(
      state.swapchainSrgbWrite.data ? actualView.getFormat().getSRGBVariant() : actualView.getFormat().getLinearVariant());
  ExecutionContext &ctx = target.getExecutionContext();

  if (image)
    ctx.verifyResident(image);

  D3D_CONTRACT_ASSERTF(!image || image->isAllowedForFramebuffer(),
    "vulkan: trying to bind non-RT image %p-%s as framebuffer attachment %u", image, image->getDebugName(), index);

  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;

  // depth
  if (index >= MRT_INDEX_DEPTH_STENCIL)
  {
    if (image)
    {
      fbs.renderPassClass.depthState =
        state.readOnlyDepth.data ? RenderPassClass::Identifier::RO_DEPTH : RenderPassClass::Identifier::RW_DEPTH;
      fbs.renderPassClass.depthStencilFormat = actualView.getFormat();
      fbs.renderPassClass.depthSamples = image->getSampleCount();
      fbs.frameBufferInfo.depthStencilAttachment =
        RenderPassClass::FramebufferDescription::AttachmentInfo(image, image->getImageView(actualView), actualView);
    }
    else
    {
      fbs.renderPassClass.depthState = RenderPassClass::Identifier::NO_DEPTH;
      fbs.renderPassClass.depthStencilFormat = FormatStore(0);
      fbs.renderPassClass.depthSamples = UCHAR_MAX;
      fbs.frameBufferInfo.depthStencilAttachment = RenderPassClass::FramebufferDescription::AttachmentInfo();
    }
  }
  else
  {
    // color
    if (image)
    {
      fbs.renderPassClass.colorTargetMask |= 1 << index;
      fbs.renderPassClass.colorFormats[index] = actualView.getFormat();
      fbs.renderPassClass.colorSamples[index] = image->getSampleCount();
      fbs.frameBufferInfo.colorAttachments[index] =
        RenderPassClass::FramebufferDescription::AttachmentInfo(image, image->getImageView(actualView), actualView);
    }
    else
    {
      fbs.renderPassClass.colorTargetMask &= ~(1 << index);
      fbs.renderPassClass.colorFormats[index] = FormatStore(0);
      fbs.renderPassClass.colorSamples[index] = UCHAR_MAX;
      fbs.frameBufferInfo.colorAttachments[index] = RenderPassClass::FramebufferDescription::AttachmentInfo();
    }
  }

  target.interruptRenderPass("attachmentChange");
}

template <>
void StateFieldFramebufferAttachment::transit(uint32_t index, FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetFramebufferAttachment cmd{index, image, view};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferAttachment::dumpLog(uint32_t index, const FrontFramebufferStateStorage &) const
{
  debug("attachment%u: %p[%s]", index, image, image ? image->getDebugName() : "none");
}

template <>
void StateFieldFramebufferReadOnlyDepth::applyTo(FrontFramebufferStateStorage &, ExecutionState &target) const
{
  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  if (fbs.renderPassClass.depthState != RenderPassClass::Identifier::NO_DEPTH)
    fbs.renderPassClass.depthState = data ? RenderPassClass::Identifier::RO_DEPTH : RenderPassClass::Identifier::RW_DEPTH;
  target.interruptRenderPass("depthRWChange");
}

template <>
void StateFieldFramebufferReadOnlyDepth::transit(FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetDepthStencilRWState cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferReadOnlyDepth::dumpLog(const FrontFramebufferStateStorage &) const
{
  debug("readOnlyDepth: %s", data ? "yes" : "no");
}

template <>
void StateFieldFramebufferClearColor::applyTo(FrontFramebufferStateStorage &, ExecutionState &target) const
{
  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    fbs.colorClearValue[i].float32[0] = data.r / 255.f;
    fbs.colorClearValue[i].float32[1] = data.g / 255.f;
    fbs.colorClearValue[i].float32[2] = data.b / 255.f;
    fbs.colorClearValue[i].float32[3] = data.a / 255.f;
  }
}

template <>
void StateFieldFramebufferClearColor::transit(FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetFramebufferClearColor cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferClearColor::dumpLog(const FrontFramebufferStateStorage &) const
{
  debug("clearColor: 0x%08X", data.u);
}

template <>
void StateFieldFramebufferClearDepth::applyTo(FrontFramebufferStateStorage &, ExecutionState &target) const
{
  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  fbs.depthStencilClearValue.depth = data;
}

template <>
void StateFieldFramebufferClearDepth::transit(FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetFramebufferClearDepth cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferClearDepth::dumpLog(const FrontFramebufferStateStorage &) const
{
  debug("clearDepth: %f", data);
}

template <>
void StateFieldFramebufferClearStencil::applyTo(FrontFramebufferStateStorage &, ExecutionState &target) const
{
  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  fbs.depthStencilClearValue.stencil = data;
}

template <>
void StateFieldFramebufferClearStencil::transit(FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetFramebufferClearStencil cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferClearStencil::dumpLog(const FrontFramebufferStateStorage &) const
{
  debug("clearStencil: %u", data);
}

template <>
void StateFieldFramebufferSwapchainSrgbWrite::dumpLog(const FrontFramebufferStateStorage &) const
{
  debug("swapchainSRGBwrite: %s", data ? "yes" : "no");
}

template <>
void StateFieldFramebufferSwapchainSrgbWrite::transit(FrontFramebufferStateStorage &, DeviceContext &target) const
{
  CmdSetFramebufferSwapchainSrgbWrite cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldFramebufferSwapchainSrgbWrite::applyTo(FrontFramebufferStateStorage &state, ExecutionState &) const
{
  state.attachments.makeDirty();
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

StateFieldFramebufferAttachment StateFieldFramebufferAttachment::empty = {nullptr, {}};

void StateFieldFramebufferAttachment::set(const StateFieldFramebufferAttachment &v)
{
  image = v.image;
  view = v.view;
}

bool StateFieldFramebufferAttachment::diff(const StateFieldFramebufferAttachment &v) const
{
  return (image != v.image) || (view != v.view);
}

void StateFieldFramebufferAttachment::set(const StateFieldFramebufferAttachment::RawFrontData &v)
{
  tex = v.tex;
  if (v.tex)
  {
    BaseTex *base = cast_to_texture_base(v.tex);
    image = base->image;
    view = base->getViewInfoRenderTarget(v.mip, v.layer);
  }
  else
  {
    image = nullptr;

    view.isArray = 0;
    view.isCubemap = 0;
    view.setMipBase(v.mip);
    view.setMipCount(1);
    view.setArrayBase(v.layer);
    view.setArrayCount(1);
    view.isRenderTarget = 1;
  }
}

// FIXME: we doing extra work that is same as in ::set, need to figure better solution
bool StateFieldFramebufferAttachment::diff(const StateFieldFramebufferAttachment::RawFrontData &v) const
{
  if (v.layer != view.getArrayBase())
    return true;

  if (v.mip != view.getMipBase())
    return true;

  if (!v.tex)
    return (image != nullptr);

  BaseTex *base = cast_to_texture_base(v.tex);
  Image *img = base->image;

  return image != img;
}
