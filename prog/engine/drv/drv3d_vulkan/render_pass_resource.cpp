#include "device.h"
#include "render_pass_resource.h"
#include <perfMon/dag_statDrv.h>

using namespace drv3d_vulkan;

PipelineBarrier::BarrierCache barrierCache;
const FrontRenderPassStateStorage *RenderPassResource::state = nullptr;
RenderPassResource::BakedAttachmentSharedData *RenderPassResource::bakedAttachments = nullptr;

constexpr uint32_t subpassDebugMarkerColor = 0xFF90EE90;
constexpr uint32_t nativePassDebugMarkerColor = 0xFFFFBF00;

// generic resource callbacks & stuff

void RenderPassDescription::fillAllocationDesc(AllocationDesc &alloc_desc) const
{
  alloc_desc.reqs = {};
  alloc_desc.canUseSharedHandle = 0;
  alloc_desc.forceDedicated = 0;
  alloc_desc.memClass = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
  alloc_desc.temporary = 0;
  alloc_desc.objectBaked = 1;
}

template <>
void RenderPassResource::onDelayedCleanupFinish<RenderPassResource::CLEANUP_DESTROY>()
{
  Device &drvDev = get_device();
  drvDev.resources.free(this);
}

RenderPassResource::RenderPassResource(const Description &in_desc, bool manage /*=true*/) :
  RenderPassResourceImplBase(in_desc, manage), activeSubpass(0)
{}

void RenderPassResource::destroyVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  get_device().getVkDevice().vkDestroyRenderPass(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

MemoryRequirementInfo RenderPassResource::getMemoryReq()
{
  MemoryRequirementInfo ret{};
  ret.requirements.alignment = 1;
  return ret;
}

VkMemoryRequirements RenderPassResource::getSharedHandleMemoryReq()
{
  fatal("vulkan: no handle reuse for render pass");
  return {};
}

void RenderPassResource::bindMemory() {}

void RenderPassResource::reuseHandle() { fatal("vulkan: no handle reuse for render pass"); }

void RenderPassResource::releaseSharedHandle() { fatal("vulkan: no handle reuse for render pass"); }

void RenderPassResource::evict() { fatal("vulkan: render pass are not evictable"); }

void RenderPassResource::restoreFromSysCopy() { fatal("vulkan: render pass are not evictable"); }

bool RenderPassResource::isEvictable() { return false; }

void RenderPassResource::shutdown()
{
  state = nullptr;
  VulkanDevice &device = get_device().getVkDevice();
  for (FbWithCreationInfo iter : compiledFBs)
    VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), iter.handle, nullptr));
}

bool RenderPassResource::nonResidentCreation() { return false; }

void RenderPassResource::makeSysCopy() { fatal("vulkan: render pass are not evictable"); }

// helpers/getters functions

uint32_t RenderPassResource::getTargetsCount() { return desc.targetCount; }

uint32_t RenderPassResource::getCurrentSubpass() { return activeSubpass; }

uint32_t RenderPassResource::getColorWriteMaskAtSubpass(uint32_t subpass)
{
  return desc.subpassAttachmentsInfos[subpass].colorWriteMask;
}

VkSampleCountFlagBits RenderPassResource::getMSAASamples(uint32_t subpass)
{
  return desc.subpassAttachmentsInfos[subpass].msaaSamples;
}

bool RenderPassResource::hasDepthAtSubpass(uint32_t subpass) { return desc.subpassAttachmentsInfos[subpass].hasDSBind; }

VkExtent2D RenderPassResource::getMaxActiveAreaExtent() { return toVkFbExtent(state->area.data); }

VkExtent2D RenderPassResource::toVkFbExtent(RenderPassArea area)
{
  VkExtent2D ret;
  ret.width = area.width + area.left;
  ret.height = area.height + area.top;
  return ret;
}

VkRect2D RenderPassResource::toVkArea(RenderPassArea area)
{
  VkRect2D ret;
  ret.offset.x = area.left;
  ret.offset.y = area.top;
  ret.extent.width = area.width;
  ret.extent.height = area.height;
  return ret;
}

bool RenderPassResource::allowSRGBWrite(uint32_t index) { return desc.targetCFlags[index] & TEXCF_SRGBWRITE; }

// execution related methods

void RenderPassResource::useWithState(const FrontRenderPassStateStorage &v) { state = &v; }

void RenderPassResource::useWithAttachments(BakedAttachmentSharedData *v) { bakedAttachments = v; }

void RenderPassResource::setFrameImageLayout(ExecutionContext &ctx, uint32_t att_index, VkImageLayout new_layout,
  ImageLayoutSync sync_type)
{
  const StateFieldRenderPassTarget &tgt = state->targets.data[att_index];
  uint16_t arrayBase = tgt.layer == d3d::RENDER_TO_WHOLE_ARRAY ? 0 : tgt.layer;
  uint16_t arrayRange = bakedAttachments->layerCounts[att_index];

  switch (sync_type)
  {
    case ImageLayoutSync::INNER:
      for (int i = 0; i < arrayRange; ++i)
        tgt.image->layout.set(tgt.mipLevel, arrayBase + i, new_layout);
      break;
    case ImageLayoutSync::END_EDGE:
      for (int i = 0; i < arrayRange; ++i)
        tgt.image->layout.set(tgt.mipLevel, arrayBase + i, new_layout);
      // fallthrough
    case ImageLayoutSync::START_EDGE:
    {
      uint32_t extOpIdx =
        sync_type == ImageLayoutSync::END_EDGE ? RenderPassDescription::EXTERNAL_OP_END : RenderPassDescription::EXTERNAL_OP_START;
      ctx.back.syncTrack.addImageAccess(
        {desc.attImageExtrenalOperations[extOpIdx][att_index].stage, desc.attImageExtrenalOperations[extOpIdx][att_index].access},
        tgt.image, new_layout, {tgt.mipLevel, 1, arrayBase, arrayRange});
      break;
    }
    default: G_ASSERTF(0, "vulkan: RenderPassResource::setFrameImageLayout %p unknown sync type %u", this, sync_type); break;
  }
}

void RenderPassResource::updateImageStatesForCurrentSubpass(ExecutionContext &ctx)
{
  ImageLayoutSync syncType;
  if (activeSubpass == 0)
    syncType = ImageLayoutSync::START_EDGE;
  else if (activeSubpass == (desc.subpasses - 1))
    syncType = ImageLayoutSync::END_EDGE;
  else
    syncType = ImageLayoutSync::INNER;

  for (uint32_t i = 0; i < desc.targetCount; ++i)
    setFrameImageLayout(ctx, i, desc.attImageLayouts[activeSubpass][i], syncType);
}

void RenderPassResource::performSelfDepsForSubpass(uint32_t subpass, VulkanCommandBufferHandle cmd_b)
{
  VkSubpassDependency &selfDep = desc.selfDeps[subpass];
  if (!selfDep.dependencyFlags)
    return;

  PipelineBarrier barrier(get_device().getVkDevice(), barrierCache, selfDep.srcStageMask, selfDep.dstStageMask);
  barrier.addMemory({selfDep.srcAccessMask, selfDep.dstAccessMask});
  barrier.addDependencyFlags(selfDep.dependencyFlags);
  barrier.submit(cmd_b);
}

void RenderPassResource::bindInputAttachments(ExecutionContext &ctx, PipelineStageStateBase &tgt, uint32_t input_index,
  uint32_t register_index, const VariatedGraphicsPipeline *pipeline)
{
  G_UNUSED(ctx);
  G_UNUSED(pipeline);
  G_ASSERTF(desc.inputAttachments[activeSubpass].size() > input_index,
    "vulkan: requested OOB input attachment\n"
    "shader declares vk::input_attachment_index(%u)\n"
    "active render pass %p [ %p ] <%s> subpass #%u declared %u input attachments!\n"
    "additional info: %s\n"
    "caller: %s",
    input_index, this, getBaseHandle(), getDebugName(), activeSubpass, desc.inputAttachments[activeSubpass].size(),
    pipeline->printDebugInfoBuffered(), ctx.getCurrentCmdCaller());
  uint32_t attIdx = desc.inputAttachments[activeSubpass][input_index];
  tgt.setTinputAttachment(register_index - spirv::T_INPUT_ATTACHMENT_OFFSET, state->targets.data[attIdx].image,
    // only const DS for now, must be changed if special const RT will be found
    desc.attImageLayouts[activeSubpass][attIdx] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, bakedAttachments->views[attIdx]);
}

void RenderPassResource::advanceSubpass(ExecutionContext &ctx)
{
  G_ASSERTF(state && bakedAttachments, "vulkan: render pass %p [ %p ] <%s> is not used in any execution context", this,
    getBaseHandle(), getDebugName());
  performSelfDepsForSubpass(activeSubpass, ctx.frameCore);
  ++activeSubpass;
  ctx.popEventRaw();
  ctx.back.syncTrack.setCurrentRenderSubpass(activeSubpass);

  if (activeSubpass != desc.subpasses)
    updateImageStatesForCurrentSubpass(ctx);
}

void RenderPassResource::beginPass(ExecutionContext &ctx)
{
  G_ASSERTF(state && bakedAttachments, "vulkan: render pass %p [ %p ] <%s> is not used in any execution context", this,
    getBaseHandle(), getDebugName());
  G_ASSERTF(activeSubpass == 0, "vulkan: render pass %p [ %p ] <%s> started twice", this, getBaseHandle(), getDebugName());

  if (ctx.isDebugEventsAllowed())
    ctx.pushEventRaw(String(64, "NRP: <%s>", getDebugName()), nativePassDebugMarkerColor);

  ctx.back.syncTrack.setCurrentRenderSubpass(0);
  updateImageStatesForCurrentSubpass(ctx);
  ctx.back.syncTrack.completeNeeded(ctx.frameCore, ctx.vkDev);

  VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr};

#if VK_KHR_imageless_framebuffer
  VkRenderPassAttachmentBeginInfoKHR rpabi = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR, nullptr};
  rpabi.attachmentCount = desc.targetCount;
  rpabi.pAttachments = ary(bakedAttachments->views);
  if (get_device().hasImagelessFramebuffer())
    chain_structs(rpbi, rpabi);
#endif

  rpbi.framebuffer = compileOrGetFB();
  rpbi.renderPass = getHandle();
  rpbi.renderArea = toVkArea(state->area.data);
  rpbi.clearValueCount = desc.targetCount;
  rpbi.pClearValues = bakedAttachments->clearValues;
  ctx.vkDev.vkCmdBeginRenderPass(ctx.frameCore, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
  ctx.pushEventRaw("subpass 0", subpassDebugMarkerColor);
}

void RenderPassResource::nextSubpass(ExecutionContext &ctx)
{
  advanceSubpass(ctx);
  G_ASSERTF(activeSubpass < desc.subpasses, "vulkan: there is only %u subpasses in RP %p [ %p ] <%s>, but asking for %u",
    desc.subpasses, this, getBaseHandle(), getDebugName(), activeSubpass);
  ctx.vkDev.vkCmdNextSubpass(ctx.frameCore, VK_SUBPASS_CONTENTS_INLINE);

  if (ctx.isDebugEventsAllowed())
    ctx.pushEventRaw(String(16, "subpass %u", activeSubpass), subpassDebugMarkerColor);
}

void RenderPassResource::endPass(ExecutionContext &ctx)
{
  advanceSubpass(ctx);
  G_ASSERTF(activeSubpass == desc.subpasses,
    "vulkan: there is %u subpasses in RP %p [ %p ] <%s>, but we ending it at activeSubpass %u", desc.subpasses, this, getBaseHandle(),
    getDebugName(), activeSubpass);
  ctx.vkDev.vkCmdEndRenderPass(ctx.frameCore);
  ctx.back.syncTrack.setCurrentRenderSubpass(ExecutionSyncTracker::SUBPASS_NON_NATIVE);

  state = nullptr;
  bakedAttachments = nullptr;
  activeSubpass = 0;

  ctx.popEventRaw();
}
