// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "swapchain.h"
#include "device.h"
#include "device_context.h"
#include "resource_manager/image.h"
#include "texture.h"

#include <3d/gpuLatency.h>
#include <3d/dag_lowLatency.h>

#include <EASTL/finally.h>

using namespace drv3d_dx12;

#if _TARGET_XBOX
#include "swapchain_xbox.inc.cpp"
#endif

// ~~~~~~~~~~~~~~ FRONTEND ~~~~~~~~~~~~~~

void frontend::Swapchain::setup(SetupInfo setup, uint32_t swapchain_index)
{
#if _TARGET_PC_WIN
  G_ASSERT((freeSwapchainsMask & (1u << swapchain_index)) == 0);
#endif
  auto &target = targets[swapchain_index];
  target.window = setup.window;
  auto &swapchainColorTex = target.swapchainColorTex;

#if _TARGET_PC_WIN
  waitableObject = eastl::move(setup.waitableObject);
#endif
  if (!swapchainColorTex)
  {
    swapchainColorTex.reset(new BaseTex{D3DResourceType::TEX, setup.colorImage->getFormat().asTexFlags() | TEXCF_RTARGET});
  }
  else
  {
    // might be changed after reset and we where unable to create with same format
    swapchainColorTex->cflg = setup.colorImage->getFormat().asTexFlags() | TEXCF_RTARGET;
  }
  auto &extent = setup.colorImage->getBaseExtent();
  swapchainColorTex->image = setup.colorImage;
  char name[64];
  snprintf(name, sizeof(name), "swapchain color target %u", swapchain_index);
  swapchainColorTex->setParams(extent.width, extent.height, 1, 1, name);

#if _TARGET_XBOX
  if (setup.secondaryColorImage)
  {
    auto &swapchainSecondaryColorTex = targets[swapchain_index].swapchainSecondaryColorTex;

    if (!swapchainSecondaryColorTex)
    {
      swapchainSecondaryColorTex.reset(
        new BaseTex{D3DResourceType::TEX, setup.secondaryColorImage->getFormat().asTexFlags() | TEXCF_RTARGET});
    }
    else
    {
      // might be changed after reset and we where unable to create with same format
      swapchainSecondaryColorTex->cflg = setup.secondaryColorImage->getFormat().asTexFlags() | TEXCF_RTARGET;
    }
    swapchainSecondaryColorTex->image = setup.secondaryColorImage;
    swapchainSecondaryColorTex->setParams(extent.width, extent.height, 1, 1, "swapchain secondary color target");
  }
#endif

  presentMode = setup.presentMode;
}

void backend::Swapchain::setupVirtualBackbuffers(uint32_t swapchain_index, BaseTex *frontend_target, Image *virtual_backbuffer)
{
  auto &swapchainInfo = swapchains[swapchain_index];

  G_ASSERT(!swapchainInfo.virtualColorTarget);
  swapchainInfo.virtualColorTarget = virtual_backbuffer;

  G_VERIFY(frontend_target->swapTextureNoLock(swapchainInfo.colorTarget.get(), swapchainInfo.virtualColorTarget));
}

void frontend::Swapchain::waitForFrameStart() const
{
#if _TARGET_PC
  if (waitableObject)
  {
    // In some cases the object times out even when the GPU is idle and waits for work,
    // this seems to happen when frames on the CPU take very long (during loading for example),
    // and the GPU finishes all outstanding frames before we wait here.
    auto result = WaitForSingleObjectEx(waitableObject.get(), MAX_WAIT_OBJECT_TIMEOUT_MS, FALSE);
    if (WAIT_OBJECT_0 != result)
    {
      // don't report it as error, we can go on, so just warn about it.
      logwarn("DX12: Wait for low latency waitable object timed out after %ums, with result 0x%08X", MAX_WAIT_OBJECT_TIMEOUT_MS,
        result);
    }
  }
#endif
}

#if _TARGET_PC_WIN
uint32_t frontend::Swapchain::allocateSwapchainIndex()
{
  D3D_CONTRACT_ASSERTF_RETURN(freeSwapchainsMask != 0, max_swapchain_count,
    "DX12: Too many swapchains created, consider to use `d3d::window_destroyed` to free deleted windows resources!");
  uint32_t swapchainIndex = __bsf(freeSwapchainsMask);
  freeSwapchainsMask &= ~(1u << swapchainIndex);
  return swapchainIndex;
}

void frontend::Swapchain::freeSwapchainIndex(uint32_t index)
{
  G_ASSERT(index < max_swapchain_count);
  G_ASSERT((freeSwapchainsMask & (1u << index)) == 0);
  freeSwapchainsMask |= (1u << index);
}
#endif

void frontend::Swapchain::shutdownTarget(uint32_t swapchain_index)
{
  TargetInfo &target = targets[swapchain_index];
  target.swapchainColorTex.reset();
#if _TARGET_XBOX
  target.swapchainSecondaryColorTex.reset();
#endif
}

void frontend::Swapchain::shutdown()
{
  for (uint32_t index = 0; index < max_swapchain_count; index++)
    shutdownTarget(index);
  currentTargetIndex = 0;
#if _TARGET_PC_WIN
  freeSwapchainsMask = ~1u;
#endif
}

// ~~~~~~~~~~~~~~ BACKEND  ~~~~~~~~~~~~~~

#if _TARGET_PC_WIN
eastl::optional<uint32_t> frontend::Swapchain::findSwapchainByWindow(HWND window) const
{
  for (auto index : LsbVisitor{~freeSwapchainsMask})
  {
    if (targets[index].window == window)
      return index;
  }
  return eastl::nullopt;
}
#endif

backend::Swapchain::SwapchainInfo &backend::Swapchain::getCurrentSwapchain() { return swapchains[currentSwapchainIndex]; }

const backend::Swapchain::SwapchainInfo &backend::Swapchain::getCurrentSwapchain() const { return swapchains[currentSwapchainIndex]; }

#if _TARGET_PC_WIN
void backend::Swapchain::setPresentWindow(uint32_t index) { currentSwapchainIndex = index; }
#endif

void backend::Swapchain::registerSwapchainView(D3DDevice *device, Image *image, ImageViewInfo info)
{
  auto &currentSwapchain = getCurrentSwapchain();
  auto &colorTarget = currentSwapchain.colorTarget;
  auto &swapchainViewSet = currentSwapchain.swapchainViewSet;
  auto &colorTargets = currentSwapchain.colorTargets;

#if !_TARGET_XBOX
  G_UNUSED(image);
  G_UNUSED(colorTarget);
#else
  if (image == colorTarget.get())
#endif
  {
    swapchainViewSet.push_back(info);

    D3D12_DESCRIPTOR_HEAP_TYPE descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER; // use invalid type

    if (info.state.isSRV())
    {
      auto desc = info.state.asSRVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, image->isMultisampled());
      for (auto &buffer : colorTargets)
      {
        auto descriptor = swapchainBufferSRVHeap.allocate(device);
        device->CreateShaderResourceView(buffer.buffer.Get(), &desc, descriptor);
        buffer.viewTable.push_back(descriptor);
        G_ASSERT(buffer.viewTable.size() == swapchainViewSet.size());
      }
      descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    }
    else if (info.state.isUAV())
    {
      auto desc = info.state.asUAVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D);
      for (auto &buffer : colorTargets)
      {
        auto descriptor = swapchainBufferSRVHeap.allocate(device);
        device->CreateUnorderedAccessView(buffer.buffer.Get(), nullptr, &desc, descriptor);
        buffer.viewTable.push_back(descriptor);
        G_ASSERT(buffer.viewTable.size() == swapchainViewSet.size());
      }
      descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    }
    else if (info.state.isRTV())
    {
      auto desc = info.state.asRTVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, image->isMultisampled());
      for (auto &buffer : colorTargets)
      {
        auto descriptor = swapchainBufferRTVHeap.allocate(device);
        device->CreateRenderTargetView(buffer.buffer.Get(), &desc, descriptor);
        buffer.viewTable.push_back(descriptor);
        G_ASSERT(buffer.viewTable.size() == swapchainViewSet.size());
      }
      descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    }
    else
    {
      DAG_FATAL("DX12: Invalid view type for swapchain");
    }

    // descriptor is still blank, need to copy the correct descriptor into the target descriptor of the image
    device->CopyDescriptorsSimple(1, info.handle, colorTargets[currentSwapchain.currentColorTargetIndex].viewTable.back(),
      descriptorType);
  }
#if _TARGET_XBOX
  else if (image == currentSwapchain.secondaryColorTarget.get())
  {
    auto &secondarySwapchainViewSet = currentSwapchain.secondarySwapchainViewSet;
    auto &secondaryColorTargets = currentSwapchain.secondaryColorTargets;
    if (!autoGameDvr)
    {
      secondarySwapchainViewSet.push_back(info);

      D3D12_DESCRIPTOR_HEAP_TYPE descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER; // use invalid type

      if (info.state.isSRV())
      {
        auto desc = info.state.asSRVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, image->isMultisampled());
        for (auto &buffer : secondaryColorTargets)
        {
          auto descriptor = swapchainBufferSRVHeap.allocate(device);
          device->CreateShaderResourceView(buffer.buffer.Get(), &desc, descriptor);
          buffer.viewTable.push_back(descriptor);
          G_ASSERT(buffer.viewTable.size() == secondarySwapchainViewSet.size());
        }
        descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      }
      else if (info.state.isRTV())
      {
        auto desc = info.state.asRTVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, image->isMultisampled());
        for (auto &buffer : secondaryColorTargets)
        {
          auto descriptor = swapchainBufferRTVHeap.allocate(device);
          device->CreateRenderTargetView(buffer.buffer.Get(), &desc, descriptor);
          buffer.viewTable.push_back(descriptor);
          G_ASSERT(buffer.viewTable.size() == secondarySwapchainViewSet.size());
        }
        descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      }
      else
      {
        DAG_FATAL("DX12: Invalid view type for secondary swapchain");
      }

      // descriptor is still blank, need to copy the correct descriptor into the target descriptor of the image
      device->CopyDescriptorsSimple(1, info.handle, secondaryColorTargets[currentSwapchain.currentColorTargetIndex].viewTable.back(),
        descriptorType);
    }
  }
  else
  {
    DAG_FATAL("DX12: Invalid image for swapchainView register");
  }
#endif
}


#if _TARGET_PC_WIN

static bool is_tearing_supported(DXGIFactory *factory)
{
  BOOL allowTearing = FALSE;
  HRESULT hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
  return SUCCEEDED(hr) && allowTearing;
}

bool backend::Swapchain::adopt(Device &device, frontend::Swapchain &fe, DXGISwapChain *external_swapchain, SwapchainCreateInfo &&sci)
{
  swapchainBufferSRVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  swapchainBufferRTVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

  DXGIFactory *factory = nullptr;
  HWND window;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
  DXGI_SWAP_CHAIN_DESC1 desc1;
  external_swapchain->GetParent(IID_PPV_ARGS(&factory));
  external_swapchain->GetHwnd(&window);
  external_swapchain->GetFullscreenDesc(&fullscreenDesc);
  external_swapchain->GetDesc1(&desc1);

  G_ASSERT(currentSwapchainIndex == 0);
  getCurrentSwapchain().swapchain = external_swapchain;

  auto res = postSetup(device, fe, eastl::move(sci), true, factory, currentSwapchainIndex);
  factory->Release();
  return res;
}

bool backend::Swapchain::setup(Device &device, frontend::Swapchain &fe, DXGIFactory *factory, ID3D12CommandQueue *queue,
  SwapchainCreateInfo &&sci, uint32_t swapchain_index, bool disable_frame_latency)
{
  swapchainBufferSRVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  swapchainBufferRTVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

  const bool isThereDisplay = [] {
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    return EnumDisplayDevices(nullptr, 0, &dd, 0) != FALSE;
  }();
  const bool hasLatencyObject = isThereDisplay && !disable_frame_latency;

  presentMode = sci.presentMode;

  FormatStore colorFormat = FormatStore::fromCreateFlags(TEXFMT_DEFAULT);
  isTearingSupported = is_tearing_supported(factory);

  DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {
    .Width = sci.resolution.width,
    .Height = sci.resolution.height,
    .Format = colorFormat.asLinearDxGiFormat(),
    .SampleDesc = {.Count = 1u},
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
    .BufferCount = FRAME_FRAME_BACKLOG_LENGTH,
    // fastest mode, allows composer to scribble on it to avoid extra copy
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    .Flags = (hasLatencyObject ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0u) |
             (isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u),
  };

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {.Windowed = TRUE};

  // create with default size (target size)
  ComPtr<IDXGISwapChain1> swapchainLevel1;

  if (DX12_CHECK_FAIL(factory->CreateSwapChainForHwnd(queue, sci.window, &swapchainDesc, &fullscreenDesc, nullptr, &swapchainLevel1)))
    return false;


  auto &swapchain = swapchains[swapchain_index].swapchain;
  auto &output = swapchains[swapchain_index].output;

  swapchainLevel1.As(&swapchain);
  if (!swapchain)
  {
    return false;
  }
  output = eastl::move(sci.output);

  return postSetup(device, fe, eastl::move(sci), hasLatencyObject, factory, swapchain_index);
}

bool backend::Swapchain::postSetup(Device &device, frontend::Swapchain &fe, SwapchainCreateInfo &&sci, bool has_waitable_object,
  DXGIFactory *factory, uint32_t swapchain_index)
{
  DX12_DEBUG_RESULT(factory->MakeWindowAssociation(sci.window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN));

  auto &swapchainInfo = swapchains[swapchain_index];
  auto &swapchain = swapchainInfo.swapchain;
  auto &output = swapchainInfo.output;
  auto &colorTarget = swapchainInfo.colorTarget;

  if (has_waitable_object)
  {
    DX12_DEBUG_RESULT(swapchain->SetMaximumFrameLatency(FRAME_LATENCY));
  }

  // when no output is explicitly set, we grab it from the swapchain
  if (!output)
  {
    DX12_DEBUG_RESULT(swapchain->GetContainingOutput(&output));
  }

  DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
  DX12_DEBUG_RESULT(swapchain->GetDesc1(&swapchainDesc));

  if (sci.resolution.width * sci.resolution.height == 0)
  {
    sci.resolution.width = swapchainDesc.Width;
    sci.resolution.height = swapchainDesc.Height;
  }

  bufferResize(device, sci, swapchain_index);

  fe.setup(
    {
      .window = sci.window,
      .colorImage = colorTarget.get(),
      .waitableObject{has_waitable_object ? swapchain->GetFrameLatencyWaitableObject() : nullptr},
      .presentMode = sci.presentMode,
    },
    swapchain_index);

  return true;
}

void backend::Swapchain::onFrameBegin(D3DDevice *device, uint32_t swapchain_index)
{
  swapchains[swapchain_index].currentColorTargetIndex = swapchains[swapchain_index].swapchain->GetCurrentBackBufferIndex();
  updateColorTextureObject(device, swapchain_index);
}

void backend::Swapchain::swapchainPresent(uint32_t frame_id, uint32_t swapchain_index)
{
  if (swapchain_index == 0)
  {
    auto *lowLatencyModule = GpuLatency::getInstance();
    if (lowLatencyModule)
      lowLatencyModule->setMarker(frame_id, lowlatency::LatencyMarkerType::PRESENT_START);
    eastl::finally deferredPresentEnd([&] {
      if (lowLatencyModule)
        lowLatencyModule->setMarker(frame_id, lowlatency::LatencyMarkerType::PRESENT_END);
    });
  }

  const bool useVSync = isVsyncOn();
  UINT interval = useVSync ? 1 : 0;
  // ALLOW_TEARING is required for disable v-sync
  // ALLOW_TEARING requires interval=0 and not exclusive fullscreen
  // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
  UINT flags = (!useVSync && isTearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;
  const auto &swapchain = swapchains[swapchain_index].swapchain;
  auto resultCode = DX12_CHECK_RESULT(swapchain->Present(interval, flags));
  if (FAILED(resultCode))
  {
    logdbg("DX12: swapchain->Present(interval:%u, flags: %08X)", interval, flags);
  }
}

void backend::Swapchain::present(Device &, uint32_t frame_id)
{
  numFramesCompletedByBackend++;
  swapchainPresent(frame_id, getCurrentSwapchainIndex());
}

void backend::Swapchain::secondaryPresent(uint32_t swapchain_index, uint32_t frame_id) { swapchainPresent(frame_id, swapchain_index); }
#endif

void backend::Swapchain::prepareForShutdown(Device &device, int swapchain_index)
{
  clearViews(device, swapchain_index);
  if (swapchains[swapchain_index].virtualColorTarget)
  {
    device.getContext().destroyImage(swapchains[swapchain_index].virtualColorTarget, true);
    swapchains[swapchain_index].virtualColorTarget = nullptr;
  }
}

void backend::Swapchain::prepareForShutdown(Device &device)
{
#if _TARGET_XBOX
  // can be null if we had to abort a during device init
  if (device.queues[DeviceQueueType::GRAPHICS].getHandle())
  {
    // have to "null" flip otherwise one swapchain image is still held by the display manager and we crash
    xbox_queue_dummy_present(device.queues[DeviceQueueType::GRAPHICS].getHandle());
  }
#endif
  for (int i = 0; i < max_swapchain_count; i++)
  {
    prepareForShutdown(device, i);
  }
  swapchainBufferSRVHeap.freeAll();
  swapchainBufferRTVHeap.freeAll();
}

void backend::Swapchain::clearViews(Device &device, uint32_t swapchain_index)
{
  auto prepareViewForShutDown = [&](const ImageViewInfo &view) {
    if (!view.state.isValid())
      return;
    if (view.state.isRTV())
    {
      device.resources.freeTextureRTVDescriptor(view.handle);
    }
    else if (view.state.isDSV())
    {
      device.resources.freeTextureDSVDescriptor(view.handle);
    }
    else if (view.state.isSRV() || view.state.isUAV())
    {
      device.resources.freeTextureSRVDescriptor(view.handle);
    }
    else
    {
      DAG_FATAL("DX12: Found a ViewState which is neither SRV, UAV, RTV or DSV");
    }
  };
  auto &info = swapchains[swapchain_index];
  if (info.colorTarget)
  {
    // caller of this already holds context lock
    prepareViewForShutDown(info.colorTarget->getRecentView());
    for (auto &view : info.colorTarget->getOldViews())
    {
      prepareViewForShutDown(view);
      if (view.state.isSRV() || view.state.isUAV())
        swapchainBufferSRVHeap.free(view.handle);
      else if (view.state.isRTV())
        swapchainBufferRTVHeap.free(view.handle);
    }
    info.colorTarget->removeAllViews();
    info.colorTarget->replaceResource(nullptr);
  }
#if _TARGET_XBOX
  if (info.secondaryColorTarget)
  {
    // caller of this already holds context lock
    prepareViewForShutDown(info.secondaryColorTarget->getRecentView());
    for (auto &view : info.secondaryColorTarget->getOldViews())
      prepareViewForShutDown(view);

    info.secondaryColorTarget->removeAllViews();
    info.secondaryColorTarget->replaceResource(nullptr);
  }
  info.secondaryColorTargets.clear();
  info.secondarySwapchainViewSet.clear();
#endif
  info.colorTargets.clear();
  info.swapchainViewSet.clear();
}

#if _TARGET_PC_WIN
void backend::Swapchain::preRecovery()
{
  for (auto &info : swapchains)
  {
    info.colorTargets.clear();
    info.output.Reset();
    info.swapchain.Reset();
    if (info.colorTarget)
    {
      info.colorTarget->removeAllViews();
      info.colorTarget->replaceResource(nullptr);
    }
    info.swapchainViewSet.clear();
    info.virtualColorTarget = nullptr;
  }
  swapchainBufferSRVHeap.shutdown();
  swapchainBufferRTVHeap.shutdown();
}
#endif

void backend::Swapchain::shutdownSwapchainInfo(uint32_t swapchain_index)
{
  SwapchainInfo &info = swapchains[swapchain_index];
#if _TARGET_XBOX
  info.secondaryColorTargets.clear();
#endif
  info.colorTargets.clear();
#if _TARGET_PC_WIN
  info.output.Reset();
#endif
#if !_TARGET_XBOX
  info.swapchain.Reset();
#endif
#if _TARGET_XBOX
  info.secondaryColorTarget.reset();
  info.secondarySwapchainViewSet.clear();
#endif
  info.colorTarget.reset();
  info.swapchainViewSet.clear();
}

void backend::Swapchain::shutdown()
{
  for (uint32_t index = 0; index < max_swapchain_count; index++)
    shutdownSwapchainInfo(index);
  swapchainBufferSRVHeap.shutdown();
  swapchainBufferRTVHeap.shutdown();
#if _TARGET_PC_WIN
  currentSwapchainIndex = 0;
#endif
}

void backend::Swapchain::updateColorTextureObject(D3DDevice *device, uint32_t swapchain_index)
{
  auto &swapchainInfo = swapchains[swapchain_index];
  auto &colorTargets = swapchainInfo.colorTargets;
  auto &colorTarget = swapchainInfo.colorTarget;
  auto &swapchainViewSet = swapchainInfo.swapchainViewSet;

  auto &currentSwapchainBufferInfo = colorTargets[swapchainInfo.currentColorTargetIndex];
  colorTarget->replaceResource(currentSwapchainBufferInfo.buffer.Get());
  auto source = begin(currentSwapchainBufferInfo.viewTable);
  for (auto &descriptor : swapchainViewSet)
  {
    if (descriptor.state.isSRV() || descriptor.state.isUAV())
    {
      device->CopyDescriptorsSimple(1, descriptor.handle, *source++, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    else if (descriptor.state.isRTV())
    {
      device->CopyDescriptorsSimple(1, descriptor.handle, *source++, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
  }

#if _TARGET_XBOX
  if (!autoGameDvr)
  {
    auto &secondaryColorTarget = swapchainInfo.secondaryColorTarget;
    auto &secondaryColorTargets = swapchainInfo.secondaryColorTargets;
    auto &secondarySwapchainViewSet = swapchainInfo.secondarySwapchainViewSet;

    auto &currentSecondarySwapchainBufferInfo = secondaryColorTargets[swapchainInfo.currentColorTargetIndex];
    secondaryColorTarget->replaceResource(currentSecondarySwapchainBufferInfo.buffer.Get());
    auto secondarySource = begin(currentSecondarySwapchainBufferInfo.viewTable);
    for (auto &descriptor : secondarySwapchainViewSet)
    {
      if (descriptor.state.isRTV())
      {
        device->CopyDescriptorsSimple(1, descriptor.handle, *secondarySource++, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      }
    }
  }
#endif
}

#if _TARGET_PC_WIN

static bool is_hdr_colorspace_supported(const ComPtr<DXGISwapChain> &swapchain)
{
  DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
  UINT colorSpaceSupport = 0;
  return DX12_DEBUG_OK(swapchain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) &&
         ((colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT);
}

void backend::Swapchain::bufferResize(Device &device, const SwapchainProperties &props, uint32_t swapchain_index)
{
  auto &swapchain = swapchains[swapchain_index].swapchain;
  auto &output = swapchains[swapchain_index].output;
  auto &colorTarget = swapchains[swapchain_index].colorTarget;
  auto &colorTargets = swapchains[swapchain_index].colorTargets;

  // have to always resize the buffer, even if size has not changed
  DXGI_SWAP_CHAIN_DESC currentDesc;
  DX12_DEBUG_RESULT(swapchain->GetDesc(&currentDesc));
  logdbg("DX12: bufferResize: ResizeBuffers(%u, %u, %u, %u, 0x%08X)", currentDesc.BufferCount, props.resolution.width,
    props.resolution.height, static_cast<uint32_t>(currentDesc.BufferDesc.Format), static_cast<uint32_t>(currentDesc.Flags));

  bool hasColorSpaceSupport = is_hdr_colorspace_supported(swapchain);
  bool useHdr = props.enableHdr && is_hdr_available(output) && hasColorSpaceSupport || props.forceHdr;
  FormatStore colorFormat = FormatStore::fromCreateFlags(useHdr ? TEXFMT_A2R10G10B10 : TEXFMT_DEFAULT);

  DX12_DEBUG_RESULT(swapchain->ResizeBuffers(currentDesc.BufferCount, props.resolution.width, props.resolution.height,
    colorFormat.asLinearDxGiFormat(), currentDesc.Flags));

  if (hasColorSpaceSupport)
    DX12_DEBUG_RESULT(
      swapchain->SetColorSpace1(useHdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709));

  hdr_changed(useHdr ? get_hdr_caps(output) : eastl::nullopt);

  swapchains[swapchain_index].currentColorTargetIndex = 0;

  colorTargets.resize(currentDesc.BufferCount);

  for (size_t i = 0; i < colorTargets.size(); ++i)
  {
    DX12_DEBUG_RESULT(swapchain->GetBuffer(static_cast<UINT>(i), COM_ARGS(&colorTargets[i].buffer)));
    if (nullptr != colorTargets[i].buffer.Get())
    {
      char stringBuf[MAX_OBJECT_NAME_LENGTH];
      auto ln = sprintf_s(stringBuf, "Swapchain texture#%u", i);
      device.nameResource(colorTargets[i].buffer.Get(), {stringBuf, static_cast<size_t>(ln)});
    }
    else
    {
      // Workaround: swapImage is null for some reason when quitting the game with ALT+F4 in
      // exclusive fullscreen mode
      logdbg("DX12: swapImage for index %d was null in Swapchain::getSwapchainBuffers, we assume we "
             "are quitting the application.",
        i);
    }
  }

  auto idBase = device.getSwapchainColorGlobalId(swapchain_index);
  Extent3D ext{props.resolution.width, props.resolution.height, 1};
  if (!colorTarget)
  {
    colorTarget.reset(new Image({}, ComPtr<ID3D12Resource>{}, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_TEXTURE_LAYOUT_UNKNOWN,
      colorFormat, ext, MipMapCount::make(1), ArrayLayerCount::make(1), idBase, 0));
    colorTarget->setGPUChangeable(true);
  }
  else
  {
    colorTarget->updateExtents(ext);
    colorTarget->updateFormat(colorFormat);
  }

  device.getContext().back.sharedContextState.resourceStates.setTextureState(
    device.getContext().back.sharedContextState.initialResourceStateSet, idBase, 1, D3D12_RESOURCE_STATE_PRESENT);

  updateColorTextureObject(device.device.get(), swapchain_index);

  // Recreate virtual backbuffer if it exists
  auto &swapchainInfo = swapchains[swapchain_index];
  if (swapchainInfo.virtualColorTarget)
  {
    device.getContext().destroyImage(swapchainInfo.virtualColorTarget, true);
    swapchainInfo.virtualColorTarget = device.createVirtualBackbuffer(swapchainInfo.colorTarget.get());
    device.getContext().back.sharedContextState.resourceStates.setTextureState(
      device.getContext().back.sharedContextState.initialResourceStateSet,
      swapchainInfo.virtualColorTarget->getGlobalSubResourceIdBase(), 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
  }
}

#endif