#include "device.h"
#include "swapchain.h"

using namespace drv3d_vulkan;

ExecutionContext::ExecutionContext(RenderWork &work_item) :
  data(work_item),
  device(get_device()),
  back(get_device().getContext().getBackend()),
  swapchain(get_device().swapchain),
  vkDev(get_device().getVkDevice())
{
  back.executionState.setExecutionContext(this);
  back.pipelineCompilationTime = 0;
#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  tlsDbgActiveInstance = this;
#endif
}

void ExecutionContext::beginCmd(const void *ptr) { cmd = ptr; }

void ExecutionContext::endCmd() { ++cmdIndex; }

GraphicsPipelineStateDescription &ExecutionContext::getGraphicsPipelineState()
{
  return back.executionState.get<BackGraphicsState, BackGraphicsState>().pipelineState;
}

FramebufferState &ExecutionContext::getFramebufferState()
{
  return back.executionState.get<BackGraphicsState, BackGraphicsState>().framebufferState;
}

uint32_t ExecutionContext::beginVertexUserData(uint32_t stride)
{
  using Bind = StateFieldGraphicsVertexBufferStride;
  uint32_t oldStride = back.pipelineState.get<StateFieldGraphicsVertexBufferStrides, Bind, FrontGraphicsState>().data;
  if (back.pipelineState.set<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({0, stride}))
    invalidateActiveGraphicsPipeline();

  return oldStride;
}

void ExecutionContext::endVertexUserData(uint32_t stride)
{
  using Bind = StateFieldGraphicsVertexBufferStride;
  if (back.pipelineState.set<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({0, stride}))
    invalidateActiveGraphicsPipeline();
}

void ExecutionContext::invalidateActiveGraphicsPipeline()
{
  back.executionState.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

bool ExecutionContext::isPartOfFramebuffer(Image *image)
{
  if (getFramebufferState().frontendFrameBufferInfo.depthStencilAttachment.image == image)
    return true;
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    if (getFramebufferState().frontendFrameBufferInfo.colorAttachments[i].image == image)
      return true;
  return false;
}

bool ExecutionContext::isPartOfFramebuffer(Image *image, ValueRange<uint8_t> mip_range, ValueRange<uint16_t> array_range)
{
  if (getFramebufferState().frontendFrameBufferInfo.depthStencilAttachment.image == image)
  {
    auto mipIndex = getFramebufferState().frontendFrameBufferInfo.depthStencilAttachment.view.getMipBase();
    auto arrayIndex = getFramebufferState().frontendFrameBufferInfo.depthStencilAttachment.view.getArrayBase();
    if (mip_range.isInside(mipIndex) && array_range.isInside(arrayIndex))
      return true;
  }
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (getFramebufferState().frontendFrameBufferInfo.colorAttachments[i].image == image)
    {
      auto mipIndex = getFramebufferState().frontendFrameBufferInfo.colorAttachments[i].view.getMipBase();
      auto arrayIndex = getFramebufferState().frontendFrameBufferInfo.colorAttachments[i].view.getArrayBase();
      if (mip_range.isInside(mipIndex) && array_range.isInside(arrayIndex))
        return true;
    }
  }
  return false;
}

Image *ExecutionContext::getSwapchainColorImage() { return swapchain.getColorImage(); }

void ExecutionContext::holdPreRotateStateForOneFrame() { swapchain.holdPreRotatedStateForOneFrame(); }

void ExecutionContext::startPreRotate(uint32_t binding_slot)
{
  swapchain.preRotateStart(binding_slot, back.contextState.frame.get(), *this);
}

Image *ExecutionContext::getSwapchainPrimaryDepthStencilImage()
{
  return getSwapchainDepthStencilImage(swapchain.getActiveMode().extent);
}

Image *ExecutionContext::getSwapchainDepthStencilImage(VkExtent2D extent)
{
  return swapchain.getDepthStencilImageForExtent(extent, back.contextState.frame.get());
}
