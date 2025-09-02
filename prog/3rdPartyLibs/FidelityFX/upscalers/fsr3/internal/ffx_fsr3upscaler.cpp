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

#include <algorithm>    // for max used inside SPD CPU code.
#include <cmath>        // for fabs, abs, sinf, sqrt, etc.
#include <string.h>     // for memset
#include <cfloat>       // for FLT_EPSILON
#include "../include/ffx_fsr3upscaler.h"

#define FFX_CPU

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wno-narrowing"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4838)
#endif

#include "../../../api/internal/gpu/ffx_core.h"
#include "../../../api/internal/ffx_object_management.h"
#if !defined(_GAMING_XBOX)
#include "../../../api/internal/git_hash_branch.h"
#endif // !defined(_GAMING_XBOX)
#include "../include/gpu/fsr1/ffx_fsr1.h"
#include "../include/gpu/spd/ffx_spd.h"
#include "../include/gpu/fsr3upscaler/ffx_fsr3upscaler_callbacks_hlsl.h"
#include "../include/gpu/fsr3upscaler/ffx_fsr3upscaler_resources.h"
#include "../include/gpu/fsr3upscaler/ffx_fsr3upscaler_common.h"

// max queued frames for descriptor management
static const uint32_t FSR3UPSCALER_MAX_QUEUED_FRAMES = 16;

#include "ffx_fsr3upscaler_private.h"
#include "ffx_fsr3upscaler_shaderblobs.h"

#ifdef FFX_BACKEND_DX12
#include "../../../backend/dx12/ffx_dx12.h"
#endif // FFX_BACKEND_DX12

// lists to map shader resource bindpoint name to resource identifier
typedef struct ResourceBinding
{
    uint32_t    index;
    wchar_t     name[64];
}ResourceBinding;

static const ResourceBinding srvTextureBindingTable[] =
{
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR,                              L"r_input_color_jittered"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY,                        L"r_input_opaque_only"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS,                     L"r_input_motion_vectors"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_DEPTH,                              L"r_input_depth" },
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE,                           L"r_input_exposure"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FRAME_INFO,                               L"r_frame_info"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK,                      L"r_reactive_mask"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK,  L"r_transparency_and_composition_mask"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH,     L"r_reconstructed_previous_nearest_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                   L"r_dilated_motion_vectors"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH,                            L"r_dilated_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR,                  L"r_internal_upscaled_color"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION,                             L"r_accumulation"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY,                             L"r_luma_history" },
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT,                               L"r_rcas_input"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LANCZOS_LUT,                              L"r_lanczos_lut"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS,                                 L"r_spd_mips"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS,                   L"r_dilated_reactive_masks"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS,                                L"r_new_locks"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH,                           L"r_farthest_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH_MIP1,                      L"r_farthest_depth_mip1"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SHADING_CHANGE,                           L"r_shading_change"},

    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_CURRENT_LUMA,                             L"r_current_luma"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREVIOUS_LUMA,                            L"r_previous_luma"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_INSTABILITY,                         L"r_luma_instability"},
};

static const ResourceBinding uavTextureBindingTable[] =
{
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH,     L"rw_reconstructed_previous_nearest_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                   L"rw_dilated_motion_vectors"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH,                            L"rw_dilated_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR,                  L"rw_internal_upscaled_color"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION,                             L"rw_accumulation"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY,                             L"rw_luma_history"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT,                          L"rw_upscaled_output"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS,                   L"rw_dilated_reactive_masks"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FRAME_INFO,                               L"rw_frame_info"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT,                         L"rw_spd_global_atomic"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS,                                L"rw_new_locks"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTOREACTIVE,                             L"rw_output_autoreactive"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SHADING_CHANGE,                           L"rw_shading_change"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH,                           L"rw_farthest_depth"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH_MIP1,                      L"rw_farthest_depth_mip1"},

    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_CURRENT_LUMA,                             L"rw_current_luma"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_INSTABILITY,                         L"rw_luma_instability"},

    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_0,                         L"rw_spd_mip0"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_1,                         L"rw_spd_mip1"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_2,                         L"rw_spd_mip2"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_3,                         L"rw_spd_mip3"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_4,                         L"rw_spd_mip4"},
    {FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_5,                         L"rw_spd_mip5"},


};

static const ResourceBinding constantBufferBindingTable[] =
{
    {FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_FSR3UPSCALER,   L"cbFSR3Upscaler"},
    {FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_SPD,            L"cbSPD"},
    {FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_RCAS,           L"cbRCAS"},
    {FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_GENREACTIVE,    L"cbGenerateReactive"},
};

typedef struct Fsr3UpscalerRcasConstants {

    uint32_t                    rcasConfig[4];
} FfxRcasConstants;

typedef struct Fsr3UpscalerSpdConstants {

    uint32_t                    mips;
    uint32_t                    numworkGroups;
    uint32_t                    workGroupOffset[2];
    uint32_t                    renderSize[2];
} Fsr3UpscalerSpdConstants;

typedef struct Fsr3UpscalerGenerateReactiveConstants
{
    float       scale;
    float       threshold;
    float       binaryValue;
    uint32_t    flags;

} Fsr3UpscalerGenerateReactiveConstants;

typedef struct Fsr3UpscalerGenerateReactiveConstants2
{
    float       autoTcThreshold;
    float       autoTcScale;
    float       autoReactiveScale;
    float       autoReactiveMax;

} Fsr3UpscalerGenerateReactiveConstants2;

typedef union Fsr3UpscalerSecondaryUnion {

    Fsr3UpscalerRcasConstants               rcas;
    Fsr3UpscalerSpdConstants                spd;
    Fsr3UpscalerGenerateReactiveConstants2  autogenReactive;
} Fsr3UpscalerSecondaryUnion;

// Lanczos
static float lanczos2(float value)
{
    return abs(value) < FFX_EPSILON ? 1.f : (sinf(FFX_PI * value) / (FFX_PI * value)) * (sinf(0.5f * FFX_PI * value) / (0.5f * FFX_PI * value));
}

// Calculate halton number for index and base.
static float halton(int32_t index, int32_t base)
{
    float f = 1.0f, result = 0.0f;

    for (int32_t currentIndex = index; currentIndex > 0;) {

        f /= (float)base;
        result = result + f * (float)(currentIndex % base);
        currentIndex = (uint32_t)(floorf((float)(currentIndex) / (float)(base)));
    }

    return result;
}

#ifndef BUILD_AMDXCFFX //exclude FSR3 code when building driver DLL to reduce dependencies

static void fsr3upscalerDebugCheckDispatch(FfxFsr3UpscalerContext_Private* context, const FfxFsr3UpscalerDispatchDescription* params)
{
    if (params->commandList == nullptr)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"commandList is null");
    }

    if (params->color.resource == nullptr)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"color resource is null");
    }

    if (params->depth.resource == nullptr)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"depth resource is null");
    }

    if (params->motionVectors.resource == nullptr)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"motionVectors resource is null");
    }

    if (params->exposure.resource != nullptr)
    {
        if ((context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE) == FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE)
        {
            FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"exposure resource provided, however auto exposure flag is present");
        }
    }

    if (params->output.resource == nullptr)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"output resource is null");
    }

    if (fabs(params->jitterOffset.x) > 1.0f || fabs(params->jitterOffset.y) > 1.0f)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"jitterOffset contains value outside of expected range [-1.0, 1.0]");
    }

    if ((params->motionVectorScale.x > (float)context->contextDescription.maxRenderSize.width) ||
        (params->motionVectorScale.y > (float)context->contextDescription.maxRenderSize.height))
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"motionVectorScale contains scale value greater than maxRenderSize");
    }
    if ((params->motionVectorScale.x == 0.0f) ||
        (params->motionVectorScale.y == 0.0f))
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"motionVectorScale contains zero scale value");
    }

    if ((params->renderSize.width > context->contextDescription.maxRenderSize.width) ||
        (params->renderSize.height > context->contextDescription.maxRenderSize.height))
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"renderSize is greater than context maxRenderSize");
    }
    if ((params->renderSize.width == 0) ||
        (params->renderSize.height == 0))
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"renderSize contains zero dimension");
    }

    if (params->sharpness < 0.0f || params->sharpness > 1.0f)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"sharpness contains value outside of expected range [0.0, 1.0]");
    }

    if (params->frameTimeDelta < 1.0f)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING, L"frameTimeDelta is less than 1.0f - this value should be milliseconds (~16.6f for 60fps)");
    }

    if (params->preExposure == 0.0f)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"preExposure provided as 0.0f which is invalid");
    }

    bool infiniteDepth = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE) == FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE;
    bool inverseDepth = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) == FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED;

    if (inverseDepth)
    {
        if (params->cameraNear < params->cameraFar)
        {
            FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                L"FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED flag is present yet cameraNear is less than cameraFar");
        }
        if (infiniteDepth)
        {
            if (params->cameraNear != FLT_MAX)
            {
                FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                    L"FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE and FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED present, yet cameraNear != FLT_MAX");
            }
        }
        if (params->cameraFar < 0.075f)
        {
            FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                L"FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED present, cameraFar value is very low which may result in depth separation artefacting");
        }
    }
    else
    {
        if (params->cameraNear > params->cameraFar)
        {
            FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                L"cameraNear is greater than cameraFar in non-inverted-depth context");
        }
        if (infiniteDepth)
        {
            if (params->cameraFar != FLT_MAX)
            {
                FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                    L"FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE present, yet cameraFar != FLT_MAX");
            }
        }
        if (params->cameraNear < 0.075f)
        {
            FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_WARNING,
                L"cameraNear value is very low which may result in depth separation artefacting");
        }
    }

    if (params->cameraFovAngleVertical <= 0.0f)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"cameraFovAngleVertical is 0.0f - this value should be > 0.0f");
    }
    if (params->cameraFovAngleVertical > FFX_PI)
    {
        FFX_PRINT_MESSAGE(FFX_MESSAGE_TYPE_ERROR, L"cameraFovAngleVertical is greater than 180 degrees/PI");
    }
}

static FfxErrorCode patchResourceBindings(FfxPipelineState* inoutPipeline)
{
    for (uint32_t srvIndex = 0; srvIndex < inoutPipeline->srvTextureCount; ++srvIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(srvTextureBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(srvTextureBindingTable[mapIndex].name, inoutPipeline->srvTextureBindings[srvIndex].name))
                break;
        }
        if (mapIndex == _countof(srvTextureBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->srvTextureBindings[srvIndex].resourceIdentifier = srvTextureBindingTable[mapIndex].index;
    }

    for (uint32_t uavIndex = 0; uavIndex < inoutPipeline->uavTextureCount; ++uavIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(uavTextureBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(uavTextureBindingTable[mapIndex].name, inoutPipeline->uavTextureBindings[uavIndex].name))
                break;
        }
        if (mapIndex == _countof(uavTextureBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->uavTextureBindings[uavIndex].resourceIdentifier = uavTextureBindingTable[mapIndex].index;
    }

    for (uint32_t cbIndex = 0; cbIndex < inoutPipeline->constCount; ++cbIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(constantBufferBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(constantBufferBindingTable[mapIndex].name, inoutPipeline->constantBufferBindings[cbIndex].name))
                break;
        }
        if (mapIndex == _countof(constantBufferBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->constantBufferBindings[cbIndex].resourceIdentifier = constantBufferBindingTable[mapIndex].index;
    }

    return FFX_OK;
}

static uint32_t getPipelinePermutationFlags(uint32_t contextFlags, FfxFsr3UpscalerPass passId, bool fp16, bool force64, bool useLut)
{
    // work out what permutation to load.
    uint32_t flags = 0;
    flags |= (contextFlags & FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE) ? FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT : 0;
    flags |= (contextFlags & FFX_FSR3UPSCALER_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS) ? 0 : FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS;
    flags |= (contextFlags & FFX_FSR3UPSCALER_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) ? FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS : 0;
    flags |= (contextFlags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) ? FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED : 0;
    flags |= (passId == FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN) ? FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING : 0;
    flags |= (useLut) ? FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE : 0;
    flags |= (force64) ? FSR3UPSCALER_SHADER_PERMUTATION_FORCE_WAVE64 : 0;
#if defined(_GAMING_XBOX)
    /** On Xbox we enable 16-bit math, and use 32-bit within the shader only where it's necessary. */
    flags |= (fp16) ? FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16 : 0;
#else
    flags |= (fp16 && (passId != FFX_FSR3UPSCALER_PASS_RCAS)) ? FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16 : 0;
#endif // defined(_GAMING_XBOX)
    return flags;
}

static FfxErrorCode createPipelineStates(FfxFsr3UpscalerContext_Private* context)
{
    FFX_ASSERT(context);

    FfxPipelineDescription pipelineDescription = {};
    pipelineDescription.contextFlags = context->contextDescription.flags;
    pipelineDescription.stage        = FFX_BIND_COMPUTE_SHADER_STAGE;

    // Samplers
    pipelineDescription.samplerCount = 2;
    FfxSamplerDescription samplerDescs[2] = { { FFX_FILTER_TYPE_MINMAGMIP_POINT, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_BIND_COMPUTE_SHADER_STAGE },
                                            { FFX_FILTER_TYPE_MINMAGMIP_LINEAR, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_BIND_COMPUTE_SHADER_STAGE} };
    pipelineDescription.samplers = samplerDescs;

    // Root constants
    pipelineDescription.rootConstantBufferCount = 2;
    FfxRootConstantDescription rootConstantDescs[2] = { {sizeof(Fsr3UpscalerConstants) / sizeof(uint32_t), FFX_BIND_COMPUTE_SHADER_STAGE },
                                                        { sizeof(Fsr3UpscalerSecondaryUnion) / sizeof(uint32_t), FFX_BIND_COMPUTE_SHADER_STAGE } };
    pipelineDescription.rootConstants = rootConstantDescs;

    // Query device capabilities
    FfxDeviceCapabilities capabilities;
    context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &capabilities);

    // Setup a few options used to determine permutation flags
    bool haveShaderModel66 = capabilities.maximumSupportedShaderModel >= FFX_SHADER_MODEL_6_6;
    bool supportedFP16     = capabilities.fp16Supported;
    bool canForceWave64    = false;
    bool useLut            = false;

    const uint32_t waveLaneCountMin = capabilities.waveLaneCountMin;
    const uint32_t waveLaneCountMax = capabilities.waveLaneCountMax;
    if (waveLaneCountMin == 32 && waveLaneCountMax == 64)
    {
        useLut         = true;
        canForceWave64 = haveShaderModel66;
    }
    else
    {
        canForceWave64 = false;
    }

    // Work out what permutation to load.
    uint32_t contextFlags = context->contextDescription.flags;

    // Set up pipeline descriptor (basically RootSignature and binding)
    FfxShaderBlob shaderBlob = {};
    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_LUMA_PYRAMID,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_LUMA_PYRAMID, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-LUMA-PYRAMID");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineLumaPyramid));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_RCAS, getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_RCAS, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-RCAS");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineRCAS));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_GENERATE_REACTIVE,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_GENERATE_REACTIVE, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-GEN_REACTIVE");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineGenerateReactive));

    pipelineDescription.rootConstantBufferCount = 1;

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_PREPARE_INPUTS,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_PREPARE_INPUTS, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-PREPARE-INPUTS");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelinePrepareInputs));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_PREPARE_REACTIVITY,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_PREPARE_REACTIVITY, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-PREPARE-REACTIVITY");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelinePrepareReactivity));
    
    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_SHADING_CHANGE,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_SHADING_CHANGE, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-SHADING-CHANGE");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineShadingChange));
    
    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_ACCUMULATE,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_ACCUMULATE, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-ACCUMULATE");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineAccumulate));
    
    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-ACCUM_SHARP");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineAccumulateSharpen));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_SHADING_CHANGE_PYRAMID,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_SHADING_CHANGE_PYRAMID, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-SHADING-CHANGE-PYRAMID");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineShadingChangePyramid));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_LUMA_INSTABILITY,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_LUMA_INSTABILITY, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-LUMA-INSTABILITY");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineLumaInstability));

    fsr3UpscalerGetPermutationBlobByIndex(FFX_FSR3UPSCALER_PASS_DEBUG_VIEW,
                                          getPipelinePermutationFlags(contextFlags, FFX_FSR3UPSCALER_PASS_DEBUG_VIEW, supportedFP16, canForceWave64, useLut),
                                          &shaderBlob);
    wcscpy_s(pipelineDescription.name, L"FSR3-DEBUG-VIEW");
    FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(&context->contextDescription.backendInterface, &shaderBlob,
        &pipelineDescription, context->effectContextId, &context->pipelineDebugView));

    // for each pipeline: re-route/fix-up IDs based on names
    FFX_VALIDATE(patchResourceBindings(&context->pipelinePrepareInputs));
    FFX_VALIDATE(patchResourceBindings(&context->pipelinePrepareReactivity));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineShadingChange));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineAccumulate));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineLumaPyramid));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineAccumulateSharpen));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineRCAS));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineGenerateReactive));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineTcrAutogenerate));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineShadingChangePyramid));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineLumaInstability));
    FFX_VALIDATE(patchResourceBindings(&context->pipelineDebugView));

    return FFX_OK;
}

static FfxErrorCode generateReactiveMaskInternal(FfxFsr3UpscalerContext_Private* contextPrivate, const FfxFsr3UpscalerDispatchDescription* params);

static void getInternalResourceDescriptions(const FfxApiDimensions2D* maxRenderSize, const FfxApiDimensions2D* maxUpscaleSize, FfxFsr3UpscalerInternalResourceDescriptions* internalResourceDescriptions)
{
    internalResourceDescriptions->accumulation1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8_UNORM, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_Accumulation1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->accumulation2 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8_UNORM, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_Accumulation2", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_2, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->luma1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_Luma1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->luma2 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_Luma2", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_2, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->intermediateFp16x1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_IntermediateFp16x1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->shadingChange = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8_UNORM, maxRenderSize->width / 2, maxRenderSize->height / 2, 1, 1, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_ShadingChange", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SHADING_CHANGE, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->newLocks = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8_UNORM, maxUpscaleSize->width, maxUpscaleSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_NewLocks", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->internalUpscaled1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, maxUpscaleSize->width, maxUpscaleSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_InternalUpscaled1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->internalUpscaled2 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, maxUpscaleSize->width, maxUpscaleSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_InternalUpscaled2", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->spdMips = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16_FLOAT, maxRenderSize->width / 2, maxRenderSize->height / 2, 1, 0, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_SpdMips", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->farthestDepthMip1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16_FLOAT, maxRenderSize->width / 2, maxRenderSize->height / 2, 1, 1, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_FarthestDepthMip1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH_MIP1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->lumaHistory1 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_LumaHistory1", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_1, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    internalResourceDescriptions->lumaHistory2 = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_LumaHistory2", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_2, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };

    uint32_t atomicInitData = 0U;
    internalResourceDescriptions->spdAtomicCounter = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R32_UINT, 1, 1, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_UAV) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_SpdAtomicCounter", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT, FfxResourceInitData::FfxResourceInitValue(sizeof(atomicInitData), 0) };

    internalResourceDescriptions->dilatedReactiveMasks = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_ALIASABLE, (FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_DilatedReactiveMasks", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };

    // generate the data for the LUT.
    const uint32_t lanczos2LutWidth = 128;
    static int16_t lanczos2Weights[lanczos2LutWidth] = { };
    for (uint32_t currentLanczosWidthIndex = 0; currentLanczosWidthIndex < lanczos2LutWidth; currentLanczosWidthIndex++)
    {
        const float x = 2.0f * currentLanczosWidthIndex / float(lanczos2LutWidth - 1);
        const float y = lanczos2(x);
        lanczos2Weights[currentLanczosWidthIndex] = int16_t(roundf(y * 32767.0f));
    }
    internalResourceDescriptions->lanczosLutData = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16_SNORM, lanczos2LutWidth, 1, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_READ_ONLY) },
        FFX_API_RESOURCE_STATE_COMPUTE_READ, L"FSR3UPSCALER_LanczosLutData", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LANCZOS_LUT, {FFX_RESOURCE_INIT_DATA_TYPE_BUFFER, sizeof(lanczos2Weights), lanczos2Weights} };

    uint8_t defaultReactiveMaskData = 0U;
    internalResourceDescriptions->defaultReactivityMask = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R8_UNORM, 1, 1, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_READ_ONLY) },
        FFX_API_RESOURCE_STATE_COMPUTE_READ, L"FSR3UPSCALER_DefaultReactivityMask", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY, FfxResourceInitData::FfxResourceInitValue(sizeof(defaultReactiveMaskData), defaultReactiveMaskData) };

    static float defaultExposure[] = { 0.0f, 0.0f };
    internalResourceDescriptions->defaultExposure = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R32G32_FLOAT, 1, 1, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_READ_ONLY) },
        FFX_API_RESOURCE_STATE_COMPUTE_READ, L"FSR3UPSCALER_DefaultExposure", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE, FfxResourceInitData::FfxResourceInitBuffer(sizeof(defaultExposure), defaultExposure) };

    internalResourceDescriptions->frameInfo = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT, 1, 1, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_UAV) }, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_FrameInfo", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FRAME_INFO, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
}

static void getSharedResourceDescriptions(const FfxApiDimensions2D* maxRenderSize, const FfxApiDimensions2D* maxUpscaleSize, FfxFsr3UpscalerSharedResourceDescriptions* sharedResourceDescriptions)
{
    sharedResourceDescriptions->dilatedDepth = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R32_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_DilatedDepth", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    sharedResourceDescriptions->dilatedMotionVectors = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R16G16_FLOAT, maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_RENDERTARGET | FFX_API_RESOURCE_USAGE_UAV | FFX_API_RESOURCE_USAGE_DCC_RENDERTARGET) },
            FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_DilatedVelocity", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
    sharedResourceDescriptions->reconstructedPrevNearestDepth = { FFX_HEAP_TYPE_DEFAULT, { FFX_API_RESOURCE_TYPE_TEXTURE2D, FFX_API_SURFACE_FORMAT_R32_UINT,maxRenderSize->width, maxRenderSize->height, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, (FFX_API_RESOURCE_USAGE_UAV) },
            FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR3UPSCALER_ReconstructedPrevNearestDepth", FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };

}

static FfxErrorCode fsr3upscalerCreate(FfxFsr3UpscalerContext_Private* context, const FfxFsr3UpscalerContextDescription* contextDescription)
{
    FFX_ASSERT(context);
    FFX_ASSERT(contextDescription);

    // Setup the data for implementation.
    memset(context, 0, sizeof(FfxFsr3UpscalerContext_Private));
    context->device = contextDescription->backendInterface.device;

    memcpy(&context->contextDescription, contextDescription, sizeof(FfxFsr3UpscalerContextDescription));

    // Check version info - make sure we are linked with the right backend version
    FfxVersionNumber version = context->contextDescription.backendInterface.fpGetSDKVersion(&context->contextDescription.backendInterface);
    FFX_RETURN_ON_ERROR(version == FFX_SDK_MAKE_VERSION(2, 0, 0), FFX_ERROR_INVALID_VERSION);

    // Create the context.
    FfxErrorCode errorCode = context->contextDescription.backendInterface.fpCreateBackendContext(&context->contextDescription.backendInterface, FFX_EFFECT_FSR3UPSCALER, nullptr, &context->effectContextId);
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);

#ifndef _GAMING_XBOX
    size_t envVarExists = 0;
    getenv_s(&envVarExists, nullptr, 0, "MLSR-WATERMARK");
    if (envVarExists)
    {
        context->watermark.emplace(&context->contextDescription.backendInterface, context->effectContextId);
    }
#endif

    // call out for device caps.
    errorCode = context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &context->deviceCapabilities);
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);

    // set defaults
    context->firstExecution = true;
    context->resourceFrameIndex = 0;

    context->constants.maxUpscaleSize[0] = contextDescription->maxUpscaleSize.width;
    context->constants.maxUpscaleSize[1] = contextDescription->maxUpscaleSize.height;
    context->constants.velocityFactor = 1.0f;
    context->constants.reactivenessScale = 1.0f;
    context->constants.shadingChangeScale = 1.0f;
    context->constants.accumulationAddedPerFrame = 1.0f/3.0f;
    context->constants.minDisocclusionAccumulation = -1.0f/3.0f;

    FfxFsr3UpscalerInternalResourceDescriptions fsr3UpscalerResourceDescs = {};
    getInternalResourceDescriptions(&contextDescription->maxRenderSize, &contextDescription->maxUpscaleSize, &fsr3UpscalerResourceDescs);

    // Array of pointers to each resource description member
    const FfxCreateResourceDescription* resources[] = {
        &fsr3UpscalerResourceDescs.accumulation1,
        &fsr3UpscalerResourceDescs.accumulation2,
        &fsr3UpscalerResourceDescs.luma1,
        &fsr3UpscalerResourceDescs.luma2,
        &fsr3UpscalerResourceDescs.intermediateFp16x1,
        &fsr3UpscalerResourceDescs.shadingChange,
        &fsr3UpscalerResourceDescs.newLocks,
        &fsr3UpscalerResourceDescs.internalUpscaled1,
        &fsr3UpscalerResourceDescs.internalUpscaled2,
        &fsr3UpscalerResourceDescs.spdMips,
        &fsr3UpscalerResourceDescs.farthestDepthMip1,
        &fsr3UpscalerResourceDescs.lumaHistory1,
        &fsr3UpscalerResourceDescs.lumaHistory2,
        &fsr3UpscalerResourceDescs.spdAtomicCounter,
        &fsr3UpscalerResourceDescs.dilatedReactiveMasks,
        &fsr3UpscalerResourceDescs.lanczosLutData,
        &fsr3UpscalerResourceDescs.defaultReactivityMask,
        &fsr3UpscalerResourceDescs.defaultExposure,
        &fsr3UpscalerResourceDescs.frameInfo,
    };

    // clear the SRV resources to NULL.
    memset(context->srvResources, 0, sizeof(context->srvResources));

    const size_t resourceCount = sizeof(resources) / sizeof(resources[0]);
    for (size_t i = 0; i < resourceCount; ++i)
    {
        const FfxCreateResourceDescription* createResourceDescription = resources[i];
        FFX_VALIDATE(context->contextDescription.backendInterface.fpCreateResource(&context->contextDescription.backendInterface, createResourceDescription, context->effectContextId, &context->srvResources[createResourceDescription->id]));
    }

    // copy resources to uavResrouces list
    memcpy(context->uavResources, context->srvResources, sizeof(context->srvResources));

    // avoid compiling pipelines on first render
    {
        errorCode = createPipelineStates(context);
        FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);
    }

    return FFX_OK;
}

static FfxErrorCode fsr3upscalerRelease(FfxFsr3UpscalerContext_Private* context)
{
    FFX_ASSERT(context);

    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelinePrepareInputs, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelinePrepareReactivity, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineShadingChange, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineAccumulate, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineAccumulateSharpen, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineRCAS, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineLumaPyramid, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineGenerateReactive, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineTcrAutogenerate, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineShadingChangePyramid, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineLumaInstability, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineDebugView, context->effectContextId);
#ifndef _GAMING_XBOX
    context->watermark = std::nullopt; // calls destructor
#endif

    // Unregister external resources
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK] = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH]    = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS]                  = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH]                           = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY]                       = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR]                             = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_DEPTH]                             = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS]                    = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE]                          = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK]                     = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };

    // Unregister references
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION]                            = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR]                 = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT]                              = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT]                         = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_CURRENT_LUMA]                            = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREVIOUS_LUMA]                           = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH]                          = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_INSTABILITY]                        = { FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NULL };

    // Release the copy resources for those that had init data
    ffxSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT], context->effectContextId);
    ffxSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LANCZOS_LUT], context->effectContextId);
    ffxSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY], context->effectContextId);
    ffxSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE], context->effectContextId);

    // release internal resources
    for (int32_t currentResourceIndex = 0; currentResourceIndex < FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_COUNT; ++currentResourceIndex) {

        ffxSafeReleaseResource(&context->contextDescription.backendInterface, context->srvResources[currentResourceIndex], context->effectContextId);
    }

    // Destroy the context
    context->contextDescription.backendInterface.fpDestroyBackendContext(&context->contextDescription.backendInterface, context->effectContextId);

    return FFX_OK;
}

static void setupDeviceDepthToViewSpaceDepthParams(FfxFsr3UpscalerContext_Private* context, const FfxFsr3UpscalerDispatchDescription* params)
{
    const bool bInverted = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) == FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED;
    const bool bInfinite = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE) == FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE;

    // make sure it has no impact if near and far plane values are swapped in dispatch params
    // the flags "inverted" and "infinite" will decide what transform to use
    float fMin = FFX_MINIMUM(params->cameraNear, params->cameraFar);
    float fMax = FFX_MAXIMUM(params->cameraNear, params->cameraFar);

    if (bInverted) {
        float tmp = fMin;
        fMin = fMax;
        fMax = tmp;
    }

    // a 0 0 0   x
    // 0 b 0 0   y
    // 0 0 c d   z
    // 0 0 e 0   1

    const float fQ = fMax / (fMin - fMax);
    const float d = -1.0f; // for clarity

    const float matrix_elem_c[2][2] = {
        fQ,                     // non reversed, non infinite
        -1.0f - FLT_EPSILON,    // non reversed, infinite
        fQ,                     // reversed, non infinite
        0.0f + FLT_EPSILON      // reversed, infinite
    };

    const float matrix_elem_e[2][2] = {
        fQ * fMin,             // non reversed, non infinite
        -fMin - FLT_EPSILON,    // non reversed, infinite
        fQ * fMin,             // reversed, non infinite
        fMax,                  // reversed, infinite
    };

    context->constants.deviceToViewDepth[0] = d * matrix_elem_c[bInverted][bInfinite];
    context->constants.deviceToViewDepth[1] = matrix_elem_e[bInverted][bInfinite];

    // revert x and y coords
    const float aspect = params->renderSize.width / float(params->renderSize.height);
    const float cotHalfFovY = cosf(0.5f * params->cameraFovAngleVertical) / sinf(0.5f * params->cameraFovAngleVertical);
    const float a = cotHalfFovY / aspect;
    const float b = cotHalfFovY;

    context->constants.deviceToViewDepth[2] = (1.0f / a);
    context->constants.deviceToViewDepth[3] = (1.0f / b);
}

static void scheduleDispatch(FfxFsr3UpscalerContext_Private* context, const FfxFsr3UpscalerDispatchDescription*, const FfxPipelineState* pipeline, uint32_t dispatchX, uint32_t dispatchY)
{
    FfxComputeJobDescription jobDescriptor = {};

    for (uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex) {

        const uint32_t currentResourceId = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
        const FfxResourceInternal currentResource = context->srvResources[currentResourceId];
        jobDescriptor.srvTextures[currentShaderResourceViewIndex].resource = currentResource;
#ifdef FFX_DEBUG
        wcscpy_s(jobDescriptor.srvTextures[currentShaderResourceViewIndex].name, pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
#endif
    }

    for (uint32_t currentUnorderedAccessViewIndex = 0; currentUnorderedAccessViewIndex < pipeline->uavTextureCount; ++currentUnorderedAccessViewIndex) {

        const uint32_t currentResourceId = pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].resourceIdentifier;
#ifdef FFX_DEBUG
        wcscpy_s(jobDescriptor.uavTextures[currentUnorderedAccessViewIndex].name, pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].name);
#endif

        if (currentResourceId >= FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_0 && currentResourceId <= FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_5)
        {
            const FfxResourceInternal currentResource = context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS];
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex].resource = currentResource;
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex].mip      = currentResourceId - FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS_LEVEL_0;
        }
        else
        {
            const FfxResourceInternal currentResource = context->uavResources[currentResourceId];
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex].resource = currentResource;
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex].mip = 0;
        }
    }

    jobDescriptor.dimensions[0] = dispatchX;
    jobDescriptor.dimensions[1] = dispatchY;
    jobDescriptor.dimensions[2] = 1;
    jobDescriptor.pipeline = *pipeline;

    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex) {
#ifdef FFX_DEBUG
        wcscpy_s( jobDescriptor.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
#endif
        jobDescriptor.cbs[currentRootConstantIndex] = context->constantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
    }

    FfxGpuJobDescription dispatchJob = { FFX_GPU_JOB_COMPUTE };
    wcscpy_s(dispatchJob.jobLabel, pipeline->name);
    dispatchJob.computeJobDescriptor = jobDescriptor;

    context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &dispatchJob);
}

FFX_API FfxErrorCode ffxFsr3UpscalerGetSharedResourceDescriptions(FfxFsr3UpscalerContext* context, FfxFsr3UpscalerSharedResourceDescriptions* SharedResources)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        SharedResources,
        FFX_ERROR_INVALID_POINTER);
    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);

    getSharedResourceDescriptions(&contextPrivate->contextDescription.maxRenderSize, &contextPrivate->contextDescription.maxUpscaleSize, SharedResources);

    return FFX_OK;
}

static FfxErrorCode fsr3upscalerDispatch(FfxFsr3UpscalerContext_Private* context, const FfxFsr3UpscalerDispatchDescription* params)
{

    if ((context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEBUG_CHECKING) == FFX_FSR3UPSCALER_ENABLE_DEBUG_CHECKING)
    {
        fsr3upscalerDebugCheckDispatch(context, params);
    }

    // take a short cut to the command list
    FfxCommandList commandList = params->commandList;

    if (context->firstExecution)
    {
        FfxGpuJobDescription clearJob = { FFX_GPU_JOB_CLEAR_FLOAT };
        
        const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
        memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));

        wcscpy_s(clearJob.jobLabel, L"Clear Accumulation 1");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_1];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        wcscpy_s(clearJob.jobLabel, L"Clear Accumulation 2");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_2];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

        wcscpy_s(clearJob.jobLabel, L"Clear Temporal Luma 1");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_1];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        wcscpy_s(clearJob.jobLabel, L"Clear Temporal Luma 2");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_2];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
    }

    // Prepare per frame descriptor tables
    const bool isOddFrame = !!(context->resourceFrameIndex & 1);
    const uint32_t currentCpuOnlyTableBase = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_COUNT : 0;
    const uint32_t currentGpuTableBase = 2 * FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_COUNT * context->resourceFrameIndex;
    const uint32_t accumulationSrvResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_2 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_1;
    const uint32_t accumulationUavResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_1 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION_2;
    const uint32_t upscaledColorSrvResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1;
    const uint32_t upscaledColorUavResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2;
    const uint32_t lumaHistorySrvResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_2 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_1;
    const uint32_t lumaHistoryUavResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_1 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY_2;
    const uint32_t currentLumaSrvResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_2 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_1;
    const uint32_t currentLumaUavResourceIndex = currentLumaSrvResourceIndex;
    const uint32_t previousLumaSrvResourceIndex = isOddFrame ? FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_1 : FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_2;

    const bool resetAccumulation = params->reset || context->firstExecution;
    context->firstExecution = false;

    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->color, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->depth, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_DEPTH]);
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->motionVectors, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS]);

    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->dilatedMotionVectors, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS]);
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS];
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->dilatedDepth, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH]);
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH];
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->reconstructedPrevNearestDepth, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH]);
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH];

    // if auto exposure is enabled use the auto exposure SRV, otherwise what the app sends.
    if (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE) {
        context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FRAME_INFO];
    } else {
        if (ffxFsr3UpscalerResourceIsNull(params->exposure)) {
            context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE];
        } else {
            context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->exposure, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE]);
        }
    }

    if (ffxFsr3UpscalerResourceIsNull(params->reactive)) {
        context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY];
    }
    else {
        context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->reactive, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK]);
    }

    if (ffxFsr3UpscalerResourceIsNull(params->transparencyAndComposition)) {
        context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY];
    } else {
        context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->transparencyAndComposition, context->effectContextId, &context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK]);
    }

    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->output, context->effectContextId, &context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT]);
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION] = context->srvResources[accumulationSrvResourceIndex];
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR] = context->srvResources[upscaledColorSrvResourceIndex];
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_ACCUMULATION] = context->uavResources[accumulationUavResourceIndex];
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR] = context->uavResources[upscaledColorUavResourceIndex];
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT] = context->uavResources[upscaledColorUavResourceIndex];

    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_CURRENT_LUMA] = context->srvResources[currentLumaSrvResourceIndex];
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_CURRENT_LUMA] = context->uavResources[currentLumaUavResourceIndex];
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREVIOUS_LUMA] = context->srvResources[previousLumaSrvResourceIndex];

    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY] = context->uavResources[lumaHistoryUavResourceIndex];
    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY] = context->srvResources[lumaHistorySrvResourceIndex];

    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1];
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH] = context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1];

    context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_INSTABILITY] = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1];
    context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_INSTABILITY] = context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1];

    // actual resource size may differ from render/display resolution (e.g. due to Hw/API restrictions), so query the descriptor for UVs adjustment
    const FfxApiResourceDescription resourceDescInputColor = context->contextDescription.backendInterface.fpGetResourceDescription(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    const FfxApiResourceDescription resourceDescReactiveMask = context->contextDescription.backendInterface.fpGetResourceDescription(&context->contextDescription.backendInterface, context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK]);
    FFX_ASSERT(resourceDescInputColor.type == FFX_API_RESOURCE_TYPE_TEXTURE2D);

    context->constants.previousFrameJitterOffset[0] = context->constants.jitterOffset[0];
    context->constants.previousFrameJitterOffset[1] = context->constants.jitterOffset[1];
    context->constants.jitterOffset[0] = params->jitterOffset.x;
    context->constants.jitterOffset[1] = params->jitterOffset.y;

    context->constants.previousFrameRenderSize[0] = context->constants.renderSize[0];
    context->constants.previousFrameRenderSize[1] = context->constants.renderSize[1];
    context->constants.renderSize[0] = int32_t(params->renderSize.width ? params->renderSize.width   : resourceDescInputColor.width);
    context->constants.renderSize[1] = int32_t(params->renderSize.height ? params->renderSize.height : resourceDescInputColor.height);
    context->constants.maxRenderSize[0] = int32_t(context->contextDescription.maxRenderSize.width);
    context->constants.maxRenderSize[1] = int32_t(context->contextDescription.maxRenderSize.height);

    // compute the horizontal FOV for the shader from the vertical one.
    const float aspectRatio = (float)params->renderSize.width / (float)params->renderSize.height;
    const float cameraAngleHorizontal = atan(tan(params->cameraFovAngleVertical / 2) * aspectRatio) * 2;
    context->constants.tanHalfFOV = tanf(cameraAngleHorizontal * 0.5f);
    context->constants.viewSpaceToMetersFactor = (params->viewSpaceToMetersFactor > 0.0f) ? params->viewSpaceToMetersFactor : 1.0f;

    // compute params to enable device depth to view space depth computation in shader
    setupDeviceDepthToViewSpaceDepthParams(context, params);

    context->constants.previousFrameUpscaleSize[0] = context->constants.upscaleSize[0];
    context->constants.previousFrameUpscaleSize[1] = context->constants.upscaleSize[1];
    
    if (params->upscaleSize.height == 0 && params->upscaleSize.width == 0)
    {
        context->constants.upscaleSize[0] = context->contextDescription.maxUpscaleSize.width;
        context->constants.upscaleSize[1] = context->contextDescription.maxUpscaleSize.height;
    }
    else
    {
        context->constants.upscaleSize[0] = params->upscaleSize.width;
        context->constants.upscaleSize[1] = params->upscaleSize.height;
    }

    // To be updated if resource is larger than the actual image size
    context->constants.downscaleFactor[0]       = float(context->constants.renderSize[0]) / context->constants.upscaleSize[0];
    context->constants.downscaleFactor[1]       = float(context->constants.renderSize[1]) / context->constants.upscaleSize[1];

    // calculate pre-exposure relevant factors
    context->constants.deltaPreExposure = 1.0f;
    context->previousFramePreExposure = context->preExposure;
    context->preExposure = (params->preExposure != 0.0f) ? params->preExposure : 1.0f;

    if (context->previousFramePreExposure > 0.0f) {
        context->constants.deltaPreExposure = context->preExposure / context->previousFramePreExposure;
    }

    // motion vector data
    const int32_t* motionVectorsTargetSize = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS)
                                                 ? context->constants.upscaleSize
                                                 : context->constants.renderSize;

    context->constants.motionVectorScale[0] = (params->motionVectorScale.x / motionVectorsTargetSize[0]);
    context->constants.motionVectorScale[1] = (params->motionVectorScale.y / motionVectorsTargetSize[1]);

    // compute jitter cancellation
    if (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) {

        context->constants.motionVectorJitterCancellation[0] = (context->previousJitterOffset[0] - context->constants.jitterOffset[0]) / motionVectorsTargetSize[0];
        context->constants.motionVectorJitterCancellation[1] = (context->previousJitterOffset[1] - context->constants.jitterOffset[1]) / motionVectorsTargetSize[1];

        context->previousJitterOffset[0] = context->constants.jitterOffset[0];
        context->previousJitterOffset[1] = context->constants.jitterOffset[1];
    }

    // lock data, assuming jitter sequence length computation for now
    const int32_t jitterPhaseCount = ffxFsr3UpscalerGetJitterPhaseCount(params->renderSize.width, context->constants.upscaleSize[0]);

    // init on first frame
    if (resetAccumulation || context->constants.jitterPhaseCount == 0) {
        context->constants.jitterPhaseCount = (float)jitterPhaseCount;
    } else {
        const int32_t jitterPhaseCountDelta = (int32_t)(jitterPhaseCount - context->constants.jitterPhaseCount);
        if (jitterPhaseCountDelta > 0) {
            context->constants.jitterPhaseCount++;
        } else if (jitterPhaseCountDelta < 0) {
            context->constants.jitterPhaseCount--;
        }
    }

    // convert delta time to seconds and clamp to [0, 1].
    context->constants.deltaTime = FFX_MAXIMUM(0.0f, FFX_MINIMUM(1.0f, params->frameTimeDelta / 1000.0f));

    if (resetAccumulation) {
        context->constants.frameIndex = 0.0f;
    } else {
        context->constants.frameIndex += 1.0f;
    }

    // reactive mask bias
    const int32_t threadGroupWorkRegionDim = 8;
    const int32_t dispatchSrcX = (context->constants.renderSize[0] + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchSrcY = (context->constants.renderSize[1] + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchDstX = (context->constants.upscaleSize[0] + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchDstY = (context->constants.upscaleSize[1] + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchShadingChangePassX = (int32_t(context->constants.renderSize[0] * 0.5f) + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchShadingChangePassY = (int32_t(context->constants.renderSize[1] * 0.5f) + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

    // Clear reconstructed depth for max depth store.
    if (resetAccumulation) {

        FfxGpuJobDescription clearJob = { FFX_GPU_JOB_CLEAR_FLOAT };
        wcscpy_s(clearJob.jobLabel, L"Clear Resource");

        const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
        memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));
        clearJob.clearJobDescriptor.target = context->srvResources[accumulationSrvResourceIndex];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

        wcscpy_s(clearJob.jobLabel, L"Clear Scene Luminance");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

        // Auto exposure always used to track luma changes in locking logic
        {
            const float clearValuesExposure[]{ -1.f, 1.f, 0.f, 0.f };
            memcpy(clearJob.clearJobDescriptor.color, clearValuesExposure, 4 * sizeof(float));
            wcscpy_s(clearJob.jobLabel, L"Clear Frame Info");
            clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FRAME_INFO];
            context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        }
    }

    {
        FfxGpuJobDescription clearJob = {FFX_GPU_JOB_CLEAR_FLOAT};
        // FSR3: need to clear here since we need the content of this surface for frameinterpolation
        // so clearing in the lock pass is not an option
        const bool  bInverted = (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) == FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED;
        const float clearDepthValue[]{bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f};
        memcpy(clearJob.clearJobDescriptor.color, clearDepthValue, 4 * sizeof(float));
        wcscpy_s(clearJob.jobLabel, L"Clear Reconstructed Previous Nearest Depth");
        clearJob.clearJobDescriptor.target = context->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
    }

    // Suggested by Enduring to resolve issues with running FSR3 on console via the RHI backend in the plugin as this resource won't be cleared to 0 by default.
	{
		FfxGpuJobDescription clearJob = { FFX_GPU_JOB_CLEAR_FLOAT };
		wcscpy_s(clearJob.jobLabel, L"Clear Spd Atomic Count");
		const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
		memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));
		clearJob.clearJobDescriptor.target = context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT];
		context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
	}

    // Auto exposure
    uint32_t dispatchThreadGroupCountXY[2];
    uint32_t workGroupOffset[2];
    uint32_t numWorkGroupsAndMips[2];
    uint32_t rectInfo[4] = { 0, 0, params->renderSize.width, params->renderSize.height };
    ffxSpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

    // downsample
    Fsr3UpscalerSpdConstants luminancePyramidConstants;
    luminancePyramidConstants.numworkGroups = numWorkGroupsAndMips[0];
    luminancePyramidConstants.mips = numWorkGroupsAndMips[1];
    luminancePyramidConstants.workGroupOffset[0] = workGroupOffset[0];
    luminancePyramidConstants.workGroupOffset[1] = workGroupOffset[1];
    luminancePyramidConstants.renderSize[0] = params->renderSize.width;
    luminancePyramidConstants.renderSize[1] = params->renderSize.height;

    // compute the constants.
    Fsr3UpscalerRcasConstants rcasConsts = {};
    const float sharpenessRemapped = (-2.0f * params->sharpness) + 2.0f;
    FsrRcasCon(rcasConsts.rcasConfig, sharpenessRemapped);

    // initialize constantBuffers data
    context->contextDescription.backendInterface.fpStageConstantBufferDataFunc(&context->contextDescription.backendInterface, &context->constants,        sizeof(context->constants),        &context->constantBuffers[FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_FSR3UPSCALER]);
    context->contextDescription.backendInterface.fpStageConstantBufferDataFunc(&context->contextDescription.backendInterface, &luminancePyramidConstants, sizeof(luminancePyramidConstants), &context->constantBuffers[FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_SPD]);
    context->contextDescription.backendInterface.fpStageConstantBufferDataFunc(&context->contextDescription.backendInterface, &rcasConsts,                sizeof(rcasConsts),                &context->constantBuffers[FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_RCAS]);

    {
        FfxResourceInternal aliasableResources[] = {
            context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERMEDIATE_FP16x1],
            context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SHADING_CHANGE],
            context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS],
            // SPD_MIPS are an aliasable resource, but need to be cleared to prevent reading pixels that have never been written
            //context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS],
            context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_FARTHEST_DEPTH_MIP1],
            context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS],
        };
        for(int i = 0; i<_countof(aliasableResources); ++i)
        {
            FfxGpuJobDescription discardJob = { FFX_GPU_JOB_DISCARD };
            discardJob.discardJobDescriptor.target = aliasableResources[i];
            context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &discardJob);
        }
        // SPD counter needs to be cleared
        {
            FfxGpuJobDescription clearJob = { FFX_GPU_JOB_CLEAR_FLOAT };
            wcscpy_s(clearJob.jobLabel, L"Clear Spd Atomic Count");
            const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
            memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));
            clearJob.clearJobDescriptor.target = context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_MIPS];
            context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        }
    }

    scheduleDispatch(context, params, &context->pipelinePrepareInputs, dispatchSrcX, dispatchSrcY);
    scheduleDispatch(context, params, &context->pipelineLumaPyramid, dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1]);
    scheduleDispatch(context, params, &context->pipelineShadingChangePyramid, dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1]);
    scheduleDispatch(context, params, &context->pipelineShadingChange, dispatchShadingChangePassX, dispatchShadingChangePassY);
    scheduleDispatch(context, params, &context->pipelinePrepareReactivity, dispatchSrcX, dispatchSrcY);
    scheduleDispatch(context, params, &context->pipelineLumaInstability, dispatchSrcX, dispatchSrcY);

    scheduleDispatch(context, params, params->enableSharpening ? &context->pipelineAccumulateSharpen : &context->pipelineAccumulate, dispatchDstX, dispatchDstY);

    // RCAS
    if (params->enableSharpening)
    {

        // dispatch RCAS
        const int32_t threadGroupWorkRegionDimRCAS = 16;
        const int32_t dispatchX = (context->constants.upscaleSize[0] + (threadGroupWorkRegionDimRCAS - 1)) / threadGroupWorkRegionDimRCAS;
        const int32_t dispatchY = (context->constants.upscaleSize[1] + (threadGroupWorkRegionDimRCAS - 1)) / threadGroupWorkRegionDimRCAS;
        scheduleDispatch(context, params, &context->pipelineRCAS, dispatchX, dispatchY);
    }
    
    if (params->flags & FFX_FSR3UPSCALER_DISPATCH_DRAW_DEBUG_VIEW) {
        scheduleDispatch(context, params, &context->pipelineDebugView, dispatchDstX, dispatchDstY);
    }

#ifndef _GAMING_XBOX
    if (context->watermark)
    {
        const char* message;
        if (context->contextDescription.flags & FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE)
        {
            message = "FSR3 Upscale\nBuild Time: " __DATE__ " " __TIME__ "\nGit branch: " GIT_COMMIT_SUBJECT "  commit: " GIT_HASH "\nColorspace: LINEAR";
        }
        else
        {
            message = "FSR3 Upscale\nBuild Time: " __DATE__ " " __TIME__ "\nGit branch: " GIT_COMMIT_SUBJECT "  commit: " GIT_HASH "\nColorspace: NON-LINEAR";
        }
        context->watermark->Dispatch(context->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT], message);
    }
#endif

    context->resourceFrameIndex = (context->resourceFrameIndex + 1) % FSR3UPSCALER_MAX_QUEUED_FRAMES;

    // Fsr3UpscalerMaxQueuedFrames must be an even number.
    FFX_STATIC_ASSERT((FSR3UPSCALER_MAX_QUEUED_FRAMES & 1) == 0);

    context->contextDescription.backendInterface.fpExecuteGpuJobs(&context->contextDescription.backendInterface, commandList, context->effectContextId);

    // release dynamic resources
    context->contextDescription.backendInterface.fpUnregisterResources(&context->contextDescription.backendInterface, commandList, context->effectContextId);

    return FFX_OK;
}

FFX_API FfxErrorCode ffxFsr3UpscalerContextCreate(FfxFsr3UpscalerContext* context, const FfxFsr3UpscalerContextDescription* contextDescription)
{
    // zero context memory
    memset(context, 0, sizeof(FfxFsr3UpscalerContext));

    // check pointers are valid.
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        contextDescription,
        FFX_ERROR_INVALID_POINTER);

    // validate that all callbacks are set for the interface
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpGetSDKVersion, FFX_ERROR_INCOMPLETE_INTERFACE);
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpGetDeviceCapabilities, FFX_ERROR_INCOMPLETE_INTERFACE);
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpCreateBackendContext, FFX_ERROR_INCOMPLETE_INTERFACE);
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpDestroyBackendContext, FFX_ERROR_INCOMPLETE_INTERFACE);

    // if a scratch buffer is declared, then we must have a size
    if (contextDescription->backendInterface.scratchBuffer) {

        FFX_RETURN_ON_ERROR(contextDescription->backendInterface.scratchBufferSize, FFX_ERROR_INCOMPLETE_INTERFACE);
    }

    // ensure the context is large enough for the internal context.
    FFX_STATIC_ASSERT(sizeof(FfxFsr3UpscalerContext) >= sizeof(FfxFsr3UpscalerContext_Private));

    // create the context.
    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);
    const FfxErrorCode errorCode = fsr3upscalerCreate(contextPrivate, contextDescription);

    return errorCode;
}

FFX_API FfxErrorCode ffxFsr3UpscalerContextGetGpuMemoryUsage(FfxFsr3UpscalerContext* context, FfxApiEffectMemoryUsage* vramUsage)
{
    FFX_RETURN_ON_ERROR(context, FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(vramUsage, FFX_ERROR_INVALID_POINTER);
    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);

    FFX_RETURN_ON_ERROR(contextPrivate->device, FFX_ERROR_NULL_DEVICE);

    FfxErrorCode errorCode = contextPrivate->contextDescription.backendInterface.fpGetEffectGpuMemoryUsage(
        &contextPrivate->contextDescription.backendInterface, contextPrivate->effectContextId, vramUsage);
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);

    return FFX_OK;
}

FFX_API FfxErrorCode ffxFsr3UpscalerGetGpuMemoryUsage(FfxDevice device, FfxApiDimensions2D* maxRenderSize, FfxApiDimensions2D* maxUpscaleSize, FfxApiEffectMemoryUsage* pVramUsage)
{
    FfxFsr3UpscalerInternalResourceDescriptions fsr3UpscalerResourceDescs = {};
    getInternalResourceDescriptions(maxRenderSize, maxUpscaleSize, &fsr3UpscalerResourceDescs);

    // Array of pointers to each resource description member
    const FfxCreateResourceDescription* resources[] = {
        &fsr3UpscalerResourceDescs.accumulation1,
        &fsr3UpscalerResourceDescs.accumulation2,
        &fsr3UpscalerResourceDescs.luma1,
        &fsr3UpscalerResourceDescs.luma2,
        &fsr3UpscalerResourceDescs.intermediateFp16x1,
        &fsr3UpscalerResourceDescs.shadingChange,
        &fsr3UpscalerResourceDescs.newLocks,
        &fsr3UpscalerResourceDescs.internalUpscaled1,
        &fsr3UpscalerResourceDescs.internalUpscaled2,
        &fsr3UpscalerResourceDescs.spdMips,
        &fsr3UpscalerResourceDescs.farthestDepthMip1,
        &fsr3UpscalerResourceDescs.lumaHistory1,
        &fsr3UpscalerResourceDescs.lumaHistory2,
        &fsr3UpscalerResourceDescs.spdAtomicCounter,
        &fsr3UpscalerResourceDescs.dilatedReactiveMasks,
        &fsr3UpscalerResourceDescs.lanczosLutData,
        &fsr3UpscalerResourceDescs.defaultReactivityMask,
        &fsr3UpscalerResourceDescs.defaultExposure,
        &fsr3UpscalerResourceDescs.frameInfo,
    };

    uint64_t      size = 0;
    size_t resourceCount = sizeof(resources) / sizeof(resources[0]);
    for (size_t i = 0; i < resourceCount; ++i)
    {
#ifdef FFX_BACKEND_DX12
        FFX_VALIDATE(ffxGetResourceSizeFromDescriptionDX12(device, resources[i], &size));
#endif // FFX_BACKEND_DX12
        pVramUsage->totalUsageInBytes += size;
        if (resources[i]->resourceDescription.flags & FFX_API_RESOURCE_FLAGS_ALIASABLE)
        {
            pVramUsage->aliasableUsageInBytes += size;
        }
    }

    FfxFsr3UpscalerSharedResourceDescriptions fs3UpscalerSharedResourceDescs = {};
    getSharedResourceDescriptions(maxRenderSize, maxUpscaleSize, &fs3UpscalerSharedResourceDescs);

    // Array of pointers to each resource description member
    const FfxCreateResourceDescription* sharedResources[] = {
        &fs3UpscalerSharedResourceDescs.reconstructedPrevNearestDepth,
        &fs3UpscalerSharedResourceDescs.dilatedDepth,
        &fs3UpscalerSharedResourceDescs.dilatedMotionVectors,
    };

    resourceCount = sizeof(sharedResources) / sizeof(sharedResources[0]);
    for (size_t i = 0; i < resourceCount; ++i)
    {
#ifdef FFX_BACKEND_DX12
        FFX_VALIDATE(ffxGetResourceSizeFromDescriptionDX12(device, sharedResources[i], &size));
#endif // FFX_BACKEND_DX12
        pVramUsage->totalUsageInBytes += size;
        if (resources[i]->resourceDescription.flags & FFX_API_RESOURCE_FLAGS_ALIASABLE)
        {
            pVramUsage->aliasableUsageInBytes += size;
        }
    }
    
    return FFX_OK;
}


FfxErrorCode ffxFsr3UpscalerContextDestroy(FfxFsr3UpscalerContext* context)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);

    // destroy the context.
    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);
    const FfxErrorCode errorCode = fsr3upscalerRelease(contextPrivate);
    return errorCode;
}

FfxErrorCode ffxFsr3UpscalerContextDispatch(FfxFsr3UpscalerContext* context, const FfxFsr3UpscalerDispatchDescription* dispatchParams)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        dispatchParams,
        FFX_ERROR_INVALID_POINTER);

    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);

    // validate that renderSize is within the maximum.
    FFX_RETURN_ON_ERROR(
        dispatchParams->renderSize.width <= contextPrivate->contextDescription.maxRenderSize.width,
        FFX_ERROR_OUT_OF_RANGE);
    FFX_RETURN_ON_ERROR(
        dispatchParams->renderSize.height <= contextPrivate->contextDescription.maxRenderSize.height,
        FFX_ERROR_OUT_OF_RANGE);
    FFX_RETURN_ON_ERROR(
        dispatchParams->upscaleSize.width <= contextPrivate->contextDescription.maxUpscaleSize.width,
        FFX_ERROR_OUT_OF_RANGE);
    FFX_RETURN_ON_ERROR(
        dispatchParams->upscaleSize.height <= contextPrivate->contextDescription.maxUpscaleSize.height,
        FFX_ERROR_OUT_OF_RANGE);
    FFX_RETURN_ON_ERROR(
        contextPrivate->device,
        FFX_ERROR_NULL_DEVICE);

    // dispatch the FSR3 passes.
    const FfxErrorCode errorCode = fsr3upscalerDispatch(contextPrivate, dispatchParams);
    return errorCode;
}
#endif // #ifndef BUILD_AMDXCFFX

FFX_API float ffxFsr3UpscalerGetUpscaleRatioFromQualityMode(FfxApiUpscaleQualityMode qualityMode)
{
    switch (qualityMode) {
    case FFX_UPSCALE_QUALITY_MODE_NATIVEAA:
        return 1.0f;
    case FFX_UPSCALE_QUALITY_MODE_QUALITY:
        return 1.5f;
    case FFX_UPSCALE_QUALITY_MODE_BALANCED:
        return 1.7f;
    case FFX_UPSCALE_QUALITY_MODE_PERFORMANCE:
        return 2.0f;
    case FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE:
        return 3.0f;
    default:
        return 0.0f;
    }
}

FFX_API FfxErrorCode ffxFsr3UpscalerGetRenderResolutionFromQualityMode(
    uint32_t* renderWidth,
    uint32_t* renderHeight,
    uint32_t displayWidth,
    uint32_t displayHeight,
    FfxApiUpscaleQualityMode qualityMode)
{
    FFX_RETURN_ON_ERROR(
        renderWidth,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        renderHeight,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        FFX_UPSCALE_QUALITY_MODE_NATIVEAA <= qualityMode && qualityMode <= FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE,
        FFX_ERROR_INVALID_ENUM);

    // scale by the predefined ratios in each dimension.
    const float ratio = ffxFsr3UpscalerGetUpscaleRatioFromQualityMode(qualityMode);
    const uint32_t scaledDisplayWidth = (uint32_t)((float)displayWidth / ratio);
    const uint32_t scaledDisplayHeight = (uint32_t)((float)displayHeight / ratio);
    *renderWidth = scaledDisplayWidth;
    *renderHeight = scaledDisplayHeight;

    return FFX_OK;
}

int32_t ffxFsr3UpscalerGetJitterPhaseCount(int32_t renderWidth, int32_t displayWidth)
{
    const float basePhaseCount = 8.0f;
    const int32_t jitterPhaseCount = int32_t(basePhaseCount * pow((float(displayWidth) / renderWidth), 2.0f));
    return jitterPhaseCount;
}

FfxErrorCode ffxFsr3UpscalerGetJitterOffset(float* outX, float* outY, int32_t index, int32_t phaseCount)
{
    FFX_RETURN_ON_ERROR(
        outX,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        outY,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        phaseCount > 0,
        FFX_ERROR_INVALID_ARGUMENT);

    const float x = halton((index % phaseCount) + 1, 2) - 0.5f;
    const float y = halton((index % phaseCount) + 1, 3) - 0.5f;

    *outX = x;
    *outY = y;
    return FFX_OK;
}

FFX_API bool ffxFsr3UpscalerResourceIsNull(FfxApiResource resource)
{
    return resource.resource == NULL;
}

FfxErrorCode ffxFsr3UpscalerContextGenerateReactiveMask(FfxFsr3UpscalerContext* context, const FfxFsr3UpscalerGenerateReactiveDescription* params)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        params,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        params->commandList,
        FFX_ERROR_INVALID_POINTER);
    
    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);

    FFX_RETURN_ON_ERROR(
        contextPrivate->device,
        FFX_ERROR_NULL_DEVICE);

    // take a short cut to the command list
    FfxCommandList commandList = params->commandList;

    FfxPipelineState* pipeline = &contextPrivate->pipelineGenerateReactive;

    const int32_t threadGroupWorkRegionDim = 8;
    const int32_t dispatchSrcX = (params->renderSize.width  + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    const int32_t dispatchSrcY = (params->renderSize.height + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

    FfxComputeJobDescription jobDescriptor = {};
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->colorOpaqueOnly, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->colorPreUpscale, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->outReactive, contextPrivate->effectContextId, &contextPrivate->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTOREACTIVE]);
    
    jobDescriptor.uavTextures[0].resource = contextPrivate->uavResources[FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTOREACTIVE];

#ifdef FFX_DEBUG
    wcscpy_s(jobDescriptor.srvTextures[0].name, pipeline->srvTextureBindings[0].name);
    wcscpy_s(jobDescriptor.srvTextures[1].name, pipeline->srvTextureBindings[1].name);
    wcscpy_s(jobDescriptor.uavTextures[0].name, pipeline->uavTextureBindings[0].name);
#endif

    jobDescriptor.dimensions[0] = dispatchSrcX;
    jobDescriptor.dimensions[1] = dispatchSrcY;
    jobDescriptor.dimensions[2] = 1;
    jobDescriptor.pipeline = *pipeline;

    for (uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex) {

        const uint32_t currentResourceId = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
        const FfxResourceInternal currentResource = contextPrivate->srvResources[currentResourceId];
        jobDescriptor.srvTextures[currentShaderResourceViewIndex].resource = currentResource;
#ifdef FFX_DEBUG
        wcscpy_s(jobDescriptor.srvTextures[currentShaderResourceViewIndex].name, pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
#endif
    }

    Fsr3UpscalerGenerateReactiveConstants genReactiveConsts = {};
    genReactiveConsts.scale = params->scale;
    genReactiveConsts.threshold = params->cutoffThreshold;
    genReactiveConsts.binaryValue = params->binaryValue;
    genReactiveConsts.flags = params->flags;

    contextPrivate->contextDescription.backendInterface.fpStageConstantBufferDataFunc(&contextPrivate->contextDescription.backendInterface, &genReactiveConsts, sizeof(genReactiveConsts), &contextPrivate->constantBuffers[FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_GENREACTIVE]);

    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex)
    {
#ifdef FFX_DEBUG
        wcscpy_s(jobDescriptor.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
#endif
        jobDescriptor.cbs[currentRootConstantIndex] = contextPrivate->constantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
    }

    FfxGpuJobDescription dispatchJob = { FFX_GPU_JOB_COMPUTE };
    dispatchJob.computeJobDescriptor = jobDescriptor;

    contextPrivate->contextDescription.backendInterface.fpScheduleGpuJob(&contextPrivate->contextDescription.backendInterface, &dispatchJob);

    contextPrivate->contextDescription.backendInterface.fpExecuteGpuJobs(&contextPrivate->contextDescription.backendInterface, commandList, contextPrivate->effectContextId);

    // release dynamic resources
    contextPrivate->contextDescription.backendInterface.fpUnregisterResources(&contextPrivate->contextDescription.backendInterface, commandList, contextPrivate->effectContextId);

    return FFX_OK;
}

FFX_API FfxErrorCode ffxFsr3UpscalerSetConstant(FfxFsr3UpscalerContext* context, FfxFsr3UpscalerConfigureKey key, void* valuePtr)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);

    FfxFsr3UpscalerContext_Private* contextPrivate = (FfxFsr3UpscalerContext_Private*)(context);
    switch (key)
    {
        case FFX_FSR3UPSCALER_CONFIGURE_UPSCALE_KEY_FVELOCITYFACTOR:
        {
            float fValue = 1.0f;
            if (valuePtr != nullptr)
            {
                fValue = *(static_cast<float*>(valuePtr));
            }
            contextPrivate->constants.velocityFactor = ffxSaturate(fValue);
            break;
        }
        case FFX_FSR3UPSCALER_CONFIGURE_UPSCALE_KEY_FREACTIVENESSSCALE:
        {
            float fValue = 1.0f;
            if (valuePtr != nullptr)
            {
                fValue = *(static_cast<float*>(valuePtr));
            }
            contextPrivate->constants.reactivenessScale = ffxMax(0.f,fValue);
            break;
        }
        case FFX_FSR3UPSCALER_CONFIGURE_UPSCALE_KEY_FSHADINGCHANGESCALE:
        {
            float fValue = 1.0f;
            if (valuePtr != nullptr)
            {
                fValue = *(static_cast<float*>(valuePtr));
            }
            contextPrivate->constants.shadingChangeScale = ffxMax(0.f,fValue);
            break;
        }
        case FFX_FSR3UPSCALER_CONFIGURE_UPSCALE_KEY_FACCUMULATIONADDEDPERFRAME:
        {
            float fValue = 1.0f/3.0f;
            if (valuePtr != nullptr)
            {
                fValue = *(static_cast<float*>(valuePtr));
            }
            contextPrivate->constants.accumulationAddedPerFrame = ffxSaturate(fValue);
            break;
        }
        case FFX_FSR3UPSCALER_CONFIGURE_UPSCALE_KEY_FMINDISOCCLUSIONACCUMULATION:
        {
            float fValue = -1.0f/3.0f;
            if (valuePtr != nullptr)
            {
                fValue = *(static_cast<float*>(valuePtr));
            }
            contextPrivate->constants.minDisocclusionAccumulation = ffxMin(1.0f, ffxMax(-1.0f, fValue));
            break;
        }
        default:
            return FFX_ERROR_INVALID_ENUM;
    }
    return FFX_OK;
}

FFX_API FfxErrorCode ffxFsr3UpscalerSetGlobalDebugMessage(ffxMessageCallback fpMessage, uint32_t debugLevel)
{
    ffxSetPrintMessageCallback(fpMessage, debugLevel);
    return FFX_OK;
}

#pragma warning(pop)
