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

#define GROUP_SIZE  8
#define FSR_RCAS_DENOISE 1

#include "./ffxm_core.h"

struct RCASOutputs
{
    FfxFloat32x3 fUpscaledColor;
};

#if FFXM_HALF
#define USE_FSR_RCASH 1
#else
#define USE_FSR_RCASH 0
#endif

#if USE_FSR_RCASH
#define FSR_RCAS_H 1
FfxFloat16x4 FsrRcasLoadH(FfxInt16x2 p)
{
    FfxFloat16x4 fColor = LoadRCAS_Input(p);
    fColor.rgb = FfxFloat16x3(PrepareRgb(fColor.rgb, Exposure(), PreExposure()));
    return fColor;
}
void FsrRcasInputH(inout FfxFloat16 r,inout FfxFloat16 g,inout FfxFloat16 b)
{

}

#else
#define FSR_RCAS_F 1
FfxFloat32x4 FsrRcasLoadF(FfxInt32x2 p)
{
    FfxFloat32x4 fColor = LoadRCAS_Input(p);

    fColor.rgb = PrepareRgb(fColor.rgb, Exposure(), PreExposure());

    return fColor;
}
void FsrRcasInputF(inout FfxFloat32 r, inout FfxFloat32 g, inout FfxFloat32 b) {}
#endif

#include "./fsr1/ffxm_fsr1.h"

void CurrFilter(FFXM_MIN16_U2 pos, FFXM_PARAMETER_INOUT RCASOutputs results)
{
#if USE_FSR_RCASH
    FfxFloat16x3 c;
    FsrRcasH(c.r, c.g, c.b, pos, RCASConfig());

    c = UnprepareRgb(c, FfxFloat16(Exposure()));
#else
    FfxFloat32x3 c;
    FsrRcasF(c.r, c.g, c.b, pos, RCASConfig());

    c = UnprepareRgb(c, Exposure());
#endif
    results.fUpscaledColor = c;
}

RCASOutputs RCAS(FfxUInt32x2 gxy)
{
#ifdef FFXM_HLSL
    RCASOutputs results = (RCASOutputs)0;
#else
    RCASOutputs results;
#endif
    CurrFilter(FFXM_MIN16_U2(gxy), results);
    return results;
}
