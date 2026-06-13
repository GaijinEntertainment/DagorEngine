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

#define FSR2_BIND_SRV_INPUT_EXPOSURE                         0
#define FSR2_BIND_SRV_DILATED_REACTIVE_MASKS                 1
#if FFXM_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_SRV_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA 2
#else
#define FSR2_BIND_SRV_DILATED_MOTION_VECTORS                 2
#endif

#else
#define FSR2_BIND_SRV_INPUT_MOTION_VECTORS                   2
#endif
#define FSR2_BIND_SRV_INTERNAL_UPSCALED                      3
#define FSR2_BIND_SRV_LOCK_STATUS                            4

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_SRV_INPUT_COLOR                            5
#else
#define FSR2_BIND_SRV_PREPARED_INPUT_COLOR                   5
#endif

#define FSR2_BIND_SRV_LANCZOS_LUT                            6
#define FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT               7
#define FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS                   8
#define FSR2_BIND_SRV_AUTO_EXPOSURE                          9

#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_SRV_LUMA_HISTORY                           10
#define FSR2_BIND_SRV_TEMPORAL_REACTIVE                      11
#endif

#define FSR2_BIND_UAV_NEW_LOCKS                              12

#define FSR2_BIND_CB_FSR2                                    0

// Global mandatory defines
#if !defined(FFXM_HALF)
#define FFXM_HALF 1
#endif
#if !defined(FFXM_GPU)
#define FFXM_GPU 1
#endif
#if !defined(FFXM_HLSL)
#define FFXM_HLSL 1
#endif

#include "fsr2/ffxm_fsr2_callbacks_hlsl.h"
#include "fsr2/ffxm_fsr2_common.h"
#include "fsr2/ffxm_fsr2_sample.h"
#include "fsr2/ffxm_fsr2_upsample.h"
#include "fsr2/ffxm_fsr2_postprocess_lock_status.h"
#include "fsr2/ffxm_fsr2_reproject.h"
#include "fsr2/ffxm_fsr2_accumulate.h"

struct VertexOut
{
	float4 position : SV_POSITION;
};

struct AccumulateOutputsFS
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x3 fUpscaledColor    : SV_TARGET0;
    FfxFloat32x2 fLockStatus        : SV_TARGET1;
#if FFXM_FSR2_OPTION_APPLY_SHARPENING == 0
    FfxFloat32x3 fColor             : SV_TARGET2;
#endif
#elif !FFXM_SHADER_QUALITY_BALANCED_OR_PERFORMANCE
    FfxFloat32x4 fColorAndWeight    : SV_TARGET0;
    FfxFloat32x2 fLockStatus        : SV_TARGET1;
    FfxFloat32x4 fLumaHistory       : SV_TARGET2;
#if FFXM_FSR2_OPTION_APPLY_SHARPENING == 0
    FfxFloat32x3 fColor             : SV_TARGET3;
#endif
#else // FFXM_SHADER_QUALITY_BALANCED_OR_PERFORMANCE
    FfxFloat32x3 fUpscaledColor     : SV_TARGET0;
    FfxFloat32 fTemporalReactive    : SV_TARGET1;
    FfxFloat32x2 fLockStatus        : SV_TARGET2;
#if FFXM_FSR2_OPTION_APPLY_SHARPENING == 0
    FfxFloat32x3 fColor             : SV_TARGET3;
#endif
#endif
};

AccumulateOutputsFS main(float4 SvPosition : SV_POSITION)
{
    uint2 uPixelCoord = uint2(SvPosition.xy);
    AccumulateOutputs result = Accumulate(uPixelCoord);
    AccumulateOutputsFS output = (AccumulateOutputsFS)0;
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    output.fUpscaledColor = result.fColorAndWeight.xyz;
#elif !FFXM_SHADER_QUALITY_BALANCED_OR_PERFORMANCE
    output.fColorAndWeight = result.fColorAndWeight;
    output.fLumaHistory = result.fLumaHistory;
#else
    output.fUpscaledColor = result.fUpscaledColor;
    output.fTemporalReactive = result.fTemporalReactive;
#endif
    output.fLockStatus = result.fLockStatus;
#if FFXM_FSR2_OPTION_APPLY_SHARPENING == 0
    output.fColor = result.fColor;
#endif
    return output;
}