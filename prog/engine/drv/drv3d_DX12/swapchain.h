#pragma once

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_tab.h>
#include <supp/dag_comPtr.h>
#include <atomic>

#include "extents.h"
#include "image_view_state.h"
#include "descriptor_heap.h"
#include "winapi_helpers.h"


namespace drv3d_dx12
{
struct BaseTex;
class Image;
class Device;
class DeviceContext;

enum class PresentationMode
{
  VSYNCED,
  CONDITIONAL_VSYNCED,
  UNSYNCED
};

enum class OutputMode
{
  PRESENT,
  MINIMIZED,
};

struct SwapchainCreateInfo
{
  HWND window = nullptr;
  PresentationMode presentMode = PresentationMode::UNSYNCED;
  bool windowed = false;
  int resolutionX = 0;
  int resolutionY = 0;
#if _TARGET_XBOX
  float frameImmediateThresholdPercent = 100.f;
#endif
#if _TARGET_PC_WIN
  ComPtr<IDXGIOutput> output;

  bool enableHdr = false;
  bool forceHdr = false;
#elif _TARGET_XBOX
  bool enableHdr = false;
  bool autoGameDvr = true;
  unsigned int freqLevel = 1;
#endif
};

namespace frontend
{
class Swapchain
{
#if _TARGET_PC_WIN
  HandlePointer waitableObject;
#endif
  PresentationMode presentMode = PresentationMode::UNSYNCED;
  eastl::unique_ptr<BaseTex> swapchainColorTex;
#if _TARGET_XBOX
  eastl::unique_ptr<BaseTex> swapchainSecondaryColorTex;
#endif
  eastl::unique_ptr<BaseTex> swapchainDepthStencilTex;

public:
  struct SetupInfo
  {
#if _TARGET_PC_WIN
    HandlePointer waitableObject;
#endif
    Image *colorImage;
#if _TARGET_XBOX
    Image *secondaryColorImage;
#endif
    PresentationMode presentMode;
  };
  void setup(SetupInfo setup);
  Extent2D getExtent() const;
  void bufferResize(const Extent2D &extent);
  bool isVsyncOn() const { return PresentationMode::UNSYNCED != presentMode; }
  PresentationMode getPresentationMode() const { return presentMode; }
  void changePresentMode(PresentationMode mode) { presentMode = mode; }
  void waitForFrameStart();

  BaseTex *getColorTexture() { return swapchainColorTex.get(); }
  BaseTex *getSecondaryColorTexture()
  {
#if _TARGET_XBOX
    return swapchainSecondaryColorTex.get();
#else
    return nullptr;
#endif
  }

  BaseTex *getDepthStencilTexture(Device &device, Extent2D ext);
  BaseTex *getDepthStencilTextureAnySize() { return swapchainDepthStencilTex.get(); }

  FormatStore getColorFormat() const;
  FormatStore getSecondaryColorFormat() const;

  FormatStore getDepthStencilFormat() const { return FormatStore::fromCreateFlags(TEXFMT_DEPTH24); }

  void prepareForShutdown(Device &device);
#if _TARGET_PC_WIN
  void preRecovery();
#endif
  void shutdown()
  {
    swapchainColorTex.reset();
#if _TARGET_XBOX
    swapchainSecondaryColorTex.reset();
#endif
    swapchainDepthStencilTex.reset();
  }
};
} // namespace frontend

namespace backend
{
class Swapchain
{
  PresentationMode presentMode = PresentationMode::UNSYNCED;
  uint16_t currentColorTargetIndex = 0;
  uint16_t previousColorTargetIndex = 0;
#if _TARGET_PC_WIN
  bool isInExclusiveFullscreenModeEnabled = false;
  bool isTearingSupported = false;

  // with c++ 17 std lib would have standard defined values
  static constexpr size_t cache_line_size = 64;

  uint32_t numPresentSentToBackend{0};
  uint32_t numFramesPresentedByFrontend{0};
  alignas(cache_line_size) std::atomic<uint32_t> numFramesCompletedByBackend{0};

  void swapchainPresentOnPC();
#endif
#if DX12_HAS_GAMMA_CONTROL
  bool gammaRampApplied = false;
  DXGI_GAMMA_CONTROL gammaRamp{};
#endif
#if _TARGET_XBOX
  FRAME_PIPELINE_TOKEN frameToken{};
  float frameImmediateThresholdPercent = 100.f;
  uint32_t userFreqLevel = 1; // index of 60Hz in freqLevels from swapchain.cpp
  uint32_t systemFreqLevel = 1;
  uint32_t activeFreqLevel = 1;
#endif
#if _TARGET_PC_WIN
  ComPtr<DXGISwapChain> swapchain;
  ComPtr<IDXGIOutput> output;
#endif
  struct SwapchainBufferInfo
  {
    ComPtr<ID3D12Resource> buffer;
    eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> viewTable;
  };
  eastl::vector<SwapchainBufferInfo> colorTargets;
  eastl::unique_ptr<Image> colorTarget;

#if _TARGET_XBOX
  eastl::vector<SwapchainBufferInfo> secondaryColorTargets;
  eastl::unique_ptr<Image> secondaryColorTarget;
#endif

  bool isHdrEnabled = false;
#if _TARGET_XBOX
  bool autoGameDvr = true;
#endif

  DescriptorHeap<ShaderResouceViewStagingPolicy> swapchainBufferSRVHeap;
  DescriptorHeap<RenderTargetViewPolicy> swapchainBufferRTVHeap;
  eastl::vector<ImageViewInfo> swapchainViewSet;
#if _TARGET_XBOX
  eastl::vector<ImageViewInfo> secondarySwapchainViewSet;
#endif

public:
  void registerSwapchainView(D3DDevice *device, Image *image, ImageViewInfo info);
#if _TARGET_PC_WIN
  using SwapchainInitParameter = ID3D12CommandQueue;
#elif _TARGET_XBOX
  using SwapchainInitParameter = D3DDevice;
#endif
#if _TARGET_XBOX
  bool setup(Device &device, frontend::Swapchain &fe, SwapchainInitParameter *queue, const SwapchainCreateInfo &sci);
#else
  bool setup(Device &device, frontend::Swapchain &fe, DXGIFactory *factory, SwapchainInitParameter *queue, SwapchainCreateInfo &&sci);
#endif
  void onFrameBegin(D3DDevice *device);
  void present(Device &device);
#if DX12_HAS_GAMMA_CONTROL
  bool setGamma(float power);
#endif
#if _TARGET_XBOX
  void restoreAfterSuspend(D3DDevice *device);
  void updateFrameInterval(D3DDevice *device, bool force = false, int32_t user_freq_level = -1);
#endif
  // context lock needs to be held, to protected descriptor heaps
  void prepareForShutdown(Device &device);
#if _TARGET_PC_WIN
  void preRecovery();
#endif
  void shutdown();
  void updateColorTextureObject(D3DDevice *device);
  void bufferResize(Device &device, const Extent2D &extent, FormatStore color_format);
#if _TARGET_PC_WIN
  bool isInExclusiveFullscreenMode();
  void changeFullscreenExclusiveMode(bool is_exclusive);
  void changeFullscreenExclusiveMode(bool is_exclusive, ComPtr<IDXGIOutput> in_output);

  // returns true if there is an active present command submitted
  bool wasCurrentFramePresentSubmitted() const { return numFramesPresentedByFrontend < numPresentSentToBackend; }
  void signalPresentSentToBackend() { numPresentSentToBackend++; }
  bool swapchainPresentFromMainThread(DeviceContext &ctx);
#endif

  Image *getColorImage() const { return colorTarget.get(); }

#if _TARGET_XBOX
  Image *getSecondaryColorImage() const { return secondaryColorTarget.get(); }
#endif

#if _TARGET_PC_WIN
  IDXGIOutput *getOutput() { return output.Get(); }

  HRESULT getDesc(DXGI_SWAP_CHAIN_DESC *out_desc) { return swapchain->GetDesc(out_desc); }
#endif

  PresentationMode getPresentationMode() const { return presentMode; }

  void changePresentMode(PresentationMode mode) { presentMode = mode; }

  bool isVrrSupported() const
  {
#if _TARGET_PC_WIN
    return isTearingSupported;
#else
    return false;
#endif
  }
};
} // namespace backend
} // namespace drv3d_dx12
