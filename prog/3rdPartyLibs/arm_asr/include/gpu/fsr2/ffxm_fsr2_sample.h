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

#ifndef FFXM_FSR2_SAMPLE_H
#define FFXM_FSR2_SAMPLE_H

// suppress warnings
#ifdef FFXM_HLSL
#pragma warning(disable: 4008) // potentially divide by zero
#endif //FFXM_HLSL

struct FetchedBilinearSamples {

    FfxFloat32x4 fColor00;
    FfxFloat32x4 fColor10;

    FfxFloat32x4 fColor01;
    FfxFloat32x4 fColor11;
};

struct FetchedBicubicSamples {

    FfxFloat32x4 fColor00;
    FfxFloat32x4 fColor10;
    FfxFloat32x4 fColor20;
    FfxFloat32x4 fColor30;

    FfxFloat32x4 fColor01;
    FfxFloat32x4 fColor11;
    FfxFloat32x4 fColor21;
    FfxFloat32x4 fColor31;

    FfxFloat32x4 fColor02;
    FfxFloat32x4 fColor12;
    FfxFloat32x4 fColor22;
    FfxFloat32x4 fColor32;

    FfxFloat32x4 fColor03;
    FfxFloat32x4 fColor13;
    FfxFloat32x4 fColor23;
    FfxFloat32x4 fColor33;
};

#if FFXM_HALF
struct FetchedBilinearSamplesMin16 {

    FFXM_MIN16_F4 fColor00;
    FFXM_MIN16_F4 fColor10;

    FFXM_MIN16_F4 fColor01;
    FFXM_MIN16_F4 fColor11;
};

struct FetchedBicubicSamplesMin16 {

    FFXM_MIN16_F4 fColor00;
    FFXM_MIN16_F4 fColor10;
    FFXM_MIN16_F4 fColor20;
    FFXM_MIN16_F4 fColor30;

    FFXM_MIN16_F4 fColor01;
    FFXM_MIN16_F4 fColor11;
    FFXM_MIN16_F4 fColor21;
    FFXM_MIN16_F4 fColor31;

    FFXM_MIN16_F4 fColor02;
    FFXM_MIN16_F4 fColor12;
    FFXM_MIN16_F4 fColor22;
    FFXM_MIN16_F4 fColor32;

    FFXM_MIN16_F4 fColor03;
    FFXM_MIN16_F4 fColor13;
    FFXM_MIN16_F4 fColor23;
    FFXM_MIN16_F4 fColor33;
};

struct Fetched9TapSamplesMin16 {

    FFXM_MIN16_F4 fColor00;
    FFXM_MIN16_F4 fColor10;
    FFXM_MIN16_F4 fColor20;

    FFXM_MIN16_F4 fColor01;
    FFXM_MIN16_F4 fColor11;
    FFXM_MIN16_F4 fColor21;

    FFXM_MIN16_F4 fColor02;
    FFXM_MIN16_F4 fColor12;
    FFXM_MIN16_F4 fColor22;
};

#else //FFXM_HALF
#define FetchedBicubicSamplesMin16 FetchedBicubicSamples
#define FetchedBilinearSamplesMin16 FetchedBilinearSamples
#endif //FFXM_HALF

FfxFloat32x4 Linear(FfxFloat32x4 A, FfxFloat32x4 B, FfxFloat32 t)
{
    return A + (B - A) * t;
}

FfxFloat32x4 Bilinear(FetchedBilinearSamples BilinearSamples, FfxFloat32x2 fPxFrac)
{
    FfxFloat32x4 fColorX0 = Linear(BilinearSamples.fColor00, BilinearSamples.fColor10, fPxFrac.x);
    FfxFloat32x4 fColorX1 = Linear(BilinearSamples.fColor01, BilinearSamples.fColor11, fPxFrac.x);
    FfxFloat32x4 fColorXY = Linear(fColorX0, fColorX1, fPxFrac.y);
    return fColorXY;
}

#if FFXM_HALF
FFXM_MIN16_F4 Linear(FFXM_MIN16_F4 A, FFXM_MIN16_F4 B, FFXM_MIN16_F t)
{
    return A + (B - A) * t;
}

FFXM_MIN16_F4 Bilinear(FetchedBilinearSamplesMin16 BilinearSamples, FFXM_MIN16_F2 fPxFrac)
{
    FFXM_MIN16_F4 fColorX0 = Linear(BilinearSamples.fColor00, BilinearSamples.fColor10, fPxFrac.x);
    FFXM_MIN16_F4 fColorX1 = Linear(BilinearSamples.fColor01, BilinearSamples.fColor11, fPxFrac.x);
    FFXM_MIN16_F4 fColorXY = Linear(fColorX0, fColorX1, fPxFrac.y);
    return fColorXY;
}
#endif

FfxFloat32 Lanczos2NoClamp(FfxFloat32 x)
{
    const FfxFloat32 PI = 3.141592653589793f; // TODO: share SDK constants
    return abs(x) < FSR2_EPSILON ? 1.f : (sin(PI * x) / (PI * x)) * (sin(0.5f * PI * x) / (0.5f * PI * x));
}

FfxFloat32 Lanczos2(FfxFloat32 x)
{
    x = ffxMin(abs(x), 2.0f);
    return Lanczos2NoClamp(x);
}

#if FFXM_HALF

#if 1
FFXM_MIN16_F Lanczos2NoClamp(FFXM_MIN16_F x)
{
    const FFXM_MIN16_F PI = FFXM_MIN16_F(3.141592653589793f); // TODO: share SDK constants
    return abs(x) < FFXM_MIN16_F(FSR2_EPSILON) ? FFXM_MIN16_F(1.f) : (sin(PI * x) / (PI * x)) * (sin(FFXM_MIN16_F(0.5f) * PI * x) / (FFXM_MIN16_F(0.5f) * PI * x));
}
#endif

FFXM_MIN16_F Lanczos2(FFXM_MIN16_F x)
{
    x = ffxMin(abs(x), FFXM_MIN16_F(2.0f));
    return FFXM_MIN16_F(Lanczos2NoClamp(x));
}
#endif //FFXM_HALF

// FSR1 lanczos approximation. Input is x*x and must be <= 4.
FfxFloat32 Lanczos2ApproxSqNoClamp(FfxFloat32 x2)
{
    FfxFloat32 a = (2.0f / 5.0f) * x2 - 1;
    FfxFloat32 b = (1.0f / 4.0f) * x2 - 1;
    return ((25.0f / 16.0f) * a * a - (25.0f / 16.0f - 1)) * (b * b);
}

#if FFXM_HALF
FFXM_MIN16_F Lanczos2ApproxSqNoClamp(FFXM_MIN16_F x2)
{
    FFXM_MIN16_F a = FFXM_MIN16_F(2.0f / 5.0f) * x2 - FFXM_MIN16_F(1);
    FFXM_MIN16_F b = FFXM_MIN16_F(1.0f / 4.0f) * x2 - FFXM_MIN16_F(1);
    return (FFXM_MIN16_F(25.0f / 16.0f) * a * a - FFXM_MIN16_F(25.0f / 16.0f - 1)) * (b * b);
}
#endif //FFXM_HALF

FfxFloat32 Lanczos2ApproxSq(FfxFloat32 x2)
{
    x2 = ffxMin(x2, 4.0f);
    return Lanczos2ApproxSqNoClamp(x2);
}

#if FFXM_HALF
FFXM_MIN16_F Lanczos2ApproxSq(FFXM_MIN16_F x2)
{
    x2 = ffxMin(x2, FFXM_MIN16_F(4.0f));
    return Lanczos2ApproxSqNoClamp(x2);
}
#endif //FFXM_HALF

FfxFloat32 Lanczos2ApproxNoClamp(FfxFloat32 x)
{
    return Lanczos2ApproxSqNoClamp(x * x);
}

#if FFXM_HALF
FFXM_MIN16_F Lanczos2ApproxNoClamp(FFXM_MIN16_F x)
{
    return Lanczos2ApproxSqNoClamp(x * x);
}
#endif //FFXM_HALF

FfxFloat32 Lanczos2Approx(FfxFloat32 x)
{
    return Lanczos2ApproxSq(x * x);
}

#if FFXM_HALF
FFXM_MIN16_F Lanczos2Approx(FFXM_MIN16_F x)
{
    return Lanczos2ApproxSq(x * x);
}
#endif //FFXM_HALF

FfxFloat32 Lanczos2_UseLUT(FfxFloat32 x)
{
    return SampleLanczos2Weight(abs(x));
}

#if FFXM_HALF
FFXM_MIN16_F Lanczos2_UseLUT(FFXM_MIN16_F x)
{
    return FFXM_MIN16_F(SampleLanczos2Weight(abs(x)));
}
#endif //FFXM_HALF

FfxFloat32x4 Lanczos2_UseLUT(FfxFloat32x4 fColor0, FfxFloat32x4 fColor1, FfxFloat32x4 fColor2, FfxFloat32x4 fColor3, FfxFloat32 t)
{
    FfxFloat32 fWeight0 = Lanczos2_UseLUT(-1.f - t);
    FfxFloat32 fWeight1 = Lanczos2_UseLUT(-0.f - t);
    FfxFloat32 fWeight2 = Lanczos2_UseLUT(+1.f - t);
    FfxFloat32 fWeight3 = Lanczos2_UseLUT(+2.f - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}
#if FFXM_HALF
FFXM_MIN16_F4 Lanczos2_UseLUT(FFXM_MIN16_F4 fColor0, FFXM_MIN16_F4 fColor1, FFXM_MIN16_F4 fColor2, FFXM_MIN16_F4 fColor3, FFXM_MIN16_F t)
{
    FFXM_MIN16_F fWeight0 = Lanczos2_UseLUT(FFXM_MIN16_F(-1.f) - t);
    FFXM_MIN16_F fWeight1 = Lanczos2_UseLUT(FFXM_MIN16_F(-0.f) - t);
    FFXM_MIN16_F fWeight2 = Lanczos2_UseLUT(FFXM_MIN16_F(+1.f) - t);
    FFXM_MIN16_F fWeight3 = Lanczos2_UseLUT(FFXM_MIN16_F(+2.f) - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}
#endif

FfxFloat32x4 Lanczos2(FfxFloat32x4 fColor0, FfxFloat32x4 fColor1, FfxFloat32x4 fColor2, FfxFloat32x4 fColor3, FfxFloat32 t)
{
    FfxFloat32 fWeight0 = Lanczos2(-1.f - t);
    FfxFloat32 fWeight1 = Lanczos2(-0.f - t);
    FfxFloat32 fWeight2 = Lanczos2(+1.f - t);
    FfxFloat32 fWeight3 = Lanczos2(+2.f - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}

FfxFloat32x4 Lanczos2(FetchedBicubicSamples Samples, FfxFloat32x2 fPxFrac)
{
    FfxFloat32x4 fColorX0 = Lanczos2(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FfxFloat32x4 fColorX1 = Lanczos2(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FfxFloat32x4 fColorX2 = Lanczos2(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FfxFloat32x4 fColorX3 = Lanczos2(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FfxFloat32x4 fColorXY = Lanczos2(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FfxFloat32x4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FfxFloat32x4 fDeringingMin = fDeringingSamples[0];
    FfxFloat32x4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex) {

        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}

#if FFXM_HALF
FFXM_MIN16_F4 Lanczos2(FFXM_MIN16_F4 fColor0, FFXM_MIN16_F4 fColor1, FFXM_MIN16_F4 fColor2, FFXM_MIN16_F4 fColor3, FFXM_MIN16_F t)
{
    FFXM_MIN16_F fWeight0 = Lanczos2(FFXM_MIN16_F(-1.f) - t);
    FFXM_MIN16_F fWeight1 = Lanczos2(FFXM_MIN16_F(-0.f) - t);
    FFXM_MIN16_F fWeight2 = Lanczos2(FFXM_MIN16_F(+1.f) - t);
    FFXM_MIN16_F fWeight3 = Lanczos2(FFXM_MIN16_F(+2.f) - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}

FFXM_MIN16_F4 Lanczos2(FetchedBicubicSamplesMin16 Samples, FFXM_MIN16_F2 fPxFrac)
{
    FFXM_MIN16_F4 fColorX0 = Lanczos2(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FFXM_MIN16_F4 fColorX1 = Lanczos2(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FFXM_MIN16_F4 fColorX2 = Lanczos2(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FFXM_MIN16_F4 fColorX3 = Lanczos2(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FFXM_MIN16_F4 fColorXY = Lanczos2(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FFXM_MIN16_F4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FFXM_MIN16_F4 fDeringingMin = fDeringingSamples[0];
    FFXM_MIN16_F4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}

FFXM_MIN16_F4 Lanczos2(FFXM_MIN16_F4 fColor0, FFXM_MIN16_F4 fColor1, FFXM_MIN16_F4 fColor2, FFXM_MIN16_F t)
{
    FFXM_MIN16_F fWeight0 = Lanczos2(FFXM_MIN16_F(-1.f) - t);
    FFXM_MIN16_F fWeight1 = Lanczos2(FFXM_MIN16_F(-0.f) - t);
    FFXM_MIN16_F fWeight2 = Lanczos2(FFXM_MIN16_F(+1.f) - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2) / (fWeight0 + fWeight1 + fWeight2);
}

FFXM_MIN16_F4 Lanczos2Approx(FFXM_MIN16_F4 fColor0, FFXM_MIN16_F4 fColor1, FFXM_MIN16_F4 fColor2, FFXM_MIN16_F t)
{
    FFXM_MIN16_F fWeight0 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(-1.f) - t);
    FFXM_MIN16_F fWeight1 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(-0.f) - t);
    FFXM_MIN16_F fWeight2 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(+1.f) - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2) / (fWeight0 + fWeight1 + fWeight2);
}

FFXM_MIN16_F4 Lanczos2Approx(Fetched9TapSamplesMin16 Samples, FFXM_MIN16_F2 fPxFrac)
{
    FFXM_MIN16_F4 fColorX0 = Lanczos2Approx(Samples.fColor00, Samples.fColor10, Samples.fColor20, fPxFrac.x);
    FFXM_MIN16_F4 fColorX1 = Lanczos2Approx(Samples.fColor01, Samples.fColor11, Samples.fColor21, fPxFrac.x);
    FFXM_MIN16_F4 fColorX2 = Lanczos2Approx(Samples.fColor02, Samples.fColor12, Samples.fColor22, fPxFrac.x);
    FFXM_MIN16_F4 fColorXY = Lanczos2Approx(fColorX0, fColorX1, fColorX2, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing
    const FfxInt32 iDeringingSampleCount = 4;
    const FFXM_MIN16_F4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FFXM_MIN16_F4 fDeringingMin = fDeringingSamples[0];
    FFXM_MIN16_F4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}

#endif //FFXM_HALF


FfxFloat32x4 Lanczos2LUT(FetchedBicubicSamples Samples, FfxFloat32x2 fPxFrac)
{
    FfxFloat32x4 fColorX0 = Lanczos2_UseLUT(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FfxFloat32x4 fColorX1 = Lanczos2_UseLUT(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FfxFloat32x4 fColorX2 = Lanczos2_UseLUT(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FfxFloat32x4 fColorX3 = Lanczos2_UseLUT(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FfxFloat32x4 fColorXY = Lanczos2_UseLUT(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FfxFloat32x4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FfxFloat32x4 fDeringingMin = fDeringingSamples[0];
    FfxFloat32x4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex) {

        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}

#if FFXM_HALF
FFXM_MIN16_F4 Lanczos2LUT(FetchedBicubicSamplesMin16 Samples, FFXM_MIN16_F2 fPxFrac)
{
    FFXM_MIN16_F4 fColorX0 = Lanczos2_UseLUT(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FFXM_MIN16_F4 fColorX1 = Lanczos2_UseLUT(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FFXM_MIN16_F4 fColorX2 = Lanczos2_UseLUT(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FFXM_MIN16_F4 fColorX3 = Lanczos2_UseLUT(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FFXM_MIN16_F4 fColorXY = Lanczos2_UseLUT(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FFXM_MIN16_F4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FFXM_MIN16_F4 fDeringingMin = fDeringingSamples[0];
    FFXM_MIN16_F4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}
#endif //FFXM_HALF



FfxFloat32x4 Lanczos2Approx(FfxFloat32x4 fColor0, FfxFloat32x4 fColor1, FfxFloat32x4 fColor2, FfxFloat32x4 fColor3, FfxFloat32 t)
{
    FfxFloat32 fWeight0 = Lanczos2ApproxNoClamp(-1.f - t);
    FfxFloat32 fWeight1 = Lanczos2ApproxNoClamp(-0.f - t);
    FfxFloat32 fWeight2 = Lanczos2ApproxNoClamp(+1.f - t);
    FfxFloat32 fWeight3 = Lanczos2ApproxNoClamp(+2.f - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}

#if FFXM_HALF
FFXM_MIN16_F4 Lanczos2Approx(FFXM_MIN16_F4 fColor0, FFXM_MIN16_F4 fColor1, FFXM_MIN16_F4 fColor2, FFXM_MIN16_F4 fColor3, FFXM_MIN16_F t)
{
    FFXM_MIN16_F fWeight0 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(-1.f) - t);
    FFXM_MIN16_F fWeight1 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(-0.f) - t);
    FFXM_MIN16_F fWeight2 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(+1.f) - t);
    FFXM_MIN16_F fWeight3 = Lanczos2ApproxNoClamp(FFXM_MIN16_F(+2.f) - t);
    return (fWeight0 * fColor0 + fWeight1 * fColor1 + fWeight2 * fColor2 + fWeight3 * fColor3) / (fWeight0 + fWeight1 + fWeight2 + fWeight3);
}
#endif //FFXM_HALF

FfxFloat32x4 Lanczos2Approx(FetchedBicubicSamples Samples, FfxFloat32x2 fPxFrac)
{
    FfxFloat32x4 fColorX0 = Lanczos2Approx(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FfxFloat32x4 fColorX1 = Lanczos2Approx(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FfxFloat32x4 fColorX2 = Lanczos2Approx(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FfxFloat32x4 fColorX3 = Lanczos2Approx(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FfxFloat32x4 fColorXY = Lanczos2Approx(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FfxFloat32x4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FfxFloat32x4 fDeringingMin = fDeringingSamples[0];
    FfxFloat32x4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}

#if FFXM_HALF
FFXM_MIN16_F4 Lanczos2Approx(FetchedBicubicSamplesMin16 Samples, FFXM_MIN16_F2 fPxFrac)
{
    FFXM_MIN16_F4 fColorX0 = Lanczos2Approx(Samples.fColor00, Samples.fColor10, Samples.fColor20, Samples.fColor30, fPxFrac.x);
    FFXM_MIN16_F4 fColorX1 = Lanczos2Approx(Samples.fColor01, Samples.fColor11, Samples.fColor21, Samples.fColor31, fPxFrac.x);
    FFXM_MIN16_F4 fColorX2 = Lanczos2Approx(Samples.fColor02, Samples.fColor12, Samples.fColor22, Samples.fColor32, fPxFrac.x);
    FFXM_MIN16_F4 fColorX3 = Lanczos2Approx(Samples.fColor03, Samples.fColor13, Samples.fColor23, Samples.fColor33, fPxFrac.x);
    FFXM_MIN16_F4 fColorXY = Lanczos2Approx(fColorX0, fColorX1, fColorX2, fColorX3, fPxFrac.y);

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    // Deringing

    // TODO: only use 4 by checking jitter
    const FfxInt32 iDeringingSampleCount = 4;
    const FFXM_MIN16_F4 fDeringingSamples[4] = {
        Samples.fColor11,
        Samples.fColor21,
        Samples.fColor12,
        Samples.fColor22,
    };

    FFXM_MIN16_F4 fDeringingMin = fDeringingSamples[0];
    FFXM_MIN16_F4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < iDeringingSampleCount; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }

    fColorXY = clamp(fColorXY, fDeringingMin, fDeringingMax);
#endif
    return fColorXY;
}
#endif

// Clamp by offset direction. Assuming iPxSample is already in range and iPxOffset is compile time constant.
FfxInt32x2 ClampCoord(FfxInt32x2 iPxSample, FfxInt32x2 iPxOffset, FfxInt32x2 iTextureSize)
{
    FfxInt32x2 result = iPxSample + iPxOffset;
    result.x = (iPxOffset.x < 0) ? ffxMax(result.x, 0) : result.x;
    result.x = (iPxOffset.x > 0) ? ffxMin(result.x, iTextureSize.x - 1) : result.x;
    result.y = (iPxOffset.y < 0) ? ffxMax(result.y, 0) : result.y;
    result.y = (iPxOffset.y > 0) ? ffxMin(result.y, iTextureSize.y - 1) : result.y;
    return result;
}
#if FFXM_HALF
FFXM_MIN16_I2 ClampCoord(FFXM_MIN16_I2 iPxSample, FFXM_MIN16_I2 iPxOffset, FFXM_MIN16_I2 iTextureSize)
{
    FFXM_MIN16_I2 result = iPxSample + iPxOffset;
    result.x = (iPxOffset.x < FFXM_MIN16_I(0)) ? ffxMax(result.x, FFXM_MIN16_I(0)) : result.x;
    result.x = (iPxOffset.x > FFXM_MIN16_I(0)) ? ffxMin(result.x, iTextureSize.x - FFXM_MIN16_I(1)) : result.x;
    result.y = (iPxOffset.y < FFXM_MIN16_I(0)) ? ffxMax(result.y, FFXM_MIN16_I(0)) : result.y;
    result.y = (iPxOffset.y > FFXM_MIN16_I(0)) ? ffxMin(result.y, iTextureSize.y - FFXM_MIN16_I(1)) : result.y;
    return result;
}
#endif //FFXM_HALF


#define DeclareCustomFetchBicubicSamplesWithType(SampleType, TextureType, AddrType, Name, LoadTexture)               \
    SampleType Name(AddrType iPxSample, AddrType iTextureSize)                                          \
    {                                                                                                   \
        SampleType Samples;                                                                             \
                                                                                                        \
        Samples.fColor00 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, -1), iTextureSize)));    \
        Samples.fColor10 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, -1), iTextureSize)));    \
        Samples.fColor20 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, -1), iTextureSize)));    \
        Samples.fColor30 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+2, -1), iTextureSize)));    \
                                                                                                        \
        Samples.fColor01 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, +0), iTextureSize)));    \
        Samples.fColor11 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +0), iTextureSize)));    \
        Samples.fColor21 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +0), iTextureSize)));    \
        Samples.fColor31 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+2, +0), iTextureSize)));    \
                                                                                                        \
        Samples.fColor02 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, +1), iTextureSize)));    \
        Samples.fColor12 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +1), iTextureSize)));    \
        Samples.fColor22 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +1), iTextureSize)));    \
        Samples.fColor32 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+2, +1), iTextureSize)));    \
                                                                                                        \
        Samples.fColor03 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, +2), iTextureSize)));    \
        Samples.fColor13 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +2), iTextureSize)));    \
        Samples.fColor23 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +2), iTextureSize)));    \
        Samples.fColor33 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+2, +2), iTextureSize)));    \
                                                                                                        \
        return Samples;                                                                                 \
    }

#define DeclareCustomFetch9TapSamplesWithType(SampleType, TextureType, AddrType, Name, LoadTexture)               \
    SampleType Name(AddrType iPxSample, AddrType iTextureSize)                                          \
    {                                                                                                   \
        SampleType Samples;                                                                             \
                                                                                                        \
        Samples.fColor00 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, -1), iTextureSize)));    \
        Samples.fColor10 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, -1), iTextureSize)));    \
        Samples.fColor20 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, -1), iTextureSize)));    \
                                                                                                        \
        Samples.fColor01 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, +0), iTextureSize)));    \
        Samples.fColor11 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +0), iTextureSize)));    \
        Samples.fColor21 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +0), iTextureSize)));    \
                                                                                                        \
        Samples.fColor02 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(-1, +1), iTextureSize)));    \
        Samples.fColor12 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +1), iTextureSize)));    \
        Samples.fColor22 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +1), iTextureSize)));    \
                                                                                                        \
        return Samples;                                                                                 \
    }

#define DeclareCustomFetchBicubicSamples(Name, LoadTexture)                                             \
    DeclareCustomFetchBicubicSamplesWithType(FetchedBicubicSamples, FfxFloat32x4, FfxInt32x2, Name, LoadTexture)

#define DeclareCustomFetchBicubicSamplesMin16(Name, LoadTexture)                                        \
    DeclareCustomFetchBicubicSamplesWithType(FetchedBicubicSamplesMin16, FFXM_MIN16_F4, FfxInt32x2, Name, LoadTexture)

#define DeclareCustomFetch9TapSamplesMin16(Name, LoadTexture)                                        \
    DeclareCustomFetch9TapSamplesWithType(Fetched9TapSamplesMin16, FFXM_MIN16_F4, FfxInt32x2, Name, LoadTexture)

#define DeclareCustomFetchBilinearSamplesWithType(SampleType, TextureType,AddrType, Name, LoadTexture)  \
    SampleType Name(AddrType iPxSample, AddrType iTextureSize)                                          \
    {                                                                                                   \
        SampleType Samples;                                                                             \
        Samples.fColor00 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +0), iTextureSize)));           \
        Samples.fColor10 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +0), iTextureSize)));           \
        Samples.fColor01 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+0, +1), iTextureSize)));           \
        Samples.fColor11 = TextureType(LoadTexture(ClampCoord(iPxSample, AddrType(+1, +1), iTextureSize)));           \
        return Samples;                                                                                 \
    }

#define DeclareCustomFetchBilinearSamples(Name, LoadTexture)                                             \
    DeclareCustomFetchBilinearSamplesWithType(FetchedBilinearSamples, FfxFloat32x4, FfxInt32x2, Name, LoadTexture)

#define DeclareCustomFetchBilinearSamplesMin16(Name, LoadTexture)                                        \
    DeclareCustomFetchBilinearSamplesWithType(FetchedBilinearSamplesMin16, FFXM_MIN16_F4, FfxInt32x2, Name, LoadTexture)

// BE CAREFUL: there is some precision issues and (3253, 125) leading to (3252.9989778, 125.001102)
// is common, so iPxSample can "jitter"
#define DeclareCustomTextureSample(Name, InterpolateSamples, FetchSamples)                                           \
    FfxFloat32x4 Name(FfxFloat32x2 fUvSample, FfxInt32x2 iTextureSize)                                               \
    {                                                                                                                \
        FfxFloat32x2 fPxSample = (fUvSample * FfxFloat32x2(iTextureSize)) - FfxFloat32x2(0.5f, 0.5f);                \
        /* Clamp base coords */                                                                                      \
        fPxSample.x = ffxMax(0.0f, ffxMin(FfxFloat32(iTextureSize.x), fPxSample.x));                                 \
        fPxSample.y = ffxMax(0.0f, ffxMin(FfxFloat32(iTextureSize.y), fPxSample.y));                                 \
        /* */                                                                                                        \
        FfxInt32x2 iPxSample = FfxInt32x2(floor(fPxSample));                                                         \
        FfxFloat32x2 fPxFrac = ffxFract(fPxSample);                                                                  \
        FfxFloat32x4 fColorXY = FfxFloat32x4(InterpolateSamples(FetchSamples(iPxSample, iTextureSize), fPxFrac));    \
        return fColorXY;                                                                                             \
    }

#define DeclareCustomTextureSampleMin16(Name, InterpolateSamples, FetchSamples)                                      \
    FFXM_MIN16_F4 Name(FfxFloat32x2 fUvSample, FfxInt32x2 iTextureSize)                                               \
    {                                                                                                                \
        FfxFloat32x2 fPxSample = (fUvSample * FfxFloat32x2(iTextureSize)) - FfxFloat32x2(0.5f, 0.5f);                \
        /* Clamp base coords */                                                                                      \
        fPxSample.x = ffxMax(0.0f, ffxMin(FfxFloat32(iTextureSize.x), fPxSample.x));                                 \
        fPxSample.y = ffxMax(0.0f, ffxMin(FfxFloat32(iTextureSize.y), fPxSample.y));                                 \
        /* */                                                                                                        \
        FfxInt32x2 iPxSample = FfxInt32x2(floor(fPxSample));                                                         \
        FFXM_MIN16_F2 fPxFrac = FFXM_MIN16_F2(ffxFract(fPxSample));                                                    \
        FFXM_MIN16_F4 fColorXY = FFXM_MIN16_F4(InterpolateSamples(FetchSamples(iPxSample, iTextureSize), fPxFrac));    \
        return fColorXY;                                                                                             \
    }

#define FFXM_FSR2_CONCAT_ID(x, y) x ## y
#define FFXM_FSR2_CONCAT(x, y) FFXM_FSR2_CONCAT_ID(x, y)
#define FFXM_FSR2_SAMPLER_1D_0 Lanczos2
#define FFXM_FSR2_SAMPLER_1D_1 Lanczos2LUT
#define FFXM_FSR2_SAMPLER_1D_2 Lanczos2Approx

#define FFXM_FSR2_GET_LANCZOS_SAMPLER1D(x) FFXM_FSR2_CONCAT(FFXM_FSR2_SAMPLER_1D_, x)

#endif //!defined( FFXM_FSR2_SAMPLE_H )
