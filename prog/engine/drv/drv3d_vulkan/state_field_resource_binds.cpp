// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state_field_resource_binds.h"
#include "buffer.h"
#include "texture.h"
#include "util/backtrace.h"
#include "device_context.h"
#include "sampler_cache.h"
#include "buffer_alignment.h"
#include <drv/3d/dag_renderTarget.h>
#include "backend.h"
#include "execution_context.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldURegisterSet, uRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldTRegisterSet, tRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldBRegisterSet, bRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldSRegisterSet, sRegs, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldImmediateConst, immConst, StateFieldResourceBindsStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGlobalConstBuffer, globalConstBuf, StateFieldResourceBindsStorage);

bool isConstDepthStencilTargetForNativeRP(Image *img, ImageViewState view, RenderPassResource *rp)
{
  // avoid checking if image is not depth usable
  if (0 == ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)&img->getUsage()))
    return false;

  return rp->isCurrentDepthROAttachment(img, view);
}

bool isConstDepthStencilTarget(Image *img, ImageViewState view)
{
  if (0 == ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)&img->getUsage()))
    return false;

  FramebufferState &fbs = Backend::State::exec.get<BackGraphicsState, BackGraphicsState>().framebufferState;

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
void StateFieldURegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &target) const
{
  PipelineStageStateBase &sts = target.getResBinds(state.stage);

  if (data.buffer)
  {
    target.getExecutionContext().verifyResident(data.buffer.buffer);
    sts.setUbuffer(index, data.buffer);
  }
  else if (data.image)
  {
    VulkanImageViewHandle ivh;
    Image *actualImage = nullptr;

    target.getExecutionContext().verifyResident(data.image);

    ivh = data.image->getImageView(data.imageView);
    actualImage = data.image;

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
void StateFieldBRegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &target) const
{
  PipelineStageStateBase &sts = target.getResBinds(state.stage);

  if (sts.GCBSlot == index)
  {
    if (!data.buffer)
    {
      if (state.globalConstBuf.data)
      {
        target.getExecutionContext().verifyResident(state.globalConstBuf.data.buffer);
        sts.setBbuffer(index, state.globalConstBuf.data);
      }
      else
        sts.setBempty(index);
      sts.bGCBActive = true;
      return;
    }
    sts.bGCBActive = false;
  }

  if (data.buffer)
  {
    target.getExecutionContext().verifyResident(data.buffer);
    sts.setBbuffer(index, data);
  }
  else
    sts.setBempty(index);
}

template <>
void StateFieldTRegister::dumpLog(uint32_t index, const StateFieldResourceBindsStorage &state) const
{
  if (data.type == TRegister::TYPE_BUF)
    debug("regT %s:%u buf %p", formatShaderStage(state.stage), index, data.buf.buffer);
  else if (data.type == TRegister::TYPE_IMG)
    debug("regT %s:%u img %p", formatShaderStage(state.stage), index, data.img.ptr);
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
  PipelineStageStateBase &sts = target.getResBinds(state.stage);

  if (data.type == TRegister::TYPE_BUF)
  {
    target.getExecutionContext().verifyResident(data.buf.buffer);
    sts.setTbuffer(index, data.buf);
  }
  else if (data.type == TRegister::TYPE_IMG)
  {

    // very rare branch, will trigger if texture without stub is used before being loaded with contents
    if (DAGOR_UNLIKELY(!data.img.ptr))
    {
      D3D_ERROR("vulkan: used broken texture at slot %s:%u, check stub config for it and usage at caller site:\n%s",
        formatShaderStage(state.stage), index, target.getExecutionContext().getCurrentCmdCaller());
      return sts.setTempty(index);
    }
    target.getExecutionContext().verifyResident(data.img.ptr);

    VulkanImageViewHandle ivh = data.img.ptr->getImageView(data.img.view);
    bool isConstDs = false;
    RenderPassResource *rpRes = target.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
    if (rpRes)
      isConstDs = isConstDepthStencilTargetForNativeRP(data.img.ptr, data.img.view, rpRes);
    else
      isConstDs = (state.stage != STAGE_CS) && isConstDepthStencilTarget(data.img.ptr, data.img.view);

    sts.setTtexture(index, data.img.ptr, data.img.view, isConstDs, ivh);
  }
#if D3D_HAS_RAY_TRACING
  else if (data.type == TRegister::TYPE_AS)
    sts.setTas(index, data.rtas);
#endif
  else
    sts.setTempty(index);
}


template <>
void StateFieldSRegister::dumpLog(uint32_t index, const StateFieldResourceBindsStorage &state) const
{
  if (data.type == SRegister::TYPE_NULL)
    debug("regS %s:%u empty", formatShaderStage(state.stage), index);
  else if (data.type == SRegister::TYPE_RES)
    debug("regS %s:%u res sampler_info %p state %llu colorHandle %u compareHandle %u", formatShaderStage(state.stage), index,
      data.resPtr, data.resPtr->samplerInfo.state, data.resPtr->samplerInfo.colorSampler().value,
      data.resPtr->samplerInfo.compareSampler().value);
  else if (data.type == SRegister::TYPE_STATE)
    debug("regS %s:%u state %llu", formatShaderStage(state.stage), index, data.state);
  else
    debug("regS uknown sampler type %u", data.type);
}

template <>
void StateFieldSRegister::transit(uint32_t index, StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdTransitSRegister cmd{state.stage, index, data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldSRegister::applyTo(uint32_t index, StateFieldResourceBindsStorage &state, ExecutionState &target) const
{
  PipelineStageStateBase &sts = target.getResBinds(state.stage);

  if (data.type == SRegister::TYPE_NULL)
  {
    sts.setSempty(index);
    return;
  }

  const SamplerInfo *splInfo;
  if (data.type == SRegister::TYPE_RES)
    splInfo = &data.resPtr->samplerInfo;
  else
  {
    G_ASSERTF(data.type == SRegister::TYPE_STATE, "vulkan: unknown sampler type %u", data.type);
    TRegister &tReg = state.tRegs.data[index].data;
    // use cached sampler only when image is available
    if (tReg.type == TRegister::TYPE_IMG)
      splInfo = tReg.img.ptr->getSampler(data.state);
    else
      splInfo = Globals::samplers.get(data.state);
  }
  sts.setSSampler(index, splInfo);
}

template <>
bool StateFieldResourceBinds::handleObjectRemoval(Buffer *object)
{
  bool ret = false;
  StateFieldBRegister *bRegs = &get<StateFieldBRegisterSet, StateFieldBRegister>();
  StateFieldTRegister *tRegs = &get<StateFieldTRegisterSet, StateFieldTRegister>();
  StateFieldURegister *uRegs = &get<StateFieldURegisterSet, StateFieldURegister>();

  if (get<StateFieldGlobalConstBuffer, BufferRef>().buffer == object)
    ret |= true;

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
bool StateFieldResourceBinds::replaceResource(BufferRef old_obj, const BufferRef &new_obj, uint32_t flags)
{
  bool ret = false;
  if (SBCF_BIND_CONSTANT & flags)
  {
    StateFieldBRegister *bRegs = &get<StateFieldBRegisterSet, StateFieldBRegister>();
    const BufferRef bReg = new_obj;

    for (uint32_t j = 0; j < spirv::B_REGISTER_INDEX_MAX; ++j)
      if (bRegs[j].data == old_obj)
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
      if (tRegs[j].data.type == TRegister::TYPE_BUF && tRegs[j].data.buf == old_obj)
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
      if (uRegs[j].data.buffer == old_obj)
      {
        set<StateFieldURegisterSet, StateFieldURegister::Indexed>({j, uReg});
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
bool StateFieldResourceBinds::handleObjectRemoval(SamplerResource *object)
{
  bool ret = false;
  StateFieldSRegister *sRegs = &get<StateFieldSRegisterSet, StateFieldSRegister>();

  for (uint32_t j = 0; j < spirv::S_REGISTER_INDEX_MAX; ++j)
    if (sRegs[j].data.type == SRegister::TYPE_RES && sRegs[j].data.resPtr == object)
    {
      set<StateFieldSRegisterSet, uint32_t>(j);
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

  if (getRO<StateFieldGlobalConstBuffer, BufferRef>().buffer == object)
    return true;

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
bool StateFieldResourceBinds::isReferenced(SamplerResource *object) const
{
  const StateFieldSRegister *sRegs = &getRO<StateFieldSRegisterSet, StateFieldSRegister>();

  for (uint32_t j = 0; j < spirv::S_REGISTER_INDEX_MAX; ++j)
    if (sRegs[j].data.type == SRegister::TYPE_RES && sRegs[j].data.resPtr == object)
      return true;

  return false;
}

template <>
void StateFieldImmediateConst::dumpLog(const StateFieldResourceBindsStorage &state) const
{
  debug("%s: immediate consts [%08X, %08X, %08X, %08X]", formatShaderStage(state.stage), data[0], data[1], data[2], data[3]);
}

template <>
void StateFieldImmediateConst::applyTo(StateFieldResourceBindsStorage &state, ExecutionState &target) const
{
  // TODO: reimplement with vkCmdPushConstants

  // don't bind buffer if disabled
  // user binding will be restored by state apply order
  if (!enabled)
    return;

  ImmediateConstBuffer &icb = Backend::immediateConstBuffers[state.stage];
  PipelineStageStateBase &sts = target.getResBinds(state.stage);

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
  if (data)
    debug("%s: global cb [buf: %p handle: %p, offset: %u, size: %u]", formatShaderStage(state.stage), data.buffer,
      generalize(data.buffer->getHandle()), data.bufOffset(0), data.visibleDataSize);
  else
    debug("%s: global cb [empty]", formatShaderStage(state.stage));
}

template <>
void StateFieldGlobalConstBuffer::applyTo(StateFieldResourceBindsStorage &state, ExecutionState &target) const
{
  PipelineStageStateBase &sts = target.getResBinds(state.stage);
  if (data && sts.bGCBActive)
  {
    target.getExecutionContext().verifyResident(data.buffer);
    sts.setBbuffer(sts.GCBSlot, data);
  }
}

template <>
void StateFieldGlobalConstBuffer::transit(StateFieldResourceBindsStorage &state, DeviceContext &target) const
{
  CmdSetConstRegisterBuffer cmd{data, state.stage};
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

    buf->markNonCoherentRangeLoc(0, offset - alignedElementSize, true);
  }
}

void ImmediateConstBuffer::init() { alignedElementSize = Globals::VK::bufAlign.alignSize(element_size); }

BufferRef ImmediateConstBuffer::push(const uint32_t *data)
{
  Buffer *buf = ring[ringIdx];

  if (!buf || offset >= buf->getBlockSize())
  {
    if (buf)
    {
      flushWrites();
      Backend::gpuJob.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*buf);
    }

    buf = Buffer::create(alignedElementSize * initial_blocks, DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER, 1,
      BufferMemoryFlags::NONE);
    offset = 0;
    ring[ringIdx] = buf;
  }

  memcpy(buf->ptrOffsetLoc(offset), data, element_size);

  BufferRef ret(buf);
  ret.offset = offset;
  ret.visibleDataSize = alignedElementSize;
  offset += alignedElementSize;
  return ret;
}

void ImmediateConstBuffer::onFlush()
{
  flushWrites();

  ringIdx = (ringIdx + 1) % ring_size;
  offset = 0;
}

void ImmediateConstBuffer::shutdown()
{
  for (Buffer *i : ring)
  {
    if (i)
    {
      G_ASSERTF(Backend::State::pendingCleanups.removeWithReferenceCheck(i, Backend::State::pipe),
        "vulkan: immediate cb is still bound");
      Backend::gpuJob.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*i);
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

  GenericBufferInterface *gb = (GenericBufferInterface *)sb;
  buf = gb->getBufferRef();
  if (!buf)
    buf = gb->fillFrameMemWithDummyData();
  type = TYPE_BUF;
}

TRegister::TRegister(BaseTex *texture) : type(TYPE_NULL) //-V1077
{
  if (!texture)
    return;

  img.ptr = texture->getDeviceImage();
  img.view = texture->getViewInfo();

  G_ASSERTF(img.ptr, "vulkan: trying to bind texture %p:%s without underlying image", texture, texture->getResName());
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
    image = texture->getDeviceImage();
    imageView = texture->getViewInfoUAV(mip_level, face, as_uint);
  }
}

URegister::URegister(Sbuffer *sb)
{
  if (!sb)
    return;

  GenericBufferInterface *gb = (GenericBufferInterface *)sb;
  buffer = gb->getBufferRef();
  if (!buffer)
    buffer = gb->fillFrameMemWithDummyData();
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
