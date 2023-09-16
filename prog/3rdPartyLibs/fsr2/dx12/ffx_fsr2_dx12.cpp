// This file is part of the FidelityFX SDK.
//
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

#include <codecvt>  // convert string to wstring
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include "d3dx12.h"
#include "../ffx_fsr2.h"
#include "ffx_fsr2_dx12.h"
#include "shaders/ffx_fsr2_shaders_dx12.h"  // include all the precompiled D3D12 shaders for the FSR2 passes
#include "../ffx_fsr2_private.h"

// DX12 prototypes for functions in the backend interface
FfxErrorCode GetDeviceCapabilitiesDX12(FfxFsr2Interface* backendInterface, FfxDeviceCapabilities* deviceCapabilities, FfxDevice device);
FfxErrorCode CreateBackendContextDX12(FfxFsr2Interface* backendInterface, FfxDevice device);
FfxErrorCode DestroyBackendContextDX12(FfxFsr2Interface* backendInterface);
FfxErrorCode CreateResourceDX12(FfxFsr2Interface* backendInterface, const FfxCreateResourceDescription* desc, FfxResourceInternal* outTexture);
FfxErrorCode RegisterResourceDX12(FfxFsr2Interface* backendInterface, const FfxResource* inResource, FfxResourceInternal* outResourceInternal);
FfxErrorCode UnregisterResourcesDX12(FfxFsr2Interface* backendInterface);
FfxResourceDescription GetResourceDescriptorDX12(FfxFsr2Interface* backendInterface, FfxResourceInternal resource);
FfxErrorCode DestroyResourceDX12(FfxFsr2Interface* backendInterface, FfxResourceInternal resource);
FfxErrorCode CreatePipelineDX12(FfxFsr2Interface* backendInterface, FfxFsr2Pass passId, const FfxPipelineDescription*  desc, FfxPipelineState* outPass);
FfxErrorCode DestroyPipelineDX12(FfxFsr2Interface* backendInterface, FfxPipelineState* pipeline);
FfxErrorCode ScheduleGpuJobDX12(FfxFsr2Interface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode ExecuteGpuJobsDX12(FfxFsr2Interface* backendInterface, FfxCommandList commandList);

#define FSR2_MAX_QUEUED_FRAMES  ( 4)
#define FSR2_MAX_RESOURCE_COUNT (64)
#define FSR2_DESC_RING_SIZE     (FSR2_MAX_QUEUED_FRAMES * FFX_FSR2_PASS_COUNT * FSR2_MAX_RESOURCE_COUNT)
#define FSR2_MAX_BARRIERS       (16)
#define FSR2_MAX_GPU_JOBS       (32)
#define FSR2_MAX_SAMPLERS       ( 2)
#define UPLOAD_JOB_COUNT        (16)

typedef struct BackendContext_DX12 {
    
    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        wchar_t                 resourceName[64] = {};
#endif
        ID3D12Resource*         resourcePtr;
        FfxResourceDescription  resourceDescription;
        FfxResourceStates       state;
        uint32_t                srvDescIndex;
        uint32_t                uavDescIndex;
        uint32_t                uavDescCount;
    } Resource;

    ID3D12Device*           device = nullptr;

    FfxGpuJobDescription    gpuJobs[FSR2_MAX_GPU_JOBS] = {};
    uint32_t                gpuJobCount;

    uint32_t                nextStaticResource;
    uint32_t                nextDynamicResource;
    Resource                resources[FSR2_MAX_RESOURCE_COUNT];
    ID3D12DescriptorHeap*   descHeapSrvCpu;

    uint32_t                nextStaticUavDescriptor;
    uint32_t                nextDynamicUavDescriptor; 
    ID3D12DescriptorHeap*   descHeapUavCpu;
    ID3D12DescriptorHeap*   descHeapUavGpu;

    uint32_t                descRingBufferSize;
    uint32_t                descRingBufferBase;
    ID3D12DescriptorHeap*   descRingBuffer;

    D3D12_RESOURCE_BARRIER  barriers[FSR2_MAX_BARRIERS];
    uint32_t                barrierCount;
} BackendContext_DX12;

FFX_API size_t ffxFsr2GetScratchMemorySizeDX12()
{
    return FFX_ALIGN_UP(sizeof(BackendContext_DX12), sizeof(uint64_t));
}

// populate interface with DX12 pointers.
FfxErrorCode ffxFsr2GetInterfaceDX12(
    FfxFsr2Interface* outInterface,
    ID3D12Device* device,
    void* scratchBuffer,
    size_t scratchBufferSize) {

    FFX_RETURN_ON_ERROR(
        outInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxFsr2GetScratchMemorySizeDX12(),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    outInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesDX12;
    outInterface->fpCreateBackendContext = CreateBackendContextDX12;
    outInterface->fpDestroyBackendContext = DestroyBackendContextDX12;
    outInterface->fpCreateResource = CreateResourceDX12;
    outInterface->fpRegisterResource = RegisterResourceDX12;
    outInterface->fpUnregisterResources = UnregisterResourcesDX12;
    outInterface->fpGetResourceDescription = GetResourceDescriptorDX12;
    outInterface->fpDestroyResource = DestroyResourceDX12;
    outInterface->fpCreatePipeline = CreatePipelineDX12;
    outInterface->fpDestroyPipeline = DestroyPipelineDX12;
    outInterface->fpScheduleGpuJob = ScheduleGpuJobDX12;
    outInterface->fpExecuteGpuJobs = ExecuteGpuJobsDX12;
    outInterface->scratchBuffer = scratchBuffer;
    outInterface->scratchBufferSize = scratchBufferSize;

    return FFX_OK;
}

void TIF(HRESULT result)
{
    if (FAILED(result)) {

        wchar_t errorMessage[256];
        memset(errorMessage, 0, 256);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, 255, NULL);
        char errA[256];
        size_t returnSize;
        wcstombs_s(&returnSize, errA, 255, errorMessage, 255);
#ifdef _DEBUG
        int32_t msgboxID = MessageBoxW(NULL, errorMessage, L"Error", MB_OK);
#endif
        // throw 1;
    }
}

// fix up format in case resource passed to FSR2 was created as typeless
static DXGI_FORMAT convertFormat(DXGI_FORMAT format)
{
    switch (format) {
        // Handle Depth
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        // Handle color: assume FLOAT for 16 and 32 bit channels, else UNORM
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:
            return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:
            return DXGI_FORMAT_R8_UNORM;
        default:
            return format;
    }
}

ID3D12Resource* getDX12ResourcePtr(BackendContext_DX12* backendContext, int32_t resourceIndex)
{
    FFX_ASSERT(NULL != backendContext);
    return reinterpret_cast<ID3D12Resource*>(backendContext->resources[resourceIndex].resourcePtr);
}

// Create a FfxFsr2Device from a ID3D12Device*
FfxDevice ffxGetDeviceDX12(ID3D12Device* dx12Device)
{
    FFX_ASSERT(NULL != dx12Device);
    return reinterpret_cast<FfxDevice>(dx12Device);
}

FfxCommandList ffxGetCommandListDX12(ID3D12CommandList* cmdList)
{
    FFX_ASSERT(NULL != cmdList);
    return reinterpret_cast<FfxCommandList>(cmdList);
}

D3D12_RESOURCE_STATES ffxGetDX12StateFromResourceState(FfxResourceStates state)
{
    switch (state) {

        case(FFX_RESOURCE_STATE_GENERIC_READ):
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case(FFX_RESOURCE_STATE_COMPUTE_READ):
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case FFX_RESOURCE_STATE_COPY_SRC:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case FFX_RESOURCE_STATE_COPY_DEST:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

DXGI_FORMAT ffxGetDX12FormatFromSurfaceFormat(FfxSurfaceFormat surfaceFormat)
{
    switch (surfaceFormat) {

        case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16B16A16_UNORM):
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case(FFX_SURFACE_FORMAT_R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case(FFX_SURFACE_FORMAT_R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case(FFX_SURFACE_FORMAT_R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case(FFX_SURFACE_FORMAT_R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case(FFX_SURFACE_FORMAT_R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case(FFX_SURFACE_FORMAT_R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case(FFX_SURFACE_FORMAT_R8_UINT):
            return DXGI_FORMAT_R8_UINT;
        case(FFX_SURFACE_FORMAT_R8G8_UNORM):
            return DXGI_FORMAT_R8G8_UNORM;
        case(FFX_SURFACE_FORMAT_R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_RESOURCE_FLAGS ffxGetDX12ResourceFlags(FfxResourceUsage flags)
{
    D3D12_RESOURCE_FLAGS dx12ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET) dx12ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (flags & FFX_RESOURCE_USAGE_UAV) dx12ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return dx12ResourceFlags;
}

FfxSurfaceFormat ffxGetSurfaceFormatDX12(DXGI_FORMAT format)
{
    switch (format) {

        case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
            return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
        case(DXGI_FORMAT_R32G32B32A32_FLOAT):
            return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
        case(DXGI_FORMAT_R16G16B16A16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        case(DXGI_FORMAT_R16G16B16A16_UNORM):
            return FFX_SURFACE_FORMAT_R16G16B16A16_UNORM;
        case(DXGI_FORMAT_R32G32_FLOAT):
            return FFX_SURFACE_FORMAT_R32G32_FLOAT;
        case(DXGI_FORMAT_R32_UINT):
            return FFX_SURFACE_FORMAT_R32_UINT;
        case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
            return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
        case(DXGI_FORMAT_R8G8B8A8_UNORM):
            return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case(DXGI_FORMAT_R11G11B10_FLOAT):
            return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        case(DXGI_FORMAT_R16G16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16_FLOAT;
        case(DXGI_FORMAT_R16G16_UINT):
            return FFX_SURFACE_FORMAT_R16G16_UINT;
        case(DXGI_FORMAT_R16_FLOAT):
            return FFX_SURFACE_FORMAT_R16_FLOAT;
        case(DXGI_FORMAT_R16_UINT):
            return FFX_SURFACE_FORMAT_R16_UINT;
        case(DXGI_FORMAT_R16_UNORM):
            return FFX_SURFACE_FORMAT_R16_UNORM;
        case(DXGI_FORMAT_R16_SNORM):
            return FFX_SURFACE_FORMAT_R16_SNORM;
        case(DXGI_FORMAT_R8_UNORM):
            return FFX_SURFACE_FORMAT_R8_UNORM;
        case(DXGI_FORMAT_R8_UINT):
            return FFX_SURFACE_FORMAT_R8_UINT;
        default:
            return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

// register a DX12 resource to the backend
FfxResource ffxGetResourceDX12(FfxFsr2Context* context, ID3D12Resource* dx12Resource, const wchar_t* name, FfxResourceStates state, UINT shaderComponentMapping)
{
    FfxResource resource = {};
    resource.resource = reinterpret_cast<void*>(dx12Resource);
    resource.state = state;
    resource.descriptorData = (uint64_t)shaderComponentMapping;

    if (dx12Resource) {
        resource.description.flags = FFX_RESOURCE_FLAGS_NONE;
        resource.description.width = (uint32_t)dx12Resource->GetDesc().Width;
        resource.description.height = dx12Resource->GetDesc().Height;
        resource.description.depth = dx12Resource->GetDesc().DepthOrArraySize;
        resource.description.mipCount = dx12Resource->GetDesc().MipLevels;
        resource.description.format = ffxGetSurfaceFormatDX12(dx12Resource->GetDesc().Format);

        switch (dx12Resource->GetDesc().Dimension) {

        case D3D12_RESOURCE_DIMENSION_BUFFER:
            resource.description.type = FFX_RESOURCE_TYPE_BUFFER;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            break;
        default:
            break;
        }
    }
#ifdef _DEBUG
    if (name) {
        wcscpy_s(resource.name, name);
    }
#endif

    return resource;
}

ID3D12Resource* ffxGetDX12ResourcePtr(FfxFsr2Context* context, uint32_t resId)
{
    FfxFsr2Context_Private* contextPrivate = (FfxFsr2Context_Private*)(context);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)(contextPrivate->contextDescription.callbacks.scratchBuffer);

    if (resId > FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK)
        return backendContext->resources[contextPrivate->uavResources[resId].internalIndex].resourcePtr;
    else // Input resources are present only in srvResources array
        return backendContext->resources[contextPrivate->srvResources[resId].internalIndex].resourcePtr;
}

FfxErrorCode RegisterResourceDX12(
    FfxFsr2Interface* backendInterface,
    const FfxResource* inFfxResource,
    FfxResourceInternal* outFfxResourceInternal
)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)(backendInterface->scratchBuffer);
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource* dx12Resource = reinterpret_cast<ID3D12Resource*>(inFfxResource->resource);

    FfxResourceStates state = inFfxResource->state;
    uint32_t shaderComponentMapping = (uint32_t)inFfxResource->descriptorData;

    if (dx12Resource == nullptr) {

        outFfxResourceInternal->internalIndex = FFX_FSR2_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    FFX_ASSERT(backendContext->nextDynamicResource > backendContext->nextStaticResource);
    outFfxResourceInternal->internalIndex = backendContext->nextDynamicResource--;
    
    BackendContext_DX12::Resource* backendResource = &backendContext->resources[outFfxResourceInternal->internalIndex];
    backendResource->resourcePtr = dx12Resource;
    backendResource->state = state;
#ifdef _DEBUG
    const wchar_t* name = inFfxResource->name;
    if (name) {
        wcscpy_s(backendResource->resourceName, name);
    }
#endif

    // create resource views
    if (dx12Resource) {

        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
        dx12UavDescription.Format = convertFormat(dx12Resource->GetDesc().Format);
        dx12SrvDescription.Shader4ComponentMapping = shaderComponentMapping;
        dx12SrvDescription.Format = convertFormat(dx12Resource->GetDesc().Format);

        switch (dx12Resource->GetDesc().Dimension) {

            case D3D12_RESOURCE_DIMENSION_BUFFER:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
                backendResource->resourceDescription.width = uint32_t(dx12Resource->GetDesc().Width);
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                dx12SrvDescription.Texture1D.MipLevels = dx12Resource->GetDesc().MipLevels;
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
                backendResource->resourceDescription.format = ffxGetSurfaceFormatDX12(dx12Resource->GetDesc().Format);
                backendResource->resourceDescription.width = uint32_t(dx12Resource->GetDesc().Width);
                backendResource->resourceDescription.mipCount = uint32_t(dx12Resource->GetDesc().MipLevels);
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                dx12SrvDescription.Texture2D.MipLevels = dx12Resource->GetDesc().MipLevels;
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                backendResource->resourceDescription.format = ffxGetSurfaceFormatDX12(dx12Resource->GetDesc().Format);
                backendResource->resourceDescription.width = uint32_t(dx12Resource->GetDesc().Width);
                backendResource->resourceDescription.height = uint32_t(dx12Resource->GetDesc().Height);
                backendResource->resourceDescription.mipCount = uint32_t(dx12Resource->GetDesc().MipLevels);
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                dx12SrvDescription.Texture3D.MipLevels = dx12Resource->GetDesc().MipLevels;
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
                backendResource->resourceDescription.format = ffxGetSurfaceFormatDX12(dx12Resource->GetDesc().Format);
                backendResource->resourceDescription.width = uint32_t(dx12Resource->GetDesc().Width);
                backendResource->resourceDescription.height = uint32_t(dx12Resource->GetDesc().Height);
                backendResource->resourceDescription.depth = uint32_t(dx12Resource->GetDesc().DepthOrArraySize);
                backendResource->resourceDescription.mipCount = uint32_t(dx12Resource->GetDesc().MipLevels);
                break;
            
            default:
                break;
        }

        // set up resorce view descriptors
        if (dx12Resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {

            // CPU readable
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
            cpuHandle.ptr += outFfxResourceInternal->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, cpuHandle);

            // UAV
            if (dx12Resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                const int32_t uavDescriptorsCount = (dx12Resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? dx12Resource->GetDesc().MipLevels : 1;
                FFX_ASSERT(backendContext->nextDynamicUavDescriptor - uavDescriptorsCount + 1 > backendContext->nextStaticResource);

                backendResource->uavDescCount = uavDescriptorsCount;
                backendResource->uavDescIndex = backendContext->nextDynamicUavDescriptor - uavDescriptorsCount + 1;

                for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorsCount; ++currentMipIndex) {

                    dx12UavDescription.Texture2D.MipSlice = currentMipIndex;

                    cpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                    cpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, cpuHandle);

                    cpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                    cpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, cpuHandle);
                }

                backendContext->nextDynamicUavDescriptor -= uavDescriptorsCount;
            }
        }
    }

    return FFX_OK;
}

// dispose dynamic resources: This should be called at the end of the frame
FfxErrorCode UnregisterResourcesDX12(FfxFsr2Interface* backendInterface)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)(backendInterface->scratchBuffer);

    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);

    backendContext->nextDynamicResource = FSR2_MAX_RESOURCE_COUNT - 1;
    backendContext->nextDynamicUavDescriptor = FSR2_MAX_RESOURCE_COUNT - 1;

    return FFX_OK;
}

// query device capabilities to select the optimal shader permutation
FfxErrorCode GetDeviceCapabilitiesDX12(FfxFsr2Interface* backendInterface, FfxDeviceCapabilities* deviceCapabilities, FfxDevice device)
{
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(device);

    FFX_UNUSED(backendInterface);
    FFX_ASSERT(NULL != deviceCapabilities);
    FFX_ASSERT(NULL != dx12Device);
    
    // check if we have shader model 6.6
    bool haveShaderModel66 = true;
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)))) {

        switch (shaderModel.HighestShaderModel) {

            case D3D_SHADER_MODEL_5_1:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
                break;

            case D3D_SHADER_MODEL_6_0:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
                break;

            case D3D_SHADER_MODEL_6_1:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_1;
                break;

            case D3D_SHADER_MODEL_6_2:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_2;
                break;

            case D3D_SHADER_MODEL_6_3:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_3;
                break;

            case D3D_SHADER_MODEL_6_4:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_4;
                break;

            case D3D_SHADER_MODEL_6_5:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_5;
                break;

            case D3D_SHADER_MODEL_6_6:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
                break;

            default:
                deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
                break;
        }
    } else {

        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
    }

    // check if we can force wave64 mode.
    D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12Options1 = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12Options1, sizeof(d3d12Options1)))) {

        const uint32_t waveLaneCountMin = d3d12Options1.WaveLaneCountMin;
        const uint32_t waveLaneCountMax = d3d12Options1.WaveLaneCountMax;
        deviceCapabilities->waveLaneCountMin = waveLaneCountMin;
        deviceCapabilities->waveLaneCountMax = waveLaneCountMax;
    }

    // check if we have 16bit floating point.
    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options)))) {

        deviceCapabilities->fp16Supported = !!(d3d12Options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
    }

    // check if we have raytracing support
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12Options5 = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12Options5, sizeof(d3d12Options5)))) {

        deviceCapabilities->raytracingSupported = (d3d12Options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }

    return FFX_OK;
}

// initialize the DX12 backend
FfxErrorCode CreateBackendContextDX12(FfxFsr2Interface* backendInterface, FfxDevice device)
{
    HRESULT result = S_OK;
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(device);

    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != dx12Device);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    memset(backendContext, 0, sizeof(*backendContext));

    if (dx12Device != NULL) {

        dx12Device->AddRef();
        backendContext->device = dx12Device;
    }
    
    // init resource linked list
    backendContext->nextStaticResource = 1;
    backendContext->nextDynamicResource = FSR2_MAX_RESOURCE_COUNT - 1;
    backendContext->nextStaticUavDescriptor = 0;
    backendContext->nextDynamicUavDescriptor = FSR2_MAX_RESOURCE_COUNT - 1;

    backendContext->resources[0] = {};

    // CPUVisible
    D3D12_DESCRIPTOR_HEAP_DESC descHeap;
    descHeap.NumDescriptors = FSR2_MAX_RESOURCE_COUNT;
    descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descHeap.NodeMask = 0;

    result = dx12Device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&backendContext->descHeapSrvCpu));
    result = dx12Device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&backendContext->descHeapUavCpu));

    // GPU
    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    result = dx12Device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&backendContext->descHeapUavGpu));

    // descriptor ring buffer
    descHeap.NumDescriptors = FSR2_DESC_RING_SIZE;
    backendContext->descRingBufferSize = descHeap.NumDescriptors;
    backendContext->descRingBufferBase = 0;
    result = dx12Device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&backendContext->descRingBuffer));

    return FFX_OK;
}

// deinitialize the DX12 backend
FfxErrorCode DestroyBackendContextDX12(FfxFsr2Interface* backendInterface)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    backendContext->descHeapSrvCpu->Release();
    backendContext->descHeapUavCpu->Release();
    backendContext->descHeapUavGpu->Release();
    backendContext->descRingBuffer->Release();

    for (uint32_t currentStaticResourceIndex = 0; currentStaticResourceIndex < backendContext->nextStaticResource; ++currentStaticResourceIndex) {

        if (backendContext->resources[currentStaticResourceIndex].resourcePtr) {

            backendContext->resources[currentStaticResourceIndex].resourcePtr->Release();
            backendContext->resources[currentStaticResourceIndex].resourcePtr = nullptr;
        }
    }

    backendContext->nextStaticResource = 0;

    if (backendContext->device != NULL) {

        backendContext->device->Release();
        backendContext->device = NULL;
    }

    return FFX_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxErrorCode CreateResourceDX12(
    FfxFsr2Interface* backendInterface,
    const FfxCreateResourceDescription* createResourceDescription,
    FfxResourceInternal* outTexture
)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != createResourceDescription);
    FFX_ASSERT(NULL != outTexture);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    ID3D12Device* dx12Device = backendContext->device;

    FFX_ASSERT(NULL != dx12Device);

    D3D12_HEAP_PROPERTIES dx12HeapProperties = {};
    dx12HeapProperties.Type = (createResourceDescription->heapType == FFX_HEAP_TYPE_DEFAULT) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC dx12ResourceDescription = {};
    dx12ResourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    dx12ResourceDescription.Width = 1;
    dx12ResourceDescription.Height = 1;
    dx12ResourceDescription.MipLevels = 1;
    dx12ResourceDescription.DepthOrArraySize = 1;
    dx12ResourceDescription.SampleDesc.Count = 1;
    dx12ResourceDescription.Flags = ffxGetDX12ResourceFlags(createResourceDescription->usage);
    
    FFX_ASSERT(backendContext->nextStaticResource + 1 < backendContext->nextDynamicResource);

    outTexture->internalIndex = backendContext->nextStaticResource++;
    BackendContext_DX12::Resource* backendResource = &backendContext->resources[outTexture->internalIndex];
    backendResource->resourceDescription = createResourceDescription->resourceDescription;

    switch (createResourceDescription->resourceDescription.type) {

        case FFX_RESOURCE_TYPE_BUFFER:
            dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
            dx12ResourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            break;

        case FFX_RESOURCE_TYPE_TEXTURE1D:
            dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
            dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
            dx12ResourceDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
            break;

        case FFX_RESOURCE_TYPE_TEXTURE2D:
            dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
            dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
            dx12ResourceDescription.Height = createResourceDescription->resourceDescription.height;
            dx12ResourceDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
            break;

        case FFX_RESOURCE_TYPE_TEXTURE3D:
            dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
            dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
            dx12ResourceDescription.Height = createResourceDescription->resourceDescription.height;
            dx12ResourceDescription.DepthOrArraySize = createResourceDescription->resourceDescription.depth;
            dx12ResourceDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
            break;

        default:
            break;
    }

    ID3D12Resource* dx12Resource = nullptr;
    if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD) {

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT dx12Footprint = {};

        UINT rowCount;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;

        dx12Device->GetCopyableFootprints(&dx12ResourceDescription, 0, 1, 0, &dx12Footprint, &rowCount, &rowSizeInBytes, &totalBytes);

        D3D12_HEAP_PROPERTIES dx12UploadHeapProperties = {};
        dx12UploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC dx12UploadBufferDescription = {};

        dx12UploadBufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dx12UploadBufferDescription.Width = totalBytes;
        dx12UploadBufferDescription.Height = 1;
        dx12UploadBufferDescription.DepthOrArraySize = 1;
        dx12UploadBufferDescription.MipLevels = 1;
        dx12UploadBufferDescription.Format = DXGI_FORMAT_UNKNOWN;
        dx12UploadBufferDescription.SampleDesc.Count = 1;
        dx12UploadBufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        TIF(dx12Device->CreateCommittedResource(&dx12HeapProperties, D3D12_HEAP_FLAG_NONE, &dx12UploadBufferDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Resource)));
        backendResource->state = FFX_RESOURCE_STATE_GENERIC_READ;

        D3D12_RANGE dx12EmptyRange = {};
        void* uploadBufferData = nullptr;
        TIF(dx12Resource->Map(0, &dx12EmptyRange, &uploadBufferData));

        const uint8_t* src = static_cast<uint8_t*>(createResourceDescription->initData);
        uint8_t* dst = static_cast<uint8_t*>(uploadBufferData);
        for (uint32_t currentRowIndex = 0; currentRowIndex < createResourceDescription->resourceDescription.height; ++currentRowIndex) {

            memcpy(dst, src, rowSizeInBytes);
            src += rowSizeInBytes;
            dst += dx12Footprint.Footprint.RowPitch;
        }

        dx12Resource->Unmap(0, nullptr);
        dx12Resource->SetName(createResourceDescription->name);
        backendResource->resourcePtr = dx12Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif
        return FFX_OK;

    } else {

        const FfxResourceStates resourceStates = (createResourceDescription->initData && (createResourceDescription->heapType != FFX_HEAP_TYPE_UPLOAD)) ? FFX_RESOURCE_STATE_COPY_DEST : createResourceDescription->initalState;
        const D3D12_RESOURCE_STATES dx12ResourceStates = ffxGetDX12StateFromResourceState(resourceStates);

        TIF(dx12Device->CreateCommittedResource(&dx12HeapProperties, D3D12_HEAP_FLAG_NONE, &dx12ResourceDescription, dx12ResourceStates, nullptr, IID_PPV_ARGS(&dx12Resource)));
        backendResource->state = resourceStates;

        dx12Resource->SetName(createResourceDescription->name);
        backendResource->resourcePtr = dx12Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif

        // Create SRVs and UAVs
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};
            D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
            dx12UavDescription.Format = convertFormat(dx12Resource->GetDesc().Format);
            dx12SrvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            dx12SrvDescription.Format = convertFormat(dx12Resource->GetDesc().Format);

            switch (dx12Resource->GetDesc().Dimension) {

                case D3D12_RESOURCE_DIMENSION_BUFFER:
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    break;

                case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    dx12SrvDescription.Texture1D.MipLevels = dx12Resource->GetDesc().MipLevels;
                    break;

                case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    dx12SrvDescription.Texture2D.MipLevels = dx12Resource->GetDesc().MipLevels;
                    break;

                case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    dx12SrvDescription.Texture3D.MipLevels = dx12Resource->GetDesc().MipLevels;
                    break;

                default:
                    break;
            }

            if (dx12Resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {

                // CPU readable
                D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += outTexture->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, dx12CpuHandle);

                // UAV
                if (dx12Resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                    const int32_t uavDescriptorCount = (dx12Resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? dx12Resource->GetDesc().MipLevels : 1;
                    FFX_ASSERT(backendContext->nextStaticUavDescriptor + uavDescriptorCount < backendContext->nextDynamicResource);

                    backendResource->uavDescCount = uavDescriptorCount;
                    backendResource->uavDescIndex = backendContext->nextStaticUavDescriptor;

                    for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorCount; ++currentMipIndex) {

                        dx12UavDescription.Texture2D.MipSlice = currentMipIndex;

                        dx12CpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                        dx12CpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);

                        dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                        dx12CpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);
                    }

                    backendContext->nextStaticUavDescriptor += uavDescriptorCount;
                }
            }
        }

        // create upload resource and upload job
        if (createResourceDescription->initData) {

            FfxResourceInternal copySrc;
            FfxCreateResourceDescription uploadDescription = { *createResourceDescription };
            uploadDescription.heapType = FFX_HEAP_TYPE_UPLOAD;
            uploadDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;
            uploadDescription.initalState = FFX_RESOURCE_STATE_GENERIC_READ;

            backendInterface->fpCreateResource(backendInterface, &uploadDescription, &copySrc);

            // setup the upload job
            FfxGpuJobDescription copyJob = {

                FFX_GPU_JOB_COPY
            };
            copyJob.copyJobDescriptor.src = copySrc;
            copyJob.copyJobDescriptor.dst = *outTexture;

            backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
        }
    }

    return FFX_OK;
}

FfxResourceDescription GetResourceDescriptorDX12(
    FfxFsr2Interface* backendInterface, 
    FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->resources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

FfxErrorCode CreatePipelineDX12(
    FfxFsr2Interface* backendInterface,
    FfxFsr2Pass pass,
    const FfxPipelineDescription* pipelineDescription,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    ID3D12Device* dx12Device = backendContext->device;

    // check if we have shader model 6.6
    bool haveShaderModel66 = true;
    D3D12_FEATURE_DATA_SHADER_MODEL dx12ShaderModel = { D3D_SHADER_MODEL_6_6 };

    HRESULT result = dx12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &dx12ShaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL));

    if (SUCCEEDED(result))
        haveShaderModel66 = dx12ShaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6;
    else
        haveShaderModel66 = false;

    // check if we can force wave64 mode.
    D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12Options1 = {};
    bool canForceWave64 = false;
    bool useLut = false;
    result = dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12Options1, sizeof(d3d12Options1));
    if (SUCCEEDED(result)) {

        const uint32_t waveLaneCountMin = d3d12Options1.WaveLaneCountMin;
        const uint32_t waveLaneCountMax = d3d12Options1.WaveLaneCountMax;

        if (waveLaneCountMin == 32 && waveLaneCountMax == 64) {

            useLut = true;
            canForceWave64 = haveShaderModel66;
        }
        else {

            canForceWave64 = false;
        }
    }

    // check if we have 16bit floating point.
    bool supportedFP16 = false;
    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {};
    result = dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options));
    if (SUCCEEDED(result)) {

        supportedFP16 = !!(d3d12Options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
    }

    // work out what permutation to load.
    uint32_t flags = 0;
    flags |= (pipelineDescription->contextFlags & FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE) ? FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT : 0;
    flags |= (pipelineDescription->contextFlags & FFX_FSR2_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS) ? 0 : FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS;
    flags |= (pipelineDescription->contextFlags & FFX_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) ? FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS : 0;
    flags |= (pipelineDescription->contextFlags & FFX_FSR2_ENABLE_DEPTH_INVERTED) ? FSR2_SHADER_PERMUTATION_DEPTH_INVERTED : 0;
    flags |= (pass == FFX_FSR2_PASS_ACCUMULATE_SHARPEN) ? FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING : 0;
    flags |= (useLut) ? FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE : 0;
    flags |= (canForceWave64) ? FSR2_SHADER_PERMUTATION_FORCE_WAVE64 : 0;
    flags |= (supportedFP16 && (pass != FFX_FSR2_PASS_RCAS)) ? FSR2_SHADER_PERMUTATION_ALLOW_FP16 : 0;

    const Fsr2ShaderBlobDX12 shaderBlob = fsr2GetPermutationBlobByIndexDX12(pass, flags);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    // set up root signature
    // easiest implementation: simply create one root signature per pipeline
    // should add some management later on to avoid unnecessarily re-binding the root signature
    {
        const D3D12_STATIC_SAMPLER_DESC dx12PointClampSamplerDescription {

            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0,
            16,
            D3D12_COMPARISON_FUNC_NEVER,
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            0.f,
            D3D12_FLOAT32_MAX,
            2, // s2
            0,
            D3D12_SHADER_VISIBILITY_ALL,
        };

        const D3D12_STATIC_SAMPLER_DESC dx12LinearClampSamplerDescription {

            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0,
            16,
            D3D12_COMPARISON_FUNC_NEVER,
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            0.f,
            D3D12_FLOAT32_MAX,
            3, // s3
            0,
            D3D12_SHADER_VISIBILITY_ALL,
        };

        FFX_ASSERT(pipelineDescription->samplerCount <= FSR2_MAX_SAMPLERS);
        const size_t samplerCount = pipelineDescription->samplerCount;
        D3D12_STATIC_SAMPLER_DESC dx12SamplerDescriptions[FSR2_MAX_SAMPLERS];
        for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex) {

            dx12SamplerDescriptions[currentSamplerIndex] = pipelineDescription->samplers[currentSamplerIndex] == FFX_FILTER_TYPE_POINT ? dx12PointClampSamplerDescription : dx12LinearClampSamplerDescription;
            dx12SamplerDescriptions[currentSamplerIndex].ShaderRegister = (UINT)currentSamplerIndex;
        }

        // storage for maximum number of descriptor ranges.
        const int32_t maximumDescriptorRangeSize = 2;
        D3D12_DESCRIPTOR_RANGE dx12Ranges[maximumDescriptorRangeSize] = {};
        int32_t currentDescriptorRangeIndex = 0;

        // storage for maximum number of root parameters.
        const int32_t maximumRootParameters = 10;
        D3D12_ROOT_PARAMETER dx12RootParameters[maximumRootParameters] = {};
        int32_t currentRootParameterIndex = 0;

        int32_t uavCount = FFX_FSR2_RESOURCE_IDENTIFIER_COUNT;
        int32_t srvCount = FFX_FSR2_RESOURCE_IDENTIFIER_COUNT;
        if (uavCount > 0) {

            FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
            D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
            memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
            dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            dx12DescriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            dx12DescriptorRange->BaseShaderRegister = 0;
            dx12DescriptorRange->NumDescriptors = uavCount;
            currentDescriptorRangeIndex++;

            FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
            D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
            memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
            dx12RootParameterSlot->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
            currentRootParameterIndex++;
        }

        if (srvCount > 0) {

            FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
            D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
            memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
            dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            dx12DescriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            dx12DescriptorRange->BaseShaderRegister = 0;
            dx12DescriptorRange->NumDescriptors = srvCount;
            currentDescriptorRangeIndex++;

            FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
            D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
            memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
            dx12RootParameterSlot->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
            currentRootParameterIndex++;
        }

        for (int32_t currentRangeIndex = 0; currentRangeIndex < currentDescriptorRangeIndex; currentRangeIndex++) {

            dx12RootParameters[currentRangeIndex].DescriptorTable.pDescriptorRanges = &dx12Ranges[currentRangeIndex];
        }

        
        FFX_ASSERT(pipelineDescription->rootConstantBufferCount <= FFX_MAX_NUM_CONST_BUFFERS);
        size_t rootConstantsSize = pipelineDescription->rootConstantBufferCount;
        uint32_t rootConstants[FFX_MAX_NUM_CONST_BUFFERS];

        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipelineDescription->rootConstantBufferCount; ++currentRootConstantIndex) {

            rootConstants[currentRootConstantIndex] = pipelineDescription->rootConstantBufferSizes[currentRootConstantIndex];
        }

        for (int32_t currentRootConstantIndex = 0; currentRootConstantIndex < (int32_t)pipelineDescription->rootConstantBufferCount; currentRootConstantIndex++) {

            FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
            D3D12_ROOT_PARAMETER* rootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
            memset(rootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
            rootParameterSlot->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            rootParameterSlot->Constants.ShaderRegister = currentRootConstantIndex;
            rootParameterSlot->Constants.Num32BitValues = pipelineDescription->rootConstantBufferSizes[currentRootConstantIndex];
            currentRootParameterIndex++;
        }

        D3D12_ROOT_SIGNATURE_DESC dx12RootSignatureDescription = {};
        dx12RootSignatureDescription.NumParameters = currentRootParameterIndex;
        dx12RootSignatureDescription.pParameters = dx12RootParameters;
        dx12RootSignatureDescription.NumStaticSamplers = (UINT)samplerCount;
        dx12RootSignatureDescription.pStaticSamplers = dx12SamplerDescriptions;

        ID3DBlob* outBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        //Query D3D12SerializeRootSignature from d3d12.dll handle
        typedef HRESULT(__stdcall* D3D12SerializeRootSignatureType)(const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION, ID3DBlob**, ID3DBlob**);

        //Do not pass hD3D12 handle to the FreeLibrary function, as GetModuleHandle will not increment refcount
        HMODULE d3d12ModuleHandle = GetModuleHandleW(L"D3D12.dll");

        if (NULL != d3d12ModuleHandle) {

            D3D12SerializeRootSignatureType dx12SerializeRootSignatureType = (D3D12SerializeRootSignatureType)GetProcAddress(d3d12ModuleHandle, "D3D12SerializeRootSignature");

            if (nullptr != dx12SerializeRootSignatureType) {

                HRESULT result = dx12SerializeRootSignatureType(&dx12RootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &outBlob, &errorBlob);
                if (FAILED(result)) {

                    return FFX_ERROR_BACKEND_API_ERROR;
                }

                size_t blobSize = outBlob->GetBufferSize();
                int64_t* blobData = (int64_t*)outBlob->GetBufferPointer();

                result = dx12Device->CreateRootSignature(0, outBlob->GetBufferPointer(), outBlob->GetBufferSize(), IID_PPV_ARGS(reinterpret_cast<ID3D12RootSignature**>(&outPipeline->rootSignature)));
                if (FAILED(result)) {

                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            } else {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        } else {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(outPipeline->rootSignature);

    // populate the pass.
    outPipeline->srvCount = shaderBlob.srvCount;
    outPipeline->uavCount = shaderBlob.uavCount;
    outPipeline->constCount = shaderBlob.cbvCount;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvCount; ++srvIndex)
    {
        outPipeline->srvResourceBindings[srvIndex].slotIndex = shaderBlob.boundSRVResources[srvIndex];
        wcscpy_s(outPipeline->srvResourceBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVResourceNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavCount; ++uavIndex)
    {
        outPipeline->uavResourceBindings[uavIndex].slotIndex = shaderBlob.boundUAVResources[uavIndex];
        wcscpy_s(outPipeline->uavResourceBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVResourceNames[uavIndex]).c_str());
    }
    for (uint32_t cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->cbResourceBindings[cbIndex].slotIndex = shaderBlob.boundCBVResources[cbIndex];
        wcscpy_s(outPipeline->cbResourceBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundCBVResourceNames[cbIndex]).c_str());
    }

    // create the PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC dx12PipelineStateDescription = {};
    dx12PipelineStateDescription.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    dx12PipelineStateDescription.pRootSignature = dx12RootSignature;
    dx12PipelineStateDescription.CS.pShaderBytecode = shaderBlob.data;
    dx12PipelineStateDescription.CS.BytecodeLength = shaderBlob.size;
    result = dx12Device->CreateComputePipelineState(&dx12PipelineStateDescription, IID_PPV_ARGS(reinterpret_cast<ID3D12PipelineState**>(&outPipeline->pipeline)));

    if (FAILED(result)) {

        return FFX_ERROR_BACKEND_API_ERROR;
    }

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobDX12(
    FfxFsr2Interface* backendInterface,
    const FfxGpuJobDescription* job
)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != job);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FSR2_MAX_GPU_JOBS);
 
    backendContext->gpuJobs[backendContext->gpuJobCount] = *job;

    if (job->jobType == FFX_GPU_JOB_COMPUTE) {

        // needs to copy SRVs and UAVs in case they are on the stack only
        FfxComputeJobDescription* computeJob = &backendContext->gpuJobs[backendContext->gpuJobCount].computeJobDescriptor;
        const uint32_t numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex< numConstBuffers; ++currentRootConstantIndex)
        {
            computeJob->cbs[currentRootConstantIndex].uint32Size = job->computeJobDescriptor.cbs[currentRootConstantIndex].uint32Size;
            memcpy(computeJob->cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, computeJob->cbs[currentRootConstantIndex].uint32Size*sizeof(uint32_t));
        }
    }

    backendContext->gpuJobCount++;

    return FFX_OK;
}

void addBarrier(BackendContext_DX12* backendContext, FfxResourceInternal* resource, FfxResourceStates newState)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != resource);

    ID3D12Resource* dx12Resource = getDX12ResourcePtr(backendContext, resource->internalIndex);
    D3D12_RESOURCE_BARRIER* barrier = &backendContext->barriers[backendContext->barrierCount];

    FFX_ASSERT(backendContext->barrierCount < FSR2_MAX_BARRIERS);

    FfxResourceStates* currentState = &backendContext->resources[resource->internalIndex].state;

    if((*currentState & newState) != newState) {

        *barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            dx12Resource,
            ffxGetDX12StateFromResourceState(*currentState),
            ffxGetDX12StateFromResourceState(newState));

        *currentState = newState;
        ++backendContext->barrierCount;

    } else if(newState == FFX_RESOURCE_STATE_UNORDERED_ACCESS) {
        
        *barrier = CD3DX12_RESOURCE_BARRIER::UAV(dx12Resource);
        ++backendContext->barrierCount;
    }
}

void flushBarriers(BackendContext_DX12* backendContext, ID3D12GraphicsCommandList* dx12CommandList)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != dx12CommandList);

    if (backendContext->barrierCount > 0) {

        dx12CommandList->ResourceBarrier(backendContext->barrierCount, backendContext->barriers);
        backendContext->barrierCount = 0;
    }
}

static FfxErrorCode executeGpuJobCompute(BackendContext_DX12* backendContext, FfxGpuJobDescription* job, ID3D12Device* dx12Device, ID3D12GraphicsCommandList* dx12CommandList)
{
    ID3D12DescriptorHeap* dx12DescriptorHeap = reinterpret_cast<ID3D12DescriptorHeap*>(backendContext->descRingBuffer);

    // set root signature
    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(job->computeJobDescriptor.pipeline.rootSignature);
    dx12CommandList->SetComputeRootSignature(dx12RootSignature);

    // set descriptor heap
    dx12CommandList->SetDescriptorHeaps(1, &dx12DescriptorHeap);

    uint32_t descriptorTableIndex = 0;
    // bind UAVs
    {
        uint32_t maximumUavIndex = 0;

        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavCount; ++currentPipelineUavIndex) {

            uint32_t uavResourceIndex = job->computeJobDescriptor.pipeline.uavResourceBindings[currentPipelineUavIndex].slotIndex;
            maximumUavIndex = uavResourceIndex > maximumUavIndex ? uavResourceIndex : maximumUavIndex;
        }

        // check if this fits into the ringbuffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumUavIndex + 1 > FSR2_DESC_RING_SIZE) {

            backendContext->descRingBufferBase = 0;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Set UAVs
        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavCount; ++currentPipelineUavIndex) {

            addBarrier(backendContext, &job->computeJobDescriptor.uavs[currentPipelineUavIndex], FFX_RESOURCE_STATE_UNORDERED_ACCESS);

            // source: UAV of resource to bind
            const uint32_t resourceIndex = job->computeJobDescriptor.uavs[currentPipelineUavIndex].internalIndex;
            const uint32_t uavIndex = backendContext->resources[resourceIndex].uavDescIndex + job->computeJobDescriptor.uavMip[currentPipelineUavIndex];

            D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
            srcHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // dest: index used by pipeline, relative to ringbuffer base
            const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavResourceBindings[currentPipelineUavIndex].slotIndex;
            D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            cpuView.ptr += currentUavResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // numMips
            dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        backendContext->descRingBufferBase += maximumUavIndex + 1;
        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    // bind SRVs
    {
        uint32_t maximumSrvIndex = 0;
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvCount; ++currentPipelineSrvIndex) {

            const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvResourceBindings[currentPipelineSrvIndex].slotIndex;
            maximumSrvIndex = currentSrvResourceIndex > maximumSrvIndex ? currentSrvResourceIndex : maximumSrvIndex;
        }

        // check if this fits into the ringbuffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumSrvIndex + 1 > FSR2_DESC_RING_SIZE) {

            backendContext->descRingBufferBase = 0;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvCount; ++currentPipelineSrvIndex) {

            addBarrier(backendContext, &job->computeJobDescriptor.srvs[currentPipelineSrvIndex], FFX_RESOURCE_STATE_COMPUTE_READ);

            // source: SRV of resource to bind
            const uint32_t resourceIndex = job->computeJobDescriptor.srvs[currentPipelineSrvIndex].internalIndex;
            D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
            srcHandle.ptr += resourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // dest: index used by pipeline, relative to ringbuffer base
            const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvResourceBindings[currentPipelineSrvIndex].slotIndex;
            D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            cpuView.ptr += currentSrvResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        backendContext->descRingBufferBase += maximumSrvIndex + 1;
        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    flushBarriers(backendContext, dx12CommandList);

    // bind pipeline
    ID3D12PipelineState* dx12PipelineStateObject = reinterpret_cast<ID3D12PipelineState*>(job->computeJobDescriptor.pipeline.pipeline);
    dx12CommandList->SetPipelineState(dx12PipelineStateObject);

    // bound constant buffers not supported

    // set root constants, free local copy
    {
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex) {
            const uint32_t currentCbSlotIndex = job->computeJobDescriptor.pipeline.cbResourceBindings[currentRootConstantIndex].slotIndex;
            dx12CommandList->SetComputeRoot32BitConstants(descriptorTableIndex + currentCbSlotIndex, job->computeJobDescriptor.cbs[currentCbSlotIndex].uint32Size, job->computeJobDescriptor.cbs[currentCbSlotIndex].data, 0);
        }
    }

    // dispatch
    dx12CommandList->Dispatch(job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_DX12* backendContext, FfxGpuJobDescription* job, ID3D12Device* dx12Device, ID3D12GraphicsCommandList* dx12CommandList)
{
    ID3D12Resource* dx12ResourceSrc = getDX12ResourcePtr(backendContext, job->copyJobDescriptor.src.internalIndex);
    ID3D12Resource* dx12ResourceDst = getDX12ResourcePtr(backendContext, job->copyJobDescriptor.dst.internalIndex);
    D3D12_RESOURCE_DESC dx12ResourceDescription = dx12ResourceDst->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT dx12Footprint = {};
    UINT rowCount;
    UINT64 rowSizeInBytes;
    UINT64 totalBytes;
    dx12Device->GetCopyableFootprints(&dx12ResourceDescription, 0, 1, 0, &dx12Footprint, &rowCount, &rowSizeInBytes, &totalBytes);

    D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
    dx12SourceLocation.pResource = dx12ResourceSrc;
    dx12SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dx12SourceLocation.PlacedFootprint = dx12Footprint;

    D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
    dx12DestinationLocation.pResource = dx12ResourceDst;
    dx12DestinationLocation.SubresourceIndex = 0;
    dx12DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, dx12CommandList);

    dx12CommandList->CopyTextureRegion(&dx12DestinationLocation, 0, 0, 0, &dx12SourceLocation, nullptr);
    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_DX12* backendContext, FfxGpuJobDescription* job, ID3D12Device* dx12Device, ID3D12GraphicsCommandList* dx12CommandList)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_DX12::Resource ffxResource = backendContext->resources[idx];
    ID3D12Resource* dx12Resource = reinterpret_cast<ID3D12Resource*>(ffxResource.resourcePtr);
    uint32_t srvIndex = ffxResource.srvDescIndex;
    uint32_t uavIndex = ffxResource.uavDescIndex;

    D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
    dx12CpuHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_GPU_DESCRIPTOR_HANDLE dx12GpuHandle = backendContext->descHeapUavGpu->GetGPUDescriptorHandleForHeapStart();
    dx12GpuHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dx12CommandList->SetDescriptorHeaps(1, &backendContext->descHeapUavGpu);

    addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    flushBarriers(backendContext, dx12CommandList);

    dx12CommandList->ClearUnorderedAccessViewFloat(dx12GpuHandle, dx12CpuHandle, dx12Resource, job->clearJobDescriptor.color, 0, nullptr);

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsDX12(
    FfxFsr2Interface* backendInterface,
    FfxCommandList commandList)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FfxErrorCode errorCode = FFX_OK;

    // execute all GpuJobs
    for (uint32_t currentGpuJobIndex = 0; currentGpuJobIndex < backendContext->gpuJobCount; ++currentGpuJobIndex) {

        FfxGpuJobDescription* GpuJob = &backendContext->gpuJobs[currentGpuJobIndex];
        ID3D12GraphicsCommandList* dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList*>(commandList);
        ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);

        switch (GpuJob->jobType) {

            case FFX_GPU_JOB_CLEAR_FLOAT:
                errorCode = executeGpuJobClearFloat(backendContext, GpuJob, dx12Device, dx12CommandList);
                break;

            case FFX_GPU_JOB_COPY:
                errorCode = executeGpuJobCopy(backendContext, GpuJob, dx12Device, dx12CommandList);
                break;

            case FFX_GPU_JOB_COMPUTE:
                errorCode = executeGpuJobCompute(backendContext, GpuJob, dx12Device, dx12CommandList);
                break;

            default:
                break;
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}

FfxErrorCode DestroyResourceDX12(
    FfxFsr2Interface* backendInterface,
    FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    ID3D12Resource* dx12Resource = getDX12ResourcePtr(backendContext, resource.internalIndex);

    if (dx12Resource) {
        dx12Resource->Release();
        backendContext->resources[resource.internalIndex].resourcePtr = nullptr;
        //ffxDX12ReleaseResource(backendInterface, *resource);
    }

    return FFX_OK;
}

FfxErrorCode DestroyPipelineDX12(
    FfxFsr2Interface* backendInterface,
    FfxPipelineState* pipeline)
{
    FFX_ASSERT(backendInterface != nullptr);
    if (!pipeline) {
        return FFX_OK;
    }

    // destroy Rootsignature
    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(pipeline->rootSignature);
    if (dx12RootSignature) {
        dx12RootSignature->Release();
    }
    pipeline->rootSignature = nullptr;

    // destroy pipeline
    ID3D12PipelineState* dx12Pipeline = reinterpret_cast<ID3D12PipelineState*>(pipeline->pipeline);
    if (dx12Pipeline) {
        dx12Pipeline->Release();
    }
    pipeline->pipeline = nullptr;

    return FFX_OK;
}
