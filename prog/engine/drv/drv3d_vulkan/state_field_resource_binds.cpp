#include "device.h"
#include "state_field_resource_binds.h"
#include "buffer.h"
#include "texture.h"
#include "util/backtrace.h"
#include "device_context.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldURegisterSet, uRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldTRegisterSet, tRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldBRegisterSet, bRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldImmediateConst, immConst, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGlobalConstBuffer, globalConstBuf, StateFieldResourceBindsStorage);

bool hasResourceConflictWithFramebuffer(Image *img, ImageViewState view, ShaderStage stage)
{
  // FIXME: try to bind textures back if FB changes
  // but maybe this should be a debug only code? as this code is quite HOT and checking quite rare case here
  // looks like thing that should be removed

  // cs can't conflict with FB right now, as no async compute is in action
  if (stage == STAGE_CS)
    return false;

  if (0 == ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) & img->getUsage()))
    return false;

  ContextBackend &back = get_device().getContext().getBackend();
  FramebufferState &fbs = back.executionState.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  RenderPassClass::FramebufferDescription &fbi = fbs.frameBufferInfo;

  if (img->getUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
  {
    uint32_t i;
    uint32_t bit;
    for (i = 0, bit = 1; i < Driver3dRenderTarget::MAX_SIMRT; ++i, bit = bit << 1)
    {
      if (!(fbs.renderPassClass.colorTargetMask & bit))
        continue;

      G_ASSERTF(!fbi.colorAttachments[i].empty(), "vulkan: empty attachment %u while it used by render pass class", i);
      if (fbi.colorAttachments[i].img != img)
        continue;

      const ImageViewState &rtView = fbi.colorAttachments[i].view;

      const ValueRange<uint32_t> rtMipRange(rtView.getMipBase(), rtView.getMipBase() + rtView.getMipCount());
      const ValueRange<uint32_t> rtArrayRange(rtView.getArrayBase(), rtView.getArrayBase() + rtView.getMipCount());

      const ValueRange<uint32_t> mipRange(view.getMipBase(), view.getMipBase() + view.getMipCount());
      const ValueRange<uint32_t> arrayRange(view.getArrayBase(), view.getArrayBase() + view.getMipCount());

      if (mipRange.overlaps(rtMipRange) && arrayRange.overlaps(rtArrayRange))
        return true;
    }
  }

  if (img->getUsage() & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
  {
    if (fbs.renderPassClass.hasDepth() && !fbs.renderPassClass.hasRoDepth())
    {
      G_ASSERTF(!fbi.depthStencilAttachment.empty(), "vulkan: empty depth attachment while it used by render pass class");
      if (fbi.depthStencilAttachment.img != img)
        return false;

      const ImageViewState &dsView = fbi.depthStencilAttachment.view;

      const ValueRange<uint32_t> mipRange(view.getMipBase(), view.getMipBase() + view.getMipCount());
      const ValueRange<uint32_t> arrayRange(view.getArrayBase(), view.getArrayBase() + view.getMipCount());

      auto mipIndex = dsView.getMipBase();
      auto arrayIndex = dsView.getArrayBase();
      if (mipRange.isInside(mipIndex) && arrayRange.isInside(arrayIndex))
        return true;
    }
  }

  return false;
}

bool isConstDepthStencilTargetForNativeRP(Image *img)
{
  // avoid checking if image is not depth usable
  if (0 == ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)&img->getUsage()))
    return false;

  // check only state, as it should be properly setted up ahead of this method
  return img->layout.get(0, 0) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

bool isConstDepthStencilTarget(Image *img, ImageViewState view)
{
  if (0 == ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)&img->getUsage()))
    return false;

  ContextBackend &back = get_device().getContext().getBackend();
  FramebufferState &fbs = back.executionState.get<BackGraphicsState, BackGraphicsState>().framebufferState;

  if (fbs.renderPassClass.hasRoDepth())
  {
    const RenderPassClass::FramebufferDescription::AttachmentInfo &dsai = fbs.frameBufferInfo.depthStencilAttachment;
    G_ASSERTF(!dsai.empty(), "vulkan: empty depth attachment while it used by render pass class");
    if (dsai.img != img)
      return false;

    const ValueRange<uint32_t> mipRange(view.getMipBase(), view.getMipBase() + view.getMipCount());
    const ValueRange<uint32_t> arrayRange(view.getArrayBase(), view.getArrayBase() + view.getMipCount());

    auto mipIndex = dsai.view.getMipBase();
    auto arrayIndex = dsai.view.getArrayBase();
    if (mipRange.isInside(mipIndex) && arrayRange.isInside(arrayIndex))
      return true;
  }

  return false;
}

template <>
void StateFieldURegister::dumpLog(uint32_t index, const StateFieldResourceBindsStorage &state) const
{
  debug("regU %s:%u img %p buf %p", formatShaderStage(state.stage), index, data.image, data.buffer.buffer);
}

template <>
void StateFieldURegister::transit(uint32_t index, StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdTransitURegister cmd{state.stage, index, data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldURegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &) const
{
  // TODO: redo stage state base with tracked state, put in execution state and correct this code
  Device &drvDev = get_device();
  PipelineStageStateBase &sts = get_device().getContext().getBackend().contextState.stageState[state.stage];

  if (data.buffer)
    sts.setUbuffer(index, data.buffer);
  else if (data.image)
  {
    VulkanImageViewHandle ivh;
    Image *actualImage = nullptr;

    if (data.image->isResident())
    {
      if (!hasResourceConflictWithFramebuffer(data.image, data.imageView, state.stage))
      {
        ivh = drvDev.getImageView(data.image, data.imageView);
        actualImage = data.image;
      }
      else
      {
        // debug("vulkan: regU %s:%u:%p (%s) conflicts with FB",
        //   formatShaderStage(state.stage), index, data.image, data.image->getDebugName());
      }
    }

    sts.setUtexture(index, actualImage, data.imageView, ivh);
  }
  else
    sts.setUempty(index);
}

template <>
void StateFieldBRegister::dumpLog(uint32_t index, const StateFieldResourceBindsStorage &state) const
{
  debug("regB %s:%u buf %p", formatShaderStage(state.stage), index, data.buffer);
}

template <>
void StateFieldBRegister::transit(uint32_t index, StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdTransitBRegister cmd{state.stage, index, data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldBRegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &) const
{
  // TODO: redo stage state base with tracked state, put in execution state and correct this code
  PipelineStageStateBase &sts = get_device().getContext().getBackend().contextState.stageState[state.stage];
  if (data.buffer)
    sts.setBbuffer(index, data);
  else
    sts.setBempty(index);
}

template <>
void StateFieldTRegister::dumpLog(uint32_t index, const StateFieldResourceBindsStorage &state) const
{
  if (data.type == TRegister::TYPE_BUF)
    debug("regT %s:%u buf %p", formatShaderStage(state.stage), index, data.buf.buffer);
  else if (data.type == TRegister::TYPE_IMG)
    debug("regT %s:%u img %p spl %llu", formatShaderStage(state.stage), index, data.img.ptr, (uint64_t)data.img.sampler);
#if D3D_HAS_RAY_TRACING
  else if (data.type == TRegister::TYPE_AS)
    debug("regT %s:%u as %p", data.rtas);
#endif
  else
    debug("regT %s:%u empty", formatShaderStage(state.stage), index);
}

template <>
void StateFieldTRegister::transit(uint32_t index, StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdTransitTRegister cmd{state.stage, index, data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldTRegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &target) const
{

  // TODO: redo stage state base with tracked state, put in execution state and correct this code
  Device &drvDev = get_device();
  PipelineStageStateBase &sts = get_device().getContext().getBackend().contextState.stageState[state.stage];

  if (data.type == TRegister::TYPE_BUF)
    sts.setTbuffer(index, data.buf);
  else if (data.type == TRegister::TYPE_IMG)
  {
    if (data.isSwapchainColor)
    {
      auto image = drvDev.swapchain.getColorImage();

      // need to update to the actual format of the swapchain as the proxy texture might has
      // not the right one (a format that can not be represented by TEXFMT_)
      auto patchedView = data.img.view;
      auto reqFormat = data.img.view.getFormat();
      auto replacementFormat = image->getFormat();
      replacementFormat.isSrgb = reqFormat.isSrgb && replacementFormat.isSrgbCapableFormatType();
      patchedView.setFormat(replacementFormat);
      auto iv = drvDev.getImageView(image, patchedView);

      if (!hasResourceConflictWithFramebuffer(image, patchedView, state.stage))
        sts.setTtexture(index, image, patchedView, image->getSampler(data.img.sampler), false, iv);
      else
      {
        // debug("vulkan: regT %s:%u:%p (%s) conflicts with FB",
        //   formatShaderStage(state.stage), index, image, image->getDebugName());
        sts.setTempty(index);
      }
    }
    else if (data.isSwapchainDepth)
    {
      Swapchain &swapchain = drvDev.swapchain;
      auto image = swapchain.getDepthStencilImageForExtent(swapchain.getActiveMode().extent,
        get_device().getContext().getBackend().contextState.frame.get());
      auto iv = drvDev.getImageView(image, data.img.view);

      if (!hasResourceConflictWithFramebuffer(image, data.img.view, state.stage))
      {
        bool isConstDs = isConstDepthStencilTarget(image, data.img.view) && (state.stage != STAGE_CS);
        sts.setTtexture(index, image, data.img.view, image->getSampler(data.img.sampler), isConstDs, iv);
      }
      else
      {
        // debug("vulkan: regT %s:%u:%p (%s) conflicts with FB",
        //   formatShaderStage(state.stage), index, image, image->getDebugName());
        sts.setTempty(index);
      }
    }
    else
    {
      // very rare branch, will trigger if texture without stub is used before being loaded with contents
      if (DAGOR_UNLIKELY(!data.img.ptr))
      {
        logerr("vulkan: used broken texture at slot %s:%u, check stub config for it and usage at caller site:\n%s",
          formatShaderStage(state.stage), index, target.getExecutionContext().getCurrentCmdCaller());
        return sts.setTempty(index);
      }

      VulkanImageViewHandle ivh;
      Image *actualImage = nullptr;
      bool isConstDs = false;
      // FIXME: this is too HOT code to handle it here, need to handle it at residency change time
      // if (data.img.ptr->isResident())
      {
        RenderPassResource *rpRes = target.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
        if (rpRes)
        {
          ivh = drvDev.getImageView(data.img.ptr, data.img.view);
          isConstDs = isConstDepthStencilTargetForNativeRP(data.img.ptr);
          actualImage = data.img.ptr;
        }
        else if (!hasResourceConflictWithFramebuffer(data.img.ptr, data.img.view, state.stage))
        {
          ivh = drvDev.getImageView(data.img.ptr, data.img.view);
          isConstDs = isConstDepthStencilTarget(data.img.ptr, data.img.view) && (state.stage != STAGE_CS);
          actualImage = data.img.ptr;
        }
        else
        {
          return sts.setTempty(index);
          // debug("vulkan: regT %s:%u:%p (%s) conflicts with FB",
          //   formatShaderStage(state.stage), index, data.img.ptr, data.img.ptr->getDebugName());
        }
      }

      sts.setTtexture(index, actualImage, data.img.view, actualImage ? actualImage->getSampler(data.img.sampler) : nullptr, isConstDs,
        ivh);
    }
  }
#if D3D_HAS_RAY_TRACING
  else if (data.type == TRegister::TYPE_AS)
    sts.setTas(index, data.rtas);
#endif
  else
    sts.setTempty(index);
}

template <>
bool StateFieldResourceBinds::handleObjectRemoval(Buffer *object)
{
  bool ret = false;
  StateFieldBRegister *bRegs = &get<StateFieldBRegisterSet, StateFieldBRegister>();
  StateFieldTRegister *tRegs = &get<StateFieldTRegisterSet, StateFieldTRegister>();
  StateFieldURegister *uRegs = &get<StateFieldURegisterSet, StateFieldURegister>();

  for (uint32_t j = 0; j < spirv::B_REGISTER_INDEX_MAX; ++j)
  {
    if (bRegs[j].data.buffer == object)
    {
      set<StateFieldBRegisterSet, uint32_t>(j);
      ret |= true;
    }
  }

  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
  {
    if (tRegs[j].data.type == TRegister::TYPE_BUF && tRegs[j].data.buf.buffer == object)
    {
      set<StateFieldTRegisterSet, uint32_t>(j);
      ret |= true;
    }
  }

  for (uint32_t j = 0; j < spirv::U_REGISTER_INDEX_MAX; ++j)
  {
    if (uRegs[j].data.buffer.buffer == object)
    {
      set<StateFieldURegisterSet, uint32_t>(j);
      ret |= true;
    }
  }
  return ret;
}

template <>
bool StateFieldResourceBinds::replaceResource(Buffer *old_obj, const BufferRef &new_obj, uint32_t flags)
{
  bool ret = false;
  if (SBCF_BIND_CONSTANT & flags)
  {
    StateFieldBRegister *bRegs = &get<StateFieldBRegisterSet, StateFieldBRegister>();
    const BufferRef bReg = new_obj;

    for (uint32_t j = 0; j < spirv::B_REGISTER_INDEX_MAX; ++j)
      if (bRegs[j].data.buffer == old_obj)
      {
        set<StateFieldBRegisterSet, StateFieldBRegister::Indexed>({j, bReg});
        ret |= true;
      }
  }

  if (SBCF_BIND_SHADER_RES & flags)
  {
    StateFieldTRegister *tRegs = &get<StateFieldTRegisterSet, StateFieldTRegister>();
    TRegister tReg;
    tReg.buf = new_obj;
    tReg.type = TRegister::TYPE_BUF;

    for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
      if (tRegs[j].data.type == TRegister::TYPE_BUF && tRegs[j].data.buf.buffer == old_obj)
      {
        set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({j, tReg});
        ret |= true;
      }
  }

  if (SBCF_BIND_UNORDERED & flags)
  {
    StateFieldURegister *uRegs = &get<StateFieldURegisterSet, StateFieldURegister>();
    URegister uReg;
    uReg.buffer = new_obj;

    for (uint32_t j = 0; j < spirv::U_REGISTER_INDEX_MAX; ++j)
      if (uRegs[j].data.buffer.buffer == old_obj)
      {
        set<StateFieldURegisterSet, StateFieldURegister::Indexed>({j, uReg});
        ret |= true;
      }
  }

  return ret;
}

template <>
bool StateFieldResourceBinds::handleObjectRemoval(Image *object)
{
  bool ret = false;
  StateFieldTRegister *tRegs = &get<StateFieldTRegisterSet, StateFieldTRegister>();
  StateFieldURegister *uRegs = &get<StateFieldURegisterSet, StateFieldURegister>();

  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
    if (tRegs[j].data.type == TRegister::TYPE_IMG && tRegs[j].data.img.ptr == object)
    {
      set<StateFieldTRegisterSet, uint32_t>(j);
      ret |= true;
    }

  for (uint32_t j = 0; j < spirv::U_REGISTER_INDEX_MAX; ++j)
    if (uRegs[j].data.image == object)
    {
      set<StateFieldURegisterSet, uint32_t>(j);
      ret |= true;
    }

  return ret;
}

template <>
bool StateFieldResourceBinds::isReferenced(Image *object) const
{
  const StateFieldTRegister *tRegs = &getRO<StateFieldTRegisterSet, StateFieldTRegister>();
  const StateFieldURegister *uRegs = &getRO<StateFieldURegisterSet, StateFieldURegister>();

  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
    if (tRegs[j].data.type == TRegister::TYPE_IMG && tRegs[j].data.img.ptr == object)
      return true;

  for (uint32_t j = 0; j < spirv::U_REGISTER_INDEX_MAX; ++j)
    if (uRegs[j].data.image == object)
      return true;

  return false;
}

template <>
bool StateFieldResourceBinds::isReferenced(Buffer *object) const
{
  const StateFieldBRegister *bRegs = &getRO<StateFieldBRegisterSet, StateFieldBRegister>();
  const StateFieldTRegister *tRegs = &getRO<StateFieldTRegisterSet, StateFieldTRegister>();
  const StateFieldURegister *uRegs = &getRO<StateFieldURegisterSet, StateFieldURegister>();

  for (uint32_t j = 0; j < spirv::B_REGISTER_INDEX_MAX; ++j)
    if (bRegs[j].data.buffer == object)
      return true;

  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
    if (tRegs[j].data.type == TRegister::TYPE_BUF && tRegs[j].data.buf.buffer == object)
      return true;

  for (uint32_t j = 0; j < spirv::U_REGISTER_INDEX_MAX; ++j)
    if (uRegs[j].data.buffer.buffer == object)
      return true;

  return false;
}

template <>
void StateFieldImmediateConst::dumpLog(const StateFieldResourceBindsStorage &state) const
{
  debug("%s: immediate consts [%08X, %08X, %08X, %08X]", formatShaderStage(state.stage), data[0], data[1], data[2], data[3]);
}

template <>
void StateFieldImmediateConst::applyTo(StateFieldResourceBindsStorage &state, ExecutionState &) const
{
  // TODO: reimplement with vkCmdPushConstants

  // don't bind buffer if disabled
  // user binding will be restored by state apply order
  if (!enabled)
    return;

  ContextBackend &back = get_device().getContext().getBackend();
  ImmediateConstBuffer &icb = back.immediateConstBuffers[state.stage];
  PipelineStageStateBase &sts = back.contextState.stageState[state.stage];

  sts.setBbuffer(IMMEDAITE_CB_REGISTER_NO, icb.push(&data[0]));
}

template <>
void StateFieldImmediateConst::transit(StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdSetImmediateConsts cmd{state.stage, enabled};
  memcpy(cmd.data, data, sizeof(data));
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGlobalConstBuffer::dumpLog(const StateFieldResourceBindsStorage &state) const
{
  debug("%s: global cb [handle: %p, offset: %u, size: %u]", formatShaderStage(state.stage), ref.buffer, ref.offset, ref.size);
}

template <>
void StateFieldGlobalConstBuffer::applyTo(StateFieldResourceBindsStorage &state, ExecutionState &) const
{
  ContextBackend &back = get_device().getContext().getBackend();
  PipelineStageStateBase &sts = back.contextState.stageState[state.stage];

  sts.constRegisterLastBuffer = ref.buffer;
  sts.constRegisterLastBufferOffset = ref.offset;
  sts.constRegisterLastBufferRange = ref.size;
  sts.bRegisterValidMask &= ~1ul;
}

template <>
void StateFieldGlobalConstBuffer::transit(StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdSetConstRegisterBuffer cmd{ref.buffer, ref.offset, ref.size, state.stage};
  target.dispatchCommandNoLock(cmd);
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void ImmediateConstBuffer::flushWrites()
{
  if (offset)
  {
    Buffer *buf = ring[ringIdx];
    G_ASSERTF(buf, "vulkan: ImmediateConstBuffer: buffer or offset changed independently to each other");

    VkDeviceSize absOffset = (offset - 1) * blockSize;
    buf->markNonCoherentRange(buf->dataOffset(0), absOffset, true);
  }
}

BufferRef ImmediateConstBuffer::push(const uint32_t *data)
{
  Buffer *buf = ring[ringIdx];

  if (!buf || offset >= buf->getMaxDiscardLimit())
  {
    if (buf)
    {
      flushWrites();
      get_device().getContext().getBackend().contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*buf);
    }

    buf = get_device().createBuffer(element_size, DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER, initial_blocks,
      BufferMemoryFlags::NONE);
    blockSize = buf->dataSize();
    offset = 0;
    ring[ringIdx] = buf;
  }

  VkDeviceSize absOffset = offset * blockSize;
  memcpy(buf->dataPointer(absOffset), data, element_size);

  BufferRef ret(buf);
  ret.discardIndex = offset;
  ++offset;
  return ret;
}

void ImmediateConstBuffer::onFlush()
{
  flushWrites();

  ringIdx = (ringIdx + 1) % ring_size;
  offset = 0;
}

void ImmediateConstBuffer::shutdown(ExecutionContext &ctx)
{
  for (Buffer *i : ring)
  {
    if (i)
    {
      G_ASSERTF(ctx.back.pipelineStatePendingCleanups.removeWithReferenceCheck(i, ctx.back.pipelineState),
        "vulkan: immediate cb is still bound");
      ctx.back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*i);
    }
  }
}

void StateFieldResourceBindsStorage::makeDirty()
{
  uRegs.makeDirty();
  tRegs.makeDirty();
  bRegs.makeDirty();
}

void StateFieldResourceBindsStorage::clearDirty()
{
  uRegs.clearDirty();
  tRegs.clearDirty();
  bRegs.clearDirty();
}

// MSVC decides that zero-init ctors in union is non-trivial, making itself unhappy about any ctors
// but code is initializing relevant union fields, so just silence this warning up
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4582)
#endif

TRegister::TRegister(Sbuffer *sb) : type(TYPE_NULL) //-V730
{
  if (!sb)
    return;

  buf = BufferRef{((GenericBufferInterface *)sb)->getDeviceBuffer()};
  type = TYPE_BUF;
}

TRegister::TRegister(BaseTexture *in_texture) : type(TYPE_NULL) //-V1077
{
  if (!in_texture)
    return;

  BaseTex *texture = cast_to_texture_base(in_texture);

  if (texture->isStub())
    texture = texture->getStubTex();

  img.ptr = texture->getDeviceImage(false);
  img.view = texture->getViewInfo();
  img.sampler = texture->samplerState;

  isSwapchainColor = (texture == d3d::get_backbuffer_tex()) ? 1 : 0;
  isSwapchainDepth = (texture == d3d::get_backbuffer_tex_depth()) ? 1 : 0;
  type = TYPE_IMG;
}

#if D3D_HAS_RAY_TRACING
TRegister::TRegister(RaytraceAccelerationStructure *in_as) : type(TYPE_NULL) //-V730
{
  if (!in_as)
    return;

  rtas = in_as;
  type = TYPE_AS;
}
#endif

#ifdef _MSC_VER
// C4582 off
#pragma warning(pop)
#endif

URegister::URegister(BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint)
{
  BaseTex *texture = cast_to_texture_base(tex);
  if (texture)
  {
    image = texture->getDeviceImage(true);
    imageView = texture->getViewInfoUAV(mip_level, face, as_uint);
  }
}

URegister::URegister(Sbuffer *sb)
{
  if (sb)
    buffer = BufferRef{((GenericBufferInterface *)sb)->getDeviceBuffer()};
}

// bool StateFieldTRegister::diff(const BufferRef& new_buf) const
//{
//   return (new_buf != data.buffer);
// }

// void StateFieldTRegister::set(const BufferRef& new_buf)
//{
//   data.buffer = new_buf;
//   data.image = nullptr;
//   data.sampler = nullptr;
// #if D3D_HAS_RAY_TRACING
//   data.as = nullptr;
// #endif
// }
