#include "device_context.h"
#include "device.h"

using namespace drv3d_vulkan;

// framebuffer based renderpass handlers

namespace
{
PipelineBarrier::BarrierCache barrierCache;
};

void ExecutionContext::syncConstDepthReadWithInternalStore()
{
  PipelineBarrier barrier(vkDev, barrierCache, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
  barrier.addMemory({VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT});
  barrier.addDependencyFlags(VK_DEPENDENCY_BY_REGION_BIT);
  barrier.submit(frameCore);
}

void ExecutionContext::ensureStateForColorAttachments()
{
  uint8_t usageMask = getFramebufferState().renderPassClass.colorTargetMask;
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (!(usageMask & (1 << i)))
      continue;
    const RenderPassClass::FramebufferDescription::AttachmentInfo &colorAtt =
      getFramebufferState().frameBufferInfo.colorAttachments[i];
    G_ASSERTF(colorAtt.img, "vulkan: no color attachment specified for render pass class with used attachment %u", i);
    colorAtt.img->checkDead();

    back.syncTrack.addImageAccess(
      ExecutionSyncTracker::LogicAddress::forAttachmentWithLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), colorAtt.img,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      {colorAtt.view.getMipBase(), colorAtt.view.getMipCount(), colorAtt.view.getArrayBase(), colorAtt.view.getArrayCount()});
  }
}

void ExecutionContext::ensureStateForDepthAttachment()
{
  FramebufferState &fbs = getFramebufferState();
  bool ro = fbs.renderPassClass.hasRoDepth();
  if (fbs.renderPassClass.hasDepth())
  {
    RenderPassClass::FramebufferDescription::AttachmentInfo &dsai = fbs.frameBufferInfo.depthStencilAttachment;
    G_ASSERTF(dsai.img, "vulkan: no depth attachment specified for render pass class with enabled depth");
    dsai.img->checkDead();

    G_ASSERTF(fbs.renderPassClass.colorSamples[0] == dsai.img->getSampleCount(),
      "Renderpass attachments sample count doesn't match target image sample count  (%d vs %d)!", fbs.renderPassClass.colorSamples[0],
      dsai.img->getSampleCount());

    ImageViewState ivs = dsai.view;

    VkImageLayout dsLayout = ro ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    back.syncTrack.addImageAccess(ExecutionSyncTracker::LogicAddress::forAttachmentWithLayout(dsLayout), dsai.img, dsLayout,
      {ivs.getMipBase(), ivs.getMipCount(), ivs.getArrayBase(), ivs.getArrayCount()});

    back.contextState.stageState[STAGE_PS].syncDepthROStateInT(dsai.img, ivs.getMipBase(), ivs.getArrayBase(), ro);
    back.contextState.stageState[STAGE_VS].syncDepthROStateInT(dsai.img, ivs.getMipBase(), ivs.getArrayBase(), ro);
  }
  else
  {
    back.contextState.stageState[STAGE_PS].syncDepthROStateInT(nullptr, 0, 0, ro);
    back.contextState.stageState[STAGE_VS].syncDepthROStateInT(nullptr, 0, 0, ro);
  }
}

void ExecutionContext::beginPassInternal(RenderPassClass *pass_class, VulkanFramebufferHandle fb_handle, VkRect2D area)
{
  // input verify
  G_ASSERT(area.offset.x >= 0);
  G_ASSERT(area.offset.y >= 0);

  // shortcuts
  FramebufferState &fbs = getFramebufferState();
  const RenderPassClass::Identifier &passIdent = fbs.renderPassClass;

  // prepare attachemnts states
  if (passIdent.colorTargetMask)
    ensureStateForColorAttachments();

  StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> clearValues;
  if (fbs.clearMode & ~CLEAR_DISCARD)
    clearValues = pass_class->constructClearValueSet(fbs.colorClearValue, fbs.depthStencilClearValue);

  // MSAA split check, on TBDR this results in garbadge on screen
  if (!pass_class->verifyPass(fbs.clearMode))
  {
    generateFaultReport();
    fatal("vulkan: MSAA pass was wrongly splitted around caller %s", getCurrentCmdCaller());
  }

  VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr};
#if VK_KHR_imageless_framebuffer
  VkRenderPassAttachmentBeginInfoKHR rpabi = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR, nullptr};
  if (device.hasImagelessFramebuffer())
  {
    auto attachments = passIdent.squash<VkImageView>(array_size(fbs.frameBufferInfo.colorAttachments),
      fbs.frameBufferInfo.colorAttachments, fbs.frameBufferInfo.depthStencilAttachment,
      [](const RenderPassClass::FramebufferDescription::AttachmentInfo &attachment) { return attachment.viewHandle.value; });
    rpabi.attachmentCount = attachments.size();
    rpabi.pAttachments = attachments.data();

    chain_structs(rpbi, rpabi);
  }
#endif
  rpbi.renderPass = pass_class->getPass(vkDev, fbs.clearMode);
  rpbi.framebuffer = fb_handle;
  rpbi.renderArea = area;
  rpbi.clearValueCount = clearValues.size();
  rpbi.pClearValues = clearValues.data();
  VULKAN_LOG_CALL(vkDev.vkCmdBeginRenderPass(frameCore, &rpbi, VK_SUBPASS_CONTENTS_INLINE));

  // save resulting state
  back.executionState.set<StateFieldGraphicsRenderPassArea, VkRect2D, BackGraphicsState>(area);
  back.executionState.set<StateFieldGraphicsRenderPassClass, RenderPassClass *, BackGraphicsState>(pass_class);
  back.executionState.set<StateFieldGraphicsFramebuffer, VulkanFramebufferHandle, BackGraphicsState>(fb_handle);
  back.executionState.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NORMAL_PASS);

  invalidateActiveGraphicsPipeline();
}

void ExecutionContext::endPass(const char *why)
{
  // FIXME: unif_state2: should not be there as get, but should be processed first on apply to exec stage
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  if (inPassState == InPassStateFieldType::NATIVE_PASS)
    fatal("vulkan: trying to end native rp via internal break <%s>, caller\n%s", why, getCurrentCmdCaller());

  if (inPassState == InPassStateFieldType::NORMAL_PASS)
  {
    // when no-store exts are unavailable, we should sync const DS read with "fake" write
    // its happens between pass end and external dependency barrier
    if (back.executionState.getRO<StateFieldGraphicsRenderPassClass, RenderPassClass, BackGraphicsState>()
          .getIdentifier()
          .hasRoDepth() &&
        !get_device().hasAttachmentNoStoreOp())
      syncConstDepthReadWithInternalStore();
    VULKAN_LOG_CALL(vkDev.vkCmdEndRenderPass(frameCore));
    performSyncAtRenderPassEnd();
  }
  else
    G_ASSERTF(false, "vulkan: pass end without active pass");

  back.executionState.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NONE);
  back.executionState.set<StateFieldGraphicsFramebuffer, VulkanFramebufferHandle, BackGraphicsState>(VulkanFramebufferHandle());
  back.executionState.set<StateFieldGraphicsRenderPassClass, RenderPassClass *, BackGraphicsState>(nullptr);
}

// native render pass handlers

void ExecutionContext::nextNativeSubpass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NATIVE_PASS,
    "vulkan: trying to advance native rp subpass while no native RP is active, caller\n%s", getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = back.executionState.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  G_ASSERTF(rpRes, "vulkan: native rp missing when trying to advance subpass, caller\n%s", getCurrentCmdCaller());

  rpRes->nextSubpass(*this);
}

void ExecutionContext::beginNativePass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NONE, "vulkan: trying to begin native rp while no native RP is active, caller\n%s",
    getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = back.executionState.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  G_ASSERTF(rpRes, "vulkan: native rp missing when trying to start one, caller\n%s", getCurrentCmdCaller());

  rpRes->beginPass(*this);

  back.executionState.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NATIVE_PASS);
}

void ExecutionContext::endNativePass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NATIVE_PASS,
    "vulkan: trying to end native rp while no native RP is active, caller\n%s", getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = back.executionState.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  G_ASSERTF(rpRes, "vulkan: native rp close missing native rp resource while native rp is active, caller\n%s", getCurrentCmdCaller());

  rpRes->endPass(*this);

  performSyncAtRenderPassEnd();

  back.executionState.set<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>(nullptr);
  back.executionState.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NONE);
}

void ExecutionContext::nativeRenderPassChanged()
{
  uint32_t nextSubpass =
    back.pipelineState.getRO<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>();
  if (nextSubpass == StateFieldRenderPassSubpassIdx::InvalidSubpass)
    // transit into custom stage to avoid starting FB pass right after native one
    beginCustomStage("endNativeRP");
  else
  {
    ensureActivePass();
    back.executionState.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
    applyStateChanges();
  }
  invalidateActiveGraphicsPipeline();
}

void ExecutionContext::performSyncAtRenderPassEnd()
{
  TIME_PROFILE(vulkan_sync_at_render_pass_end);
  // apply sync to previous cmd buffer, so barriers are applied before already recorded
  // rendering operations of closed render pass
  back.syncTrack.completeNeeded(back.contextState.cmdListsToSubmit.back(), vkDev);
}

void ExecutionContext::interruptFrameCoreForRenderPassStart()
{
  // called from applyStateChanges, so front is processed and back graphics is forced to be applied via setDirty
  // i.e. after cmd buffer switch all state should be reapplied properly
  switchFrameCore();
  // only non re-applied state is one we don't track with tracked state approach
  // invalidate them by hand
  back.contextState.onFrameStateInvalidate();
  back.executionState.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::FullInvalidate, BackGraphicsState>({});
}
