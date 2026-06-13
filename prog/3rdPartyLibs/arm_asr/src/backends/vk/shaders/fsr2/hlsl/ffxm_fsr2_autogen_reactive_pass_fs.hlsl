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

#define FSR2_BIND_SRV_INPUT_OPAQUE_ONLY                     0
#define FSR2_BIND_SRV_INPUT_COLOR                           1

#define FSR2_BIND_CB_FSR2                                   0
#define FSR2_BIND_CB_REACTIVE                               1

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

struct GenReactiveMaskOutputs
{
    FfxFloat32 fReactiveMask : SV_TARGET0;
};

struct VertexOut
{
	float4 position : SV_POSITION;
};

GenReactiveMaskOutputs main(float4 SvPosition : SV_POSITION)
{
    uint2 uPixelCoord = uint2(SvPosition.xy);

    float3 ColorPreAlpha    = LoadOpaqueOnly( FFXM_MIN16_I2(uPixelCoord) ).rgb;
    float3 ColorPostAlpha   = LoadInputColor(uPixelCoord).rgb;

    if (GenReactiveFlags() & FFXM_FSR2_AUTOREACTIVEFLAGS_APPLY_TONEMAP)
    {
        ColorPreAlpha = Tonemap(ColorPreAlpha);
        ColorPostAlpha = Tonemap(ColorPostAlpha);
    }

    if (GenReactiveFlags() & FFXM_FSR2_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP)
    {
        ColorPreAlpha = InverseTonemap(ColorPreAlpha);
        ColorPostAlpha = InverseTonemap(ColorPostAlpha);
    }

    float out_reactive_value = 0.f;
    float3 delta = abs(ColorPostAlpha - ColorPreAlpha);

    out_reactive_value = (GenReactiveFlags() & FFXM_FSR2_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX) ? max(delta.x, max(delta.y, delta.z)) : length(delta);
    out_reactive_value *= GenReactiveScale();

    out_reactive_value = (GenReactiveFlags() & FFXM_FSR2_AUTOREACTIVEFLAGS_APPLY_THRESHOLD) ? (out_reactive_value < GenReactiveThreshold() ? 0 : GenReactiveBinaryValue()) : out_reactive_value;

    GenReactiveMaskOutputs results = (GenReactiveMaskOutputs)0;
    results.fReactiveMask = out_reactive_value;

    return results;
}
