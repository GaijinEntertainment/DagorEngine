#include "device.h"
#include "state_field_render_pass.h"
#include "render_pass_resource.h"
#include "texture.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRenderPassTargets, targets, FrontRenderPassStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRenderPassResource, resource, FrontRenderPassStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRenderPassResource, nativeRenderPass, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRenderPassSubpassIdx, subpassIndex, FrontRenderPassStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRenderPassArea, area, FrontRenderPassStateStorage);

template <>
void StateFieldRenderPassTarget::applyTo(uint32_t index, FrontRenderPassStateStorage &state, ExecutionState &target) const
{
  RenderPassResource::BakedAttachmentSharedData::PerAttachment att(
    target.get<BackGraphicsState, BackGraphicsState>().nativeRenderPassAttachments, index);

  // clear to null
  att.view = VulkanImageViewHandle();
#if VK_KHR_imageless_framebuffer
  if (get_device().hasImagelessFramebuffer())
    att.info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO_KHR, nullptr};
#endif

  if (!state.resource.ptr || !image)
    return;

  image->checkDead();
  G_ASSERTF(image->isAllowedForFramebuffer(), "vulkan: trying to bind non-RT image %p-%s as NRP attachment %u", image,
    image->getDebugName(), index);

  static_assert(sizeof(att.clearValue.color.uint32) == sizeof(clearValue.asUint), "unit vk clear value have different size");
  static_assert(offsetof(VkClearDepthStencilValue, stencil) == offsetof(ResourceClearValue, asStencil),
    "stencil vk clear value have different size");
  static_assert(offsetof(VkClearDepthStencilValue, depth) == offsetof(ResourceClearValue, asDepth),
    "depth vk clear value have different size");

  // regardless of checks above i think this still can break
  memcpy(att.clearValue.color.uint32, clearValue.asUint, sizeof(clearValue.asUint));

  FormatStore fmt = image->getFormat();

  ImageViewState ivs;
  ivs.isCubemap = 0;
  ivs.setFormat(state.resource.ptr->allowSRGBWrite(index) ? fmt : fmt.getLinearVariant());
  ivs.setMipBase(mipLevel);
  ivs.setMipCount(1);

  if (layer >= d3d::RENDER_TO_WHOLE_ARRAY)
  {
    ivs.isArray = 1;
    ivs.setArrayBase(0);
    ivs.setArrayCount(image->getArrayLayers());
  }
  else
  {
    ivs.isArray = 0;
    ivs.setArrayBase(layer);
    ivs.setArrayCount(1);
  }
  ivs.isRenderTarget = 1;

  att.view = get_device().getImageView(image, ivs);
  att.layers = ivs.isArray ? ivs.getArrayCount() : 1;

#if VK_KHR_imageless_framebuffer
  if (get_device().hasImagelessFramebuffer())
  {
    att.info.usage = image->getUsage();
    att.info.flags = image->getCreateFlags();
    VkExtent3D baseExtent = image->getBaseExtent();
    att.info.width = baseExtent.width;
    att.info.height = baseExtent.height;
    att.info.layerCount = att.layers;

    viewFormatListFrom(fmt, att.info.usage, att.formats);
  }
#endif
}

template <>
void StateFieldRenderPassTarget::transit(uint32_t index, FrontRenderPassStateStorage &, DeviceContext &target) const
{
  CmdSetRenderPassTarget cmd{index, image, mipLevel, layer};

  static_assert(sizeof(cmd.clearValueArr) == sizeof(clearValue.asUint));
  memcpy(cmd.clearValueArr, clearValue.asUint, sizeof(cmd.clearValueArr));

  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldRenderPassTarget::dumpLog(uint32_t index, const FrontRenderPassStateStorage &) const
{
  debug("target%u: %p[%s], mip %u, layer %u, clearValue ui32x4[%08lX, %08lX, %08lX, %08lX]", index, image,
    image ? image->getDebugName() : "none", mipLevel, layer, clearValue.asUint[0], clearValue.asUint[1], clearValue.asUint[2],
    clearValue.asUint[3]);
}

template <>
void StateFieldRenderPassResource::applyTo(FrontRenderPassStateStorage &state, ExecutionState &) const
{
  if (!ptr)
    return;

  ptr->checkDead();
  ptr->useWithState(state);
}

template <>
void StateFieldRenderPassResource::transit(FrontRenderPassStateStorage &, DeviceContext &target) const
{
  CmdSetRenderPassResource cmd{ptr};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldRenderPassResource::dumpLog(const FrontRenderPassStateStorage &) const
{
  debug("rp: %p", ptr);
}

template <>
void StateFieldRenderPassResource::applyTo(BackGraphicsStateStorage &, ExecutionContext &target) const
{
  if (!ptr)
    return;

  ptr->checkDead();
  ptr->useWithAttachments(&target.back.executionState.get<BackGraphicsState, BackGraphicsState>().nativeRenderPassAttachments);
}

template <>
void StateFieldRenderPassResource::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("native rp: %p", ptr);
}

template <>
void StateFieldRenderPassSubpassIdx::applyTo(FrontRenderPassStateStorage &state, ExecutionState &target) const
{
  // reopen CR always
  target.set<StateFieldGraphicsConditionalRenderingScopeOpener, ConditionalRenderingState::InvalidateTag, BackGraphicsState>({});
  target.set<StateFieldGraphicsConditionalRenderingScopeCloser, ConditionalRenderingState::InvalidateTag, BackScopeState>({});

  // opening the pass
  if (data == 0)
  {
    // assign res ptr
    target.set<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>(state.resource.ptr);

    // close fb RP and open native one
    target.set<StateFieldGraphicsNativeRenderPassScopeCloser, bool, BackScopeState>(true);
    target.set<StateFieldGraphicsRenderPassScopeCloser, bool, BackScopeState>(false);
    target.set<StateFieldGraphicsRenderPassScopeOpener, bool, BackGraphicsState>(true);
  }
  // closing the pass
  else if (data == InvalidSubpass)
  {
    // StateFieldRenderPassResource will be cleared automatically
    RenderPassResource *rpRes = target.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
    if (rpRes)
    {
      // close native RP and open fb one
      target.set<StateFieldGraphicsNativeRenderPassScopeCloser, bool, BackScopeState>(false);
      target.set<StateFieldGraphicsRenderPassScopeCloser, bool, BackScopeState>(true);
      target.set<StateFieldGraphicsRenderPassScopeOpener, bool, BackGraphicsState>(true);
    }
  }
  // advance to next subpass
  else
    target.set<StateFieldGraphicsRenderPassScopeOpener, bool, BackGraphicsState>(true);
}

template <>
void StateFieldRenderPassSubpassIdx::transit(FrontRenderPassStateStorage &, DeviceContext &target) const
{
  CmdSetRenderPassSubpassIdx cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldRenderPassSubpassIdx::dumpLog(const FrontRenderPassStateStorage &) const
{
  debug("subpass: %p", data);
}

template <>
void StateFieldRenderPassArea::applyTo(FrontRenderPassStateStorage &, ExecutionState &) const
{}

template <>
void StateFieldRenderPassArea::transit(FrontRenderPassStateStorage &, DeviceContext &target) const
{
  CmdSetRenderPassArea cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldRenderPassArea::dumpLog(const FrontRenderPassStateStorage &) const
{
  debug("area: [%u, %u] - [%u, %u], Z [%f, %f]", data.left, data.top, data.width, data.height, data.minZ, data.maxZ);
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

StateFieldRenderPassTarget StateFieldRenderPassTarget::empty = {nullptr, 0, 0, {}};

void StateFieldRenderPassTarget::set(const StateFieldRenderPassTarget &v)
{
  image = v.image;
  layer = v.layer;
  mipLevel = v.mipLevel;
  clearValue = v.clearValue;
}

void StateFieldRenderPassTarget::set(const RenderPassTarget &v)
{
  image = v.resource.tex ? cast_to_texture_base(v.resource.tex)->getDeviceImage(true) : nullptr;
  layer = v.resource.layer;
  mipLevel = v.resource.mip_level;
  clearValue = v.clearValue;
}

bool StateFieldRenderPassTarget::diff(const RenderPassTarget &v) const
{
  if (image != (v.resource.tex ? cast_to_texture_base(v.resource.tex)->getDeviceImage(true) : nullptr))
    return true;
  if (layer != v.resource.layer)
    return true;
  if (mipLevel != v.resource.mip_level)
    return true;
  // omit this?
  // if (clearValue != v.clearValue)
  //   return true;
  return false;
}

bool StateFieldRenderPassTarget::diff(const StateFieldRenderPassTarget &v) const
{
  return v.image != image || v.layer != layer || v.mipLevel != mipLevel;
}