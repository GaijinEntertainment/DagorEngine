// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "3d/dag_lowLatency.h"
#include "descriptor_heap.h"
#include "extents.h"
#include "image_view_state.h"
#include "image_global_subresource_id.h"

#include <atomic>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <generic/dag_tab.h>
#include <supp/dag_comPtr.h>
#include <winapi_helpers.h>
#include <EASTL/array.h>
#include <EASTL/optional.h>


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
  XBOX_MEMBER bool preferHfr = false;
};

struct SwapchainCreateInfo : SwapchainProperties
{
  HWND window = nullptr;
  PresentationMode presentMode = PresentationMode::UNSYNCED;
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
  struct TargetInfo
  {
    HWND window = nullptr;
    eastl::unique_ptr<BaseTex> swapchainColorTex;
#if _TARGET_XBOX
    eastl::unique_ptr<BaseTex> swapchainSecondaryColorTex;
#endif
  };
#if _TARGET_PC_WIN
  HandlePointer waitableObject;
  C_ASSERT(max_swapchain_count <= 32);
  uint32_t freeSwapchainsMask = ~1u;
#endif
  eastl::array<TargetInfo, max_swapchain_count> targets{};
  uint32_t currentTargetIndex = 0;
  PresentationMode presentMode = PresentationMode::UNSYNCED;

public:
  struct SetupInfo
  {
    HWND window = nullptr;
    Image *colorImage = nullptr;
#if _TARGET_XBOX
    Image *secondaryColorImage = nullptr;
#elif _TARGET_PC_WIN
    HandlePointer waitableObject;
#endif
    PresentationMode presentMode = PresentationMode::UNSYNCED;
  };
  void setCurrentSwapchainIndex(uint32_t index) { currentTargetIndex = index; }
  void setup(SetupInfo setup, uint32_t swapchain_index);
  Extent2D getCurrentExtent() const;
  Extent2D getExtent(uint32_t swapchain_index) const;
  void bufferResize(const Extent2D &extent, uint32_t swapchain_index);
  bool isVsyncOn() const { return PresentationMode::UNSYNCED != presentMode; }
  PresentationMode getPresentationMode() const { return presentMode; }
  void changePresentMode(PresentationMode mode) { presentMode = mode; }
  void waitForFrameStart() const;
#if _TARGET_PC_WIN
  void disableLatencyWait() { waitableObject.reset(); }
  uint32_t allocateSwapchainIndex();
  void freeSwapchainIndex(uint32_t index);
#endif

  uint32_t getCurrentSwapchainIndex() const { return currentTargetIndex; }
#if _TARGET_PC_WIN
  eastl::optional<uint32_t> findSwapchainByWindow(HWND window) const;
#endif
  BaseTex *getColorTexture(uint32_t swapchain_index) const { return targets[swapchain_index].swapchainColorTex.get(); }
  BaseTex *getSecondaryColorTexture(uint32_t swapchain_index) const
  {
#if _TARGET_XBOX
    return targets[swapchain_index].swapchainSecondaryColorTex.get();
#else
    G_UNUSED(swapchain_index);
    return nullptr;
#endif
  }

  BaseTex *getCurrentColorTexture() const { return getColorTexture(currentTargetIndex); }
  BaseTex *getCurrentSecondaryColorTexture() const { return getSecondaryColorTexture(currentTargetIndex); }

  FormatStore getCurrentColorFormat() const;
  FormatStore getCurrentSecondaryColorFormat() const;

#if _TARGET_PC_WIN
  void preRecovery();
#endif
  void shutdown();
  void shutdownTarget(uint32_t swapchain_index);
};
} // namespace frontend

namespace backend
{
class Swapchain
{

  WIN_MEMBER std::atomic<uint32_t> numFramesCompletedByBackend = 0;

  XBOX_MEMBER FRAME_PIPELINE_TOKEN frameToken = {};

  struct SwapchainBufferInfo
  {
    ComPtr<ID3D12Resource> buffer;
    dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> viewTable;
  };

  // Structure to hold swapchain information for multiple windows
  struct SwapchainInfo
  {
    ComPtr<DXGISwapChain> swapchain;
    ComPtr<IDXGIOutput> output;
    eastl::unique_ptr<Image> colorTarget;
    Image *virtualColorTarget = nullptr;
    dag::Vector<SwapchainBufferInfo> colorTargets;
    dag::Vector<ImageViewInfo> swapchainViewSet;
    uint16_t currentColorTargetIndex = 0;

#if _TARGET_XBOX
    eastl::unique_ptr<Image> secondaryColorTarget;
    dag::Vector<SwapchainBufferInfo> secondaryColorTargets;
    dag::Vector<ImageViewInfo> secondarySwapchainViewSet;
#endif
  };

  // Multiple swapchains support
  eastl::array<SwapchainInfo, max_swapchain_count> swapchains;
  WIN_MEMBER uint32_t currentSwapchainIndex = 0;

  DescriptorHeap<ShaderResourceViewStagingPolicy> swapchainBufferSRVHeap;
  DescriptorHeap<RenderTargetViewPolicy> swapchainBufferRTVHeap;

  PresentationMode presentMode = PresentationMode::UNSYNCED;

  XBOX_MEMBER float frameImmediateThresholdPercent = 100.f;
  XBOX_MEMBER uint32_t userFreqLevel = 1; // index of 60Hz in freqLevels from swapchain.cpp
  XBOX_MEMBER uint32_t systemFreqLevel = 1;
  XBOX_MEMBER uint32_t activeFreqLevel = 1;
  XBOX_MEMBER bool isVrrSupportedValue = false;

  bool isHdrEnabled = false;
  XBOX_MEMBER bool autoGameDvr = true;
  XBOX_MEMBER bool hfrSupported = false;
  XBOX_MEMBER bool hfrEnabled = false;

  WIN_MEMBER bool isTearingSupported = false;

  void swapchainPresent(uint32_t frame_id = 0, uint32_t swapchain_index = 0);

  SwapchainInfo &getCurrentSwapchain();
  const SwapchainInfo &getCurrentSwapchain() const;

public:
  void setupVirtualBackbuffers(uint32_t swapchain_index, BaseTex *frontend_target, Image *virtual_backbuffer);
  void registerSwapchainView(D3DDevice *device, Image *image, ImageViewInfo info);
#if _TARGET_XBOX
  bool setup(Device &device, frontend::Swapchain &fe, const SwapchainCreateInfo &sci);
#else
  bool setup(Device &device, frontend::Swapchain &fe, DXGIFactory *factory, ID3D12CommandQueue *queue, SwapchainCreateInfo &&sci,
    uint32_t swapchain_index, bool disable_frame_latency = false);
  bool adopt(Device &device, frontend::Swapchain &fe, DXGISwapChain *external_swapchain, SwapchainCreateInfo &&sci);
#endif
  void onFrameBegin(D3DDevice *device, uint32_t swapchain_index);
  void present(Device &device, uint32_t frame_id = 0);
#if _TARGET_XBOX
  void restoreAfterSuspend(D3DDevice *device);
  void updateFrameInterval(D3DDevice *device, bool force = false, int32_t user_freq_level = -1);
#elif _TARGET_PC_WIN
  void preRecovery();
  void secondaryPresent(uint32_t swapchain_index, uint32_t frame_id);
#endif
  // context lock needs to be held, to protected descriptor heaps
  void clearViews(Device &device, uint32_t swapchain_index);
  void shutdownSwapchainInfo(uint32_t swapchain_index);
  void prepareForShutdown(Device &device, int swapchain_index);
  void prepareForShutdown(Device &device);
  void shutdown();
  void updateColorTextureObject(D3DDevice *device, uint32_t swapchain_index);
  void bufferResize(Device &device, const SwapchainProperties &props, uint32_t swapchain_index);

  Image *getColorImage(uint32_t index) const { return swapchains[index].colorTarget.get(); }
  Image *getVirtualColorImage(uint32_t index) const { return swapchains[index].virtualColorTarget; }
#if _TARGET_PC_WIN
  void setPresentWindow(uint32_t index);
#endif
  uint32_t getCurrentSwapchainIndex() const { return currentSwapchainIndex; }

#if _TARGET_XBOX
  Image *getSecondaryColorImage() const { return getCurrentSwapchain().secondaryColorTarget.get(); }

  constexpr uint32_t getPresentedFrameCount() const { return 0; }

  DXGISwapChain *getDxgiSwapchain() const { return nullptr; }
#elif _TARGET_PC_WIN
  IDXGIOutput *getOutput() const { return getCurrentSwapchain().output.Get(); }
  HRESULT getDesc(DXGI_SWAP_CHAIN_DESC *out_desc) const { return getCurrentSwapchain().swapchain->GetDesc(out_desc); }

  auto getPresentedFrameCount() const { return numFramesCompletedByBackend.load(std::memory_order_relaxed); }

  DXGISwapChain *getDxgiSwapchain() const { return getCurrentSwapchain().swapchain.Get(); }
#endif
  // returns true if there is an active present command submitted
  bool isVsyncOn() const { return PresentationMode::UNSYNCED != presentMode; }
  PresentationMode getPresentationMode() const { return presentMode; }
  void changePresentMode(PresentationMode mode) { presentMode = mode; }

  XBOX_FUNC bool isHfrEnabled() const { return hfrEnabled; }
  XBOX_FUNC bool isHfrSupported() const { return hfrSupported; }

#if _TARGET_XBOX
  bool isVrrSupported() const { return isVrrSupportedValue; }
#else
  bool isVrrSupported() const { return isTearingSupported; }
#endif

#if !_TARGET_XBOX
  bool postSetup(Device &device, frontend::Swapchain &fe, SwapchainCreateInfo &&sci, bool has_waitable_object, DXGIFactory *factory,
    uint32_t swapchain_index);
#endif
};
} // namespace backend
} // namespace drv3d_dx12
