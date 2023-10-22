#include "device.h"
#include "fsr2_wrapper.h"
#include <fsr2/ffx_fsr2.h>
#include <fsr2/ffx_fsr2_interface.h>
#include <fsr2/dx12/ffx_fsr2_dx12.h>
#include <type_traits>

using namespace drv3d_dx12;

static float getUpscaleRatio(int quality)
{
  enum class Fsr2Quality
  {
    QUALITY = 0,
    BALANCED,
    PERFORMANCE,
    ULTRA_PERFORMANCE
  };
  switch ((Fsr2Quality)quality)
  {
    case Fsr2Quality::QUALITY: return 1.5f;
    case Fsr2Quality::BALANCED: return 1.7f;
    case Fsr2Quality::PERFORMANCE: return 2.0f;
    case Fsr2Quality::ULTRA_PERFORMANCE: return 3.0f;
  }
  return 1.5f;
}

bool Fsr2Wrapper::init(void *device)
{
  size_t memory_size = ffxFsr2GetScratchMemorySizeDX12();
  scratchBuffer.resize(memory_size);
  contextDescr->device = ffxGetDeviceDX12(static_cast<ID3D12Device *>(device));
  contextDescr->flags = FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
  if (ffxFsr2GetInterfaceDX12(&contextDescr->callbacks, static_cast<ID3D12Device *>(device), scratchBuffer.data(), memory_size) ==
      FFX_OK)
  {
    state = Fsr2State::SUPPORTED;
    return true;
  }
  state = Fsr2State::INIT_ERROR;
  return false;
}
bool Fsr2Wrapper::createContext(uint32_t target_width, uint32_t target_height, int quality)
{
  float upscaleRatio = getUpscaleRatio(quality);
  renderWidth = ceil(target_width / upscaleRatio);
  renderHeight = ceil(target_height / upscaleRatio);
  contextDescr->displaySize = {target_width, target_height};
  contextDescr->maxRenderSize = {(uint32_t)renderWidth, (uint32_t)renderHeight};
  if (ffxFsr2ContextCreate(context.get(), contextDescr.get()) == FFX_OK)
  {
    state = Fsr2State::READY;
    return true;
  }
  state = Fsr2State::INIT_ERROR;
  return false;
}
void Fsr2Wrapper::evaluateFsr2(void *cmdList, const void *p)
{
  if (state != Fsr2State::READY)
    return;
  FfxFsr2DispatchDescription dispatchDescription{};
  Fsr2ParamsDx12 *params = (Fsr2ParamsDx12 *)p;
  dispatchDescription.commandList = ffxGetCommandListDX12(static_cast<ID3D12CommandList *>(cmdList));
  dispatchDescription.color =
    ffxGetResourceDX12(context.get(), params->inColor->getHandle(), nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);
  dispatchDescription.depth =
    ffxGetResourceDX12(context.get(), params->inDepth->getHandle(), nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);
  dispatchDescription.motionVectors =
    ffxGetResourceDX12(context.get(), params->inMotionVectors->getHandle(), nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);
  dispatchDescription.output =
    ffxGetResourceDX12(context.get(), params->outColor->getHandle(), nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
  dispatchDescription.jitterOffset = {params->jitterOffsetX, params->jitterOffsetY};
  dispatchDescription.renderSize = {params->renderSizeX, params->renderSizeY};
  dispatchDescription.frameTimeDelta = params->frameTimeDelta;
  dispatchDescription.motionVectorScale = {params->motionVectorScaleX, params->motionVectorScaleY};
  dispatchDescription.cameraNear = params->cameraNear;
  dispatchDescription.cameraFar = params->cameraFar;
  dispatchDescription.enableSharpening = params->sharpness >= 0;
  dispatchDescription.sharpness = params->sharpness;
  dispatchDescription.cameraFovAngleVertical = params->cameraFovAngleVertical;
  ffxFsr2ContextDispatch(context.get(), &dispatchDescription);
}
void Fsr2Wrapper::shutdown()
{
  if (state == Fsr2State::READY)
    ffxFsr2ContextDestroy(context.get());
  state = Fsr2State::NOT_CHECKED;
}
Fsr2State Fsr2Wrapper::getFsr2State() const { return state; }
void Fsr2Wrapper::getFsr2RenderingResolution(int &width, int &height) const
{
  width = renderWidth;
  height = renderHeight;
}
Fsr2Wrapper::Fsr2Wrapper()
{
  contextDescr.reset(new FfxFsr2ContextDescription{});
  context.reset(new FfxFsr2Context{});
}
Fsr2Wrapper::~Fsr2Wrapper() { shutdown(); }
