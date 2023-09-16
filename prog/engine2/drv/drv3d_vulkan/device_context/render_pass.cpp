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
  barrier.submit(getFrameCmdBuffer());
}

void ExecutionContext::ensureStateForColorAttachments(uint8_t usage_mask, const FramebufferInfo &fbi)
{
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (!(usage_mask & (1 << i)))
      continue;

    const FramebufferInfo::AttachmentInfo &colorAtt = fbi.colorAttachments[i];
    colorAtt.image->checkDead();
    bool change = setFrameImageState(colorAtt.image, SN_RENDER_TARGET, colorAtt.view.getMipRange(), colorAtt.view.getArrayRange());
    if (change)
    {
      // debug("missed render target state!");
    }
  }
}

void ExecutionContext::ensureStateForDepthAttachment(const RenderPassClass::Identifier &pass, const FramebufferInfo &fbi, bool ro)
{
  G_ASSERTF(pass.depthState == fbi.depthStencilMask,
    "Renderpass Identifier depthState and frontend framebuffer depthStencil mask must be the same here (%d vs %d)!", pass.depthState,
    fbi.depthStencilMask);

  if (pass.depthState != RenderPassClass::Identifier::NO_DEPTH)
  {
    fbi.depthStencilAttachment.image->checkDead();

    G_ASSERTF(pass.colorSamples[0] == fbi.depthStencilAttachment.image->getSampleCount(),
      "Renderpass attachments sample count doesn't match target image sample count  (%d vs %d)!", pass.colorSamples[0],
      fbi.depthStencilAttachment.image->getSampleCount());

    bool change = setFrameImageState(fbi.depthStencilAttachment.image, ro ? SN_CONST_RENDER_TARGET : SN_RENDER_TARGET,
      fbi.depthStencilAttachment.view.getMipRange(), fbi.depthStencilAttachment.view.getArrayRange());
    if (change)
    {
      // debug("missed render target state!");
    }
    back.contextState.stageState[STAGE_PS].syncDepthROStateInT(fbi.depthStencilAttachment.image,
      fbi.depthStencilAttachment.view.getMipBase(), fbi.depthStencilAttachment.view.getArrayBase(), ro);
    back.contextState.stageState[STAGE_VS].syncDepthROStateInT(fbi.depthStencilAttachment.image,
      fbi.depthStencilAttachment.view.getMipBase(), fbi.depthStencilAttachment.view.getArrayBase(), ro);
  }
  else
  {
    back.contextState.stageState[STAGE_PS].syncDepthROStateInT(nullptr, 0, 0, ro);
    back.contextState.stageState[STAGE_VS].syncDepthROStateInT(nullptr, 0, 0, ro);
  }
}

void ExecutionContext::beginPassInternal(RenderPassClass *pass_class, const FramebufferInfo &pass_images, VulkanFramebufferHandle fb,
  VkRect2D area, uint32_t clear_mode)
{
  // input verify
  G_ASSERT(area.offset.x >= 0);
  G_ASSERT(area.offset.y >= 0);

  // shortcuts
  const RenderPassClass::Identifier &passIdent = pass_class->getIdentifier();
  bool constDS = pass_images.hasConstDepthStencilAttachment();
  FramebufferState &fbs = getFramebufferState();

  // prepare attachemnts states
  if (passIdent.colorTargetMask)
    ensureStateForColorAttachments(passIdent.colorTargetMask, pass_images);

  StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> clearValues;
  if (clear_mode & ~CLEAR_DISCARD)
  {
    clearValues = pass_class->constructClearValueSet(fbs.colorClearValue, fbs.depthStencilClearValue);
  }

  // MSAA split check, on TBDR this results in garbadge on screen
  if (!pass_class->verifyPass(clear_mode))
  {
    generateFaultReport();
    fatal("vulkan: MSAA pass was wrongly splitted around caller %s", getCurrentCmdCaller());
  }

  VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr};
#if VK_KHR_imageless_framebuffer
  VkRenderPassAttachmentBeginInfoKHR rpabi = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR, nullptr};
  if (device.hasImagelessFramebuffer())
  {
    auto attachments = passIdent.squash<VkImageView>(array_size(fbs.colorViews), fbs.colorViews, fbs.depthStencilView,
      [](const VulkanImageViewHandle &attachment) { return attachment.value; });
    rpabi.attachmentCount = attachments.size();
    rpabi.pAttachments = attachments.data();

    chain_structs(rpbi, rpabi);
  }
#endif
  rpbi.renderPass = pass_class->getPass(vkDev, clear_mode);
  rpbi.framebuffer = fb;
  rpbi.renderArea = area;
  rpbi.clearValueCount = clearValues.size();
  rpbi.pClearValues = clearValues.data();
  VULKAN_LOG_CALL(vkDev.vkCmdBeginRenderPass(getFrameCmdBuffer(), &rpbi, VK_SUBPASS_CONTENTS_INLINE));

  // save resulting state

  fbs.viewRect = area;
  fbs.clearMode = clear_mode;
  fbs.roDepth = constDS;

  back.executionState.set<StateFieldGraphicsRenderPassClass, RenderPassClass *, BackGraphicsState>(pass_class);
  back.executionState.set<StateFieldGraphicsFramebuffer, VulkanFramebufferHandle, BackGraphicsState>(fb);
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
    if (getFramebufferState().roDepth && !get_device().hasAttachmentNoStoreOp())
      syncConstDepthReadWithInternalStore();
    VULKAN_LOG_CALL(vkDev.vkCmdEndRenderPass(getFrameCmdBuffer()));
  }

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
