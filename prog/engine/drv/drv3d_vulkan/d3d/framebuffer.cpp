// Copyright (C) Gaijin Games KFT.  All rights reserved.

// all framebuffer related stuff
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>

#include "driver.h"
#include "pipeline_state.h"
#include "swapchain.h"
#include "texture.h"
#include "frontend.h"
#include "device_context.h"
#include "global_lock.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccessor
{
  PipelineState &pipeState;
  VulkanDevice &dev;
  DeviceContext &ctx;

  LocalAccessor() : pipeState(Frontend::State::pipe), dev(Globals::VK::dev), ctx(Globals::ctx) { VERIFY_GLOBAL_LOCK_ACQUIRED(); }
};

StateFieldFramebufferAttachment::RawFrontData make_back_buffer_attachment()
{
  StateFieldFramebufferAttachment::RawFrontData bb;
  bb.tex = Frontend::swapchain.getCurrentTargetTex();
  bb.layer = 0;
  bb.mip = 0;
  return bb;
}

} // namespace

bool d3d::set_srgb_backbuffer_write(bool on)
{
  LocalAccessor la;
  // srgb toggle happens multiple times
  // and should not bother swapchain if it is was enabled for it
  const SwapchainMode &currentMode = Frontend::swapchain.getMode();
  if (!currentMode.enableSrgb && on)
  {
    SwapchainMode newMode(currentMode);
    newMode.enableSrgb = on;
    Frontend::swapchain.setMode(newMode);
  }

  // df proprogated to 0 attachment on apply as attachments is nested field
  bool changed = la.pipeState.set<StateFieldFramebufferSwapchainSrgbWrite, bool, FrontGraphicsState, FrontFramebufferState>(on);
  return changed ? !on : on;
}

bool d3d::copy_from_current_render_target(BaseTexture *to_tex)
{
  LocalAccessor la;

  D3D_CONTRACT_ASSERTF(to_tex, "vulkan: can't copy to null texture");
  TextureInterfaceBase *dstTex = cast_to_texture_base(to_tex);
  Image *dstImg = dstTex->image;

  StateFieldFramebufferAttachment &fbAttachment =
    la.pipeState.get<StateFieldFramebufferAttachments, StateFieldFramebufferAttachment, FrontGraphicsState, FrontFramebufferState>();
  Image *srcImg = fbAttachment.image;

  D3D_CONTRACT_ASSERTF(srcImg != dstImg, "vulkan: can't copy render target to itself");

  VkImageBlit blit;
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;

  D3D_CONTRACT_ASSERTF(srcImg, "vulkan: can't copy from unset render target");
  blit.srcOffsets[1] = toOffset(srcImg->getBaseExtent());
  blit.dstOffsets[1] = toOffset(dstTex->pars.getMipExtent(0));
  blit.srcOffsets[1].z = 1;
  blit.dstOffsets[1].z = 1;

  Globals::ctx.blitImage(srcImg, dstImg, blit, /*whole_subres*/ true);
  return true;
}

bool d3d::set_render_target()
{
  LocalAccessor la;
  using Bind = StateFieldFramebufferAttachment;

  la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
    {0, make_back_buffer_attachment()});
  la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
    {MRT_INDEX_DEPTH_STENCIL, Bind::empty});
  la.pipeState.set<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(false);
  for (uint32_t i = 1; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>({i, Bind::empty});

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  return true;
}

bool d3d::set_depth(Texture *tex, DepthAccess access) { return set_depth(tex, 0, access); }

bool d3d::set_depth(BaseTexture *tex, int layer, DepthAccess access)
{
  LocalAccessor la;
  using Bind = StateFieldFramebufferAttachment;

  if (!tex)
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, Bind::empty});
  else
  {
    // depth tex is always considered used as we implictly read from it on draw
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, {tex, 0, layer}});
  }

  la.pipeState.set<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(
    access == DepthAccess::SampledRO);

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  return true;
}

bool d3d::set_render_target(int ri, Texture *tex, uint8_t level) { return d3d::set_render_target(ri, tex, 0, level); }

bool d3d::set_render_target(int ri, BaseTexture *tex, int layer, uint8_t level)
{
  D3D_CONTRACT_ASSERTF(ri >= 0, "vulkan: no meaning of negative render target index is present");

  LocalAccessor la;
  using Bind = StateFieldFramebufferAttachment;

  la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
    {(uint32_t)ri, {tex, level, layer}});

  if (0 == ri)
  {
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, Bind::empty});

    la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  }
  return true;
}

bool d3d::set_render_target(const Driver3dRenderTarget &rt)
{
  LocalAccessor la;
  using Bind = StateFieldFramebufferAttachment;

  if (rt.isBackBufferColor())
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
      {0, make_back_buffer_attachment()});
  else
  {
    if (rt.isColorUsed(0))
    {
      const Driver3dRenderTarget::RTState &rts = rt.color[0];
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
        {0, {rts.tex, rts.level, rts.layer}});
    }
    else
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>({0, Bind::empty});
  }

  if (rt.isDepthUsed())
  {
    const Driver3dRenderTarget::RTState &rts = rt.depth;
    // depth tex is always considered used as we implictly read from it on draw
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, {rts.tex, rts.level, rts.layer}});
  }
  else
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, Bind::empty});

  for (uint32_t i = 1; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (rt.isColorUsed(i))
    {
      const Driver3dRenderTarget::RTState &rts = rt.color[i];
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
        {i, {rts.tex, rts.level, rts.layer}});
    }
    else
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>({i, Bind::empty});
  }

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  la.pipeState.set<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(rt.isDepthReadOnly());
  return true;
}

void d3d::get_render_target(Driver3dRenderTarget &out_rt)
{
  LocalAccessor la;
  out_rt = la.pipeState.get<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>().asDriverRT();
}

bool d3d::get_target_size(int &w, int &h)
{
  LocalAccessor la;
  const Driver3dRenderTarget &rt = la.pipeState.get<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>().asDriverRT();

  if (rt.isBackBufferColor())
    return d3d::get_render_target_size(w, h, nullptr, 0);
  else if (rt.isColorUsed(0))
    return d3d::get_render_target_size(w, h, rt.color[0].tex, rt.color[0].level);
  else if (rt.isDepthUsed() && rt.depth.tex)
    return d3d::get_render_target_size(w, h, rt.depth.tex, rt.depth.level);
  else
    return d3d::get_render_target_size(w, h, nullptr, 0);
}

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, uint8_t level)
{
  if (!rt_tex)
  {
    VkExtent2D size = Frontend::swapchain.getMode().extent;
    w = size.width;
    h = size.height;
  }
  else
  {
    const VkExtent3D &size = cast_to_texture_base(*rt_tex).pars.getMipExtent(level);
    w = size.width;
    h = size.height;
  }
  return true;
}

bool d3d::clearview(int what, E3DCOLOR color, float z, uint32_t stencil)
{
  if (!what)
    return true;

  LocalAccessor la;
  la.pipeState.set<StateFieldFramebufferClearColor, E3DCOLOR, FrontGraphicsState, FrontFramebufferState>(color);
  la.pipeState.set<StateFieldFramebufferClearDepth, float, FrontGraphicsState, FrontFramebufferState>(z);
  la.pipeState.set<StateFieldFramebufferClearStencil, uint8_t, FrontGraphicsState, FrontFramebufferState>((uint8_t)stencil);
  la.ctx.clearView(what);
  return true;
}

#if DAGOR_DBGLEVEL > 0
void d3d::allow_render_pass_target_load()
{
  LocalAccessor la;
  // No need to insert an extra command if the validation is disabled.
  if (Globals::cfg.bits.validateRenderPassSplits)
    Globals::ctx.allowOpLoad();
}
#endif
