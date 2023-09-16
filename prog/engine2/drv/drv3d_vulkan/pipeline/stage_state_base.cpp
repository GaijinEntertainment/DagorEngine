#include "device.h"

using namespace drv3d_vulkan;

PipelineStageStateBase::PipelineStageStateBase()
{
  for (uint32_t i = 0; i < spirv::B_REGISTER_INDEX_MAX; ++i)
    registerTable[spirv::B_CONST_BUFFER_OFFSET + i].buffer.offset = 0;
  for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
    registerTable[spirv::T_SAMPLED_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
    registerTable[spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  for (uint32_t i = 0; i < spirv::U_REGISTER_INDEX_MAX; ++i)
    registerTable[spirv::U_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  for (uint32_t i = 0; i < spirv::TOTAL_REGISTER_COUNT; ++i)
    registerTable[i].clear();
}

void PipelineStageStateBase::invalidateState()
{
  bRegisterValidMask = 0;
  tRegisterValidMask = 0;
  uRegisterValidMask = 0;
  lastDescriptorSet = VulkanNullHandle();
}

void PipelineStageStateBase::syncDepthROStateInT(Image *image, uint32_t, uint32_t, bool ro_ds)
{
  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
  {
    if (tRegisters[j].type != TRegister::TYPE_IMG)
      continue;

    bool imageMatch = tRegisters[j].img.ptr == image;
    bool dropConstFromNonCurrentTarget = !imageMatch && isConstDepthStencil.test(j);
    bool currentTargetConstMismatch = imageMatch && (isConstDepthStencil.test(j) != ro_ds);

    if (currentTargetConstMismatch && ro_ds)
    {
      isConstDepthStencil.set(j);

      getSampledImageRegister(j).image.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      getSampledImageWithCompareRegister(j).image.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else if (dropConstFromNonCurrentTarget || (currentTargetConstMismatch && !ro_ds))
    {
      isConstDepthStencil.reset(j);

      getSampledImageRegister(j).image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      getSampledImageWithCompareRegister(j).image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else
      continue;

    tRegisterValidMask &= ~(1 << j);
  }
}

void PipelineStageStateBase::setBempty(uint32_t unit)
{
  bRegisterValidMask &= ~(1 << unit);

  bRegisters[unit] = BufferRef{};

  getConstBufferRegister(unit).clear();
}

void PipelineStageStateBase::setTempty(uint32_t unit)
{
  tRegisterValidMask &= ~(1 << unit);
  tRegisters[unit].type = TRegister::TYPE_NULL;

  getBufferAsSampledImageRegister(unit).clear();
  getReadOnlyBufferRegister(unit).clear();
  getSampledImageRegister(unit).clear();
  getSampledImageWithCompareRegister(unit).clear();
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  getRaytraceAccelerationStructureRegister(unit).clear();
#endif
}

void PipelineStageStateBase::setUempty(uint32_t unit)
{
  uRegisterValidMask &= ~(1 << unit);

  uRegisters[unit].image = nullptr;
  uRegisters[unit].imageView = ImageViewState();
  uRegisters[unit].buffer = BufferRef{};

  getStorageBufferAsImageRegister(unit).clear();
  getStorageBufferRegister(unit).clear();
  getStorageBufferWithCounterRegister(unit).clear();
  getStorageImageRegister(unit).clear();
}

void PipelineStageStateBase::setTinputAttachment(uint32_t unit, Image *image, bool as_const_ds, VulkanImageViewHandle view)
{
  VkAnyDescriptorInfo &regRef = getInputAttachmentRegister(unit);
  if (regRef.image.imageView == view && regRef.type == VkAnyDescriptorInfo::TYPE_IMG)
    return;

  tRegisterValidMask &= ~(1 << unit);

  if (image)
  {
    VkImageLayout layout = as_const_ds ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    regRef.image = {VK_NULL_HANDLE, view, layout};
    regRef.type = VkAnyDescriptorInfo::TYPE_IMG;
  }
  else
    regRef.clear();
}

void PipelineStageStateBase::setTtexture(uint32_t unit, Image *image, ImageViewState view_state, SamplerInfo *sampler,
  bool as_const_ds, VulkanImageViewHandle view)
{
  tRegisterValidMask &= ~(1 << unit);

  tRegisters[unit].type = TRegister::TYPE_IMG;
  tRegisters[unit].img.ptr = image;
  tRegisters[unit].img.view = view_state;
  isConstDepthStencil.set(unit, as_const_ds);

  getBufferAsSampledImageRegister(unit).clear();
  getReadOnlyBufferRegister(unit).clear();
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  getRaytraceAccelerationStructureRegister(unit).clear();
#endif

  G_ASSERTF(image, "vulkan: binding empty tReg as image, should be filtered at bind point!");
  VkImageLayout layout = as_const_ds ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  getSampledImageRegister(unit).image = {sampler->sampler[0], view, layout};
  getSampledImageRegister(unit).type = VkAnyDescriptorInfo::TYPE_IMG;
  getSampledImageWithCompareRegister(unit).image = {sampler->sampler[1], view, layout};
  getSampledImageWithCompareRegister(unit).type = VkAnyDescriptorInfo::TYPE_IMG;
}

void PipelineStageStateBase::setUtexture(uint32_t unit, Image *image, ImageViewState view_state, VulkanImageViewHandle view)
{
  uRegisterValidMask &= ~(1 << unit);

  uRegisters[unit].image = image;
  uRegisters[unit].imageView = view_state;
  uRegisters[unit].buffer = BufferRef{};

  getStorageBufferAsImageRegister(unit).clear();
  getStorageBufferRegister(unit).clear();
  getStorageBufferWithCounterRegister(unit).clear();

  if (!is_null(view))
  {
    getStorageImageRegister(unit).image.imageView = view;
    getStorageImageRegister(unit).type = VkAnyDescriptorInfo::TYPE_IMG;
  }
  else
    getStorageImageRegister(unit).clear();
}

void PipelineStageStateBase::setTbuffer(uint32_t unit, BufferRef buffer)
{
  tRegisterValidMask &= ~(1 << unit);

  tRegisters[unit].type = TRegister::TYPE_BUF;
  tRegisters[unit].buf = buffer;

  getSampledImageRegister(unit).clear();
  getSampledImageWithCompareRegister(unit).clear();
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  getRaytraceAccelerationStructureRegister(unit).clear();
#endif

  G_ASSERTF(buffer, "vulkan: binding empty tReg as buffer, should be filtered at bind point!");
  getBufferAsSampledImageRegister(unit).texelBuffer = buffer.getView();
  getBufferAsSampledImageRegister(unit).type = VkAnyDescriptorInfo::TYPE_BUF_VIEW;
  getReadOnlyBufferRegister(unit).buffer = {buffer.getHandle(), buffer.dataOffset(0), buffer.dataSize()};
  getReadOnlyBufferRegister(unit).type = VkAnyDescriptorInfo::TYPE_BUF;
}

void PipelineStageStateBase::setUbuffer(uint32_t unit, BufferRef buffer)
{
  uRegisterValidMask &= ~(1 << unit);

  uRegisters[unit].buffer = buffer;
  uRegisters[unit].image = nullptr;
  uRegisters[unit].imageView = ImageViewState();

  getStorageBufferWithCounterRegister(unit).clear();
  getStorageImageRegister(unit).clear();

  if (buffer)
  {
    getStorageBufferAsImageRegister(unit).texelBuffer = buffer.getView();
    getStorageBufferAsImageRegister(unit).type = VkAnyDescriptorInfo::TYPE_BUF_VIEW;
    getStorageBufferRegister(unit).buffer = {buffer.getHandle(), buffer.dataOffset(0), buffer.dataSize()};
    getStorageBufferRegister(unit).type = VkAnyDescriptorInfo::TYPE_BUF;
  }
  else
  {
    getStorageBufferAsImageRegister(unit).clear();
    getStorageBufferRegister(unit).clear();
  }
}

void PipelineStageStateBase::setBbuffer(uint32_t unit, BufferRef buffer)
{
  bRegisterValidMask &= ~(1 << unit);

  bRegisters[unit] = buffer;

  if (buffer)
  {
    getConstBufferRegister(unit).buffer.buffer = buffer.getHandle();
    getConstBufferRegister(unit).buffer.range = buffer.dataSize();
    getConstBufferRegister(unit).type = VkAnyDescriptorInfo::TYPE_BUF;
  }
  else
    getConstBufferRegister(unit).clear();
}

#if D3D_HAS_RAY_TRACING
void PipelineStageStateBase::setTas(uint32_t unit, RaytraceAccelerationStructure *as)
{
  tRegisterValidMask &= ~(1 << unit);

  tRegisters[unit].type = TRegister::TYPE_AS;
  tRegisters[unit].rtas = as;

  getSampledImageRegister(unit).clear();
  getSampledImageWithCompareRegister(unit).clear();
  getBufferAsSampledImageRegister(unit).clear();
  getReadOnlyBufferRegister(unit).clear();

  G_ASSERTF(as, "vulkan: binding empty tReg as accel struct, should be filtered at bind point!");
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  getRaytraceAccelerationStructureRegister(unit).raytraceAccelerationStructure = as->getHandle();
  getRaytraceAccelerationStructureRegister(unit).type = VkAnyDescriptorInfo::TYPE_AS;
#endif
}
#endif

void PipelineStageStateBase::validateBinds(const spirv::ShaderHeader &hdr, bool t_diff, bool u_diff, bool b_diff)
{
  if (t_diff && u_diff)
    checkForHardConflicts(hdr);

  checkForDeadResources(hdr, t_diff, u_diff, b_diff);
}

#if DAGOR_DBGLEVEL > 0

void PipelineStageStateBase::checkForDeadResources(const spirv::ShaderHeader &hdr, bool t_diff, bool u_diff, bool b_diff)
{
  uint64_t i, uBit;
  if (t_diff)
    for (i = 0, uBit = 1; i < spirv::T_REGISTER_INDEX_MAX; ++i, uBit <<= 1)
    {
      // not used or valid
      if (!(hdr.tRegisterUseMask & uBit) || (tRegisterValidMask & uBit))
        continue;

      const auto &reg = tRegisters[i];
      switch (reg.type)
      {
        case TRegister::TYPE_IMG: reg.img.ptr->checkDead(); break;
        case TRegister::TYPE_BUF: reg.buf.buffer->checkDead(); break;
#if D3D_HAS_RAY_TRACING
        case TRegister::TYPE_AS: reg.rtas->checkDead(); break;
#endif
        default:;
      }
    }

  if (u_diff)
    for (i = 0, uBit = 1; i < spirv::U_REGISTER_INDEX_MAX; ++i, uBit <<= 1)
    {
      // not used or valid
      if (!(hdr.uRegisterUseMask & uBit) || (uRegisterValidMask & uBit))
        continue;

      const auto &reg = uRegisters[i];
      if (reg.image)
        reg.image->checkDead();
      else if (reg.buffer.buffer)
        reg.buffer.buffer->checkDead();
    }

  if (b_diff)
    for (i = 0, uBit = 1; i < spirv::B_REGISTER_INDEX_MAX; ++i, uBit <<= 1)
    {
      if (!(hdr.bRegisterUseMask & uBit) || (bRegisterValidMask & uBit))
        continue;

      const auto &reg = bRegisters[i];
      if (reg.buffer)
        reg.buffer->checkDead();
    }
}

void PipelineStageStateBase::checkForHardConflicts(const spirv::ShaderHeader &hdr)
{
  // right now only T vs U check
  for (uint32_t i = 1; i < spirv::U_REGISTER_INDEX_MAX; ++i)
  {
    uint32_t uBit = 1 << i;
    if (!(uBit & hdr.uRegisterUseMask) || !uRegisters[i].buffer.buffer)
      continue;

    for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
    {
      uint32_t tBit = 1 << j;
      if (!(tBit & hdr.tRegisterUseMask) || tRegisters[j].type != TRegister::TYPE_BUF)
        continue;

      if (uRegisters[i].buffer.buffer == tRegisters[j].buf.buffer)
      {
        // only triggers if shader is actually using UAV & SRV slots where same resouce is binded
        // check caller code!
        logerr("vulkan: buffer %p:%s is used as UAV:%u & SRV:%u at same time", tRegisters[j].buf.buffer,
          tRegisters[j].buf.buffer->getDebugName(), i, j);
        setTempty(j);
      }
    }
  }
}
#endif // DAGOR_DBGLEVEL > 0

void PipelineStageStateBase::checkForMissingBinds(const spirv::ShaderHeader &hdr, const ResourceDummySet &dummy_resource_table,
  ExecutionContext &)
{
  // fill out the missing slots
  for (uint32_t i = 0; i < hdr.registerCount; ++i)
  {
    uint32_t absReg = hdr.registerIndex[i];
    VkAnyDescriptorInfo &slot = registerTable[absReg];
    if (slot.type == VkAnyDescriptorInfo::TYPE_NULL)
    {
      // enable this to find who forget to bind resources
      // logerr("vulkan: empty binding for used register %u(%u) expected %u restype callsite %s", i, absReg, hdr.missingTableIndex[i],
      // ctx.getCurrentCmdCaller());
      slot = dummy_resource_table[hdr.missingTableIndex[i]];
    }
  }
}
