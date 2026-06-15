// Copyright  © 2023 Advanced Micro Devices, Inc.
// Copyright  © 2024-2025 Arm Limited.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>    // for max used inside SPD CPU code.
#include <cmath>        // for fabs, abs, sinf, sqrt, etc.
#include <string.h>     // for memset
#include <cfloat>       // for FLT_EPSILON
#include "ffxm_fsr2.h"
#define FFXM_CPU
#include "ffxm_core.h"
#include "fsr1/ffxm_fsr1.h"
#include "spd/ffxm_spd.h"
#include "fsr2/ffxm_fsr2_callbacks_hlsl.h"
#include "fsr2/ffxm_fsr2_common.h"
#include "src/shared/ffxm_object_management.h"

#include "ffxm_fsr2_maximum_bias.h"
#include "ffxm_util.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include "ffxm_fsr2_private.h"

namespace arm
{
// max queued frames for descriptor management
static const uint32_t FSR2_MAX_QUEUED_FRAMES = 16;

// lists to map shader resource bindpoint name to resource identifier
typedef struct ResourceBinding
{
    uint32_t    index;
    wchar_t     name[64];
}ResourceBinding;

static const ResourceBinding srvTextureBindingTable[] =
{
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR,                              L"r_input_color_jittered"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY,                        L"r_input_opaque_only"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS,                     L"r_input_motion_vectors"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_DEPTH,                              L"r_input_depth" },
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE,                           L"r_input_exposure"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE,                            L"r_auto_exposure"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK,                      L"r_reactive_mask"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK,  L"r_transparency_and_composition_mask"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH,     L"r_reconstructed_previous_nearest_depth"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                   L"r_dilated_motion_vectors"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_PREVIOUS_DILATED_MOTION_VECTORS,          L"r_previous_dilated_motion_vectors"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH,                            L"r_dilatedDepth"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR,                  L"r_internal_upscaled_color"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE,				 L"r_internal_temporal_reactive"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS,                              L"r_lock_status"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR,                     L"r_prepared_input_color"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY,                             L"r_luma_history" },
    {FFXM_FSR2_RESOURCE_IDENTIFIER_RCAS_INPUT,                               L"r_rcas_input"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LANCZOS_LUT,                              L"r_lanczos_lut"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE,                          L"r_imgMips"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_SHADING_CHANGE,    L"r_img_mip_shading_change"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_5,                 L"r_img_mip_5"},
    {FFXM_FSR2_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT,                L"r_upsample_maximum_bias_lut"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS,                   L"r_dilated_reactive_masks"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_NEW_LOCKS,                                L"r_new_locks"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA,                          L"r_lock_input_luma"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA,  L"r_dilated_depth_motion_vectors_input_luma"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_PREV_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA, L"r_prev_dilated_depth_motion_vectors_input_luma"},
};

static const ResourceBinding uavTextureBindingTable[] =
{
    {FFXM_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH,    L"rw_reconstructed_previous_nearest_depth"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                  L"rw_dilated_motion_vectors"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH,                           L"rw_dilatedDepth"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR,                 L"rw_internal_upscaled_color"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS,                             L"rw_lock_status"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR,                    L"rw_prepared_input_color"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY,                            L"rw_luma_history"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT,                         L"rw_upscaled_output"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_SHADING_CHANGE,   L"rw_img_mip_shading_change"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_5,                L"rw_img_mip_5"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS,                  L"rw_dilated_reactive_masks"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE,                           L"rw_auto_exposure"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT,                        L"rw_spd_global_atomic"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_NEW_LOCKS,                               L"rw_new_locks"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA,                         L"rw_lock_input_luma"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE,                            L"rw_output_autoreactive"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA, L"rw_dilated_depth_motion_vectors_input_luma"},
};

static const ResourceBinding rtTextureBindingTable[] = {
	{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR, L"rw_internal_upscaled_color"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE, L"rw_internal_temporal_reactive"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS, L"rw_lock_status"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY, L"rw_luma_history"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT, L"rw_upscaled_output"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS, L"rw_dilated_reactive_masks"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR, L"rw_prepared_input_color"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS, L"rw_dilated_motion_vectors"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH, L"rw_dilatedDepth"},
	{FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA, L"rw_lock_input_luma"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE, L"rw_output_autoreactive"},
    {FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA, L"rw_dilated_depth_motion_vectors_input_luma"},
};

static const ResourceBinding constantBufferBindingTable[] =
{
    {FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_FSR2,           L"cbFSR2"},
    {FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_SPD,            L"cbSPD"},
    {FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_RCAS,           L"cbRCAS"},
    {FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_GENREACTIVE,    L"cbGenerateReactive"},
};

#define FFXM_COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

// Broad structure of the root signature.
/*typedef enum Fsr2RootSignatureLayout {

    FSR2_ROOT_SIGNATURE_LAYOUT_UAVS,
    FSR2_ROOT_SIGNATURE_LAYOUT_SRVS,
    FSR2_ROOT_SIGNATURE_LAYOUT_CONSTANTS,
    FSR2_ROOT_SIGNATURE_LAYOUT_CONSTANTS_REGISTER_1,
    FSR2_ROOT_SIGNATURE_LAYOUT_PARAMETER_COUNT
} Fsr2RootSignatureLayout;*/

typedef struct Fsr2RcasConstants {

    uint32_t                    rcasConfig[4];
} FfxmRcasConstants;

typedef struct Fsr2SpdConstants {

    uint32_t                    mips;
    uint32_t                    numworkGroups;
    uint32_t                    workGroupOffset[2];
    uint32_t                    renderSize[2];
} Fsr2SpdConstants;

typedef struct Fsr2GenerateReactiveConstants
{
    float       scale;
    float       threshold;
    float       binaryValue;
    uint32_t    flags;

} Fsr2GenerateReactiveConstants;

typedef union Fsr2SecondaryUnion {

    Fsr2RcasConstants               rcas;
    Fsr2SpdConstants                spd;
} Fsr2SecondaryUnion;

// Lanczos
static float lanczos2(float value)
{
    return abs(value) < FFXM_EPSILON ? 1.f : (sinf(FFXM_PI * value) / (FFXM_PI * value)) * (sinf(0.5f * FFXM_PI * value) / (0.5f * FFXM_PI * value));
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

static void fsr2DebugCheckDispatch(FfxmFsr2Context_Private* context, const FfxmFsr2DispatchDescription* params)
{
    if (params->commandList == nullptr)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"commandList is null");
    }

    if (params->color.resource == nullptr)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"color resource is null");
    }

    if (params->depth.resource == nullptr)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"depth resource is null");
    }

    if (params->motionVectors.resource == nullptr)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"motionVectors resource is null");
    }

    if (params->exposure.resource != nullptr)
    {
        if ((context->contextDescription.flags & FFXM_FSR2_ENABLE_AUTO_EXPOSURE) == FFXM_FSR2_ENABLE_AUTO_EXPOSURE)
        {
            context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"exposure resource provided, however auto exposure flag is present");
        }
    }

    if (params->output.resource == nullptr)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"output resource is null");
    }

    if (fabs(params->jitterOffset.x) > 1.0f || fabs(params->jitterOffset.y) > 1.0f)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"jitterOffset contains value outside of expected range [-1.0, 1.0]");
    }

    if ((params->motionVectorScale.x > (float)context->contextDescription.maxRenderSize.width) ||
        (params->motionVectorScale.y > (float)context->contextDescription.maxRenderSize.height))
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"motionVectorScale contains scale value greater than maxRenderSize");
    }
    if ((params->motionVectorScale.x == 0.0f) ||
        (params->motionVectorScale.y == 0.0f))
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"motionVectorScale contains zero scale value");
    }

    if ((params->renderSize.width > context->contextDescription.maxRenderSize.width) ||
        (params->renderSize.height > context->contextDescription.maxRenderSize.height))
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"renderSize is greater than context maxRenderSize");
    }
    if ((params->renderSize.width == 0) ||
        (params->renderSize.height == 0))
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"renderSize contains zero dimension");
    }

    if (params->sharpness < 0.0f || params->sharpness > 1.0f)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"sharpness contains value outside of expected range [0.0, 1.0]");
    }

    if (params->frameTimeDelta < 1.0f)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING, L"frameTimeDelta is less than 1.0f - this value should be milliseconds (~16.6f for 60fps)");
    }

    if (params->preExposure == 0.0f)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"preExposure provided as 0.0f which is invalid");
    }

    bool infiniteDepth = (context->contextDescription.flags & FFXM_FSR2_ENABLE_DEPTH_INFINITE) == FFXM_FSR2_ENABLE_DEPTH_INFINITE;
    bool inverseDepth = (context->contextDescription.flags & FFXM_FSR2_ENABLE_DEPTH_INVERTED) == FFXM_FSR2_ENABLE_DEPTH_INVERTED;

    if (inverseDepth)
    {
        if (params->cameraNear < params->cameraFar)
        {
            context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                L"FFXM_FSR2_ENABLE_DEPTH_INVERTED flag is present yet cameraNear is less than cameraFar");
        }
        if (infiniteDepth)
        {
            if (params->cameraNear != FLT_MAX)
            {
                context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                    L"FFXM_FSR2_ENABLE_DEPTH_INFINITE and FFXM_FSR2_ENABLE_DEPTH_INVERTED present, yet cameraNear != FLT_MAX");
            }
        }
        if (params->cameraFar < 0.075f)
        {
            context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                L"FFXM_FSR2_ENABLE_DEPTH_INFINITE and FFXM_FSR2_ENABLE_DEPTH_INVERTED present, cameraFar value is very low which may result in depth separation artefacting");
        }
    }
    else
    {
        if (params->cameraNear > params->cameraFar)
        {
            context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                L"cameraNear is greater than cameraFar in non-inverted-depth context");
        }
        if (infiniteDepth)
        {
            if (params->cameraFar != FLT_MAX)
            {
                context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                    L"FFXM_FSR2_ENABLE_DEPTH_INFINITE and FFXM_FSR2_ENABLE_DEPTH_INVERTED present, yet cameraFar != FLT_MAX");
            }
        }
        if (params->cameraNear < 0.075f)
        {
            context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_WARNING,
                L"FFXM_FSR2_ENABLE_DEPTH_INFINITE and FFXM_FSR2_ENABLE_DEPTH_INVERTED present, cameraNear value is very low which may result in depth separation artefacting");
        }
    }

    if (params->cameraFovAngleVertical <= 0.0f)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"cameraFovAngleVertical is 0.0f - this value should be > 0.0f");
    }
    if (params->cameraFovAngleVertical > FFXM_PI)
    {
        context->contextDescription.fpMessage(FFXM_MESSAGE_TYPE_ERROR, L"cameraFovAngleVertical is greater than 180 degrees/PI");
    }
}

static FfxmErrorCode patchResourceBindings(FfxmPipelineState* inoutPipeline)
{
    for (uint32_t srvIndex = 0; srvIndex < inoutPipeline->srvTextureCount; ++srvIndex)
    {
        int32_t mapIndex = 0;
		for(mapIndex = 0; mapIndex < FFXM_COUNTOF(srvTextureBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(srvTextureBindingTable[mapIndex].name, inoutPipeline->srvTextureBindings[srvIndex].name))
                break;
        }
		if(mapIndex == FFXM_COUNTOF(srvTextureBindingTable))
            return FFXM_ERROR_INVALID_ARGUMENT;

        inoutPipeline->srvTextureBindings[srvIndex].resourceIdentifier = srvTextureBindingTable[mapIndex].index;
    }

    for (uint32_t uavIndex = 0; uavIndex < inoutPipeline->uavTextureCount; ++uavIndex)
    {
        int32_t mapIndex = 0;
		for(mapIndex = 0; mapIndex < FFXM_COUNTOF(uavTextureBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(uavTextureBindingTable[mapIndex].name, inoutPipeline->uavTextureBindings[uavIndex].name))
                break;
        }
		if(mapIndex == FFXM_COUNTOF(uavTextureBindingTable))
            return FFXM_ERROR_INVALID_ARGUMENT;

        inoutPipeline->uavTextureBindings[uavIndex].resourceIdentifier = uavTextureBindingTable[mapIndex].index;
    }

	for(uint32_t rtIndex = 0; rtIndex < inoutPipeline->rtCount; ++rtIndex)
	{
		int32_t mapIndex = 0;
		for(mapIndex = 0; mapIndex < FFXM_COUNTOF(rtTextureBindingTable); ++mapIndex)
		{
			if(0 == wcscmp(rtTextureBindingTable[mapIndex].name, inoutPipeline->rtBindings[rtIndex].name))
				break;
		}
		if(mapIndex == FFXM_COUNTOF(rtTextureBindingTable))
			return FFXM_ERROR_INVALID_ARGUMENT;

		inoutPipeline->rtBindings[rtIndex].resourceIdentifier = rtTextureBindingTable[mapIndex].index;
	}

    for (uint32_t cbIndex = 0; cbIndex < inoutPipeline->constCount; ++cbIndex)
    {
        int32_t mapIndex = 0;
		for(mapIndex = 0; mapIndex < FFXM_COUNTOF(constantBufferBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(constantBufferBindingTable[mapIndex].name, inoutPipeline->constantBufferBindings[cbIndex].name))
                break;
        }
		if(mapIndex == FFXM_COUNTOF(constantBufferBindingTable))
            return FFXM_ERROR_INVALID_ARGUMENT;

        inoutPipeline->constantBufferBindings[cbIndex].resourceIdentifier = constantBufferBindingTable[mapIndex].index;
    }

    return FFXM_OK;
}

static uint32_t getPipelinePermutationFlags(FfxmFsr2ShaderQualityMode qualityMode, uint32_t contextFlags, FfxmFsr2Pass passId, bool fp16, bool force64)
{
    // work out what permutation to load.
    uint32_t flags = 0;
    flags |= (contextFlags & FFXM_FSR2_ENABLE_HIGH_DYNAMIC_RANGE) ? FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT : 0;
    flags |= (contextFlags & FFXM_FSR2_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS) ? 0 : FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS;
    flags |= (contextFlags & FFXM_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) ? FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS : 0;
    flags |= (contextFlags & FFXM_FSR2_ENABLE_DEPTH_INVERTED) ? FSR2_SHADER_PERMUTATION_DEPTH_INVERTED : 0;
    flags |= (passId == FFXM_FSR2_PASS_ACCUMULATE_SHARPEN) ? FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING : 0;
    flags |= (force64) ? FSR2_SHADER_PERMUTATION_FORCE_WAVE64 : 0;
    flags |= (fp16) ? FSR2_SHADER_PERMUTATION_ALLOW_FP16 : 0;

	// Apply flags for shader quality presets
	flags |= (qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_BALANCED) ? FSR2_SHADER_PERMUTATION_APPLY_BALANCED_OPT : 0;
	flags |= (qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_PERFORMANCE) ? (FSR2_SHADER_PERMUTATION_APPLY_BALANCED_OPT | FSR2_SHADER_PERMUTATION_APPLY_PERFORMANCE_OPT) : 0;
    flags |= (qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_ULTRA_PERFORMANCE) ? FSR2_SHADER_PERMUTATION_APPLY_ULTRA_PERFORMANCE_OPT : 0;

	// Indicate if running on GLES 3.2
	flags |= (contextFlags & FFXM_FSR2_OPENGL_ES_3_2) ? FSR2_SHADER_PERMUTATION_PLATFORM_GLES_3_2 : 0;

    return flags;
}

static FfxmErrorCode createPipelineStates(FfxmFsr2Context_Private* context)
{
    FFXM_ASSERT(context);

    FfxmPipelineDescription pipelineDescription = {};
    pipelineDescription.contextFlags = context->contextDescription.flags;

    // Samplers
    pipelineDescription.samplerCount = 2;
    FfxmSamplerDescription samplerDescs[2] = { {0,  FFXM_FILTER_TYPE_MINMAGMIP_POINT, FFXM_ADDRESS_MODE_CLAMP, FFXM_ADDRESS_MODE_CLAMP, FFXM_ADDRESS_MODE_CLAMP, FFXM_BIND_COMPUTE_SHADER_STAGE },
                                            {1, FFXM_FILTER_TYPE_MINMAGMIP_LINEAR, FFXM_ADDRESS_MODE_CLAMP, FFXM_ADDRESS_MODE_CLAMP, FFXM_ADDRESS_MODE_CLAMP, FFXM_BIND_COMPUTE_SHADER_STAGE} };
    pipelineDescription.samplers = samplerDescs;

    // Root constants
    pipelineDescription.rootConstantBufferCount = 2;
    FfxmRootConstantDescription rootConstantDescs[2] = { {sizeof(Fsr2Constants) / sizeof(uint32_t), FFXM_BIND_COMPUTE_SHADER_STAGE },
                                                        { sizeof(Fsr2SecondaryUnion) / sizeof(uint32_t), FFXM_BIND_COMPUTE_SHADER_STAGE } };
    pipelineDescription.rootConstants = rootConstantDescs;

    // Query device capabilities
    FfxmDeviceCapabilities capabilities;
    context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &capabilities);

    // Setup a few options used to determine permutation flags
    bool haveShaderModel66 = capabilities.minimumSupportedShaderModel >= FFXM_SHADER_MODEL_6_6;
    bool supportedFP16     = capabilities.fp16Supported;
    bool canForceWave64    = false;

    const uint32_t waveLaneCountMin = capabilities.waveLaneCountMin;
    const uint32_t waveLaneCountMax = capabilities.waveLaneCountMax;
    if (waveLaneCountMin == 32 && waveLaneCountMax == 64)
    {
        // no wave64 support for permutation
        canForceWave64 = false/*haveShaderModel66*/;
    }
    else
        canForceWave64 = false;

    // Work out what permutation to load.
    uint32_t contextFlags = context->contextDescription.flags;

    // Set up pipeline descriptor (basically RootSignature and binding)
    wcscpy(pipelineDescription.name, L"FSR2-LUM_PYRAMID");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateComputePipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags,	FFXM_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID, supportedFP16, canForceWave64),
		&pipelineDescription, context->effectContextId, &context->pipelineComputeLuminancePyramid));
    wcscpy(pipelineDescription.name, L"FSR2-RCAS");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_RCAS,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_RCAS, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineRCAS));
    wcscpy(pipelineDescription.name, L"FSR2-GEN_REACTIVE");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_GENERATE_REACTIVE,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_GENERATE_REACTIVE, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineGenerateReactive));

    pipelineDescription.rootConstantBufferCount = 1;

    wcscpy(pipelineDescription.name, L"FSR2-DEPTH_CLIP");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_DEPTH_CLIP,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_DEPTH_CLIP, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineDepthClip));
    wcscpy(pipelineDescription.name, L"FSR2-RECON_PREV_DEPTH");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineReconstructPreviousDepth));
    wcscpy(pipelineDescription.name, L"FSR2-LOCK");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateComputePipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_LOCK,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_LOCK, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineLock));
    wcscpy(pipelineDescription.name, L"FSR2-ACCUMULATE");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_ACCUMULATE,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_ACCUMULATE, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineAccumulate));
    wcscpy(pipelineDescription.name, L"FSR2-ACCUM_SHARP");
    FFXM_VALIDATE(context->contextDescription.backendInterface.fpCreateGraphicsPipeline(&context->contextDescription.backendInterface, FFXM_EFFECT_FSR2, FFXM_FSR2_PASS_ACCUMULATE_SHARPEN,
		context->contextDescription.qualityMode, getPipelinePermutationFlags(context->contextDescription.qualityMode, contextFlags, FFXM_FSR2_PASS_ACCUMULATE_SHARPEN, supportedFP16, canForceWave64),
        &pipelineDescription, context->effectContextId, &context->pipelineAccumulateSharpen));

    // for each pipeline: re-route/fix-up IDs based on names
    patchResourceBindings(&context->pipelineDepthClip);
    patchResourceBindings(&context->pipelineReconstructPreviousDepth);
    patchResourceBindings(&context->pipelineLock);
    patchResourceBindings(&context->pipelineAccumulate);
    patchResourceBindings(&context->pipelineComputeLuminancePyramid);
    patchResourceBindings(&context->pipelineAccumulateSharpen);
    patchResourceBindings(&context->pipelineRCAS);
    patchResourceBindings(&context->pipelineGenerateReactive);

    return FFXM_OK;
}

static FfxmErrorCode createResourceFromDescription(FfxmFsr2Context_Private* context, const FfxmInternalResourceDescription* resDesc)
{
	const FfxmResourceType resourceType = resDesc->type;
    const FfxmResourceDescription resourceDescription = { resourceType, resDesc->format, resDesc->width, resDesc->height, 1, resDesc->mipCount, FFXM_RESOURCE_FLAGS_NONE, resDesc->usage };
    const FfxmResourceStates initialState = (resDesc->usage == FFXM_RESOURCE_USAGE_READ_ONLY) ? FFXM_RESOURCE_STATE_COMPUTE_READ : (resDesc->usage == FFXM_RESOURCE_USAGE_RENDERTARGET) ? FFXM_RESOURCE_STATE_PIXEL_WRITE : FFXM_RESOURCE_STATE_UNORDERED_ACCESS;
    const FfxmCreateResourceDescription createResourceDescription = { FFXM_HEAP_TYPE_DEFAULT, resourceDescription, initialState, resDesc->initDataSize, resDesc->initData, resDesc->name, resDesc->id };
	return context->contextDescription.backendInterface.fpCreateResource(&context->contextDescription.backendInterface, &createResourceDescription, context->effectContextId, &context->srvResources[resDesc->id]);
}

static FfxmErrorCode fsr2Create(FfxmFsr2Context_Private* context, const FfxmFsr2ContextDescription* contextDescription)
{
    FFXM_ASSERT(context);
    FFXM_ASSERT(contextDescription);

    // Setup the data for implementation.
    memset(context, 0, sizeof(FfxmFsr2Context_Private));
    context->device = contextDescription->backendInterface.device;

    memcpy(&context->contextDescription, contextDescription, sizeof(FfxmFsr2ContextDescription));

    // Setup constant buffer sizes.
    context->constantBuffers[0].num32BitEntries = sizeof(Fsr2Constants) / sizeof(uint32_t);
    context->constantBuffers[1].num32BitEntries = sizeof(Fsr2SpdConstants) / sizeof(uint32_t);
    context->constantBuffers[2].num32BitEntries = sizeof(Fsr2RcasConstants) / sizeof(uint32_t);
    context->constantBuffers[3].num32BitEntries = sizeof(Fsr2GenerateReactiveConstants) / sizeof(uint32_t);


    // Create the context.
    FfxmErrorCode errorCode = context->contextDescription.backendInterface.fpCreateBackendContext(&context->contextDescription.backendInterface, &context->effectContextId);
    FFXM_RETURN_ON_ERROR(errorCode == FFXM_OK, errorCode);

    // call out for device caps.
    errorCode = context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &context->deviceCapabilities);
    FFXM_RETURN_ON_ERROR(errorCode == FFXM_OK, errorCode);

    // set defaults
    context->firstExecution = true;
    context->resourceFrameIndex = 0;

    context->constants.displaySize[0] = contextDescription->displaySize.width;
    context->constants.displaySize[1] = contextDescription->displaySize.height;

    // generate the data for the LUT.
    const uint32_t lanczos2LutWidth = 128;
    int16_t lanczos2Weights[lanczos2LutWidth] = { };

    for (uint32_t currentLanczosWidthIndex = 0; currentLanczosWidthIndex < lanczos2LutWidth; currentLanczosWidthIndex++) {

        const float x = 2.0f * currentLanczosWidthIndex / float(lanczos2LutWidth - 1);
        const float y = lanczos2(x);
        lanczos2Weights[currentLanczosWidthIndex] = int16_t(roundf(y * 32767.0f));
    }

    // upload path only supports R16_SNORM, let's go and convert
    int16_t maximumBias[FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_WIDTH * FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_HEIGHT];
    for (uint32_t i = 0; i < FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_WIDTH * FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_HEIGHT; ++i) {

        maximumBias[i] = int16_t(roundf(ffxmFsr2MaximumBias[i] / 2.0f * 32767.0f));
    }

    uint8_t defaultReactiveMaskData = 0U;
    uint32_t atomicInitData = 0U;
    float defaultExposure[] = { 0.0f, 0.0f };
    const FfxmResourceType texture1dResourceType = (context->contextDescription.flags & FFXM_FSR2_ENABLE_TEXTURE1D_USAGE) ? FFXM_RESOURCE_TYPE_TEXTURE1D : FFXM_RESOURCE_TYPE_TEXTURE2D;

    const bool applyUltraPerformanceOptimizations = contextDescription->qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_ULTRA_PERFORMANCE;
	const bool applyPerfModeOptimizations = contextDescription->qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_PERFORMANCE;
	const bool applyBalancedModeOptimizations = contextDescription->qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_BALANCED;
	const bool isBalancedOrPerformance = applyBalancedModeOptimizations || applyPerfModeOptimizations;

	const bool preparedInputColorNeedsFP16 = !applyPerfModeOptimizations;

	const bool isOpenGLES = (contextDescription->flags & FFXM_FSR2_OPENGL_ES_3_2) != 0;

	// OpenGLES 3.2 specific: We need to workaround some GLES limitations for some resources.
	const FfxmSurfaceFormat formatR8Workaround = isOpenGLES ? FFXM_SURFACE_FORMAT_R32_FLOAT : FFXM_SURFACE_FORMAT_R8_UNORM;
	const FfxmSurfaceFormat formatR16FWorkaround = isOpenGLES ? FFXM_SURFACE_FORMAT_R32_FLOAT : FFXM_SURFACE_FORMAT_R16_FLOAT;
	const FfxmSurfaceFormat formatRG16FWorkaround = isOpenGLES ? FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT : FFXM_SURFACE_FORMAT_R16G16_FLOAT;

    // declare internal resources needed
    const FfxmInternalResourceDescription internalSurfaceDesc[] = {

        {	FFXM_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR, L"FSR2_PreparedInputColor", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_RENDERTARGET,
		 (preparedInputColorNeedsFP16 ? FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT : FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM),
		 contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE},

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH, L"FSR2_ReconstructedPrevNearestDepth", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
            FFXM_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_1, L"FSR2_InternalDilatedVelocity1", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_NONE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_2, L"FSR2_InternalDilatedVelocity2", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_NONE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH, L"FSR2_DilatedDepth", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R32_FLOAT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_1, L"FSR2_LockStatus1", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_2, L"FSR2_LockStatus2", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA, L"FSR2_LockInputLuma", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
            FFXM_SURFACE_FORMAT_R16_FLOAT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_NEW_LOCKS, L"FSR2_NewLocks", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_UAV),
			formatR8Workaround, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE},

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1, L"FSR2_InternalUpscaled1", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
			(!isBalancedOrPerformance) ? FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT : FFXM_SURFACE_FORMAT_R11G11B10_FLOAT, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2, L"FSR2_InternalUpscaled2", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET),
			(!isBalancedOrPerformance) ? FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT : FFXM_SURFACE_FORMAT_R11G11B10_FLOAT, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE, L"FSR2_ExposureMips", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
            formatR16FWorkaround, contextDescription->maxRenderSize.width / 2, contextDescription->maxRenderSize.height / 2, 0, FFXM_RESOURCE_FLAGS_ALIASABLE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT, L"FSR2_SpdAtomicCounter", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
            FFXM_SURFACE_FORMAT_R32_UINT, 1, 1, 1, FFXM_RESOURCE_FLAGS_ALIASABLE, sizeof(atomicInitData), &atomicInitData },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS, L"FSR2_DilatedReactiveMasks", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_RENDERTARGET,
            FFXM_SURFACE_FORMAT_R8G8_UNORM, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_LANCZOS_LUT, L"FSR2_LanczosLutData", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_READ_ONLY,
            FFXM_SURFACE_FORMAT_R16_SNORM, lanczos2LutWidth, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(lanczos2Weights), lanczos2Weights },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY, L"FSR2_DefaultReactiviyMask", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_READ_ONLY,
            FFXM_SURFACE_FORMAT_R8_UNORM, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(defaultReactiveMaskData), &defaultReactiveMaskData },

        {   FFXM_FSR2_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT, L"FSR2_MaximumUpsampleBias", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_READ_ONLY,
            FFXM_SURFACE_FORMAT_R16_SNORM, FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_WIDTH, FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_HEIGHT, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(maximumBias), maximumBias },

        {   FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE, L"FSR2_DefaultExposure", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_READ_ONLY,
            FFXM_SURFACE_FORMAT_R32G32_FLOAT, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(defaultExposure), defaultExposure },

        {	FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE, L"FSR2_AutoExposure", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage) (FFXM_RESOURCE_USAGE_UAV | FFXM_RESOURCE_USAGE_RENDERTARGET),
            formatRG16FWorkaround, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE },


        // only one for now, will need ping pong to respect the motion vectors
        {   FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE, L"FSR2_AutoReactive", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
            FFXM_SURFACE_FORMAT_R8_UNORM, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_NONE },
    };

    const FfxmInternalResourceDescription internalSurfaceDescUltraPerformance[] = {
		{FFXM_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH, L"FSR2_ReconstructedPrevNearestDepth333", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 FFXM_RESOURCE_USAGE_UAV, FFXM_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1,
		 FFXM_RESOURCE_FLAGS_ALIASABLE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_1, L"FSR2_DilatedDepthMotionVectorsInputLuma1", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT, contextDescription->maxRenderSize.width,
		 contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_2, L"FSR2_DilatedDepthMotionVectorsInputLuma2", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT,contextDescription->maxRenderSize.width,
		 contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_1, L"FSR2_LockStatus1", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->displaySize.width,
		 contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_2, L"FSR2_LockStatus2", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R16G16_FLOAT, contextDescription->displaySize.width,
		 contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_NEW_LOCKS, L"FSR2_NewLocks", FFXM_RESOURCE_TYPE_TEXTURE2D, (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_UAV),
		 formatR8Workaround, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1, L"FSR2_InternalUpscaled1", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R11G11B10_FLOAT,
		 contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2, L"FSR2_InternalUpscaled2", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R11G11B10_FLOAT,
		 contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE, L"FSR2_ExposureMips", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
		 formatR16FWorkaround, contextDescription->maxRenderSize.width / 2, contextDescription->maxRenderSize.height / 2, 0,
		 FFXM_RESOURCE_FLAGS_ALIASABLE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT, L"FSR2_SpdAtomicCounter", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
		 FFXM_SURFACE_FORMAT_R32_UINT, 1, 1, 1, FFXM_RESOURCE_FLAGS_ALIASABLE, sizeof(atomicInitData), &atomicInitData},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS, L"FSR2_DilatedReactiveMasks", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 FFXM_RESOURCE_USAGE_RENDERTARGET, FFXM_SURFACE_FORMAT_R8G8_UNORM, contextDescription->maxRenderSize.width,
		 contextDescription->maxRenderSize.height, 1, FFXM_RESOURCE_FLAGS_ALIASABLE},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_LANCZOS_LUT, L"FSR2_LanczosLutData", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_READ_ONLY,
		 FFXM_SURFACE_FORMAT_R16_SNORM, lanczos2LutWidth, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(lanczos2Weights), lanczos2Weights},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY, L"FSR2_DefaultReactiviyMask", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 FFXM_RESOURCE_USAGE_READ_ONLY, FFXM_SURFACE_FORMAT_R8_UNORM, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(defaultReactiveMaskData),
		 &defaultReactiveMaskData},

		{FFXM_FSR2_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT, L"FSR2_MaximumUpsampleBias", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 FFXM_RESOURCE_USAGE_READ_ONLY, FFXM_SURFACE_FORMAT_R16_SNORM, FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_WIDTH, FFXM_FSR2_MAXIMUM_BIAS_TEXTURE_HEIGHT, 1,
		 FFXM_RESOURCE_FLAGS_NONE, sizeof(maximumBias), maximumBias},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE, L"FSR2_DefaultExposure", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 FFXM_RESOURCE_USAGE_READ_ONLY, FFXM_SURFACE_FORMAT_R32G32_FLOAT, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE, sizeof(defaultExposure),
		 defaultExposure},

		{FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE, L"FSR2_AutoExposure", FFXM_RESOURCE_TYPE_TEXTURE2D,
		 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_UAV | FFXM_RESOURCE_USAGE_RENDERTARGET), formatRG16FWorkaround, 1, 1, 1, FFXM_RESOURCE_FLAGS_NONE},

		// only one for now, will need ping pong to respect the motion vectors
		{FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE, L"FSR2_AutoReactive", FFXM_RESOURCE_TYPE_TEXTURE2D, FFXM_RESOURCE_USAGE_UAV,
		 FFXM_SURFACE_FORMAT_R8_UNORM, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1,
		 FFXM_RESOURCE_FLAGS_NONE},
	};

    // clear the SRV resources to NULL.
    memset(context->srvResources, 0, sizeof(context->srvResources));

	// Generally used resources by all presets
    if (applyUltraPerformanceOptimizations)
    {
        for(int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFXM_ARRAY_ELEMENTS(internalSurfaceDescUltraPerformance); ++currentSurfaceIndex)
		{
			FFXM_VALIDATE(createResourceFromDescription(context, &internalSurfaceDescUltraPerformance[currentSurfaceIndex]));
		}
    }
    else
    {
        for (int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFXM_ARRAY_ELEMENTS(internalSurfaceDesc); ++currentSurfaceIndex)
        {
            FFXM_VALIDATE(createResourceFromDescription(context, &internalSurfaceDesc[currentSurfaceIndex]));
        }
    }

	// Additional textures used by either balanced or performance presets
	if(isBalancedOrPerformance)
	{
		const FfxmInternalResourceDescription internalBalancedSurfaceDesc[] = {
			{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_1, L"FSR2_InternalReactive1", FFXM_RESOURCE_TYPE_TEXTURE2D,
			 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R8_SNORM,
			 contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

			{FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_2, L"FSR2_InternalReactive2", FFXM_RESOURCE_TYPE_TEXTURE2D,
			 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R8_SNORM,
			 contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},
		};

		for(int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFXM_ARRAY_ELEMENTS(internalBalancedSurfaceDesc); ++currentSurfaceIndex)
		{
			FFXM_VALIDATE(createResourceFromDescription(context, &internalBalancedSurfaceDesc[currentSurfaceIndex]));
		}
	}
	else
	{
		// Quality preset specific
		const FfxmInternalResourceDescription internalQualitySurfaceDesc[] = {
			{FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_1, L"FSR2_LumaHistory1", FFXM_RESOURCE_TYPE_TEXTURE2D,
			 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM, contextDescription->displaySize.width,
			 contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},

			{FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_2, L"FSR2_LumaHistory2", FFXM_RESOURCE_TYPE_TEXTURE2D,
			 (FfxmResourceUsage)(FFXM_RESOURCE_USAGE_RENDERTARGET), FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM, contextDescription->displaySize.width,
			 contextDescription->displaySize.height, 1, FFXM_RESOURCE_FLAGS_NONE},
		};

		for(int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFXM_ARRAY_ELEMENTS(internalQualitySurfaceDesc); ++currentSurfaceIndex)
		{
			FFXM_VALIDATE(createResourceFromDescription(context, &internalQualitySurfaceDesc[currentSurfaceIndex]));
		}
	}

    // copy resources to uavResrouces list
    memcpy(context->uavResources, context->srvResources, sizeof(context->srvResources));
	// copy resources to uavResrouces list
	memcpy(context->rtResources, context->srvResources, sizeof(context->srvResources));

    // avoid compiling pipelines on first render
    {
        errorCode = createPipelineStates(context);
        FFXM_RETURN_ON_ERROR(errorCode == FFXM_OK, errorCode);
    }
    return FFXM_OK;
}

static FfxmErrorCode fsr2Release(FfxmFsr2Context_Private* context)
{
    FFXM_ASSERT(context);

    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineDepthClip, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineReconstructPreviousDepth, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineLock, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineAccumulate, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineAccumulateSharpen, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineRCAS, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineComputeLuminancePyramid, context->effectContextId);
    ffxmSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineGenerateReactive, context->effectContextId);

    // unregister resources not created internally
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_DEPTH] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
	context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE] = {FFXM_FSR2_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_RCAS_INPUT] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT] = { FFXM_FSR2_RESOURCE_IDENTIFIER_NULL };

    // Release the copy resources for those that had init data
    ffxmSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT]);
    ffxmSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LANCZOS_LUT]);
    ffxmSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY]);
    ffxmSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT]);
    ffxmSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE]);

    // release internal resources
    for (int32_t currentResourceIndex = 0; currentResourceIndex < FFXM_FSR2_RESOURCE_IDENTIFIER_COUNT; ++currentResourceIndex) {

        ffxmSafeReleaseResource(&context->contextDescription.backendInterface, context->srvResources[currentResourceIndex]);
    }

    // Destroy the context
    context->contextDescription.backendInterface.fpDestroyBackendContext(&context->contextDescription.backendInterface, context->effectContextId);

    return FFXM_OK;
}

static void setupDeviceDepthToViewSpaceDepthParams(FfxmFsr2Context_Private* context, const FfxmFsr2DispatchDescription* params)
{
    const bool bInverted = (context->contextDescription.flags & FFXM_FSR2_ENABLE_DEPTH_INVERTED) == FFXM_FSR2_ENABLE_DEPTH_INVERTED;
    const bool bInfinite = (context->contextDescription.flags & FFXM_FSR2_ENABLE_DEPTH_INFINITE) == FFXM_FSR2_ENABLE_DEPTH_INFINITE;

    // make sure it has no impact if near and far plane values are swapped in dispatch params
    // the flags "inverted" and "infinite" will decide what transform to use
    float fMin = FFXM_MINIMUM(params->cameraNear, params->cameraFar);
    float fMax = FFXM_MAXIMUM(params->cameraNear, params->cameraFar);

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

static void scheduleDispatch(FfxmFsr2Context_Private* context, const FfxmFsr2DispatchDescription* params, const FfxmPipelineState* pipeline, uint32_t dispatchX, uint32_t dispatchY)
{
    FfxmGpuJobDescription dispatchJob = {FFXM_GPU_JOB_COMPUTE};

    for (uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex) {

        const uint32_t currentResourceId = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
        const FfxmResourceInternal currentResource = context->srvResources[currentResourceId];
        dispatchJob.computeJobDescriptor.srvTextures[currentShaderResourceViewIndex] = currentResource;
        wcscpy(dispatchJob.computeJobDescriptor.srvTextureNames[currentShaderResourceViewIndex],
                 pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
    }

    for (uint32_t currentUnorderedAccessViewIndex = 0; currentUnorderedAccessViewIndex < pipeline->uavTextureCount; ++currentUnorderedAccessViewIndex) {

        const uint32_t currentResourceId = pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].resourceIdentifier;
        wcscpy(dispatchJob.computeJobDescriptor.uavTextureNames[currentUnorderedAccessViewIndex],
                 pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].name);

        if (currentResourceId >= FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_0 && currentResourceId <= FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_12)
        {
            const FfxmResourceInternal currentResource = context->uavResources[FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE];
            dispatchJob.computeJobDescriptor.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
            dispatchJob.computeJobDescriptor.uavTextureMips[currentUnorderedAccessViewIndex] =
                currentResourceId - FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_0;
        }
        else
        {
            const FfxmResourceInternal currentResource = context->uavResources[currentResourceId];
            dispatchJob.computeJobDescriptor.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
            dispatchJob.computeJobDescriptor.uavTextureMips[currentUnorderedAccessViewIndex] = 0;
        }
    }

    dispatchJob.computeJobDescriptor.dimensions[0] = dispatchX;
    dispatchJob.computeJobDescriptor.dimensions[1] = dispatchY;
    dispatchJob.computeJobDescriptor.dimensions[2] = 1;
    dispatchJob.computeJobDescriptor.pipeline      = *pipeline;

    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex) {
        wcscpy(dispatchJob.computeJobDescriptor.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
        dispatchJob.computeJobDescriptor.cbs[currentRootConstantIndex] = context->constantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
    }

    context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &dispatchJob);
}

static void scheduleFragment(FfxmFsr2Context_Private* context, const FfxmFsr2DispatchDescription* params, FfxmPipelineState* pipeline,
							 uint32_t width, uint32_t height)
{
	FfxmGpuJobDescription fragmentJob = {FFXM_GPU_JOB_FRAGMENT};

	for(uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex)
	{

		const uint32_t currentResourceId = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
		const FfxmResourceInternal currentResource = context->srvResources[currentResourceId];
		fragmentJob.fragmentJobDescription.srvTextures[currentShaderResourceViewIndex] = currentResource;
		wcscpy(fragmentJob.fragmentJobDescription.srvTextureNames[currentShaderResourceViewIndex],
			   pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
	}

	for(uint32_t currentUnorderedAccessViewIndex = 0; currentUnorderedAccessViewIndex < pipeline->uavTextureCount; ++currentUnorderedAccessViewIndex)
	{

		const uint32_t currentResourceId = pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].resourceIdentifier;
		wcscpy(fragmentJob.fragmentJobDescription.uavTextureNames[currentUnorderedAccessViewIndex],
			   pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].name);

		if(currentResourceId >= FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_0
		   && currentResourceId <= FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_12)
		{
			const FfxmResourceInternal currentResource = context->uavResources[FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE];
			fragmentJob.fragmentJobDescription.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
			fragmentJob.fragmentJobDescription.uavTextureMips[currentUnorderedAccessViewIndex] =
				currentResourceId - FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_0;
		}
		else
		{
			const FfxmResourceInternal currentResource = context->uavResources[currentResourceId];
			fragmentJob.fragmentJobDescription.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
			fragmentJob.fragmentJobDescription.uavTextureMips[currentUnorderedAccessViewIndex] = 0;
		}
	}

	for(uint32_t currentRtIndex = 0; currentRtIndex < pipeline->rtCount; ++currentRtIndex)
	{
		const uint32_t currentResourceId = pipeline->rtBindings[currentRtIndex].resourceIdentifier;
		const FfxmResourceInternal currentResource = context->rtResources[currentResourceId];
		fragmentJob.fragmentJobDescription.rtTextures[currentRtIndex] = currentResource;
		wcscpy(fragmentJob.fragmentJobDescription.rtTextureNames[currentRtIndex],
			   pipeline->rtBindings[currentRtIndex].name);
	}

	fragmentJob.fragmentJobDescription.viewport[0] = width;
	fragmentJob.fragmentJobDescription.viewport[1] = height;
	fragmentJob.fragmentJobDescription.pipeline = pipeline;

	for(uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex)
	{
		wcscpy(fragmentJob.fragmentJobDescription.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
		fragmentJob.fragmentJobDescription.cbs[currentRootConstantIndex] =
			context->constantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
	}

	context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &fragmentJob);
}

static FfxmErrorCode fsr2Dispatch(FfxmFsr2Context_Private* context, const FfxmFsr2DispatchDescription* params)
{
    if ((context->contextDescription.flags & FFXM_FSR2_ENABLE_DEBUG_CHECKING) == FFXM_FSR2_ENABLE_DEBUG_CHECKING)
    {
        fsr2DebugCheckDispatch(context, params);
    }

    // take a short cut to the command list
    FfxmCommandList commandList = params->commandList;

    if (context->firstExecution)
    {
        FfxmGpuJobDescription clearJob = { FFXM_GPU_JOB_CLEAR_FLOAT };

        const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
        memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));

        clearJob.clearJobDescriptor.target = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_1];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        clearJob.clearJobDescriptor.target = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_2];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        clearJob.clearJobDescriptor.target = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
    }

    // Prepare per frame descriptor tables
    const bool isOddFrame = !!(context->resourceFrameIndex & 1);
    const uint32_t currentCpuOnlyTableBase = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_COUNT : 0;
    const uint32_t currentGpuTableBase = 2 * FFXM_FSR2_RESOURCE_IDENTIFIER_COUNT * context->resourceFrameIndex;
    const uint32_t lockStatusSrvResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_1;
    const uint32_t lockStatusRtResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS_2;
    const uint32_t upscaledColorSrvResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1;
    const uint32_t upscaledColorRtResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR_2;
	const uint32_t temporalReactiveSrvResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_1;
	const uint32_t temporalReactiveRtResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE_2;
    const uint32_t dilatedMotionVectorsResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_1;
    const uint32_t previousDilatedMotionVectorsResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DILATED_MOTION_VECTORS_2;
    const uint32_t lumaHistorySrvResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_1;
    const uint32_t lumaHistoryRtResourceIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY_2;

    const uint32_t dilatedDepthMotionVectorsInputLumaIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_2 : FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_1;
	const uint32_t previousDilatedDepthMotionVectorsInputLumaIndex = isOddFrame ? FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_1 : FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA_2;

    const bool resetAccumulation = params->reset || context->firstExecution;
    context->firstExecution = false;

    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->color, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->depth, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_DEPTH]);
    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->motionVectors, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS]);

    // if auto exposure is enabled use the auto exposure SRV, otherwise what the app sends.
    if (context->contextDescription.flags & FFXM_FSR2_ENABLE_AUTO_EXPOSURE) {
        context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE] = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE];
    } else {
        if (ffxmFsr2ResourceIsNull(params->exposure)) {
            context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE] = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE];
        } else {
            context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->exposure, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE]);
        }
    }

    if (ffxmFsr2ResourceIsNull(params->reactive)) {
        context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK] = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY];
    }
    else {
        context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->reactive, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK]);
    }

    if (ffxmFsr2ResourceIsNull(params->transparencyAndComposition)) {
        context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK] = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_REACTIVITY];
    } else {
        context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->transparencyAndComposition, context->effectContextId, &context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK]);
    }

    context->contextDescription.backendInterface.fpRegisterResource(&context->contextDescription.backendInterface, &params->output, context->effectContextId, &context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT]);
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS] = context->srvResources[lockStatusSrvResourceIndex];
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR] = context->srvResources[upscaledColorSrvResourceIndex];
	context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE] = context->srvResources[temporalReactiveSrvResourceIndex];
    context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS] = context->rtResources[lockStatusRtResourceIndex];
    context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR] = context->rtResources[upscaledColorRtResourceIndex];
	context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INTERNAL_TEMPORAL_REACTIVE] = context->rtResources[temporalReactiveRtResourceIndex];
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_RCAS_INPUT] = context->rtResources[upscaledColorRtResourceIndex];

    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS] = context->srvResources[dilatedMotionVectorsResourceIndex];
    context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS] = context->rtResources[dilatedMotionVectorsResourceIndex];
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_PREVIOUS_DILATED_MOTION_VECTORS] = context->srvResources[previousDilatedMotionVectorsResourceIndex];

    context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY] = context->rtResources[lumaHistoryRtResourceIndex];
    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY] = context->srvResources[lumaHistorySrvResourceIndex];

    context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA] = context->srvResources[dilatedDepthMotionVectorsInputLumaIndex];
	context->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA] = context->rtResources[dilatedDepthMotionVectorsInputLumaIndex];
	context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_PREV_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA] = context->srvResources[previousDilatedDepthMotionVectorsInputLumaIndex];

    // actual resource size may differ from render/display resolution (e.g. due to Hw/API restrictions), so query the descriptor for UVs adjustment
    const FfxmResourceDescription resourceDescInputColor = context->contextDescription.backendInterface.fpGetResourceDescription(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    const FfxmResourceDescription resourceDescLockStatus = context->contextDescription.backendInterface.fpGetResourceDescription(&context->contextDescription.backendInterface, context->srvResources[lockStatusSrvResourceIndex]);
    const FfxmResourceDescription resourceDescReactiveMask = context->contextDescription.backendInterface.fpGetResourceDescription(&context->contextDescription.backendInterface, context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK]);
    FFXM_ASSERT(resourceDescInputColor.type == FFXM_RESOURCE_TYPE_TEXTURE2D);
    FFXM_ASSERT(resourceDescLockStatus.type == FFXM_RESOURCE_TYPE_TEXTURE2D);

    context->constants.jitterOffset[0] = params->jitterOffset.x;
    context->constants.jitterOffset[1] = params->jitterOffset.y;
    context->constants.renderSize[0] = int32_t(params->renderSize.width ? params->renderSize.width   : resourceDescInputColor.width);
    context->constants.renderSize[1] = int32_t(params->renderSize.height ? params->renderSize.height : resourceDescInputColor.height);
    context->constants.maxRenderSize[0] = int32_t(context->contextDescription.maxRenderSize.width);
    context->constants.maxRenderSize[1] = int32_t(context->contextDescription.maxRenderSize.height);
    context->constants.inputColorResourceDimensions[0] = resourceDescInputColor.width;
    context->constants.inputColorResourceDimensions[1] = resourceDescInputColor.height;

    // compute the horizontal FOV for the shader from the vertical one.
    const float aspectRatio = (float)params->renderSize.width / (float)params->renderSize.height;
    const float cameraAngleHorizontal = atan(tan(params->cameraFovAngleVertical / 2) * aspectRatio) * 2;
    context->constants.tanHalfFOV = tanf(cameraAngleHorizontal * 0.5f);
    context->constants.viewSpaceToMetersFactor = (params->viewSpaceToMetersFactor > 0.0f) ? params->viewSpaceToMetersFactor : 1.0f;

    // compute params to enable device depth to view space depth computation in shader
    setupDeviceDepthToViewSpaceDepthParams(context, params);

    // To be updated if resource is larger than the actual image size
    context->constants.downscaleFactor[0] = float(context->constants.renderSize[0]) / context->contextDescription.displaySize.width;
    context->constants.downscaleFactor[1] = float(context->constants.renderSize[1]) / context->contextDescription.displaySize.height;
    context->constants.previousFramePreExposure = context->constants.preExposure;
    context->constants.preExposure = (params->preExposure != 0) ? params->preExposure : 1.0f;

    // motion vector data
    const int32_t* motionVectorsTargetSize = (context->contextDescription.flags & FFXM_FSR2_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS) ? context->constants.displaySize : context->constants.renderSize;

    context->constants.motionVectorScale[0] = (params->motionVectorScale.x / motionVectorsTargetSize[0]);
    context->constants.motionVectorScale[1] = (params->motionVectorScale.y / motionVectorsTargetSize[1]);

    // compute jitter cancellation
    if (context->contextDescription.flags & FFXM_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) {

        context->constants.motionVectorJitterCancellation[0] = (context->previousJitterOffset[0] - context->constants.jitterOffset[0]) / motionVectorsTargetSize[0];
        context->constants.motionVectorJitterCancellation[1] = (context->previousJitterOffset[1] - context->constants.jitterOffset[1]) / motionVectorsTargetSize[1];

        context->previousJitterOffset[0] = context->constants.jitterOffset[0];
        context->previousJitterOffset[1] = context->constants.jitterOffset[1];
    }

    // lock data, assuming jitter sequence length computation for now
    const int32_t jitterPhaseCount = ffxmFsr2GetJitterPhaseCount(params->renderSize.width, context->contextDescription.displaySize.width);

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
    context->constants.deltaTime = FFXM_MAXIMUM(0.0f, FFXM_MINIMUM(1.0f, params->frameTimeDelta / 1000.0f));

    if (resetAccumulation) {
        context->constants.frameIndex = 0;
    } else {
        context->constants.frameIndex++;
    }

    // shading change usage of the SPD mip levels.
    context->constants.lumaMipLevelToUse = uint32_t(FFXM_FSR2_SHADING_CHANGE_MIP_LEVEL);

    const float mipDiv = float(2 << context->constants.lumaMipLevelToUse);
    context->constants.lumaMipDimensions[0] = uint32_t(context->constants.maxRenderSize[0] / mipDiv);
    context->constants.lumaMipDimensions[1] = uint32_t(context->constants.maxRenderSize[1] / mipDiv);

    // reactive mask bias
    const int32_t threadGroupWorkRegionDim = 8;
    const int32_t dispatchSrcX = FFXM_DIVIDE_ROUNDING_UP(context->constants.renderSize[0], threadGroupWorkRegionDim);
    const int32_t dispatchSrcY = FFXM_DIVIDE_ROUNDING_UP(context->constants.renderSize[1], threadGroupWorkRegionDim);
    const int32_t dispatchDstX = FFXM_DIVIDE_ROUNDING_UP(context->contextDescription.displaySize.width, threadGroupWorkRegionDim);
    const int32_t dispatchDstY = FFXM_DIVIDE_ROUNDING_UP(context->contextDescription.displaySize.height, threadGroupWorkRegionDim);

    const bool applyUltraPerformanceOptimizations = context->contextDescription.qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_ULTRA_PERFORMANCE;
	const bool applyPerfModeOptimizations = context->contextDescription.qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_PERFORMANCE;
	const bool applyBalancedModeOptimizations = context->contextDescription.qualityMode == FFXM_FSR2_SHADER_QUALITY_MODE_BALANCED;
	const bool isBalancedOrPerformance = applyBalancedModeOptimizations || applyPerfModeOptimizations;

    // Clear reconstructed depth for max depth store.
    if (resetAccumulation) {

        FfxmGpuJobDescription clearJob = { FFXM_GPU_JOB_CLEAR_FLOAT };

        // LockStatus resource has no sign bit, callback functions are compensating for this.
        // Clearing the resource must follow the same logic.
        float clearValuesLockStatus[4]{};
        clearValuesLockStatus[LOCK_LIFETIME_REMAINING] = 0.0f;
        clearValuesLockStatus[LOCK_TEMPORAL_LUMA] = 0.0f;

        memcpy(clearJob.clearJobDescriptor.color, clearValuesLockStatus, 4 * sizeof(float));
        clearJob.clearJobDescriptor.target = context->srvResources[lockStatusSrvResourceIndex];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

        const float clearValuesToZeroFloat[]{ 0.f, 0.f, 0.f, 0.f };
        memcpy(clearJob.clearJobDescriptor.color, clearValuesToZeroFloat, 4 * sizeof(float));
        clearJob.clearJobDescriptor.target = context->srvResources[upscaledColorSrvResourceIndex];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

		if(isBalancedOrPerformance)
		{
			clearJob.clearJobDescriptor.target = context->srvResources[temporalReactiveSrvResourceIndex];
			context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
		}

        clearJob.clearJobDescriptor.target = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_SCENE_LUMINANCE];
        context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);

        //if (context->contextDescription.flags & FFXM_FSR2_ENABLE_AUTO_EXPOSURE)
        // Auto exposure always used to track luma changes in locking logic
        {
            // 65504 == FP16 max, must match resetAutoExposureAverageSmoothing; AUTO_EXPOSURE is RG16F.
            const float clearValuesExposure[]{ -1.f, 65504.0f, 0.f, 0.f };
            memcpy(clearJob.clearJobDescriptor.color, clearValuesExposure, 4 * sizeof(float));
            clearJob.clearJobDescriptor.target = context->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE];
            context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &clearJob);
        }
    }

    // Auto exposure
    uint32_t dispatchThreadGroupCountXY[2];
    uint32_t workGroupOffset[2];
    uint32_t numWorkGroupsAndMips[2];
    uint32_t rectInfo[4] = { 0, 0, params->renderSize.width, params->renderSize.height };
    ffxSpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

    // downsample
    Fsr2SpdConstants luminancePyramidConstants;
    luminancePyramidConstants.numworkGroups = numWorkGroupsAndMips[0];
    luminancePyramidConstants.mips = numWorkGroupsAndMips[1];
    luminancePyramidConstants.workGroupOffset[0] = workGroupOffset[0];
    luminancePyramidConstants.workGroupOffset[1] = workGroupOffset[1];
    luminancePyramidConstants.renderSize[0] = params->renderSize.width;
    luminancePyramidConstants.renderSize[1] = params->renderSize.height;

    // compute the constants.
    Fsr2RcasConstants rcasConsts = {};
    const float sharpenessRemapped = (-2.0f * params->sharpness) + 2.0f;
    FsrRcasCon(rcasConsts.rcasConfig, sharpenessRemapped);

    // initialize constantBuffers data
    memcpy(&context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_FSR2].data,        &context->constants,        context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_FSR2].num32BitEntries * sizeof(uint32_t));
    memcpy(&context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_SPD].data,         &luminancePyramidConstants, context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_SPD].num32BitEntries * sizeof(uint32_t));
    memcpy(&context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_RCAS].data,        &rcasConsts,                context->constantBuffers[FFXM_FSR2_CONSTANTBUFFER_IDENTIFIER_RCAS].num32BitEntries * sizeof(uint32_t));

	const uint32_t renderW = context->constants.renderSize[0];
	const uint32_t renderH = context->constants.renderSize[1];

    if (!applyUltraPerformanceOptimizations)
    {
        scheduleDispatch(context, params, &context->pipelineComputeLuminancePyramid, dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1]);
    }
	scheduleFragment(context, params, &context->pipelineReconstructPreviousDepth, renderW, renderH);
	scheduleFragment(context, params, &context->pipelineDepthClip, renderW, renderH);

    scheduleDispatch(context, params, &context->pipelineLock, dispatchSrcX, dispatchSrcY);

    const bool sharpenEnabled = params->enableSharpening;
	scheduleFragment(context, params, sharpenEnabled ? &context->pipelineAccumulateSharpen : &context->pipelineAccumulate,
					 context->contextDescription.displaySize.width, context->contextDescription.displaySize.height);

    // RCAS
    if (sharpenEnabled) {

        // Run RCAS
        scheduleFragment(context, params, &context->pipelineRCAS, context->contextDescription.displaySize.width,
						 context->contextDescription.displaySize.height);
    }

    context->resourceFrameIndex = (context->resourceFrameIndex + 1) % FSR2_MAX_QUEUED_FRAMES;

    // Fsr2MaxQueuedFrames must be an even number.
    FFXM_STATIC_ASSERT((FSR2_MAX_QUEUED_FRAMES & 1) == 0);

    context->contextDescription.backendInterface.fpExecuteGpuJobs(&context->contextDescription.backendInterface, commandList);

    // release dynamic resources
    context->contextDescription.backendInterface.fpUnregisterResources(&context->contextDescription.backendInterface, commandList, context->effectContextId);

    return FFXM_OK;
}

FfxmErrorCode ffxmFsr2ContextCreate(FfxmFsr2Context* context, const FfxmFsr2ContextDescription* contextDescription)
{
    // zero context memory
    memset(context, 0, sizeof(FfxmFsr2Context));

    // check pointers are valid.
    FFXM_RETURN_ON_ERROR(
        context,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        contextDescription,
        FFXM_ERROR_INVALID_POINTER);

    // validate that all callbacks are set for the interface
    FFXM_RETURN_ON_ERROR(contextDescription->backendInterface.fpGetDeviceCapabilities, FFXM_ERROR_INCOMPLETE_INTERFACE);
    FFXM_RETURN_ON_ERROR(contextDescription->backendInterface.fpCreateBackendContext, FFXM_ERROR_INCOMPLETE_INTERFACE);
    FFXM_RETURN_ON_ERROR(contextDescription->backendInterface.fpDestroyBackendContext, FFXM_ERROR_INCOMPLETE_INTERFACE);

    // if a scratch buffer is declared, then we must have a size
    if (contextDescription->backendInterface.scratchBuffer) {

        FFXM_RETURN_ON_ERROR(contextDescription->backendInterface.scratchBufferSize, FFXM_ERROR_INCOMPLETE_INTERFACE);
    }

    // ensure the context is large enough for the internal context.
    FFXM_STATIC_ASSERT(sizeof(FfxmFsr2Context) >= sizeof(FfxmFsr2Context_Private));

    // create the context.
    FfxmFsr2Context_Private* contextPrivate = (FfxmFsr2Context_Private*)(context);
    const FfxmErrorCode errorCode = fsr2Create(contextPrivate, contextDescription);

    return errorCode;
}

FfxmErrorCode ffxmFsr2ContextDestroy(FfxmFsr2Context* context)
{
    FFXM_RETURN_ON_ERROR(
        context,
        FFXM_ERROR_INVALID_POINTER);

    // destroy the context.
    FfxmFsr2Context_Private* contextPrivate = (FfxmFsr2Context_Private*)(context);
    const FfxmErrorCode errorCode = fsr2Release(contextPrivate);
    return errorCode;
}

FfxmErrorCode ffxmFsr2ContextDispatch(FfxmFsr2Context* context, const FfxmFsr2DispatchDescription* dispatchParams)
{
    FFXM_RETURN_ON_ERROR(
        context,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        dispatchParams,
        FFXM_ERROR_INVALID_POINTER);

    FfxmFsr2Context_Private* contextPrivate = (FfxmFsr2Context_Private*)(context);

    // validate that renderSize is within the maximum.
    FFXM_RETURN_ON_ERROR(
        dispatchParams->renderSize.width <= contextPrivate->contextDescription.maxRenderSize.width,
        FFXM_ERROR_OUT_OF_RANGE);
    FFXM_RETURN_ON_ERROR(
        dispatchParams->renderSize.height <= contextPrivate->contextDescription.maxRenderSize.height,
        FFXM_ERROR_OUT_OF_RANGE);
    FFXM_RETURN_ON_ERROR(
        contextPrivate->device,
        FFXM_ERROR_NULL_DEVICE);

    // dispatch the FSR2 passes.
    const FfxmErrorCode errorCode = fsr2Dispatch(contextPrivate, dispatchParams);
    return errorCode;
}

float ffxmFsr2GetUpscaleRatioFactor(FfxmFsr2UpscalingRatio upscalingRatio)
{
	switch(upscalingRatio)
	{
    case FFXM_FSR2_UPSCALING_RATIO_X1_5:
        return 1.5f;
	case FFXM_FSR2_UPSCALING_RATIO_X1_7:
        return 1.7f;
	case FFXM_FSR2_UPSCALING_RATIO_X2:
        return 2.0f;
    default:
        return 0.0f;
    }
}

FfxmErrorCode ffxmFsr2GetRenderResolutionFromUpscalingRatio(
    uint32_t* renderWidth,
    uint32_t* renderHeight,
    uint32_t displayWidth,
    uint32_t displayHeight,
    FfxmFsr2UpscalingRatio upscalingRatio)
{
    FFXM_RETURN_ON_ERROR(
        renderWidth,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        renderHeight,
        FFXM_ERROR_INVALID_POINTER);
	FFXM_RETURN_ON_ERROR(FFXM_FSR2_UPSCALING_RATIO_X1_5 <= upscalingRatio && upscalingRatio <= FFXM_FSR2_UPSCALING_RATIO_X2,
        FFXM_ERROR_INVALID_ENUM);

    // scale by the predefined ratios in each dimension.
	const float ratio = ffxmFsr2GetUpscaleRatioFactor(upscalingRatio);
    const uint32_t scaledDisplayWidth = (uint32_t)((float)displayWidth / ratio);
    const uint32_t scaledDisplayHeight = (uint32_t)((float)displayHeight / ratio);
    *renderWidth = scaledDisplayWidth;
    *renderHeight = scaledDisplayHeight;

    return FFXM_OK;
}

int32_t ffxmFsr2GetJitterPhaseCount(int32_t renderWidth, int32_t displayWidth)
{
    const float basePhaseCount = 8.0f;
    const int32_t jitterPhaseCount = int32_t(basePhaseCount * pow((float(displayWidth) / renderWidth), 2.0f));
    return jitterPhaseCount;
}

FfxmErrorCode ffxmFsr2GetJitterOffset(float* outX, float* outY, int32_t index, int32_t phaseCount)
{
    FFXM_RETURN_ON_ERROR(
        outX,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        outY,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        phaseCount > 0,
        FFXM_ERROR_INVALID_ARGUMENT);

    const float x = halton((index % phaseCount) + 1, 2) - 0.5f;
    const float y = halton((index % phaseCount) + 1, 3) - 0.5f;

    *outX = x;
    *outY = y;
    return FFXM_OK;
}

FFXM_API bool ffxmFsr2ResourceIsNull(FfxmResource resource)
{
    return resource.resource == NULL;
}

FfxmErrorCode ffxmFsr2ContextGenerateReactiveMask(FfxmFsr2Context* context, const FfxmFsr2GenerateReactiveDescription* params)
{
    FFXM_RETURN_ON_ERROR(
        context,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        params,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        params->commandList,
        FFXM_ERROR_INVALID_POINTER);

    FfxmFsr2Context_Private* contextPrivate = (FfxmFsr2Context_Private*)(context);

    FFXM_RETURN_ON_ERROR(
        contextPrivate->device,
        FFXM_ERROR_NULL_DEVICE);

    // take a short cut to the command list
    FfxmCommandList commandList = params->commandList;

    FfxmPipelineState* pipeline = &contextPrivate->pipelineGenerateReactive;

    // save internal reactive resource
    FfxmResourceInternal internalReactive = contextPrivate->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE];

    FfxmFragmentJobDescription jobDescriptor = {};
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->colorOpaqueOnly, contextPrivate->effectContextId, &contextPrivate->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->colorPreUpscale, contextPrivate->effectContextId, &contextPrivate->srvResources[FFXM_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->outReactive, contextPrivate->effectContextId, &contextPrivate->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE]);

    jobDescriptor.rtTextures[0] = contextPrivate->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE];

    wcscpy(jobDescriptor.srvTextureNames[0], pipeline->srvTextureBindings[0].name);
    wcscpy(jobDescriptor.srvTextureNames[1], pipeline->srvTextureBindings[1].name);
    wcscpy(jobDescriptor.rtTextureNames[0], pipeline->rtBindings[0].name);

    jobDescriptor.viewport[0] = params->renderSize.width;
    jobDescriptor.viewport[1] = params->renderSize.height;
    jobDescriptor.pipeline = pipeline;

    for (uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex) {

        const uint32_t currentResourceId = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
        const FfxmResourceInternal currentResource = contextPrivate->srvResources[currentResourceId];
        jobDescriptor.srvTextures[currentShaderResourceViewIndex] = currentResource;
        wcscpy(jobDescriptor.srvTextureNames[currentShaderResourceViewIndex], pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
    }

    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex) {
        wcscpy(jobDescriptor.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
        jobDescriptor.cbs[currentRootConstantIndex] = contextPrivate->constantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
    }

    int32_t renderSize[2];
    renderSize[0] = params->renderSize.width;
    renderSize[1] = params->renderSize.height;
    memcpy(&jobDescriptor.cbs[0].data, renderSize, sizeof(renderSize));

    Fsr2GenerateReactiveConstants constants = {};
    constants.scale = params->scale;
    constants.threshold = params->cutoffThreshold;
    constants.binaryValue = params->binaryValue;
    constants.flags = params->flags;

    jobDescriptor.cbs[1].num32BitEntries = sizeof(constants);
    memcpy(&jobDescriptor.cbs[1].data, &constants, sizeof(constants));

    FfxmGpuJobDescription fragmentJob = { FFXM_GPU_JOB_FRAGMENT };
    fragmentJob.fragmentJobDescription = jobDescriptor;

    contextPrivate->contextDescription.backendInterface.fpScheduleGpuJob(&contextPrivate->contextDescription.backendInterface, &fragmentJob);

    contextPrivate->contextDescription.backendInterface.fpExecuteGpuJobs(&contextPrivate->contextDescription.backendInterface, commandList);

    // restore internal reactive
    contextPrivate->rtResources[FFXM_FSR2_RESOURCE_IDENTIFIER_AUTOREACTIVE] = internalReactive;

    // release dynamic resources
    contextPrivate->contextDescription.backendInterface.fpUnregisterResources(&contextPrivate->contextDescription.backendInterface, commandList, contextPrivate->effectContextId);

    return FFXM_OK;
}
} // namespace arm
