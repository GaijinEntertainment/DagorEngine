
// all framebuffer related stuff
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>

#include "driver.h"
#include "device.h"
#include "pipeline_state.h"
#include "swapchain.h"
#include "texture.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccesor
{
  PipelineState &pipeState;
  Device &drvDev;
  VulkanDevice &dev;
  Swapchain &swapchain;
  DeviceContext &ctx;

  LocalAccesor() :
    pipeState(get_device().getContext().getFrontend().pipelineState),
    drvDev(get_device()),
    dev(get_device().getVkDevice()),
    swapchain(get_device().swapchain),
    ctx(get_device().getContext())
  {}
};
} // namespace

bool d3d::set_srgb_backbuffer_write(bool on)
{
  LocalAccesor la;
  // df proprogated to 0 attachment on apply as attachments is nested field
  bool changed = la.pipeState.set<StateFieldFramebufferSwapchainSrgbWrite, bool, FrontGraphicsState, FrontFramebufferState>(on);

  // srgb toggle happens multiple times
  // and should not bother swapchain if it is was enabled for it
  const SwapchainMode &currentMode = la.swapchain.getMode();
  if (!currentMode.enableSrgb && on)
  {
    SwapchainMode newMode(currentMode);
    newMode.enableSrgb = on;
    la.swapchain.setMode(newMode);
  }

  return changed ? !on : on;
}

bool d3d::copy_from_current_render_target(BaseTexture *to_tex)
{
  LocalAccesor la;

  G_ASSERTF(to_tex, "vulkan: can't copy to null texture");
  TextureInterfaceBase *dstTex = cast_to_texture_base(to_tex);
  Image *dstImg = dstTex->getDeviceImage(false);

  StateFieldFramebufferAttachment &fbAttachment =
    la.pipeState.get<StateFieldFramebufferAttachments, StateFieldFramebufferAttachment, FrontGraphicsState, FrontFramebufferState>();
  Image *srcImg = fbAttachment.image;

  G_ASSERTF(srcImg != dstImg, "vulkan: can't copy render target to itself");

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

  if (!fbAttachment.useSwapchain)
  {
    G_ASSERTF(srcImg, "vulkan: can't copy from unset render target");
    blit.srcOffsets[1] = toOffset(srcImg->getBaseExtent());
  }
  else
  {
    VkExtent2D size = la.swapchain.getMode().extent;
    blit.srcOffsets[1].x = size.width;
    blit.srcOffsets[1].y = size.height;
  }

  if (dstTex)
    blit.dstOffsets[1] = toOffset(dstTex->getMipmapExtent(0));
  else
  {
    G_ASSERTF(!fbAttachment.useSwapchain, "vulkan: can't copy from backbuffer to backbuffer");
    VkExtent2D size = la.swapchain.getMode().extent;
    blit.dstOffsets[1].x = size.width;
    blit.dstOffsets[1].y = size.height;
  }

  blit.srcOffsets[1].z = 1;
  blit.dstOffsets[1].z = 1;

  la.drvDev.getContext().blitImage(srcImg, dstImg, blit);
  return true;
}

bool d3d::set_render_target()
{
  LocalAccesor la;
  using Bind = StateFieldFramebufferAttachment;

  la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>({0, Bind::back_buffer});
  la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
    {MRT_INDEX_DEPTH_STENCIL, Bind::back_buffer});
  la.pipeState.set<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(false);
  for (uint32_t i = 1; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>({i, Bind::empty});

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  return true;
}

bool d3d::set_depth(Texture *tex, bool read_only) { return set_depth(tex, 0, read_only); }

bool d3d::set_depth(BaseTexture *tex, int layer, bool read_only)
{
  LocalAccesor la;
  using Bind = StateFieldFramebufferAttachment;

  if (!tex)
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, Bind::empty});
  else
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, {tex, 0, layer}});

  la.pipeState.set<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(read_only);

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  return true;
}

bool d3d::set_backbuf_depth()
{
  LocalAccesor la;
  using Bind = StateFieldFramebufferAttachment;

  la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
    {MRT_INDEX_DEPTH_STENCIL, Bind::back_buffer});

  la.pipeState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::RestoreFromFramebuffer, FrontGraphicsState>({});
  return true;
}

bool d3d::set_render_target(int ri, Texture *tex, int level) { return d3d::set_render_target(ri, tex, 0, level); }

bool d3d::set_render_target(int ri, BaseTexture *tex, int layer, int level)
{
  G_ASSERTF(ri >= 0, "vulkan: no meaning of negative render target index is present");

  LocalAccesor la;
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
  LocalAccesor la;
  using Bind = StateFieldFramebufferAttachment;

  if (rt.isBackBufferColor())
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {0, Bind::back_buffer});
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

  if (rt.isBackBufferDepth())
    la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
      {MRT_INDEX_DEPTH_STENCIL, Bind::back_buffer});
  else
  {
    if (rt.isDepthUsed())
    {
      const Driver3dRenderTarget::RTState &rts = rt.depth;
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::RawIndexed, FrontGraphicsState, FrontFramebufferState>(
        {MRT_INDEX_DEPTH_STENCIL, {rts.tex, rts.level, rts.layer}});
    }
    else
      la.pipeState.set<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
        {MRT_INDEX_DEPTH_STENCIL, Bind::empty});
  }

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
  LocalAccesor la;
  out_rt = la.pipeState.get<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>().asDriverRT();
}

bool d3d::get_target_size(int &w, int &h)
{
  LocalAccesor la;
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

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev)
{
  LocalAccesor la;

  if (!rt_tex)
  {
    VkExtent2D size = la.swapchain.getMode().extent;
    w = size.width;
    h = size.height;
  }
  else
  {
    const VkExtent3D &size = cast_to_texture_base(*rt_tex).getMipmapExtent(lev);
    w = size.width;
    h = size.height;
  }
  return true;
}

bool d3d::clearview(int what, E3DCOLOR color, float z, uint32_t stencil)
{
  if (!what)
    return true;

  LocalAccesor la;
  la.pipeState.set<StateFieldFramebufferClearColor, E3DCOLOR, FrontGraphicsState, FrontFramebufferState>(color);
  la.pipeState.set<StateFieldFramebufferClearDepth, float, FrontGraphicsState, FrontFramebufferState>(z);
  la.pipeState.set<StateFieldFramebufferClearStencil, uint8_t, FrontGraphicsState, FrontFramebufferState>((uint8_t)stencil);
  la.ctx.clearView(what);

  return true;
}