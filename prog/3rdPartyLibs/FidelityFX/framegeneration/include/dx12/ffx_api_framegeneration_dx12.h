// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "../../../api/include/dx12/ffx_api_dx12.h"

#define FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX12 0x00030000u

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12 0x30001u
struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain4** swapchain;        ///< Input swap chain to wrap, output frame interpolation swapchain.
    ID3D12CommandQueue* gameQueue;      ///< Input command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12 0x30005u
struct ffxCreateContextDescFrameGenerationSwapChainNewDX12
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain4** swapchain;        ///< Output frame interpolation swapchain.
    DXGI_SWAP_CHAIN_DESC* desc;         ///< Swap chain creation parameters.
    IDXGIFactory* dxgiFactory;          ///< IDXGIFactory to use for DX12 swapchain creation.
    ID3D12CommandQueue* gameQueue;      ///< Input command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12 0x30006u
struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain4** swapchain;                     ///< Output frame interpolation swapchain.
    HWND hwnd;                                       ///< HWND handle for the calling application;
    DXGI_SWAP_CHAIN_DESC1* desc;                     ///< Swap chain creation parameters.
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc; ///< Fullscreen swap chain creation parameters.
    IDXGIFactory* dxgiFactory;                       ///< IDXGIFactory to use for DX12 swapchain creation.
    ID3D12CommandQueue* gameQueue;                   ///< Input command queue to be used for presentation.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12 0x30002u
struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12
{
    ffxConfigureDescHeader header;
    struct FfxApiResource  uiResource;   ///< Resource containing user interface for composition. May be empty.
    uint32_t               flags;        ///< Zero or combination of values from FfxApiUiCompositionFlags.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12 0x30003u
struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12
{
    ffxQueryDescHeader header;
    void** pOutCommandList;             ///< Output command list (ID3D12GraphicsCommandList) to be used for frame generation dispatch.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12 0x30004u
struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12
{
    ffxQueryDescHeader header;
    struct FfxApiResource *pOutTexture; ///< Output resource in which the frame interpolation result should be placed.
};

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12 0x30007u
struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12
{
    ffxDispatchDescHeader header;
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX12 0x30008u
struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12
{
    ffxConfigureDescHeader  header;
    uint64_t                key;        ///< Configuration key, member of the FfxApiConfigureFrameGenerationSwapChainKeyDX12 enumeration.
    uint64_t                u64;        ///< Integer value or enum value to set.
    void*                   ptr;        ///< Pointer to set or pointer to value to set.
};

//enum value matches enum FfxFrameInterpolationSwapchainConfigureKey
enum FfxApiConfigureFrameGenerationSwapChainKeyDX12
{
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK = 0,                     ///< Sets FfxWaitCallbackFunc
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING = 2,                ///< Sets FfxApiSwapchainFramePacingTuning
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12 0x00030009u
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12
{
    ffxQueryDescHeader header;
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2 0x0003000au
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2
{
    ffxQueryDescHeader header;
    void* device;                            ///< For DX12: pointer to ID3D12Device. App needs to fill out before Query() call.
    struct FfxApiDimensions2D  displaySize;  ///< App needs to fill out before Query() call.
    uint32_t backBufferFormat;               ///< The surface format for the backbuffer. One of the values from FfxApiSurfaceFormat. App needs to fill out before Query() call.
    uint32_t backBufferCount;                ///< The number of backbuffers in the swapchain. App needs to fill out before Query() call.
    struct FfxApiDimensions2D uiResourceSize;///< This is the resolution of the resource that will be used for UI composition. Set to (0,0) if providing null uiResource in  ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t uiResourceFormat;               ///< The surface format for the uiResource. One of the values from FfxApiSurfaceFormat. Set to FFX_API_SURFACE_FORMAT_UNKNOWN(0) if providing null uiResource in  ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t flags;                          ///< Zero or combination of values from FfxApiUiCompositionFlags. App needs to fill out before Query() call.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;
};
