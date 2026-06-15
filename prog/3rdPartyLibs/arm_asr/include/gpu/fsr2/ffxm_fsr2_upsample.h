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

#ifndef FFXM_FSR2_UPSAMPLE_H
#define FFXM_FSR2_UPSAMPLE_H

#define FFXM_FSR2_UPSAMPLE_USE_LANCZOS_9_TAP 0
#define FFXM_FSR2_UPSAMPLE_USE_LANCZOS_5_TAP 1

#if FFXM_SHADER_QUALITY_OPT_UPSCALING_LANCZOS_5TAP || FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#define FFXM_FSR2_UPSAMPLE_KERNEL FFXM_FSR2_UPSAMPLE_USE_LANCZOS_5_TAP
FFXM_STATIC const FfxInt32 iLanczos2SampleCount = 5;
#else
#define FFXM_FSR2_UPSAMPLE_KERNEL FFXM_FSR2_UPSAMPLE_USE_LANCZOS_9_TAP
FFXM_STATIC const FfxUInt32 iLanczos2SampleCount = 16;
#endif


void Deringing(RectificationBox clippingBox, FFXM_PARAMETER_INOUT FfxFloat32x3 fColor)
{
    fColor = clamp(fColor, clippingBox.aabbMin, clippingBox.aabbMax);
}
#if FFXM_HALF
void Deringing(RectificationBoxMin16 clippingBox, FFXM_PARAMETER_INOUT FFXM_MIN16_F3 fColor)
{
    fColor = clamp(fColor, clippingBox.aabbMin, clippingBox.aabbMax);
}
#endif

FfxFloat32 GetUpsampleLanczosWeight(FfxFloat32x2 fSrcSampleOffset, FfxFloat32 fKernelWeight)
{
    FfxFloat32x2 fSrcSampleOffsetBiased = fSrcSampleOffset * fKernelWeight.xx;
    FfxFloat32 fSampleWeight = Lanczos2ApproxSq(dot(fSrcSampleOffsetBiased, fSrcSampleOffsetBiased));
    return fSampleWeight;
}

#if FFXM_HALF
FFXM_MIN16_F GetUpsampleLanczosWeight(FFXM_MIN16_F2 fSrcSampleOffset, FFXM_MIN16_F fKernelWeight)
{
    FFXM_MIN16_F2 fSrcSampleOffsetBiased = fSrcSampleOffset * fKernelWeight.xx;
    FFXM_MIN16_F fSampleWeight = Lanczos2ApproxSq(dot(fSrcSampleOffsetBiased, fSrcSampleOffsetBiased));
    return fSampleWeight;
}
#endif

FfxFloat32 ComputeMaxKernelWeight() {
    const FfxFloat32 fKernelSizeBias = 1.0f;

    FfxFloat32 fKernelWeight = FfxFloat32(1) + (FfxFloat32(1.0f) / FfxFloat32x2(DownscaleFactor()) - FfxFloat32(1)).x * FfxFloat32(fKernelSizeBias);

    return ffxMin(FfxFloat32(1.99f), fKernelWeight);
}

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
FfxFloat32x3 ComputePreparedInputColor(FfxInt32x2 iPxLrPos)
{
    //We assume linear data. if non-linear input (sRGB, ...),
    //then we should convert to linear first and back to sRGB on output.
    FfxFloat32x3 fRgb = ffxMax(FfxFloat32x3(0, 0, 0), LoadInputColor(iPxLrPos));

    fRgb = PrepareRgb(fRgb, Exposure(), PreExposure());

#if FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    const FfxFloat32x3 fPreparedYCoCg = Tonemap(fRgb);
#else
    const FfxFloat32x3 fPreparedYCoCg = RGBToYCoCg(fRgb);
#endif

    return fPreparedYCoCg;
}
#endif

#if FFXM_HALF
FfxFloat32x4 ComputeUpsampledColorAndWeight(const AccumulationPassCommonParams params,
    FFXM_PARAMETER_INOUT RectificationBoxMin16 clippingBox, FfxFloat32 fReactiveFactor)
#else
FfxFloat32x4 ComputeUpsampledColorAndWeight(const AccumulationPassCommonParams params,
    FFXM_PARAMETER_INOUT RectificationBox clippingBox, FfxFloat32 fReactiveFactor)
#endif
{
    // We compute a sliced lanczos filter with 2 lobes (other slices are accumulated temporaly)
    FfxFloat32x2 fDstOutputPos = FfxFloat32x2(params.iPxHrPos) + FFXM_BROADCAST_FLOAT32X2(0.5f);      // Destination resolution output pixel center position
    FfxFloat32x2 fSrcOutputPos = fDstOutputPos * DownscaleFactor();                   // Source resolution output pixel center position
    FfxInt32x2 iSrcInputPos = FfxInt32x2(floor(fSrcOutputPos));                     // TODO: what about weird upscale factors...

    FfxFloat32x2 fSrcUnjitteredPos = (FfxFloat32x2(iSrcInputPos) + FfxFloat32x2(0.5f, 0.5f)) - Jitter(); // This is the un-jittered position of the sample at offset 0,0

    FfxFloat32x2 iSrcInputUv = FfxFloat32x2(fSrcOutputPos) / FfxFloat32x2(RenderSize());
    FfxFloat32x2 unitOffsetUv = FfxFloat32x2(1.0f, 1.0f) / FfxFloat32x2(RenderSize());

    FFXM_MIN16_F4 fColorAndWeight = FFXM_MIN16_F4(0.0f, 0.0f, 0.0f, 0.0f);

    FFXM_MIN16_F2 fBaseSampleOffset = FFXM_MIN16_F2(fSrcUnjitteredPos - fSrcOutputPos);

    // Identify how much of each upsampled color to be used for this frame
    const FFXM_MIN16_F fKernelReactiveFactor = FFXM_MIN16_F(ffxMax(fReactiveFactor, FfxFloat32(params.bIsNewSample)));
    const FFXM_MIN16_F fKernelBiasMax = FFXM_MIN16_F(ComputeMaxKernelWeight() * (1.0f - fKernelReactiveFactor));

    const FFXM_MIN16_F fKernelBiasMin = FFXM_MIN16_F(ffxMax(1.0f, ((1.0f + fKernelBiasMax) * 0.3f)));
    const FFXM_MIN16_F fKernelBiasFactor = FFXM_MIN16_F(ffxMax(0.0f, ffxMax(0.25f * params.fDepthClipFactor, fKernelReactiveFactor)));
    const FFXM_MIN16_F fKernelBias = ffxLerp(fKernelBiasMax, fKernelBiasMin, fKernelBiasFactor);

    const FFXM_MIN16_F fRectificationCurveBias = FFXM_MIN16_F(ffxLerp(-2.0f, -3.0f, ffxSaturate(params.fHrVelocity / 50.0f)));

    FFXM_MIN16_F2 offsetTL;
    offsetTL.x = FFXM_MIN16_F(-1);
    offsetTL.y = FFXM_MIN16_F(-1);

    FFXM_MIN16_F2 fOffsetTL = offsetTL;

#if FFXM_FSR2_UPSAMPLE_KERNEL == FFXM_FSR2_UPSAMPLE_USE_LANCZOS_9_TAP && !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FFXM_MIN16_F3 fSamples[iLanczos2SampleCount];
    // Collect samples
    GatherPreparedInputColorRGBQuad(FfxFloat32x2(-0.5, -0.5) * unitOffsetUv + iSrcInputUv,
        fSamples[0], fSamples[1], fSamples[4], fSamples[5]);
    fSamples[2] =  LoadPreparedInputColor(FfxInt32x2(1, -1) + iSrcInputPos);
    fSamples[6] =  LoadPreparedInputColor(FfxInt32x2(1, 0)  + iSrcInputPos);
    fSamples[8] =  LoadPreparedInputColor(FfxInt32x2(-1, 1) + iSrcInputPos);
    fSamples[9] =  LoadPreparedInputColor(FfxInt32x2(0, 1)  + iSrcInputPos);
    fSamples[10] = LoadPreparedInputColor(FfxInt32x2(1, 1)  + iSrcInputPos);

    FFXM_UNROLL
    for (FfxInt32 row = 0; row < 3; row++)
    {
        FFXM_UNROLL
        for (FfxInt32 col = 0; col < 3; col++)
        {
            FfxInt32 iSampleIndex = col + (row << 2);
            const FfxInt32x2 sampleColRow = FfxInt32x2(col, row);
            const FFXM_MIN16_F2 fOffset = fOffsetTL + FFXM_MIN16_F2(sampleColRow);
            FFXM_MIN16_F2 fSrcSampleOffset = fBaseSampleOffset + fOffset;

            FfxInt32x2 iSrcSamplePos = FfxInt32x2(iSrcInputPos) + FfxInt32x2(offsetTL) + sampleColRow;
            FFXM_MIN16_F fSampleWeight = FFXM_MIN16_F(GetUpsampleLanczosWeight(fSrcSampleOffset, fKernelBias));

            fColorAndWeight += FFXM_MIN16_F4(fSamples[iSampleIndex] * fSampleWeight, fSampleWeight);

            // Update rectification box
            {
                const FFXM_MIN16_F fSrcSampleOffsetSq = dot(fSrcSampleOffset, fSrcSampleOffset);
                const FFXM_MIN16_F fBoxSampleWeight = exp(fRectificationCurveBias * fSrcSampleOffsetSq);

                const FfxBoolean bInitialSample = (row == 0) && (col == 0);
                RectificationBoxAddSample(bInitialSample, clippingBox, fSamples[iSampleIndex], fBoxSampleWeight);
            }
        }
    }
#elif FFXM_FSR2_UPSAMPLE_KERNEL == FFXM_FSR2_UPSAMPLE_USE_LANCZOS_5_TAP || FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE

    FFXM_MIN16_F3 fSamples[iLanczos2SampleCount];
    // Collect samples
    FfxInt32x2 rowCol [iLanczos2SampleCount] = {FfxInt32x2(0, -1), FfxInt32x2(-1, 0), FfxInt32x2(0, 0), FfxInt32x2(1, 0), FfxInt32x2(0, 1)};
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    fSamples[0] = ComputePreparedInputColor(rowCol[0] + iSrcInputPos);
    fSamples[1] = ComputePreparedInputColor(rowCol[1] + iSrcInputPos);
    fSamples[2] = ComputePreparedInputColor(rowCol[2] + iSrcInputPos);
    fSamples[3] = ComputePreparedInputColor(rowCol[3] + iSrcInputPos);
    fSamples[4] = ComputePreparedInputColor(rowCol[4] + iSrcInputPos);
#else
    fSamples[0] = LoadPreparedInputColor(rowCol[0] + iSrcInputPos);
    fSamples[1] = LoadPreparedInputColor(rowCol[1] + iSrcInputPos);
    fSamples[2] = LoadPreparedInputColor(rowCol[2] + iSrcInputPos);
    fSamples[3] = LoadPreparedInputColor(rowCol[3] + iSrcInputPos);
    fSamples[4] = LoadPreparedInputColor(rowCol[4] + iSrcInputPos);
#endif
    FFXM_UNROLL
    for (FfxInt32 idx = 0; idx < iLanczos2SampleCount; idx++)
    {
        const FfxInt32x2 sampleColRow = rowCol[idx];
        const FFXM_MIN16_F2 fOffset = FFXM_MIN16_F2(sampleColRow);
        FFXM_MIN16_F2 fSrcSampleOffset = fBaseSampleOffset + fOffset;

        FfxInt32x2 iSrcSamplePos = FfxInt32x2(iSrcInputPos) + FfxInt32x2(offsetTL) + sampleColRow;
        FFXM_MIN16_F fSampleWeight = FFXM_MIN16_F(GetUpsampleLanczosWeight(fSrcSampleOffset, fKernelBias));

        fColorAndWeight += FFXM_MIN16_F4(fSamples[idx] * fSampleWeight, fSampleWeight);

        // Update rectification box
        {
            const FFXM_MIN16_F fSrcSampleOffsetSq = dot(fSrcSampleOffset, fSrcSampleOffset);
            const FFXM_MIN16_F fBoxSampleWeight = exp(fRectificationCurveBias * fSrcSampleOffsetSq);

            const FfxBoolean bInitialSample = (idx == 0);
            RectificationBoxAddSample(bInitialSample, clippingBox, fSamples[idx], fBoxSampleWeight);
        }
    }

#endif

    RectificationBoxComputeVarianceBoxData(clippingBox);

    fColorAndWeight.w *= FFXM_MIN16_F(fColorAndWeight.w > FSR2_EPSILON);

    if (fColorAndWeight.w > FSR2_EPSILON) {
        // Normalize for deringing (we need to compare colors)
        fColorAndWeight.xyz = fColorAndWeight.xyz / fColorAndWeight.w;
        fColorAndWeight.w = FFXM_MIN16_F(fColorAndWeight.w*fUpsampleLanczosWeightScale);

        Deringing(clippingBox, fColorAndWeight.xyz);
    }
    return fColorAndWeight;
}

#endif //!defined( FFXM_FSR2_UPSAMPLE_H )
