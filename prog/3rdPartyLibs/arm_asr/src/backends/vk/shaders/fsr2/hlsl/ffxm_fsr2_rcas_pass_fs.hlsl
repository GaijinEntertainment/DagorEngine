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

#define FSR2_BIND_SRV_INPUT_EXPOSURE        0
#define FSR2_BIND_SRV_RCAS_INPUT            1

#define FSR2_BIND_CB_FSR2                   0
#define FSR2_BIND_CB_RCAS                   1

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
#include "fsr2/ffxm_fsr2_rcas.h"

struct VertexOut
{
	float4 position : SV_POSITION;
};

struct RCASOutputsFS
{
    FfxFloat32x3 fUpscaledColor    : SV_TARGET0;
};

RCASOutputsFS main(float4 SvPosition : SV_POSITION)
{
    uint2 uPixelCoord = uint2(SvPosition.xy);
    RCASOutputs result = RCAS(uPixelCoord);
    RCASOutputsFS output = (RCASOutputsFS)0;
    output.fUpscaledColor = result.fUpscaledColor;
    return output;
}
