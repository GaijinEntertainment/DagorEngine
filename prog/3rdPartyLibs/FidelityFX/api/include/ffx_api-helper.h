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

#include "ffx_api.h"
#if defined(FFX_UPSCALER)
#include "../../upscalers/include/ffx_upscale.h"
#endif //#if defined(FFX_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
#include "../../framegeneration/include/ffx_framegeneration.h"
#include "../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"
#endif //#if defined(FFX_FRAMEGENERATION)


#include "ffx_provider.h"

ffxReturnCode_t ffxQueryFallback(ffxContext* context, ffxQueryDescHeader* header, ffxReturnCode_t retCode)
{
    if (context == nullptr)
    {
        if (retCode != FFX_API_RETURN_OK)
        {
            //pass-through retCode
            //Fill in valid default output. Otherwise if case app doesn't check retCode, and have not previously zero out the struct, app would read random values.
            if (header->type == FFX_API_QUERY_DESC_TYPE_GET_VERSIONS)
            {
                auto desc = reinterpret_cast<ffxQueryDescGetVersions*>(header);
                desc->outputCount = 0;
            }
#if defined(FFX_UPSCALER)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetRenderResolutionFromQualityMode*>(header);
                if (desc->pOutRenderWidth != nullptr)
                {
                    *desc->pOutRenderWidth = 0u;
                }
                if (desc->pOutRenderHeight != nullptr)
                {
                    *desc->pOutRenderHeight = 0u;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode*>(header);
                if (desc->pOutUpscaleRatio != nullptr)
                {
                    *desc->pOutUpscaleRatio = 0.0f;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetJitterPhaseCount*>(header);
                if (desc->pOutPhaseCount != nullptr)
                {
                    *desc->pOutPhaseCount = 0;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetJitterOffset*>(header);
                if (desc->pOutX != nullptr)
                {
                    *desc->pOutX = 0.0f;
                }
                if (desc->pOutY != nullptr)
                {
                    *desc->pOutY = 0.0f;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetGPUMemoryUsage*>(header);
                if (desc->gpuMemoryUsageUpscaler != nullptr)
                {
                    desc->gpuMemoryUsageUpscaler->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageUpscaler->aliasableUsageInBytes = 0u;
                }
            }
#endif //#if defined(FFX_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE)
            {
                auto desc = reinterpret_cast<ffxQueryDescFrameGenerationGetGPUMemoryUsage*>(header);
                if (desc->gpuMemoryUsageFrameGeneration != nullptr)
                {
                    desc->gpuMemoryUsageFrameGeneration->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGeneration->aliasableUsageInBytes = 0u;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12)
            {
                auto desc = reinterpret_cast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12*>(header);
                if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
                {
                    desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                    retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#endif  // defined(FFX_FRAMEGENERATION)
#if defined(FFX_UPSCALER)
            // Fixup retCode for new DESCTYPE that are not supported by oldest FSR driver provider (Adrenaline 25.3.1)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetResourceRequirements*>(header);
                desc->required_resources = ~uint64_t(0);
                desc->optional_resources = ~uint64_t(0);
                if (header->type == FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE)
                    retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetGPUMemoryUsageV2*>(header);
                if (desc->gpuMemoryUsageUpscaler != nullptr)
                {
                    desc->gpuMemoryUsageUpscaler->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageUpscaler->aliasableUsageInBytes = 0u;
                    retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#endif // defined(FFX_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2)
            {
                auto desc = reinterpret_cast<ffxQueryDescFrameGenerationGetGPUMemoryUsageV2*>(header);
                if (desc->gpuMemoryUsageFrameGeneration != nullptr)
                {
                    desc->gpuMemoryUsageFrameGeneration->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGeneration->aliasableUsageInBytes = 0u;
                    retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2)
            {
                auto desc = reinterpret_cast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2*>(header);
                if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
                {
                    desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                    retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#endif  // defined(FFX_FRAMEGENERATION)
        }
    }
    else
    {
        if (retCode == FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE)
        {
            auto provider = GetAssociatedProvider(context);
            if (header->type == FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION)
            {
                auto desc = reinterpret_cast<ffxQueryGetProviderVersion*>(header);
                desc->versionId = provider->GetId();
                desc->versionName = provider->GetVersionName();
                retCode = FFX_API_RETURN_OK;
            }
            // on a older driver or effect DLL that doesn't support this new query DESTYPE, fill in default output.
#if defined(FFX_UPSCALER)            
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS)
            {
                auto desc = reinterpret_cast<ffxQueryDescUpscaleGetResourceRequirements*>(header);
                desc->required_resources = ~uint64_t(0);
                desc->optional_resources = ~uint64_t(0);
                retCode = FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
            }
#endif //defined(FFX_UPSCALER)
        }
    }
    return retCode;
}
