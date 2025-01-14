// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_pass_resource.h"
#include <perfMon/dag_statDrv.h>
#include "globals.h"
#include "resource_manager.h"
#include "pipeline_barrier.h"
#include "pipeline/manager.h"
#include "front_render_pass_state.h"
#include "backend.h"
#include "execution_context.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

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
  Globals::Mem::res.free(this);
}

RenderPassResource::RenderPassResource(const Description &in_desc, bool manage /*=true*/) :
  RenderPassResourceImplBase(in_desc, manage), activeSubpass(0)
{}

void RenderPassResource::destroyVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;

  Globals::VK::dev.vkDestroyRenderPass(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

MemoryRequirementInfo RenderPassResource::getMemoryReq()
{
  MemoryRequirementInfo ret{};
  ret.requirements.alignment = 1;
  ret.requirements.memoryTypeBits = 0xFFFFFFFF;
  return ret;
}

VkMemoryRequirements RenderPassResource::getSharedHandleMemoryReq()
{
  DAG_FATAL("vulkan: no handle reuse for render pass");
  return {};
}

void RenderPassResource::bindMemory() {}

void RenderPassResource::reuseHandle() { DAG_FATAL("vulkan: no handle reuse for render pass"); }

void RenderPassResource::releaseSharedHandle() { DAG_FATAL("vulkan: no handle reuse for render pass"); }

void RenderPassResource::evict() { DAG_FATAL("vulkan: render pass are not evictable"); }

void RenderPassResource::restoreFromSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: render pass are not evictable"); }

bool RenderPassResource::isEvictable() { return false; }

void RenderPassResource::shutdown()
{
  state = nullptr;
  VulkanDevice &device = Globals::VK::dev;
  for (FbWithCreationInfo iter : compiledFBs)
    VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), iter.handle, nullptr));
}

bool RenderPassResource::nonResidentCreation() { return false; }

void RenderPassResource::makeSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: render pass are not evictable"); }

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

bool RenderPassResource::hasDepthAtSubpass(uint32_t subpass)
{
  return desc.subpassAttachmentsInfos[subpass].depthStencilAttachment >= 0;
}

bool RenderPassResource::isDepthAtSubpassRO(uint32_t subpass)
{
  G_ASSERT(hasDepthAtSubpass(subpass));
  int32_t dsAtt = desc.subpassAttachmentsInfos[subpass].depthStencilAttachment;
  return desc.attImageLayouts[subpass][dsAtt] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

bool RenderPassResource::hasDepthAtCurrentSubpass() { return desc.subpassAttachmentsInfos[activeSubpass].depthStencilAttachment >= 0; }

bool RenderPassResource::isCurrentDepthRO()
{
  G_ASSERT(hasDepthAtCurrentSubpass());
  int32_t dsAtt = desc.subpassAttachmentsInfos[activeSubpass].depthStencilAttachment;
  return desc.attImageLayouts[activeSubpass][dsAtt] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

Image *RenderPassResource::getCurrentDepth()
{
  G_ASSERT(hasDepthAtCurrentSubpass());
  int32_t dsAtt = desc.subpassAttachmentsInfos[activeSubpass].depthStencilAttachment;
  return state->targets.data[dsAtt].image;
}

bool RenderPassResource::isCurrentDepthROAttachment(const Image *img, ImageViewState ivs) const
{
  int32_t dsAtt = desc.subpassAttachmentsInfos[activeSubpass].depthStencilAttachment;
  if (dsAtt < 0)
    return false;

  if (desc.attImageLayouts[activeSubpass][dsAtt] != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    return false;

  const StateFieldRenderPassTarget &dsTgt = state->targets.data[dsAtt];
  if (dsTgt.image != img)
    return false;

  const ValueRange<uint32_t> mipRange(ivs.getMipBase(), ivs.getMipBase() + ivs.getMipCount());
  return mipRange.isInside(dsTgt.mipLevel);
}

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

bool RenderPassResource::allowSRGBWrite(uint32_t index) { return desc.targetFormats[index].isSrgb; }

// execution related methods

void RenderPassResource::useWithState(const FrontRenderPassStateStorage &v) { state = &v; }

void RenderPassResource::useWithAttachments(BakedAttachmentSharedData *v) { bakedAttachments = v; }


void RenderPassResource::updateImageStatesForCurrentSubpass(ExecutionContext &ctx)
{
  for (uint32_t attIndex = 0; attIndex < desc.targetCount; ++attIndex)
  {
    const StateFieldRenderPassTarget &tgt = state->targets.data[attIndex];
    uint16_t arrayBase = tgt.layer == d3d::RENDER_TO_WHOLE_ARRAY ? 0 : tgt.layer;
    uint16_t arrayRange = bakedAttachments->layerCounts[attIndex];

    if (activeSubpass == 0 || activeSubpass == desc.subpasses - 1)
    {
      // all attachments must be valid
      G_ASSERTF(tgt.image != nullptr, "vulkan: attachment %u of RP %p[%p]<%s> is not specified (null)", attIndex, this,
        getBaseHandle(), getDebugName());
      ctx.verifyResident(tgt.image);
    }

    const auto &extOp = desc.attachmentOperations[activeSubpass][attIndex];

    // Not all targets are used in every subpass
    if (!extOp.stage && !extOp.access)
      continue;

    const auto newLayout = desc.attImageLayouts[activeSubpass][attIndex];
    bool discard = activeSubpass == 0;
    if (discard)
    {
      VkRect2D area = toVkArea(state->area.data);
      discard &= area.extent == tgt.image->getMipExtents2D(tgt.mipLevel);
      discard &= area.offset == VkOffset2D{0, 0};
      discard &= !desc.attachmentContentLoaded[attIndex];
    }
    Backend::sync.addNrpAttachmentAccess({extOp.stage, extOp.access}, tgt.image, newLayout, {tgt.mipLevel, 1, arrayBase, arrayRange},
      discard);
  }
}

void RenderPassResource::performSelfDepsForSubpass(uint32_t subpass)
{
  VkSubpassDependency &selfDep = desc.selfDeps[subpass];
  if (!selfDep.dependencyFlags)
    return;

  PipelineBarrier barrier(barrierCache, selfDep.srcStageMask, selfDep.dstStageMask);
  barrier.addMemory({selfDep.srcAccessMask, selfDep.dstAccessMask});
  barrier.addDependencyFlags(selfDep.dependencyFlags);
  barrier.submit();
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
  performSelfDepsForSubpass(activeSubpass);
  ++activeSubpass;
  ctx.popEventTracked();
  Backend::sync.setCurrentRenderSubpass(activeSubpass);

  if (activeSubpass != desc.subpasses)
    updateImageStatesForCurrentSubpass(ctx);
}

void RenderPassResource::beginPass(ExecutionContext &ctx)
{
  G_ASSERTF(state && bakedAttachments, "vulkan: render pass %p [ %p ] <%s> is not used in any execution context", this,
    getBaseHandle(), getDebugName());
  G_ASSERTF(activeSubpass == 0, "vulkan: render pass %p [ %p ] <%s> started twice", this, getBaseHandle(), getDebugName());

  ctx.pushEventTracked(getDebugName(), nativePassDebugMarkerColor);

  Backend::sync.setCurrentRenderSubpass(0);
  updateImageStatesForCurrentSubpass(ctx);

  Backend::gpuJob.get().execTracker.addMarker(&desc.hash, sizeof(desc.hash));

  VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr};

#if VK_KHR_imageless_framebuffer
  VkRenderPassAttachmentBeginInfoKHR rpabi = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR, nullptr};
  rpabi.attachmentCount = desc.targetCount;
  rpabi.pAttachments = ary(bakedAttachments->views);
  if (Globals::cfg.has.imagelessFramebuffer)
    chain_structs(rpbi, rpabi);
#endif

  rpbi.framebuffer = compileOrGetFB();
  rpbi.renderPass = getHandle();
  rpbi.renderArea = toVkArea(state->area.data);
  rpbi.clearValueCount = desc.targetCount;
  rpbi.pClearValues = bakedAttachments->clearValues;

  // set render pass area execution state for proper viewport change checking
  Backend::State::exec.set<StateFieldGraphicsRenderPassArea, VkRect2D, BackGraphicsState>(rpbi.renderArea);

  Backend::cb.startReorder();
  Backend::cb.wCmdBeginRenderPass(&rpbi, VK_SUBPASS_CONTENTS_INLINE);
  ctx.pushEventTracked("subpass", subpassDebugMarkerColor);
}

void RenderPassResource::nextSubpass(ExecutionContext &ctx)
{
  advanceSubpass(ctx);
  G_ASSERTF(activeSubpass < desc.subpasses, "vulkan: there is only %u subpasses in RP %p [ %p ] <%s>, but asking for %u",
    desc.subpasses, this, getBaseHandle(), getDebugName(), activeSubpass);
  Backend::cb.wCmdNextSubpass(VK_SUBPASS_CONTENTS_INLINE);

  ctx.pushEventTracked("subpass", subpassDebugMarkerColor);
}

void RenderPassResource::endPass(ExecutionContext &ctx)
{
  advanceSubpass(ctx);
  G_ASSERTF(activeSubpass == desc.subpasses,
    "vulkan: there is %u subpasses in RP %p [ %p ] <%s>, but we ending it at activeSubpass %u", desc.subpasses, this, getBaseHandle(),
    getDebugName(), activeSubpass);
  Backend::cb.wCmdEndRenderPass();
  Backend::sync.setCurrentRenderSubpass(ExecutionSyncTracker::SUBPASS_NON_NATIVE);

  state = nullptr;
  bakedAttachments = nullptr;
  activeSubpass = 0;

  ctx.popEventTracked();
}
