// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "fsr_args.h"

#include <EASTL/unique_ptr.h>

#if _TARGET_PC_WIN
struct IDXGIFactory7;
using DXGIFactory = IDXGIFactory7;
struct IDXGISwapChain3;
using DXGISwapChain = IDXGISwapChain3;
struct DXGI_SWAP_CHAIN_DESC1;
struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC;
struct ID3D12CommandQueue;
#endif

namespace amd
{
class FSRD3D12;
}

namespace drv3d_dx12
{
struct SwapchainCreateInfo;

using FsrParamsDx12 = FSRUpscalingArgs;
using FsrFgParamsDx12 = FSRFrameGenArgs;

class FsrWrapper
{
public:
  using ContextArgs = amd::FSR::ContextArgs;
  using UpscalingMode = amd::FSR::UpscalingMode;

  bool fsrInit();
  bool fsrCreateFeature(const ContextArgs &args);
  bool fsrShutdown();
  void teardownUpscaling();

  bool evaluateFsr(void *command_list, const FsrParamsDx12 &params);
  Point2 getNextJitter(uint32_t render_width, uint32_t output_width);
  IPoint2 getFsrRenderResolution(UpscalingMode mode, const IPoint2 &target_resolution) const;
  UpscalingMode getUpscalingMode() const;

  bool isLoaded() const;
  bool isUpscalingSupported() const;
  bool isFrameGenerationSupported() const;
  String getVersion() const;

  bool isFrameGenerationEnabled() const;
  bool isFrameGenerationSuppressed() const;
  void enableFrameGeneration(bool enable);
  void suppressFrameGeneration(bool suppress);
  void doScheduleGeneratedFrames(const amd::FSR::FrameGenPlatformArgs &args, void *command_list);
  int getPresentedFrameCount();
  uint64_t getMemoryUsage() const;
#if _TARGET_PC_WIN
  bool createFrameGenerationSwapchain(DXGIFactory *factory, ID3D12CommandQueue *queue, const SwapchainCreateInfo &create_info,
    const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain);
  void releaseFrameGenerationSwapchainContext();
#endif

  FsrWrapper();
  ~FsrWrapper();

private:
  eastl::unique_ptr<amd::FSRD3D12> pImpl;
};

#if _TARGET_PC_WIN
bool create_fsrfg_swapchain(DXGIFactory *factory, ID3D12CommandQueue *queue, const SwapchainCreateInfo &create_info,
  const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain);
#endif

} // namespace drv3d_dx12
