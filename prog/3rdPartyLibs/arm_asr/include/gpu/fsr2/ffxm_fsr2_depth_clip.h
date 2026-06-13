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

#ifndef FFXM_FSR2_DEPTH_CLIP_H
#define FFXM_FSR2_DEPTH_CLIP_H

// Can casue some temporal instability
#define OPT_PREFETCH_PREVDEPTH_WITH_GATHER 0

struct DepthClipOutputs
{
    FfxFloat32x2 fDilatedReactiveMasks;
#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x4 fTonemapped;
#endif
};

FFXM_STATIC const FfxFloat32 DepthClipBaseScale = 4.0f;

FfxFloat32 ComputeDepthClip(FfxFloat32x2 fUvSample, FfxFloat32 fCurrentDepthSample)
{
    FfxFloat32 fCurrentDepthViewSpace = GetViewSpaceDepth(fCurrentDepthSample);
    BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fUvSample, RenderSize());

    FfxFloat32 fDilatedSum = 0.0f;
    FfxFloat32 fDepth = 0.0f;
    FfxFloat32 fWeightSum = 0.0f;


#if OPT_PREFETCH_PREVDEPTH_WITH_GATHER
    FfxFloat32 fDepthSamples[4];
    GatherReconstructedPreviousDepthRQuad(bilinearInfo.fQuadCenterUv,
        fDepthSamples[0], fDepthSamples[1], fDepthSamples[2], fDepthSamples[3]);
#endif

    for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++)
    {
        const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
        const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

        if (IsOnScreen(iSamplePos, RenderSize()))
        {
            const FfxFloat32 fWeight = bilinearInfo.fWeights[iSampleIndex];
            if (fWeight > fReconstructedDepthBilinearWeightThreshold)
            {
#if OPT_PREFETCH_PREVDEPTH_WITH_GATHER
                const FfxFloat32 fPrevDepthSample = fDepthSamples[iSampleIndex];
#else
                const FfxFloat32 fPrevDepthSample = LoadReconstructedPrevDepth(iSamplePos);
#endif
                const FfxFloat32 fPrevNearestDepthViewSpace = GetViewSpaceDepth(fPrevDepthSample);
                const FfxFloat32 fDepthDiff = fCurrentDepthViewSpace - fPrevNearestDepthViewSpace;

                if (fDepthDiff > 0.0f) {

#if FFXM_FSR2_OPTION_INVERTED_DEPTH
                    const FfxFloat32 fPlaneDepth = ffxMin(fPrevDepthSample, fCurrentDepthSample);
#else
                    const FfxFloat32 fPlaneDepth = ffxMax(fPrevDepthSample, fCurrentDepthSample);
#endif

                    const FfxFloat32x3 fCenter = GetViewSpacePosition(FfxInt32x2(RenderSize() * 0.5f), RenderSize(), fPlaneDepth);
                    const FfxFloat32x3 fCorner = GetViewSpacePosition(FfxInt32x2(0, 0), RenderSize(), fPlaneDepth);

                    const FfxFloat32 fHalfViewportWidth = length(FfxFloat32x2(RenderSize()));
                    const FfxFloat32 fDepthThreshold = ffxMax(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace);

                    const FfxFloat32 Ksep = 1.37e-05f;
                    const FfxFloat32 Kfov = length(fCorner) / length(fCenter);
                    const FfxFloat32 fRequiredDepthSeparation = Ksep * Kfov * fHalfViewportWidth * fDepthThreshold;

                    const FfxFloat32 fResolutionFactor = ffxSaturate(length(FfxFloat32x2(RenderSize())) / length(FfxFloat32x2(1920.0f, 1080.0f)));
                    const FfxFloat32 fPower = ffxLerp(1.0f, 3.0f, fResolutionFactor);
                    fDepth += ffxPow(ffxSaturate(FfxFloat32(fRequiredDepthSeparation / fDepthDiff)), fPower) * fWeight;
                    fWeightSum += fWeight;
                }
            }
        }
    }

    return (fWeightSum > 0) ? ffxSaturate(1.0f - fDepth / fWeightSum) : 0.0f;
}

FfxFloat32 ComputeMotionDivergence(FfxInt32x2 iPxPos, FfxInt32x2 iPxInputMotionVectorSize)
{
    FfxFloat32 minconvergence = 1.0f;

    FfxFloat32x2 fMotionVectorNucleus = LoadInputMotionVector(iPxPos);
    FfxFloat32 fNucleusVelocityLr = length(fMotionVectorNucleus * RenderSize());
    FfxFloat32 fMaxVelocityUv = length(fMotionVectorNucleus);

    const FfxFloat32 MotionVectorVelocityEpsilon = 1e-02f;


    const FfxFloat32x2 fMVSize = FfxFloat32x2(iPxInputMotionVectorSize);
    FfxFloat32x2 fPxBaseUv = FfxFloat32x2(iPxPos) / fMVSize;
    FfxFloat32x2 fUnitUv = FfxFloat32x2(1.0f, 1.0f) / fMVSize;

    FFXM_MIN16_F2 fMotionVectorSamples[9];
    FFXM_MIN16_F2 fTmpDummy = FFXM_MIN16_F2(0.0f, 0.0f);
    GatherInputMotionVectorRGQuad(fPxBaseUv,
        fMotionVectorSamples[0], fMotionVectorSamples[1],
        fMotionVectorSamples[3], fMotionVectorSamples[4]);
    GatherInputMotionVectorRGQuad(fUnitUv + fPxBaseUv,
        fTmpDummy, fMotionVectorSamples[5],
        fMotionVectorSamples[7], fMotionVectorSamples[8]);
    fMotionVectorSamples[2] = LoadInputMotionVector(iPxPos + FfxInt32x2(1, -1));
    fMotionVectorSamples[6] = LoadInputMotionVector(iPxPos + FfxInt32x2(-1, 1));

    if (fNucleusVelocityLr > MotionVectorVelocityEpsilon) {
        for (FfxInt32 y = -1; y <= 1; ++y)
        {
            for (FfxInt32 x = -1; x <= 1; ++x)
            {
                FfxInt32 sampleIdx = (y + 1) * 3 + x + 1;

                FfxFloat32x2 fMotionVector = fMotionVectorSamples[sampleIdx]; //LoadInputMotionVector(sp);
                FfxFloat32 fVelocityUv = length(fMotionVector);

                fMaxVelocityUv = ffxMax(fVelocityUv, fMaxVelocityUv);
                fVelocityUv = ffxMax(fVelocityUv, fMaxVelocityUv);
                minconvergence = ffxMin(minconvergence, dot(fMotionVector / fVelocityUv, fMotionVectorNucleus / fVelocityUv));
            }
        }
    }

    return ffxSaturate(1.0f - minconvergence) * ffxSaturate(fMaxVelocityUv / 0.01f);
}

FfxFloat32 ComputeDepthDivergence(FfxInt32x2 iPxPos)
{
    const FfxFloat32 fMaxDistInMeters = GetMaxDistanceInMeters();
    FfxFloat32 fDepthMax = 0.0f;
    FfxFloat32 fDepthMin = fMaxDistInMeters;

    FfxInt32 iMaxDistFound = 0;

    FfxInt32x2 iRenderSize = RenderSize();
    const FfxFloat32x2 fRenderSize = FfxFloat32x2(iRenderSize);
    FfxFloat32x2 fPxPosBase = FfxFloat32x2(iPxPos) / fRenderSize;
    FfxFloat32x2 fUnitUv = FfxFloat32x2(1.0f, 1.0f) / fRenderSize;

    FfxFloat32 fDilatedDepthSamples[9];
    FfxFloat32 fTmpDummy = 0.0f;
    GatherDilatedDepthRQuad(fPxPosBase,
        fDilatedDepthSamples[0], fDilatedDepthSamples[1],
        fDilatedDepthSamples[3], fDilatedDepthSamples[4]);
    GatherDilatedDepthRQuad(fUnitUv + fPxPosBase,
        fTmpDummy, fDilatedDepthSamples[5],
        fDilatedDepthSamples[7], fDilatedDepthSamples[8]);
    fDilatedDepthSamples[2] = LoadDilatedDepth(iPxPos + FfxInt32x2(1, -1));
    fDilatedDepthSamples[6] = LoadDilatedDepth(iPxPos + FfxInt32x2(-1, 1));

    for (FfxInt32 y = -1; y < 2; y++)
    {
        for (FfxInt32 x = -1; x < 2; x++)
        {
            FfxInt32 sampleIdx = (y + 1) * 3 + x + 1;
            const FfxInt32x2 iOffset = FfxInt32x2(x, y);
            const FfxInt32x2 iSamplePos = iPxPos + iOffset;

            const FfxFloat32 fOnScreenFactor = IsOnScreen(iSamplePos, iRenderSize) ? 1.0f : 0.0f;
            // FfxFloat32 fDepth = GetViewSpaceDepthInMeters(LoadDilatedDepth(iSamplePos)) * fOnScreenFactor;
            FfxFloat32 fDepth = GetViewSpaceDepthInMeters(fDilatedDepthSamples[sampleIdx]) * fOnScreenFactor;

            iMaxDistFound |= FfxInt32(fMaxDistInMeters == fDepth);

            fDepthMin = ffxMin(fDepthMin, fDepth);
            fDepthMax = ffxMax(fDepthMax, fDepth);
        }
    }

    return (1.0f - fDepthMin / fDepthMax) * (FfxBoolean(iMaxDistFound) ? 0.0f : 1.0f);
}

FfxFloat32 ComputeTemporalMotionDivergence(FfxInt32x2 iPxPos)
{
    const FfxFloat32x2 fUv = FfxFloat32x2(iPxPos + 0.5f) / RenderSize();

    FfxFloat32x2 fMotionVector = LoadDilatedMotionVector(iPxPos);
    FfxFloat32x2 fReprojectedUv = fUv + fMotionVector;
    fReprojectedUv = ClampUv(fReprojectedUv, RenderSize(), MaxRenderSize());
    FfxFloat32x2 fPrevMotionVector = SamplePreviousDilatedMotionVector(fReprojectedUv);

    float fPxDistance = length(fMotionVector * DisplaySize());
    return fPxDistance > 1.0f ? ffxLerp(0.0f, 1.0f - ffxSaturate(length(fPrevMotionVector) / length(fMotionVector)), ffxSaturate(ffxPow(fPxDistance / 20.0f, 3.0f))) : 0;
}

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
void PreProcessReactiveMasks(FfxInt32x2 iPxLrPos, FfxFloat32 fMotionDivergence, FFXM_PARAMETER_INOUT DepthClipOutputs results)
{
    results.fDilatedReactiveMasks = FfxInt32x2(0.0, fMotionDivergence);
}
#else
void PreProcessReactiveMasks(FfxInt32x2 iPxLrPos, FfxFloat32 fMotionDivergence, FFXM_PARAMETER_INOUT DepthClipOutputs results)
{
    // Compensate for bilinear sampling in accumulation pass

    const FfxInt32x2 iRenderSize = RenderSize();
    const FfxFloat32x2 fRenderSize = FfxFloat32x2(iRenderSize);
    FfxFloat32x2 fPxPosBase = FfxFloat32x2(iPxLrPos) / fRenderSize;
    FfxFloat32x2 fUnitUv = FfxFloat32x2(1.0f, 1.0f) / fRenderSize;

    FFXM_MIN16_F2 fReactiveFactor = FFXM_MIN16_F2(0.0f, fMotionDivergence);
    FFXM_MIN16_F fMasksSum = FFXM_MIN16_F(0.0f);

    FFXM_MIN16_F fTmpDummy = FFXM_MIN16_F(0.0f);
    // Reactive samples
    FFXM_MIN16_F fReactiveSamples[9];
    GatherReactiveRQuad(fPxPosBase,
        fReactiveSamples[0], fReactiveSamples[1],
        fReactiveSamples[3], fReactiveSamples[4]);
    GatherReactiveRQuad(fUnitUv + fPxPosBase,
        fTmpDummy, fReactiveSamples[5],
        fReactiveSamples[7], fReactiveSamples[8]);
    fReactiveSamples[2] = FFXM_MIN16_F(LoadReactiveMask(iPxLrPos + FfxInt32x2(1, -1)));
    fReactiveSamples[6] = FFXM_MIN16_F(LoadReactiveMask(iPxLrPos + FfxInt32x2(-1, 1)));

    // Transparency and composition mask samples
    FFXM_MIN16_F fTransparencyAndCompositionSamples[9];
    GatherTransparencyAndCompositionMaskRQuad(fPxPosBase,
        fTransparencyAndCompositionSamples[0], fTransparencyAndCompositionSamples[1],
        fTransparencyAndCompositionSamples[3], fTransparencyAndCompositionSamples[4]);
    GatherTransparencyAndCompositionMaskRQuad(fUnitUv + fPxPosBase,
        fTmpDummy, fTransparencyAndCompositionSamples[5],
        fTransparencyAndCompositionSamples[7], fTransparencyAndCompositionSamples[8]);
    fTransparencyAndCompositionSamples[2] = FFXM_MIN16_F(LoadTransparencyAndCompositionMask(iPxLrPos + FfxInt32x2(1, -1)));
    fTransparencyAndCompositionSamples[6] = FFXM_MIN16_F(LoadTransparencyAndCompositionMask(iPxLrPos + FfxInt32x2(-1, 1)));

    FFXM_UNROLL
    for (FfxInt32 y = -1; y < 2; y++)
    {
        FFXM_UNROLL
        for (FfxInt32 x = -1; x < 2; x++)
        {
            FfxInt32 sampleIdx = (y + 1) * 3 + x + 1;
            fMasksSum += (fReactiveSamples[sampleIdx] + fTransparencyAndCompositionSamples[sampleIdx]);
        }
    }

    if (fMasksSum > FFXM_MIN16_F(0))
    {
        const FfxFloat32x2 InputColorSize = FfxFloat32x2(InputColorResourceDimensions());
        FfxFloat32x2 Base = FfxFloat32x2(iPxLrPos) / InputColorSize;
        FFXM_MIN16_F3 fInputColorSamples[9];
        // Input color samples
        GatherInputColorRGBQuad(Base,
            fInputColorSamples[0], fInputColorSamples[1], fInputColorSamples[3], fInputColorSamples[4]);
        fInputColorSamples[2] = LoadInputColor(iPxLrPos + FfxInt32x2(1, -1));
        fInputColorSamples[5] = LoadInputColor(iPxLrPos + FfxInt32x2(1, 0) );
        fInputColorSamples[6] = LoadInputColor(iPxLrPos + FfxInt32x2(-1, 1));
        fInputColorSamples[7] = LoadInputColor(iPxLrPos + FfxInt32x2(0, 1) );
        fInputColorSamples[8] = LoadInputColor(iPxLrPos + FfxInt32x2(1, 1) );

        FFXM_MIN16_F3 fReferenceColor = fInputColorSamples[4];

        for (FfxInt32 sampleIdx = 0; sampleIdx < 9; sampleIdx++)
        {
            FFXM_MIN16_F3 fColorSample = fInputColorSamples[sampleIdx];
            FFXM_MIN16_F fReactiveSample = fReactiveSamples[sampleIdx];
            FFXM_MIN16_F fTransparencyAndCompositionSample = fTransparencyAndCompositionSamples[sampleIdx];

            const FfxFloat32 fMaxLenSq = ffxMax(dot(fReferenceColor, fReferenceColor), dot(fColorSample, fColorSample));
            const FFXM_MIN16_F fSimilarity = dot(fReferenceColor, fColorSample) / fMaxLenSq;

            // Increase power for non-similar samples
            const FFXM_MIN16_F fPowerBiasMax = FFXM_MIN16_F(6.0f);
            const FFXM_MIN16_F fSimilarityPower = FFXM_MIN16_F(1.0f + (fPowerBiasMax - fSimilarity * fPowerBiasMax));
            const FFXM_MIN16_F fWeightedReactiveSample = ffxPow(fReactiveSample, fSimilarityPower);
            const FFXM_MIN16_F fWeightedTransparencyAndCompositionSample = ffxPow(fTransparencyAndCompositionSample, fSimilarityPower);

            fReactiveFactor = ffxMax(fReactiveFactor, FFXM_MIN16_F2(fWeightedReactiveSample, fWeightedTransparencyAndCompositionSample));
        }
    }

    results.fDilatedReactiveMasks = fReactiveFactor;
}
#endif

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

FfxFloat32 EvaluateSurface(FfxInt32x2 iPxPos, FfxFloat32x2 fMotionVector)
{
    FfxFloat32 d0 = GetViewSpaceDepth(LoadReconstructedPrevDepth(iPxPos + FfxInt32x2(0, -1)));
    FfxFloat32 d1 = GetViewSpaceDepth(LoadReconstructedPrevDepth(iPxPos + FfxInt32x2(0, 0)));
    FfxFloat32 d2 = GetViewSpaceDepth(LoadReconstructedPrevDepth(iPxPos + FfxInt32x2(0, 1)));

    return 1.0f - FfxFloat32(((d0 - d1) > (d1 * 0.01f)) && ((d1 - d2) > (d2 * 0.01f)));
}

DepthClipOutputs DepthClip(FfxInt32x2 iPxPos)
{
    FfxFloat32x2 fDepthUv = (iPxPos + 0.5f) / RenderSize();
    FfxFloat32x2 fMotionVector = LoadDilatedMotionVector(iPxPos);

    // Discard tiny mvs
    fMotionVector *= FfxFloat32(length(fMotionVector * DisplaySize()) > 0.01f);

    const FfxFloat32x2 fDilatedUv = fDepthUv + fMotionVector;
    const FfxFloat32 fDilatedDepth = LoadDilatedDepth(iPxPos);
    const FfxFloat32 fCurrentDepthViewSpace = GetViewSpaceDepth(LoadInputDepth(iPxPos));

    DepthClipOutputs results;
    results.fDilatedReactiveMasks = FfxFloat32x2(0.0, 0.0);

    // Compute prepared input color and depth clip
    FfxFloat32 fDepthClip = ComputeDepthClip(fDilatedUv, fDilatedDepth) * EvaluateSurface(iPxPos, fMotionVector);
#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x3 fPreparedYCoCg = ComputePreparedInputColor(iPxPos);
    results.fTonemapped = FfxFloat32x4(fPreparedYCoCg, fDepthClip);
#endif

    // Compute dilated reactive mask
#if FFXM_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
    FfxInt32x2 iSamplePos = iPxPos;
#else
    FfxInt32x2 iSamplePos = ComputeHrPosFromLrPos(iPxPos);
#endif

    FfxFloat32 fMotionDivergence = ComputeMotionDivergence(iSamplePos, RenderSize());
    FfxFloat32 fTemporalMotionDifference = ffxSaturate(ComputeTemporalMotionDivergence(iPxPos) - ComputeDepthDivergence(iPxPos));

    PreProcessReactiveMasks(iPxPos, ffxMax(fTemporalMotionDifference, fMotionDivergence), results);

#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    results.fDilatedReactiveMasks.x = fDepthClip;
#endif

    return results;
}

#endif //!defined( FFXM_FSR2_DEPTH_CLIPH )
