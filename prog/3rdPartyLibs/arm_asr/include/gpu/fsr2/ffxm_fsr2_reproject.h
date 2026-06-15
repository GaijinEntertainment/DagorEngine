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

#ifndef FFXM_FSR2_REPROJECT_H
#define FFXM_FSR2_REPROJECT_H

#if FFXM_HALF
FFXM_MIN16_F4 WrapHistory(FFXM_MIN16_I2 iPxSample)
{
    return FFXM_MIN16_F4(LoadHistory(iPxSample));
}
FFXM_MIN16_F4 SampleHistory(FfxFloat32x2 fUV)
{
    return SampleUpscaledHistory(fUV);
}
#else
FfxFloat32x4 WrapHistory(FfxInt32x2 iPxSample)
{
    return LoadHistory(iPxSample);
}
FfxFloat32x4 SampleHistory(FfxFloat32x2 fUV)
{
    return SampleUpscaledHistory(fUV);
}
#endif


#if FFXM_HALF

#define FFXM_FSR2_REPROJECT_CATMULL_9TAP 0
#define FFXM_FSR2_REPROJECT_LANCZOS_APPROX_9TAP 1
#define FFXM_FSR2_REPROJECT_CATMULL_5TAP 2

#if FFXM_SHADER_QUALITY_OPT_REPROJECT_CATMULL_5TAP
#define FFXM_FSR2_REPROJECT_MODE FFXM_FSR2_REPROJECT_CATMULL_5TAP
#elif FFXM_SHADER_QUALITY_OPT_REPROJECT_CATMULL_9TAP
#define FFXM_FSR2_REPROJECT_MODE FFXM_FSR2_REPROJECT_CATMULL_9TAP
#else // QUALITY
#define FFXM_FSR2_REPROJECT_MODE FFXM_FSR2_REPROJECT_CATMULL_9TAP
#endif

#if (FFXM_FSR2_REPROJECT_MODE == FFXM_FSR2_REPROJECT_CATMULL_9TAP)
struct CatmullRomSamples9Tap
{
    // bilinear sampling UV coordinates of the samples
    FfxFloat32x2 UV[3];

    // weights of the samples
    FFXM_MIN16_F2 Weight[3];

    // final multiplier (it is faster to multiply 3 RGB values than reweights the 5 weights)
    FFXM_MIN16_F FinalMultiplier;
};

CatmullRomSamples9Tap Get2DCatmullRom9Kernel(FfxFloat32x2 uv, FfxFloat32x2 size, in FfxFloat32x2 invSize)
{
    CatmullRomSamples9Tap catmullSamples;
    FfxFloat32x2 samplePos = uv * size;
    FfxFloat32x2 texPos1 = floor(samplePos - 0.5f) + 0.5f;
    FfxFloat32x2 f = samplePos - texPos1;

    FfxFloat32x2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    FfxFloat32x2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    FfxFloat32x2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    FfxFloat32x2 w3 = f * f * (-0.5f + 0.5f * f);

    catmullSamples.Weight[0] = FFXM_MIN16_F2(w0);
    catmullSamples.Weight[1] = FFXM_MIN16_F2(w1 + w2);
    catmullSamples.Weight[2] = FFXM_MIN16_F2(w3);

    FfxFloat32x2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    catmullSamples.UV[0] = FfxFloat32x2(texPos1 - 1);
    catmullSamples.UV[1] = FfxFloat32x2(texPos1 + 2);
    catmullSamples.UV[2] = FfxFloat32x2(texPos1 + offset12);

    catmullSamples.UV[0] = FfxFloat32x2(catmullSamples.UV[0]*invSize);
    catmullSamples.UV[1] = FfxFloat32x2(catmullSamples.UV[1]*invSize);
    catmullSamples.UV[2] = FfxFloat32x2(catmullSamples.UV[2]*invSize);
    return catmullSamples;
}

FFXM_MIN16_F4 HistorySample(FfxFloat32x2 fUvSample, FfxInt32x2 iTextureSize)
{
    FfxFloat32x2 fPxSample = (fUvSample * FfxFloat32x2(iTextureSize)) - FfxFloat32x2(0.5f, 0.5f);
    FfxFloat32x2 fTextureSize = FfxFloat32x2(iTextureSize);
    FfxFloat32x2 fInvTextureSize = FfxFloat32x2(1.0f, 1.0f) / fTextureSize;
    CatmullRomSamples9Tap samples = Get2DCatmullRom9Kernel(fUvSample, fTextureSize, fInvTextureSize);

    FFXM_MIN16_F4 fColor = FFXM_MIN16_F4(0.0f, 0.0f, 0.0f, 0.0f);

    FFXM_MIN16_F4 fColor00 = SampleHistory(FfxFloat32x2(samples.UV[0]));
    fColor += fColor00 * samples.Weight[0].x * samples.Weight[0].y;
    FFXM_MIN16_F4 fColor20 = SampleHistory(FfxFloat32x2(samples.UV[2].x, samples.UV[0].y));
    fColor += fColor20 * samples.Weight[1].x * samples.Weight[0].y;
    fColor += SampleHistory(FfxFloat32x2(samples.UV[1].x, samples.UV[0].y)) * samples.Weight[2].x * samples.Weight[0].y;

    FFXM_MIN16_F4 fColor02 = SampleHistory(FfxFloat32x2(samples.UV[0].x, samples.UV[2].y));
    fColor += SampleHistory(FfxFloat32x2(samples.UV[0].x, samples.UV[2].y)) * samples.Weight[0].x * samples.Weight[1].y;
    FFXM_MIN16_F4 fColor22 = SampleHistory(FfxFloat32x2(samples.UV[2]));
    fColor += fColor22 * samples.Weight[1].x * samples.Weight[1].y;
    fColor += SampleHistory(FfxFloat32x2(samples.UV[1].x, samples.UV[2].y)) * samples.Weight[2].x * samples.Weight[1].y;

    fColor += SampleHistory(FfxFloat32x2(samples.UV[0].x, samples.UV[1].y)) * samples.Weight[0].x * samples.Weight[2].y;
    fColor += SampleHistory(FfxFloat32x2(samples.UV[2].x, samples.UV[1].y)) * samples.Weight[1].x * samples.Weight[2].y;
    fColor += SampleHistory(FfxFloat32x2(samples.UV[1])) * samples.Weight[2].x * samples.Weight[2].y;

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    const FFXM_MIN16_F4 fDeringingSamples[4] = {fColor00, fColor20, fColor02, fColor22};

    FFXM_MIN16_F4 fDeringingMin = fDeringingSamples[0];
    FFXM_MIN16_F4 fDeringingMax = fDeringingSamples[0];

    FFXM_UNROLL
    for (FfxInt32 iSampleIndex = 1; iSampleIndex < 4; ++iSampleIndex)
    {
        fDeringingMin = ffxMin(fDeringingMin, fDeringingSamples[iSampleIndex]);
        fDeringingMax = ffxMax(fDeringingMax, fDeringingSamples[iSampleIndex]);
    }
    fColor = clamp(fColor, fDeringingMin, fDeringingMax);
#endif
    return fColor;
}
#elif (FFXM_FSR2_REPROJECT_MODE == FFXM_FSR2_REPROJECT_CATMULL_5TAP)
#define ARM_CATMULL_5TAP_SAMPLE_COUNT 5
struct CatmullRomSamples
{
    // bilinear sampling UV coordinates of the samples
    FfxFloat32x2 UV[ARM_CATMULL_5TAP_SAMPLE_COUNT];
    // weights of the samples
    FFXM_MIN16_F Weight[ARM_CATMULL_5TAP_SAMPLE_COUNT];
    // final multiplier (it is faster to multiply 3 RGB values than reweights the 5 weights)
    FFXM_MIN16_F FinalMultiplier;
};

void Bicubic2DCatmullRom(in FfxFloat32x2 uv, in FfxFloat32x2 size, in FfxFloat32x2 invSize, FFXM_PARAMETER_OUT FfxFloat32x2 samples[3], FFXM_PARAMETER_OUT FfxFloat32x2 weights[3])
{
    uv *= size;
    FfxFloat32x2 tc = floor(uv - 0.5) + 0.5;
    FfxFloat32x2 f  = uv - tc;
    FfxFloat32x2 f2 = f * f;
    FfxFloat32x2 f3 = f2 * f;
    FfxFloat32x2 w0 = f2 - 0.5 * (f3 + f);
    FfxFloat32x2 w1 = 1.5 * f3 - 2.5 * f2 + 1.f;
    FfxFloat32x2 w3 = 0.5 * (f3 - f2);
    FfxFloat32x2 w2 = 1.f - w0 - w1 - w3;

    samples[0] = tc - 1.f;
    samples[1] = tc + w2 / (w1 + w2);
    samples[2] = tc + 2.f;

    samples[0] *= invSize;
    samples[1] *= invSize;
    samples[2] *= invSize;
    weights[0] = w0;
    weights[1] = w1 + w2;
    weights[2] = w3;
}

CatmullRomSamples GetBicubic2DCatmullRomSamples(FfxFloat32x2 uv, FfxFloat32x2 size, in FfxFloat32x2 invSize)
{
    FfxFloat32x2 weights[3];
    FfxFloat32x2 samples[3];
    Bicubic2DCatmullRom(uv, size, invSize, samples, weights);

    CatmullRomSamples crSamples;
    // optimized by removing corner samples
    crSamples.UV[0] = FfxFloat32x2(samples[1].x, samples[0].y);
    crSamples.UV[1] = FfxFloat32x2(samples[0].x, samples[1].y);
    crSamples.UV[2] = FfxFloat32x2(samples[1].x, samples[1].y);
    crSamples.UV[3] = FfxFloat32x2(samples[2].x, samples[1].y);
    crSamples.UV[4] = FfxFloat32x2(samples[1].x, samples[2].y);

    crSamples.Weight[0] = FFXM_MIN16_F(weights[1].x * weights[0].y);
    crSamples.Weight[1] = FFXM_MIN16_F(weights[0].x * weights[1].y);
    crSamples.Weight[2] = FFXM_MIN16_F(weights[1].x * weights[1].y);
    crSamples.Weight[3] = FFXM_MIN16_F(weights[2].x * weights[1].y);
    crSamples.Weight[4] = FFXM_MIN16_F(weights[1].x * weights[2].y);

    // reweight after removing the corners
    FFXM_MIN16_F cornerWeights;
    cornerWeights = crSamples.Weight[0];
    cornerWeights += crSamples.Weight[1];
    cornerWeights += crSamples.Weight[2];
    cornerWeights += crSamples.Weight[3];
    cornerWeights += crSamples.Weight[4];
    crSamples.FinalMultiplier = FFXM_MIN16_F(1.f / cornerWeights);
    return crSamples;
}

FFXM_MIN16_F4 HistorySample(FfxFloat32x2 fUvSample, FfxInt32x2 iTextureSize)
{
    FfxFloat32x2 fTextureSize = FfxFloat32x2(iTextureSize);
    FfxFloat32x2 fInvTextureSize = FfxFloat32x2(1.0f, 1.0f) / fTextureSize;
    CatmullRomSamples samples = GetBicubic2DCatmullRomSamples(fUvSample, fTextureSize, fInvTextureSize);

    FFXM_MIN16_F4 fColor = FFXM_MIN16_F4(0.0f, 0.0f, 0.0f, 0.0f);
    fColor = SampleHistory(FfxFloat32x2(samples.UV[0])) * samples.Weight[0];
#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    FFXM_MIN16_F4 fDeringingMin = fColor;
    FFXM_MIN16_F4 fDeringingMax = fColor;
#endif
    for(FfxInt32 iSampleIndex = 1; iSampleIndex < ARM_CATMULL_5TAP_SAMPLE_COUNT; iSampleIndex++)
    {
        FFXM_MIN16_F4 fSample = SampleHistory(FfxFloat32x2(samples.UV[iSampleIndex])) * samples.Weight[iSampleIndex];
        fColor += fSample;
#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
        fDeringingMin = ffxMin(fDeringingMin, fSample);
        fDeringingMax = ffxMax(fDeringingMax, fSample);
#endif
    }

#if !FFXM_SHADER_QUALITY_OPT_DISABLE_DERINGING
    fColor = clamp(fColor, fDeringingMin, fDeringingMax);
#endif
    return fColor;
}
#elif (FFXM_FSR2_REPROJECT_MODE == FFXM_FSR2_REPROJECT_LANCZOS_APPROX_9TAP)

Fetched9TapSamplesMin16 FetchHistorySamples(FfxInt32x2 iPxSample, FfxInt32x2 iTextureSize)
{
    Fetched9TapSamplesMin16 Samples;
    FfxFloat32x2 iSrcInputUv = FfxFloat32x2(iPxSample) / FfxFloat32x2(iTextureSize);
    FfxFloat32x2 unitOffsetUv = FfxFloat32x2(1.0f, 1.0f) / FfxFloat32x2(iTextureSize);

    // Collect samples
    GatherHistoryColorRGBQuad(FfxFloat32x2(-0.5, -0.5) * unitOffsetUv + iSrcInputUv,
        Samples.fColor00, Samples.fColor10, Samples.fColor01, Samples.fColor11);
    Samples.fColor20 =  WrapHistory(FfxFloat32x2(1, -1) + iPxSample);
    Samples.fColor21 =  WrapHistory(FfxFloat32x2(1, 0) + iPxSample);
    Samples.fColor02 =  WrapHistory(FfxFloat32x2(-1, 1) + iPxSample);
    Samples.fColor12 =  WrapHistory(FfxFloat32x2(0, 1) + iPxSample);
    Samples.fColor22 = WrapHistory(FfxFloat32x2(1, 1) + iPxSample);

    return Samples;
}
//DeclareCustomFetch9TapSamplesMin16(FetchHistorySamples, WrapHistory)
DeclareCustomTextureSampleMin16(HistorySample, Lanczos2Approx, FetchHistorySamples)
#endif // FFXM_FSR2_REPROJECT_MODE

#else // !FFXM_HALF

#ifndef FFXM_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE
#define FFXM_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE 0 // Reference
#endif
DeclareCustomFetchBicubicSamples(FetchHistorySamples, WrapHistory)
DeclareCustomTextureSample(HistorySample, FFXM_FSR2_GET_LANCZOS_SAMPLER1D(FFXM_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE), FetchHistorySamples)
#endif

FfxFloat32x4 WrapLockStatus(FfxInt32x2 iPxSample)
{
    FfxFloat32x4 fSample = FfxFloat32x4(LoadLockStatus(iPxSample), 0.0f, 0.0f);
    return fSample;
}

#if FFXM_HALF
FFXM_MIN16_F4 WrapLockStatus(FFXM_MIN16_I2 iPxSample)
{
    FFXM_MIN16_F4 fSample = FFXM_MIN16_F4(LoadLockStatus(iPxSample), 0.0, 0.0);

    return fSample;
}
#endif

#if FFXM_HALF
DeclareCustomFetchBilinearSamplesMin16(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSampleMin16(LockStatusSample, Bilinear, FetchLockStatusSamples)
#else
DeclareCustomFetchBilinearSamples(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSample(LockStatusSample, Bilinear, FetchLockStatusSamples)
#endif

FfxFloat32x2 GetMotionVector(FfxInt32x2 iPxHrPos, FfxFloat32x2 fHrUv)
{
#if FFXM_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
    FfxFloat32x2 fDilatedMotionVector = LoadDilatedMotionVector(FfxInt32x2(fHrUv * RenderSize()));
#else
    FfxFloat32x2 fDilatedMotionVector = LoadInputMotionVector(iPxHrPos);
#endif

    return fDilatedMotionVector;
}

FfxBoolean IsUvInside(FfxFloat32x2 fUv)
{
    return (fUv.x >= 0.0f && fUv.x <= 1.0f) && (fUv.y >= 0.0f && fUv.y <= 1.0f);
}

void ComputeReprojectedUVs(const AccumulationPassCommonParams params, FFXM_PARAMETER_OUT FfxFloat32x2 fReprojectedHrUv, FFXM_PARAMETER_OUT FfxBoolean bIsExistingSample)
{
    fReprojectedHrUv = params.fHrUv + params.fMotionVector;

    bIsExistingSample = IsUvInside(fReprojectedHrUv);
}

#if !FFXM_HALF
void ReprojectHistoryColor(const AccumulationPassCommonParams params, FFXM_PARAMETER_OUT FfxFloat32x3 fHistoryColor, FFXM_PARAMETER_OUT FfxFloat32 fTemporalReactiveFactor, FFXM_PARAMETER_OUT FfxBoolean bInMotionLastFrame)
{
    FfxFloat32x4 fHistory = HistorySample(params.fReprojectedHrUv, DisplaySize());

    fHistoryColor = PrepareRgb(fHistory.rgb, Exposure(), PreviousFramePreExposure());

#if !FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    fHistoryColor = RGBToYCoCg(fHistoryColor);
#endif

    //Compute temporal reactivity info
    fTemporalReactiveFactor = ffxSaturate(abs(fHistory.w));
    bInMotionLastFrame = (fHistory.w < 0.0f);
}

LockState ReprojectHistoryLockStatus(const AccumulationPassCommonParams params, FFXM_PARAMETER_OUT FfxFloat32x2 fReprojectedLockStatus)
{
    LockState state = { FFXM_FALSE, FFXM_FALSE };
    const FfxFloat32 fNewLockIntensity = LoadRwNewLocks(params.iPxHrPos);
    state.NewLock = fNewLockIntensity > (127.0f / 255.0f);

    FfxFloat32 fInPlaceLockLifetime = state.NewLock ? fNewLockIntensity : 0;

    fReprojectedLockStatus = SampleLockStatus(params.fReprojectedHrUv);

    if (fReprojectedLockStatus[LOCK_LIFETIME_REMAINING] != FfxFloat32(0.0f)) {
        state.WasLockedPrevFrame = true;
    }

    return state;
}
#else //FFXM_HALF

void ReprojectHistoryColor(const AccumulationPassCommonParams params, FFXM_PARAMETER_OUT FfxFloat16x3 fHistoryColor, FFXM_PARAMETER_OUT FfxFloat16 fTemporalReactiveFactor, FFXM_PARAMETER_OUT FfxBoolean bInMotionLastFrame)
{
    FfxFloat16x4 fHistory = HistorySample(params.fReprojectedHrUv, DisplaySize());

    fHistoryColor = FfxFloat16x3(PrepareRgb(fHistory.rgb, Exposure(), PreviousFramePreExposure()));

#if !FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    fHistoryColor = RGBToYCoCg(fHistoryColor);
#endif

    //Compute temporal reactivity info
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    fTemporalReactiveFactor = 0.0;
#elif FFXM_SHADER_QUALITY_OPT_SEPARATE_TEMPORAL_REACTIVE
    fTemporalReactiveFactor = FfxFloat16(ffxSaturate(abs(SampleTemporalReactive(params.fReprojectedHrUv))));
#else
    fTemporalReactiveFactor = FfxFloat16(ffxSaturate(abs(fHistory.w)));
#endif
    bInMotionLastFrame = (fHistory.w < 0.0f);
}

LockState ReprojectHistoryLockStatus(const AccumulationPassCommonParams params, FFXM_PARAMETER_OUT FfxFloat16x2 fReprojectedLockStatus)
{
    LockState state = { FFXM_FALSE, FFXM_FALSE };
    const FfxFloat16 fNewLockIntensity = FfxFloat16(LoadRwNewLocks(params.iPxHrPos));
    state.NewLock = fNewLockIntensity > (127.0f / 255.0f);

    FfxFloat16 fInPlaceLockLifetime = state.NewLock ? fNewLockIntensity : FfxFloat16(0);

    fReprojectedLockStatus = FfxFloat16x2(SampleLockStatus(params.fReprojectedHrUv));

    if (fReprojectedLockStatus[LOCK_LIFETIME_REMAINING] != FfxFloat16(0.0f)) {
        state.WasLockedPrevFrame = true;
    }
    return state;
}

#endif

#endif //!defined( FFXM_FSR2_REPROJECT_H )
