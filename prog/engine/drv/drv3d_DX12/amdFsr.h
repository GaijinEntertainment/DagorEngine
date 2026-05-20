// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_amdFsr.h>
#include <util/dag_stdint.h>
#include <util/dag_string.h>

#include <EASTL/unique_ptr.h>

namespace drv3d_dx12
{
struct SwapchainCreateInfo;
}

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

class FSRD3D12
{
public:
  FSRD3D12();
  ~FSRD3D12();

  bool initUpscaling(const FSR::ContextArgs &args);
  void teardownUpscaling();

  Point2 getNextJitter(uint32_t render_width, uint32_t output_width);
  bool doApplyUpscaling(const FSR::UpscalingPlatformArgs &args, void *command_list) const;

  IPoint2 getRenderingResolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution) const;
  FSR::UpscalingMode getUpscalingMode() const;

  bool isLoaded() const;
  bool isUpscalingSupported() const;
  bool isFrameGenerationSupported() const;
  String getVersionString() const;

  void enableFrameGeneration(bool enable);
  void suppressFrameGeneration(bool suppress);
  void doScheduleGeneratedFrames(const FSR::FrameGenPlatformArgs &args, void *command_list);
  int getPresentedFrameCount();
  bool isFrameGenerationActive() const;
  bool isFrameGenerationSuppressed() const;

  void preRecover();
  void recover();

#if _TARGET_PC_WIN
  bool createFrameGenerationSwapchain(DXGIFactory *factory, ID3D12CommandQueue *queue,
    const drv3d_dx12::SwapchainCreateInfo &create_info, const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain);
  void releaseFrameGenerationSwapchainContext();
  uint64_t getMemorySize();
#endif

private:
  class Impl;
  eastl::unique_ptr<Impl> pImpl;
};

} // namespace amd
