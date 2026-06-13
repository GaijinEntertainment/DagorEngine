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

#define FSR2_BIND_SRV_INPUT_MOTION_VECTORS                  0
#define FSR2_BIND_SRV_INPUT_DEPTH                           1
#define FSR2_BIND_SRV_INPUT_COLOR                           2

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH      3
#else
#define FSR2_BIND_SRV_INPUT_EXPOSURE                        3
#define FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH      4
#endif

#define FSR2_BIND_CB_FSR2                                   0

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
#include "fsr2/ffxm_fsr2_reconstruct_dilated_velocity_and_previous_depth.h"

struct VertexOut
{
	float4 position : SV_POSITION;
};

struct ReconstructPrevDepthOutputsFS
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x4 fDepthMotionVectorLuma: SV_TARGET0;
#else
    FfxFloat32 fDepth           : SV_TARGET0;
    FfxFloat32x2 fMotionVector  : SV_TARGET1;
    FfxFloat32 fLuma            : SV_TARGET2;
#endif
};


ReconstructPrevDepthOutputsFS main(float4 SvPosition : SV_POSITION)
{
    uint2 uPixelCoord = uint2(SvPosition.xy);
    ReconstructPrevDepthOutputs result = ReconstructAndDilate(uPixelCoord);
    ReconstructPrevDepthOutputsFS output = (ReconstructPrevDepthOutputsFS)0;
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    output.fDepthMotionVectorLuma = FfxFloat32x4(result.fDepth, result.fMotionVector, result.fLuma);
#else
    output.fDepth = result.fDepth;
    output.fMotionVector = result.fMotionVector;
    output.fLuma = result.fLuma;
#endif
    return output;
}
