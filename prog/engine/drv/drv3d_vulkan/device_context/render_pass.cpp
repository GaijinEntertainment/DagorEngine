// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_context.h"
#include "backend.h"
#include "execution_scratch.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

// framebuffer based renderpass handlers

namespace
{
PipelineBarrier::BarrierCache barrierCache;
};

void ExecutionContext::syncConstDepthReadWithInternalStore()
{
  PipelineBarrier barrier(barrierCache, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
  barrier.addMemory({VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT});
  barrier.addDependencyFlags(VK_DEPENDENCY_BY_REGION_BIT);
  barrier.submit();
}

void ExecutionContext::ensureStateForColorAttachments(VkRect2D area)
{
  uint8_t usageMask = getFramebufferState().renderPassClass.colorTargetMask;
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (!(usageMask & (1 << i)))
      continue;
    const RenderPassClass::FramebufferDescription::AttachmentInfo &colorAtt =
      getFramebufferState().frameBufferInfo.colorAttachments[i];
    G_ASSERTF(colorAtt.img, "vulkan: no color attachment specified for render pass class with used attachment %u", i);
    verifyResident(colorAtt.img);
    colorAtt.img->checkDead();

    if ((getFramebufferState().clearMode & (CLEAR_TARGET | CLEAR_DISCARD_TARGET)) &&
        (colorAtt.img->getMipExtents2D(colorAtt.view.getMipBase()) == area.extent) && area.offset == VkOffset2D{0, 0})
      Backend::sync.addImageWriteDiscard(LogicAddress::forAttachmentWithLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), colorAtt.img,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        {colorAtt.view.getMipBase(), colorAtt.view.getMipCount(), colorAtt.view.getArrayBase(), colorAtt.view.getArrayCount()});
    else
      Backend::sync.addImageAccess(LogicAddress::forAttachmentWithLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), colorAtt.img,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        {colorAtt.view.getMipBase(), colorAtt.view.getMipCount(), colorAtt.view.getArrayBase(), colorAtt.view.getArrayCount()});
  }
}

void ExecutionContext::ensureStateForNativeRPDepthAttachment()
{
  RenderPassResource *rpRes = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  if (rpRes->hasDepthAtCurrentSubpass())
  {
    bool ro = rpRes->isCurrentDepthRO();
    Image *depthImg = rpRes->getCurrentDepth();
    Backend::State::exec.getResBinds(STAGE_PS).syncDepthROStateInT(depthImg, 0, 0, ro);
    Backend::State::exec.getResBinds(STAGE_VS).syncDepthROStateInT(depthImg, 0, 0, ro);
  }
  else
  {
    Backend::State::exec.getResBinds(STAGE_PS).syncDepthROStateInT(nullptr, 0, 0, false);
    Backend::State::exec.getResBinds(STAGE_VS).syncDepthROStateInT(nullptr, 0, 0, false);
  }
}

void ExecutionContext::ensureStateForDepthAttachment(VkRect2D area)
{
  FramebufferState &fbs = getFramebufferState();
  bool ro = fbs.renderPassClass.hasRoDepth();
  if (fbs.renderPassClass.hasDepth())
  {
    RenderPassClass::FramebufferDescription::AttachmentInfo &dsai = fbs.frameBufferInfo.depthStencilAttachment;
    G_ASSERTF(dsai.img, "vulkan: no depth attachment specified for render pass class with enabled depth");
    verifyResident(dsai.img);
    dsai.img->checkDead();

    G_ASSERTF(fbs.renderPassClass.depthSamples == dsai.img->getSampleCount(),
      "Renderpass attachments sample count doesn't match target image sample count  (%d vs %d)!", fbs.renderPassClass.depthSamples,
      dsai.img->getSampleCount());

    ImageViewState ivs = dsai.view;

    VkImageLayout dsLayout = ro ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (!ro && (getFramebufferState().clearMode & (CLEAR_ZBUFFER | CLEAR_DISCARD_ZBUFFER)) &&
        (dsai.img->getMipExtents2D(ivs.getMipBase()) == area.extent) && area.offset == VkOffset2D{0, 0})
      Backend::sync.addImageWriteDiscard(LogicAddress::forAttachmentWithLayout(dsLayout), dsai.img, dsLayout,
        {ivs.getMipBase(), ivs.getMipCount(), ivs.getArrayBase(), ivs.getArrayCount()});
    else
      Backend::sync.addImageAccess(LogicAddress::forAttachmentWithLayout(dsLayout), dsai.img, dsLayout,
        {ivs.getMipBase(), ivs.getMipCount(), ivs.getArrayBase(), ivs.getArrayCount()});

    Backend::State::exec.getResBinds(STAGE_PS).syncDepthROStateInT(dsai.img, ivs.getMipBase(), ivs.getArrayBase(), ro);
    Backend::State::exec.getResBinds(STAGE_VS).syncDepthROStateInT(dsai.img, ivs.getMipBase(), ivs.getArrayBase(), ro);
  }
  else
  {
    Backend::State::exec.getResBinds(STAGE_PS).syncDepthROStateInT(nullptr, 0, 0, ro);
    Backend::State::exec.getResBinds(STAGE_VS).syncDepthROStateInT(nullptr, 0, 0, ro);
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
    ensureStateForColorAttachments(area);

  uint32_t opLoadMask = pass_class->getAttachmentsLoadMask(fbs.clearMode);

  // MSAA split check, on TBDR this results in garbage on screen.
  // We treat loading previous contents of any attachment as RP split.
  if (opLoadMask != 0 && pass_class->getIdentifier().hasMsaaAttachments())
  {
    generateFaultReport();
    DAG_FATAL("vulkan: MSAA pass was wrongly splitted around caller %s", getCurrentCmdCaller());
  }

#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_ANDROID || _TARGET_C3)
  // General renderpass split check. We don't want renderpasses to be split (besides special cases) for performance reasons on TBDR.
  bool allowOpLoad = fbs.clearMode & fbs.CLEAR_LOAD;
  // If we make depth to be RO, there is no other way than that we want to load it.
  uint32_t notAllowedLoadBits = ~(passIdent.hasRoDepth() ? Driver3dRenderTarget::DEPTH : 0);
  if (Globals::cfg.bits.validateRenderPassSplits && (opLoadMask & notAllowedLoadBits) != 0 && !allowOpLoad)
  {
    generateFaultReport();
    DAG_FATAL("vulkan: render pass was wrongly split (load color mask = 0x%X, load depth = %d) around caller %s",
      opLoadMask & Driver3dRenderTarget::COLOR_MASK, bool(opLoadMask & Driver3dRenderTarget::DEPTH), getCurrentCmdCaller());
  }
  // CLEAR_LOAD is supposed to affect only the current pass. So reset it here to not affect the next passes.
  // Also it has to be reset before using clearMode in the subsequent code.
  fbs.clearMode &= ~fbs.CLEAR_LOAD;
#endif

  StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> clearValues;
  if (fbs.clearMode & ~CLEAR_DISCARD)
    clearValues = pass_class->constructClearValueSet(fbs.colorClearValue, fbs.depthStencilClearValue);

  VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr};
#if VK_KHR_imageless_framebuffer
  VkRenderPassAttachmentBeginInfoKHR rpabi = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR, nullptr};
  StaticTab<VkImageView, Driver3dRenderTarget::MAX_SIMRT + 1> attachments;
  if (Globals::cfg.has.imagelessFramebuffer)
  {
    attachments = passIdent.squash<VkImageView>(array_size(fbs.frameBufferInfo.colorAttachments), fbs.frameBufferInfo.colorAttachments,
      fbs.frameBufferInfo.depthStencilAttachment,
      [](const RenderPassClass::FramebufferDescription::AttachmentInfo &attachment) { return attachment.viewHandle.value; });
    rpabi.attachmentCount = attachments.size();
    rpabi.pAttachments = attachments.data();

    chain_structs(rpbi, rpabi);
  }
#endif
  Backend::gpuJob.get().execTracker.addMarker(&passIdent, sizeof(RenderPassClass::Identifier));
  rpbi.renderPass = pass_class->getPass(vkDev, fbs.clearMode);
  rpbi.framebuffer = fb_handle;
  rpbi.renderArea = area;
  rpbi.clearValueCount = clearValues.size();
  rpbi.pClearValues = clearValues.data();

  Backend::cb.startReorder();
  VULKAN_LOG_CALL(Backend::cb.wCmdBeginRenderPass(&rpbi, VK_SUBPASS_CONTENTS_INLINE));

  // save resulting state
  Backend::State::exec.set<StateFieldGraphicsRenderPassArea, VkRect2D, BackGraphicsState>(area);
  Backend::State::exec.set<StateFieldGraphicsRenderPassClass, RenderPassClass *, BackGraphicsState>(pass_class);
  Backend::State::exec.set<StateFieldGraphicsFramebuffer, VulkanFramebufferHandle, BackGraphicsState>(fb_handle);
  Backend::State::exec.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NORMAL_PASS);

  invalidateActiveGraphicsPipeline();
}

void ExecutionContext::endPass(const char *why)
{
  // FIXME: unif_state2: should not be there as get, but should be processed first on apply to exec stage
  InPassStateFieldType inPassState = Backend::State::exec.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  if (inPassState == InPassStateFieldType::NATIVE_PASS)
    DAG_FATAL("vulkan: trying to end native rp via internal break <%s>, caller\n%s", why, getCurrentCmdCaller());

  if (inPassState == InPassStateFieldType::NORMAL_PASS)
  {
    // when no-store exts are unavailable, we should sync const DS read with "fake" write
    // its happens between pass end and external dependency barrier
    if (Backend::State::exec.getRO<StateFieldGraphicsRenderPassClass, RenderPassClass, BackGraphicsState>()
          .getIdentifier()
          .hasRoDepth() &&
        !Globals::cfg.has.attachmentNoStoreOp)
      syncConstDepthReadWithInternalStore();
    VULKAN_LOG_CALL(Backend::cb.wCmdEndRenderPass());
    finishReorderAndPerformSync();
  }
  else
    G_ASSERTF(false, "vulkan: pass end without active pass");

  Backend::State::exec.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NONE);
  Backend::State::exec.set<StateFieldGraphicsFramebuffer, VulkanFramebufferHandle, BackGraphicsState>(VulkanFramebufferHandle());
  Backend::State::exec.set<StateFieldGraphicsRenderPassClass, RenderPassClass *, BackGraphicsState>(nullptr);
}

// native render pass handlers

void ExecutionContext::nextNativeSubpass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = Backend::State::exec.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NATIVE_PASS,
    "vulkan: trying to advance native rp subpass while no native RP is active, caller\n%s", getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  G_ASSERTF(rpRes, "vulkan: native rp missing when trying to advance subpass, caller\n%s", getCurrentCmdCaller());

  // NOTE: it is really important to do the sync before advancing the
  // subpass (or ending the render pass), as sync operations
  // recorded while starting/ending/advancing have to be mixed with
  // the operations that are performed by the commands inside the
  // subpass itself.
  Backend::cb.pauseReorder();
  Backend::sync.completeNeeded();
  Backend::cb.continueReorder();

  rpRes->nextSubpass(*this);
}

void ExecutionContext::beginNativePass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = Backend::State::exec.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NONE, "vulkan: trying to begin native rp while no native RP is active, caller\n%s",
    getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
  G_ASSERTF(rpRes, "vulkan: native rp missing when trying to start one, caller\n%s", getCurrentCmdCaller());

  rpRes->beginPass(*this);

  Backend::State::exec.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NATIVE_PASS);
}

void ExecutionContext::endNativePass()
{
#if DAGOR_DBGLEVEL > 0
  InPassStateFieldType inPassState = Backend::State::exec.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  G_ASSERTF(inPassState == InPassStateFieldType::NATIVE_PASS,
    "vulkan: trying to end native rp while no native RP is active, caller\n%s", getCurrentCmdCaller());
#endif

  RenderPassResource *rpRes = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();

  finishReorderAndPerformSync();
  if (rpRes)
    rpRes->endPass(*this);
  else
  {
    // frame IS corrupted, game will render garbage or trigger device lost, but we give it one more chance
    D3D_ERROR("vulkan: native rp close missing native rp resource while native rp is active, caller\n%s", getCurrentCmdCaller());
    Backend::cb.wCmdEndRenderPass();
  }

  Backend::State::exec.set<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>(nullptr);
  Backend::State::exec.set<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>(InPassStateFieldType::NONE);
}

void ExecutionContext::processBufferCopyReorders(uint32_t pass_index)
{
  bool inCustomStage = false;
  uint32_t startOffset = reorderedBufferCopyOffset;
  for (; reorderedBufferCopyOffset < data.reorderedBufferCopies.size(); ++reorderedBufferCopyOffset)
  {
    ReorderedBufferCopy &i = data.reorderedBufferCopies[reorderedBufferCopyOffset];
    if (i.pass != pass_index)
      break;

    if (!inCustomStage)
    {
      beginCustomStage("ReorderedBufferCopies");
      inCustomStage = true;
    }
    verifyResident(i.src.buffer);
    verifyResident(i.dst.buffer);

    uint32_t sourceBufOffset = i.src.bufOffset(i.srcOffset);
    uint32_t destBufOffset = i.dst.bufOffset(i.dstOffset);

    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, i.src.buffer,
      {sourceBufOffset, i.size});
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, i.dst.buffer,
      {destBufOffset, i.size});
  }

  if (!inCustomStage)
    return;

  // use batch sync, to avoid overwriting content that we reordered (will complain if we do that)
  Backend::sync.completeNeeded();

  for (uint32_t iter = startOffset; iter < reorderedBufferCopyOffset; ++iter)
  {
    ReorderedBufferCopy &i = data.reorderedBufferCopies[iter];
    VkBufferCopy bc;
    bc.srcOffset = i.src.bufOffset(i.srcOffset);
    bc.dstOffset = i.dst.bufOffset(i.dstOffset);
    bc.size = i.size;
    VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(i.src.buffer->getHandle(), i.dst.buffer->getHandle(), 1, &bc));
  }
}

void ExecutionContext::nativeRenderPassChanged()
{
  // try to skip RP if there is no draws
  uint32_t index = Backend::State::pipe.getRO<StateFieldRenderPassIndex, uint32_t, FrontGraphicsState, FrontRenderPassState>();
  uint32_t nextSubpass =
    Backend::State::pipe.getRO<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>();

  if (nextSubpass == 0)
    processBufferCopyReorders(index);

  if (data.nativeRPDrawCounter[index] == 0)
  {
    RenderPassResource *fRP =
      Backend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>();
    RenderPassResource *bRP = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
    // pass end of skipped pass
    if (!fRP && !bRP)
      return;
    // check that RP is no-op without draws
    if (fRP && fRP->isNoOpWithoutDraws())
      return;
  }

  if (nextSubpass == StateFieldRenderPassSubpassIdx::InvalidSubpass)
    // transit into custom stage to avoid starting FB pass right after native one
    beginCustomStage("endNativeRP");
  else
  {
    ensureActivePass();
    Backend::State::exec.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
    applyStateChanges();
  }
  invalidateActiveGraphicsPipeline();
}

void ExecutionContext::finishReorderAndPerformSync()
{
  // barrier then reordered work
  Backend::cb.stopReorder();
  if (!Backend::cb.isInReorder())
  {
    Backend::sync.completeNeeded();
    Backend::cb.flush();
  }
}

void ExecutionContext::interruptFrameCoreForRenderPassStart()
{
  // if we inside completion delay, no need to interrupt frame core as it is interrupted in advance (by delay!)
  if (Backend::sync.isCompletionDelayed())
    return;
  // called from applyStateChanges, so front is processed and back graphics is forced to be applied via setDirty
  // i.e. after cmd buffer switch all state should be reapplied properly
  switchFrameCore(frameCoreQueue);
  // only non re-applied state is one we don't track with tracked state approach
  // invalidate them by hand
  Backend::State::exec.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::FullInvalidate, BackGraphicsState>({});
}
