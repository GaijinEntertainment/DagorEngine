#include "device.h"

#include <3d/dag_lowLatency.h>

using namespace drv3d_dx12;

#if _TARGET_XBOX
#include "swapchain_xbox.inc.cpp"
#endif

// ~~~~~~~~~~~~~~ FRONTEND ~~~~~~~~~~~~~~

void frontend::Swapchain::setup(SetupInfo setup)
{
#if _TARGET_PC_WIN
  waitableObject = eastl::move(setup.waitableObject);
#endif
  if (!swapchainColorTex)
  {
    swapchainColorTex.reset(new BaseTex{RES3D_TEX, setup.colorImage->getFormat().asTexFlags() | TEXCF_RTARGET});
  }
  else
  {
    // might be changed after reset and we where unable to create with same format
    swapchainColorTex->cflg = setup.colorImage->getFormat().asTexFlags() | TEXCF_RTARGET;
  }
  auto &extent = setup.colorImage->getBaseExtent();
  swapchainColorTex->setParams(extent.width, extent.height, 1, 1, "swapchain color target");
  swapchainColorTex->tex.image = setup.colorImage;

#if _TARGET_XBOX
  if (setup.secondaryColorImage)
  {
    if (!swapchainSecondaryColorTex)
    {
      swapchainSecondaryColorTex.reset(new BaseTex{RES3D_TEX, setup.secondaryColorImage->getFormat().asTexFlags() | TEXCF_RTARGET});
    }
    else
    {
      // might be changed after reset and we where unable to create with same format
      swapchainSecondaryColorTex->cflg = setup.secondaryColorImage->getFormat().asTexFlags() | TEXCF_RTARGET;
    }
    swapchainSecondaryColorTex->setParams(extent.width, extent.height, 1, 1, "swapchain secondary color target");
    swapchainSecondaryColorTex->tex.image = setup.secondaryColorImage;
  }
#endif

  if (!swapchainDepthStencilTex)
  {
    swapchainDepthStencilTex.reset(new BaseTex{RES3D_TEX, TEXFMT_DEPTH24 | TEXCF_RTARGET});
  }
  swapchainDepthStencilTex->setParams(extent.width, extent.height, 1, 1, "swapchain depth stencil target");

  presentMode = setup.presentMode;
}

void frontend::Swapchain::waitForFrameStart()
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

BaseTex *frontend::Swapchain::getDepthStencilTexture(Device &device, Extent2D ext)
{
  if (swapchainDepthStencilTex->tex.image)
  {
    const auto &depthExt = swapchainDepthStencilTex->tex.image->getBaseExtent();
    if (depthExt.width >= ext.width && depthExt.height >= ext.height)
    {
      return swapchainDepthStencilTex.get();
    }

    // adjust request to max of any
    ext.width = max(ext.width, depthExt.width);
    ext.height = max(ext.height, depthExt.height);

    // enqueue for deletion
    device.resources.destroyTextureOnFrameCompletion(swapchainDepthStencilTex->tex.image);
    swapchainDepthStencilTex->tex.image = nullptr;
  }

  ImageInfo ii;
  ii.type = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  ii.usage = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  ii.size.width = ext.width;
  ii.size.height = ext.height;
  ii.size.depth = 1;
  ii.arrays = ArrayLayerCount::make(1);
  ii.mips = MipMapCount::make(1);
  ii.format = getDepthStencilFormat();
  ii.memoryClass = DeviceMemoryClass::DEVICE_RESIDENT_IMAGE;
  ii.allocateSubresourceIDs = true;
  swapchainDepthStencilTex->tex.image = device.createImageNoContextLock(ii, "swapchain depth stencil target");
  swapchainDepthStencilTex->setParams(ext.width, ext.height, 1, 1, "swapchain depth stencil target");
  return swapchainDepthStencilTex.get();
}

void frontend::Swapchain::prepareForShutdown(Device &device)
{
  if (swapchainDepthStencilTex && swapchainDepthStencilTex->tex.image)
  {
    device.resources.destroyTextureOnFrameCompletion(swapchainDepthStencilTex->tex.image);
    swapchainDepthStencilTex->tex.image = nullptr;
  }
}

// ~~~~~~~~~~~~~~ BACKEND  ~~~~~~~~~~~~~~

void backend::Swapchain::registerSwapchainView(D3DDevice *device, Image *image, ImageViewInfo info)
{
#if !_TARGET_XBOX
  G_UNUSED(image);
#else
  if (image == colorTarget.get())
#endif
  {
    swapchainViewSet.push_back(info);

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
    }
    else
    {
      fatal("DX12: Invalid view type for swapchain");
    }
  }
#if _TARGET_XBOX
  else if (image == secondaryColorTarget.get())
  {
    if (!autoGameDvr)
    {
      if (info.state.isRTV())
      {
        secondarySwapchainViewSet.push_back(info);

        auto desc = info.state.asRTVDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, image->isMultisampled());

        for (auto &buffer : secondaryColorTargets)
        {
          auto descriptor = swapchainBufferRTVHeap.allocate(device);
          device->CreateRenderTargetView(buffer.buffer.Get(), &desc, descriptor);
          buffer.viewTable.push_back(descriptor);
          G_ASSERT(buffer.viewTable.size() == secondarySwapchainViewSet.size());
        }
      }
      else
      {
        fatal("DX12: Invalid view type for secondary swapchain");
      }
    }
  }
  else
  {
    fatal("DX12: Invalid image for swapchainView register");
  }
#endif
}


#if _TARGET_PC_WIN

static bool is_tearing_supported(DXGIFactory *factory)
{
  ComPtr<IDXGIFactory5> factory5;
  HRESULT hr = factory->QueryInterface(COM_ARGS(&factory5));
  BOOL allowTearing = FALSE;

  if (SUCCEEDED(hr))
  {
    hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
  }

  return SUCCEEDED(hr) && allowTearing;
}

bool backend::Swapchain::setup(Device &device, frontend::Swapchain &fe, DXGIFactory *factory, SwapchainInitParameter *queue,
  SwapchainCreateInfo &&sci)
{
  swapchainBufferSRVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  swapchainBufferRTVHeap.init(device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

  presentMode = sci.presentMode;

  frontend::Swapchain::SetupInfo frontSetupInfo;
  Extent2D extent;
  FormatStore colorFormat;

  // create with default size (target size)
  ComPtr<IDXGISwapChain1> swapchainLevel1;
  DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
  colorFormat = FormatStore::fromCreateFlags(TEXFMT_DEFAULT);
  swapchainDesc.Format = colorFormat.asLinearDxGiFormat();
  // fastest mode, allows composer to scribble on it to avoid extra copy
  swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  isTearingSupported = is_tearing_supported(factory);
  swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT |
                        (isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
  swapchainDesc.SampleDesc.Count = 1;
  swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
  swapchainDesc.BufferCount = FRAME_FRAME_BACKLOG_LENGTH;
  swapchainDesc.Width = sci.resolutionX;
  swapchainDesc.Height = sci.resolutionY;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
  fullscreenDesc.Windowed = TRUE;
  if (DX12_CHECK_FAIL(factory->CreateSwapChainForHwnd(queue, sci.window, &swapchainDesc, &fullscreenDesc, nullptr, &swapchainLevel1)))
    return false;

  swapchainLevel1.As(&swapchain);
  if (!swapchain)
  {
    return false;
  }
  output = eastl::move(sci.output);

  DX12_DEBUG_RESULT(factory->MakeWindowAssociation(sci.window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN));

  DX12_DEBUG_RESULT(swapchain->SetMaximumFrameLatency(FRAME_LATENCY));

  // when no output is explicitly set, we grab it from the swapchain
  if (!output)
  {
    DX12_DEBUG_RESULT(swapchain->GetContainingOutput(&output));
  }

  // (output can be different from the one it was created with)
  DX12_DEBUG_RESULT(swapchain->SetFullscreenState(!sci.windowed, sci.windowed ? nullptr : output.Get()));

  swapchain->GetDesc1(&swapchainDesc);

  if (!sci.windowed)
  {
    DX12_DEBUG_RESULT(swapchain->SetFullscreenState(TRUE, output.Get()));

    // "For the flip presentation model, after you transition the display state to full screen, you
    // must call ResizeBuffers to ensure that your call to IDXGISwapChain1::Present1 succeeds."
    // Source:
    // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-setfullscreenstate
    DXGI_SWAP_CHAIN_DESC currentDesc;
    swapchain->GetDesc(&currentDesc);
    swapchain->ResizeBuffers(currentDesc.BufferCount, swapchainDesc.Width, swapchainDesc.Height, currentDesc.BufferDesc.Format,
      currentDesc.Flags);
    swapchain->GetDesc1(&swapchainDesc);
  }

  bool isHdrAvailable = is_hdr_available(output);
  if ((sci.enableHdr && isHdrAvailable) || sci.forceHdr)
  {
    // The CheckColorSpaceSupport with DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 will fail if the
    // backbuffer's format isn't 16 bit.
    colorFormat = FormatStore::fromDXGIFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
    DXGI_SWAP_CHAIN_DESC currentDesc;
    swapchain->GetDesc(&currentDesc);
    currentDesc.BufferDesc.Format = colorFormat.asLinearDxGiFormat();
    swapchain->ResizeBuffers(currentDesc.BufferCount, swapchainDesc.Width, swapchainDesc.Height, currentDesc.BufferDesc.Format,
      currentDesc.Flags);
    swapchain->GetDesc1(&swapchainDesc);

    DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
    UINT colorSpaceSupport = 0;
    if (DX12_DEBUG_OK(swapchain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) &&
        ((colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
      isHdrEnabled = DX12_DEBUG_OK(swapchain->SetColorSpace1(colorSpace));
      change_hdr(true, output);
    }
    else
    {
      logwarn("DX12: 16 bit hdr format isn't supported!");
      colorFormat = FormatStore::fromCreateFlags(TEXFMT_DEFAULT);
    }
  }
  else
  {
    change_hdr();
  }
  extent.width = swapchainDesc.Width;
  extent.height = swapchainDesc.Height;

  frontSetupInfo.waitableObject.reset(swapchain->GetFrameLatencyWaitableObject());

  // keep track to allow correct device reset behavior
  isInExclusiveFullscreenModeEnabled = isInExclusiveFullscreenMode();

  bufferResize(device, extent, colorFormat);

  frontSetupInfo.colorImage = colorTarget.get();
  frontSetupInfo.presentMode = sci.presentMode;
  fe.setup(eastl::move(frontSetupInfo));

  return true;
}


void backend::Swapchain::onFrameBegin(D3DDevice *device)
{
  previousColorTargetIndex = currentColorTargetIndex;
  currentColorTargetIndex = swapchain->GetCurrentBackBufferIndex();
  updateColorTextureObject(device);
}


void backend::Swapchain::swapchainPresentOnPC()
{
  SCOPED_LATENCY_MARKER(lowlatency::get_current_render_frame(), PRESENT_START, PRESENT_END);
  UINT interval = presentMode == PresentationMode::VSYNCED ? 1 : 0;
  // ALLOW_TEARING is required for disable v-sync
  // ALLOW_TEARING requires interval=0 and not exclusive fullscreen
  // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
  UINT flags = ((presentMode != PresentationMode::VSYNCED) && isTearingSupported && !isInExclusiveFullscreenModeEnabled)
                 ? DXGI_PRESENT_ALLOW_TEARING
                 : 0;
  swapchain->Present(interval, flags);
}

bool backend::Swapchain::swapchainPresentFromMainThread(DeviceContext &ctx)
{
  // wait for backend finish the frame
  while (numPresentSentToBackend != numFramesCompletedByBackend)
  {
    ::wait(numFramesCompletedByBackend, numPresentSentToBackend - 1);
  };

  swapchainPresentOnPC();
  ctx.onSwapchainSwapCompleted();
  numFramesPresentedByFrontend++;

  return true;
}


void backend::Swapchain::present(Device &device)
{
  if (device.getContext().hasWorkerThread())
  {
    numFramesCompletedByBackend++;
    notify_one(numFramesCompletedByBackend);
  }
  else
  {
    swapchainPresentOnPC();
  }
}
#endif


#if DX12_HAS_GAMMA_CONTROL
bool backend::Swapchain::setGamma(float power)
{
  if (isHdrEnabled) // Not supported, Present fails on Xbox.
    return false;

#if !_TARGET_XBOX
  // can not set gamma if not exclusive
  BOOL fullscreen;
  // fail is ok here
  if (DX12_DEBUG_FAIL(swapchain->GetFullscreenState(&fullscreen, nullptr)))
    return false;
  if (!fullscreen)
    return true;
#endif

  DXGI_GAMMA_CONTROL_CAPABILITIES gcc = {};
  // fail is ok here
  if (DX12_DEBUG_FAIL(output->GetGammaControlCapabilities(&gcc)))
    return false;

  if (gcc.NumGammaControlPoints <= 1)
  {
    debug("DX12: setGamma: NumGammaControlPoints %u, no effect", gcc.NumGammaControlPoints);
    return true;
  }

  if (power <= 0.f)
    power = 1.f;

  // Must be 0 and 1 on Xbox.
  gammaRamp.Offset.Red = gammaRamp.Offset.Green = gammaRamp.Offset.Blue = 0.f;
  gammaRamp.Scale.Red = gammaRamp.Scale.Green = gammaRamp.Scale.Blue = 1.f;
  eastl::transform(gcc.ControlPointPositions, gcc.ControlPointPositions + gcc.NumGammaControlPoints, gammaRamp.GammaCurve,
    [p = 1 / power](auto v) //
    {
      auto f = powf(v, p);
      return DXGI_RGB{f, f, f};
    });

  if (DX12_DEBUG_FAIL(output->SetGammaControl(&gammaRamp)))
  {
    return false;
  }

  gammaRampApplied = true;
  return true;
}
#endif


void backend::Swapchain::prepareForShutdown(Device &device)
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
      fatal("DX12: Found a ViewState which is neither SRV, UAV, RTV or DSV");
    }
  };
  if (colorTarget)
  {
    // caller of this already holds context lock
    prepareViewForShutDown(colorTarget->getRecentView());
    for (auto &view : colorTarget->getOldViews())
      prepareViewForShutDown(view);
    colorTarget->removeAllViews();
    colorTarget->replaceResource(nullptr);
  }
#if _TARGET_XBOX
  if (secondaryColorTarget)
  {
    // caller of this already holds context lock
    prepareViewForShutDown(secondaryColorTarget->getRecentView());
    for (auto &view : secondaryColorTarget->getOldViews())
      prepareViewForShutDown(view);

    secondaryColorTarget->removeAllViews();
    secondaryColorTarget->replaceResource(nullptr);
  }
  secondaryColorTargets.clear();
  secondarySwapchainViewSet.clear();
#endif

#if _TARGET_XBOX
  // can be null if we had to abort a during device init
  if (device.queues[DeviceQueueType::GRAPHICS].getHandle())
  {
    // have to "null" flip otherwise one swapchain image is still held by the display manager and we crash
    xbox_queue_dummy_present(device.queues[DeviceQueueType::GRAPHICS].getHandle());
  }
#endif
  colorTargets.clear();
  swapchainViewSet.clear();
  swapchainBufferSRVHeap.freeAll();
  swapchainBufferRTVHeap.freeAll();
}

#if _TARGET_PC_WIN
void backend::Swapchain::preRecovery()
{
  colorTargets.clear();
  output.Reset();
  if (swapchain)
    swapchain->SetFullscreenState(FALSE, nullptr);
  swapchain.Reset();
#if DX12_HAS_GAMMA_CONTROL
  gammaRampApplied = false;
#endif
  colorTarget->removeAllViews();
  colorTarget->replaceResource(nullptr);
  swapchainBufferSRVHeap.shutdown();
  swapchainBufferRTVHeap.shutdown();
  swapchainViewSet.clear();
}
#endif

void backend::Swapchain::shutdown()
{
#if _TARGET_XBOX
  secondaryColorTargets.clear();
#endif
  colorTargets.clear();
#if _TARGET_PC_WIN
  output.Reset();
#endif
#if !_TARGET_XBOX
  if (swapchain)
    swapchain->SetFullscreenState(FALSE, nullptr);
  swapchain.Reset();
#endif
#if _TARGET_XBOX
  secondaryColorTarget.reset();
  secondarySwapchainViewSet.clear();
#endif
  colorTarget.reset();
  swapchainBufferSRVHeap.shutdown();
  swapchainBufferRTVHeap.shutdown();
  swapchainViewSet.clear();
}

void backend::Swapchain::updateColorTextureObject(D3DDevice *device)
{
  auto &currentSwapchainBufferInfo = colorTargets[currentColorTargetIndex];
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
    auto &currentSecondarySwapchainBufferInfo = secondaryColorTargets[currentColorTargetIndex];
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

void backend::Swapchain::bufferResize(Device &device, const Extent2D &extent, FormatStore color_format)
{
  // have to always resize the buffer, even if size has not changed
  DXGI_SWAP_CHAIN_DESC currentDesc;
  swapchain->GetDesc(&currentDesc);
  debug("DX12: ResizeBuffers(%u, %u, %u, %u, 0x%08X)", currentDesc.BufferCount, extent.width, extent.height,
    static_cast<uint32_t>(currentDesc.BufferDesc.Format), static_cast<uint32_t>(currentDesc.Flags));
  swapchain->ResizeBuffers(currentDesc.BufferCount, extent.width, extent.height, currentDesc.BufferDesc.Format, currentDesc.Flags);

  if (isInExclusiveFullscreenMode())
  {
    swapchain->GetDesc(&currentDesc);
    swapchain->ResizeTarget(&currentDesc.BufferDesc);
  }

  previousColorTargetIndex = currentDesc.BufferCount - 1;
  currentColorTargetIndex = 0;

  colorTargets.resize(currentDesc.BufferCount);

  for (size_t i = 0; i < colorTargets.size(); ++i)
  {
    swapchain->GetBuffer(static_cast<UINT>(i), COM_ARGS(&colorTargets[i].buffer));
#if DX12_DOES_SET_DEBUG_NAMES
    if (colorTargets[i].buffer.Get() != nullptr)
    {
      wchar_t stringBuf[MAX_OBJECT_NAME_LENGTH];
      swprintf_s(stringBuf, L"Swapchain texture#%u", i);
      DX12_SET_DEBUG_OBJ_NAME(colorTargets[i].buffer, stringBuf);
    }
#endif
    if (colorTargets[i].buffer.Get() == nullptr)
    {
      // Workaround: swapImage is null for some reason when quitting the game with ALT+F4 in
      // exclusive fullscreen mode
      debug("DX12: swapImage for index %d was null in Swapchain::getSwapchainBuffers, we assume we "
            "are quitting the application.",
        i);
    }
  }

  auto idBase = device.getSwapchainColorGlobalId();
  Extent3D ext{extent.width, extent.height, 1};
  if (!colorTarget)
  {
    colorTarget.reset(new Image({}, ComPtr<ID3D12Resource>{}, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_TEXTURE_LAYOUT_UNKNOWN,
      color_format, ext, MipMapCount::make(1), ArrayLayerCount::make(1), idBase, false));
    colorTarget->setGPUChangeable(true);
  }
  else
  {
    colorTarget->updateExtents(ext);
    colorTarget->updateFormat(color_format);
  }

  device.getContext().back.sharedContextState.resourceStates.setTextureState(
    device.getContext().back.sharedContextState.initialResourceStateSet, idBase, 1, D3D12_RESOURCE_STATE_PRESENT);

  updateColorTextureObject(device.device.get());
}


bool backend::Swapchain::isInExclusiveFullscreenMode()
{
  BOOL isExclusive = TRUE;
  if (swapchain)
  {
    swapchain->GetFullscreenState(&isExclusive, nullptr);
  }
  else
  {
    isExclusive = isInExclusiveFullscreenModeEnabled;
  }
  return FALSE != isExclusive;
}


void backend::Swapchain::changeFullscreenExclusiveMode(bool is_exclusive)
{
  DX12_DEBUG_RESULT(swapchain->SetFullscreenState(is_exclusive, is_exclusive ? output.Get() : nullptr));
  debug("DX12: Swapchain full screen exclusive mode changed. is_exclusive: %s", is_exclusive ? "true" : "false");
  isInExclusiveFullscreenModeEnabled = isInExclusiveFullscreenMode();
}

void backend::Swapchain::changeFullscreenExclusiveMode(bool is_exclusive, ComPtr<IDXGIOutput> in_output)
{
  eastl::swap(output, in_output);
  changeFullscreenExclusiveMode(is_exclusive);
  // when no output is explicitly set, we grab it from the swapchain
  if (!output)
  {
    DX12_DEBUG_RESULT(swapchain->GetContainingOutput(&output));
  }
  isInExclusiveFullscreenModeEnabled = isInExclusiveFullscreenMode();
}
#endif
