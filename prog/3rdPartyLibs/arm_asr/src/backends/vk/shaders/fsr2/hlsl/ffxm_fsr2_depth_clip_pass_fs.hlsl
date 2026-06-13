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

#define FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH      0
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_SRV_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA 1
#define FSR2_BIND_SRV_PREV_DILATED_DEPTH_MOTION_VECTORS_INPUT_LUMA 2
#else
#define FSR2_BIND_SRV_DILATED_MOTION_VECTORS                1
#define FSR2_BIND_SRV_DILATED_DEPTH                         2
#define FSR2_BIND_SRV_REACTIVE_MASK                         3
#define FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK     4
#define FSR2_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS       5
#endif
#define FSR2_BIND_SRV_INPUT_MOTION_VECTORS                  6
#define FSR2_BIND_SRV_INPUT_COLOR                           7
#define FSR2_BIND_SRV_INPUT_DEPTH                           8
#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FSR2_BIND_SRV_INPUT_EXPOSURE                        9
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
#include "fsr2/ffxm_fsr2_depth_clip.h"

struct VertexOut
{
	float4 position : SV_POSITION;
};


struct DepthClipOutputsFS
{
    FfxFloat32x2 fDilatedReactiveMasks    : SV_TARGET0;
#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x4 fTonemapped              : SV_TARGET1;
#endif
};

DepthClipOutputsFS main(float4 SvPosition : SV_POSITION)
{
    uint2 uPixelCoord = uint2(SvPosition.xy);
    DepthClipOutputs result = DepthClip(uPixelCoord);
    DepthClipOutputsFS output = (DepthClipOutputsFS)0;
    output.fDilatedReactiveMasks = result.fDilatedReactiveMasks;
#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    output.fTonemapped = result.fTonemapped;
#endif
    return output;
}
