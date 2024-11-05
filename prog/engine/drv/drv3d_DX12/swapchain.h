// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_tab.h>
#include <supp/dag_comPtr.h>
#include <atomic>
#include <winapi_helpers.h>

#include "extents.h"
#include "image_view_state.h"
#include "descriptor_heap.h"


namespace drv3d_dx12
{
class BaseTex;
class Image;
class Device;
class DeviceContext;

enum class PresentationMode
{
  VSYNCED,
  CONDITIONAL_VSYNCED,
  UNSYNCED
};

struct SwapchainProperties
{
  Extent2D resolution = {};
  bool enableHdr = false;
  WIN_MEMBER bool forceHdr = false;
  XBOX_MEMBER bool autoGameDvr = true;
};

struct SwapchainCreateInfo : SwapchainProperties
{
  HWND window = nullptr;
  PresentationMode presentMode = PresentationMode::UNSYNCED;
  WIN_MEMBER bool windowed = false;
  XBOX_MEMBER float frameImmediateThresholdPercent = 100.f;
  XBOX_MEMBER unsigned int freqLevel = 1;

#if _TARGET_PC_WIN
  ComPtr<IDXGIOutput> output;
#endif
};

namespace frontend
{
class Swapchain
{
  eastl::unique_ptr<BaseTex> swapchainColorTex;
#if _TARGET_XBOX
  eastl::unique_ptr<BaseTex> swapchainSecondaryColorTex;
#elif _TARGET_PC_WIN
  HandlePointer waitableObject;
#endif
  PresentationMode presentMode = PresentationMode::UNSYNCED;

public:
  struct SetupInfo
  {
    Image *colorImage;
#if _TARGET_XBOX
    Image *secondaryColorImage;
#elif _TARGET_PC_WIN
    HandlePointer waitableObject;
#endif
    PresentationMode presentMode;
  };
  void setup(SetupInfo setup);
  Extent2D getExtent() const;
  void bufferResize(const Extent2D &extent);
  bool isVsyncOn() const { return PresentationMode::UNSYNCED != presentMode; }
  PresentationMode getPresentationMode() const { return presentMode; }
  void changePresentMode(PresentationMode mode) { presentMode = mode; }
  void waitForFrameStart() const;

  BaseTex *getColorTexture() const { return swapchainColorTex.get(); }
  BaseTex *getSecondaryColorTexture() const
  {
#if _TARGET_XBOX
    return swapchainSecondaryColorTex.get();
#else
    return nullptr;
#endif
  }

  FormatStore getColorFormat() const;
  FormatStore getSecondaryColorFormat() const;

#if _TARGET_PC_WIN
  void preRecovery();
#endif
  void prepareForShutdown(Device &device);
  void shutdown();
};
} // namespace frontend

namespace backend
{
class Swapchain
{
  uint16_t currentColorTargetIndex = 0;
  uint16_t previousColorTargetIndex = 0;

  WIN_MEMBER uint32_t numPresentSentToBackend = 0;
  WIN_MEMBER uint32_t numFramesPresentedByFrontend = 0;
  WIN_MEMBER std::atomic<uint32_t> numFramesCompletedByBackend = 0;

  XBOX_MEMBER FRAME_PIPELINE_TOKEN frameToken = {};

  struct SwapchainBufferInfo
  {
    ComPtr<ID3D12Resource> buffer;
    dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> viewTable;
  };

  alignas(std::hardware_destructive_interference_size) //
    eastl::unique_ptr<Image> colorTarget;
  dag::Vector<SwapchainBufferInfo> colorTargets;
  dag::Vector<ImageViewInfo> swapchainViewSet;

#if _TARGET_XBOX
  eastl::unique_ptr<Image> secondaryColorTarget;
  dag::Vector<SwapchainBufferInfo> secondaryColorTargets;
  dag::Vector<ImageViewInfo> secondarySwapchainViewSet;
#elif _TARGET_PC_WIN
  ComPtr<DXGISwapChain> swapchain;
  ComPtr<IDXGIOutput> output;
#endif

  DescriptorHeap<ShaderResouceViewStagingPolicy> swapchainBufferSRVHeap;
  DescriptorHeap<RenderTargetViewPolicy> swapchainBufferRTVHeap;

  PresentationMode presentMode = PresentationMode::UNSYNCED;

  XBOX_MEMBER float frameImmediateThresholdPercent = 100.f;
  XBOX_MEMBER uint32_t userFreqLevel = 1; // index of 60Hz in freqLevels from swapchain.cpp
  XBOX_MEMBER uint32_t systemFreqLevel = 1;
  XBOX_MEMBER uint32_t activeFreqLevel = 1;

  bool isHdrEnabled = false;
  XBOX_MEMBER bool autoGameDvr = true;

  WIN_MEMBER bool isInExclusiveFullscreenModeEnabled = false;
  WIN_MEMBER bool isTearingSupported = false;

  void swapchainPresent();

public:
  void registerSwapchainView(D3DDevice *device, Image *image, ImageViewInfo info);
#if _TARGET_XBOX
  bool setup(Device &device, frontend::Swapchain &fe, const SwapchainCreateInfo &sci);
#else
  bool setup(Device &device, frontend::Swapchain &fe, DXGIFactory *factory, ID3D12CommandQueue *queue, SwapchainCreateInfo &&sci);
#endif
  void onFrameBegin(D3DDevice *device);
  void present(Device &device);
#if _TARGET_XBOX
  void restoreAfterSuspend(D3DDevice *device);
  void updateFrameInterval(D3DDevice *device, bool force = false, int32_t user_freq_level = -1);
#elif _TARGET_PC_WIN
  void preRecovery();
#endif
  // context lock needs to be held, to protected descriptor heaps
  void prepareForShutdown(Device &device);
  void shutdown();
  void updateColorTextureObject(D3DDevice *device);
  void bufferResize(Device &device, const SwapchainProperties &props);

  Image *getColorImage() const { return colorTarget.get(); }
#if _TARGET_XBOX
  Image *getSecondaryColorImage() const { return secondaryColorTarget.get(); }

  constexpr bool swapchainPresentFromMainThread(DeviceContext &) { return true; }

  constexpr uint32_t getPresentedFrameCount() const { return 0; }
#elif _TARGET_PC_WIN
  IDXGIOutput *getOutput() const { return output.Get(); }
  HRESULT getDesc(DXGI_SWAP_CHAIN_DESC *out_desc) const { return swapchain->GetDesc(out_desc); }

  bool isInExclusiveFullscreenMode() const;
  void changeFullscreenExclusiveMode(bool is_exclusive);
  void changeFullscreenExclusiveMode(bool is_exclusive, ComPtr<IDXGIOutput> in_output);

  bool swapchainPresentFromMainThread(DeviceContext &ctx);
  void signalPresentSentToBackend() { numPresentSentToBackend++; }

  auto getPresentedFrameCount() const { return numFramesCompletedByBackend.load(std::memory_order_relaxed); }
#endif
  // returns true if there is an active present command submitted
  WIN_FUNC bool wasCurrentFramePresentSubmitted() const { return numFramesPresentedByFrontend < numPresentSentToBackend; }

  PresentationMode getPresentationMode() const { return presentMode; }
  void changePresentMode(PresentationMode mode) { presentMode = mode; }

  WIN_FUNC bool isVrrSupported() const { return isTearingSupported; }
};
} // namespace backend
} // namespace drv3d_dx12
