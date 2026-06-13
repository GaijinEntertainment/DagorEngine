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

#ifndef FFXM_FSR2_ACCUMULATE_H
#define FFXM_FSR2_ACCUMULATE_H

struct AccumulateOutputs
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    FfxFloat32x4 fColorAndWeight;
    FfxFloat32x2 fLockStatus;
#else
#if !FFXM_SHADER_QUALITY_OPT_SEPARATE_TEMPORAL_REACTIVE
    FfxFloat32x4 fColorAndWeight;
#else
    FfxFloat32x3 fUpscaledColor;
    FfxFloat32 fTemporalReactive;
#endif
    FfxFloat32x2 fLockStatus;
    FfxFloat32x4 fLumaHistory;
#endif
#if (FFXM_FSR2_OPTION_APPLY_SHARPENING == 0)
    FfxFloat32x3 fColor;
#endif
};

FfxFloat32 GetPxHrVelocity(FfxFloat32x2 fMotionVector)
{
    return length(fMotionVector * DisplaySize());
}
#if FFXM_HALF
FFXM_MIN16_F GetPxHrVelocity(FFXM_MIN16_F2 fMotionVector)
{
    return length(fMotionVector * FFXM_MIN16_F2(DisplaySize()));
}
#endif

void Accumulate(const AccumulationPassCommonParams params, FFXM_PARAMETER_INOUT FfxFloat32x3 fHistoryColor, FfxFloat32x3 fAccumulation, FFXM_PARAMETER_IN FfxFloat32x4 fUpsampledColorAndWeight)
{
    // Aviod invalid values when accumulation and upsampled weight is 0
    fAccumulation = ffxMax(FSR2_EPSILON.xxx, fAccumulation + fUpsampledColorAndWeight.www);

#if FFXM_FSR2_OPTION_HDR_COLOR_INPUT
#if FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    fHistoryColor = Tonemap(fHistoryColor);
#else
    //YCoCg -> RGB -> Tonemap -> YCoCg (Use RGB tonemapper to avoid color desaturation)
    fUpsampledColorAndWeight.xyz = RGBToYCoCg(Tonemap(YCoCgToRGB(fUpsampledColorAndWeight.xyz)));
    fHistoryColor = RGBToYCoCg(Tonemap(YCoCgToRGB(fHistoryColor)));
#endif
#endif

    const FfxFloat32x3 fAlpha = fUpsampledColorAndWeight.www / fAccumulation;
    fHistoryColor = ffxLerp(fHistoryColor, fUpsampledColorAndWeight.xyz, fAlpha);

#if !FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    fHistoryColor = YCoCgToRGB(fHistoryColor);
#endif

#if FFXM_FSR2_OPTION_HDR_COLOR_INPUT
    fHistoryColor = InverseTonemap(fHistoryColor);
#endif
}

#if FFXM_HALF
void RectifyHistory(
    const AccumulationPassCommonParams params,
    RectificationBoxMin16 clippingBox,
    FFXM_PARAMETER_INOUT FfxFloat32x3 fHistoryColor,
    FFXM_PARAMETER_INOUT FfxFloat32x3 fAccumulation,
    FfxFloat32 fLockContributionThisFrame,
    FfxFloat32 fTemporalReactiveFactor,
    FfxFloat32 fLumaInstabilityFactor)
#else
void RectifyHistory(
    const AccumulationPassCommonParams params,
    RectificationBox clippingBox,
    FFXM_PARAMETER_INOUT FfxFloat32x3 fHistoryColor,
    FFXM_PARAMETER_INOUT FfxFloat32x3 fAccumulation,
    FfxFloat32 fLockContributionThisFrame,
    FfxFloat32 fTemporalReactiveFactor,
    FfxFloat32 fLumaInstabilityFactor)
#endif
{
    FfxFloat32 fScaleFactorInfluence = ffxMin(20.0f, ffxPow(FfxFloat32(1.0f / length(DownscaleFactor().x * DownscaleFactor().y)), 3.0f));

    const FfxFloat32 fVecolityFactor = ffxSaturate(params.fHrVelocity / 20.0f);
    const FfxFloat32 fBoxScaleT = ffxMax(params.fDepthClipFactor, ffxMax(params.fAccumulationMask, fVecolityFactor));
    FfxFloat32 fBoxScale = ffxLerp(fScaleFactorInfluence, 1.0f, fBoxScaleT);

    FfxFloat32x3 fScaledBoxVec = clippingBox.boxVec * fBoxScale;
    FfxFloat32x3 boxMin = clippingBox.boxCenter - fScaledBoxVec;
    FfxFloat32x3 boxMax = clippingBox.boxCenter + fScaledBoxVec;
    FfxFloat32x3 boxCenter = clippingBox.boxCenter;
    FfxFloat32 boxVecSize = length(clippingBox.boxVec);

    boxMin = ffxMax(clippingBox.aabbMin, boxMin);
    boxMax = ffxMin(clippingBox.aabbMax, boxMax);
#if FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
    boxMin = InverseTonemap(boxMin);
    boxMax = InverseTonemap(boxMax);
#endif

    if (any(FFXM_GREATER_THAN(boxMin, fHistoryColor)) || any(FFXM_GREATER_THAN(fHistoryColor, boxMax))) {

        const FfxFloat32x3 fClampedHistoryColor = clamp(fHistoryColor, boxMin, boxMax);

        FfxFloat32x3 fHistoryContribution = ffxMax(fLumaInstabilityFactor, fLockContributionThisFrame).xxx;

        const FfxFloat32 fReactiveFactor = params.fDilatedReactiveFactor;
        const FfxFloat32 fReactiveContribution = 1.0f - ffxPow(fReactiveFactor, 1.0f / 2.0f);
        fHistoryContribution *= fReactiveContribution;

        // Scale history color using rectification info, also using accumulation mask to avoid potential invalid color protection
        fHistoryColor = ffxLerp(fClampedHistoryColor, fHistoryColor, ffxSaturate(fHistoryContribution));

        // Scale accumulation using rectification info
        const FfxFloat32x3 fAccumulationMin = ffxMin(fAccumulation, FFXM_BROADCAST_FLOAT32X3(0.1f));
        fAccumulation = ffxLerp(fAccumulationMin, fAccumulation, ffxSaturate(fHistoryContribution));
    }
}

void FinalizeLockStatus(const AccumulationPassCommonParams params, FfxFloat32x2 fLockStatus, FfxFloat32 fUpsampledWeight, FFXM_PARAMETER_INOUT AccumulateOutputs result)
{
    // we expect similar motion for next frame
    // kill lock if that location is outside screen, avoid locks to be clamped to screen borders
    FfxFloat32x2 fEstimatedUvNextFrame = params.fHrUv - params.fMotionVector;
    if (IsUvInside(fEstimatedUvNextFrame) == false) {
        KillLock(fLockStatus);
    }
    else {
        // Decrease lock lifetime
        const FfxFloat32 fLifetimeDecreaseLanczosMax = FfxFloat32(JitterSequenceLength()) * FfxFloat32(fAverageLanczosWeightPerFrame);
        const FfxFloat32 fLifetimeDecrease = FfxFloat32(fUpsampledWeight / fLifetimeDecreaseLanczosMax);
        fLockStatus[LOCK_LIFETIME_REMAINING] = ffxMax(FfxFloat32(0), fLockStatus[LOCK_LIFETIME_REMAINING] - fLifetimeDecrease);
    }

    result.fLockStatus = fLockStatus;
}


FfxFloat32x3 ComputeBaseAccumulationWeight(const AccumulationPassCommonParams params, FfxFloat32 fThisFrameReactiveFactor, FfxBoolean bInMotionLastFrame, FfxFloat32 fUpsampledWeight, LockState lockState)
{
    // Always assume max accumulation was reached
    FfxFloat32 fBaseAccumulation = fMaxAccumulationLanczosWeight * FfxFloat32(params.bIsExistingSample) * (1.0f - fThisFrameReactiveFactor) * (1.0f - params.fDepthClipFactor);

    fBaseAccumulation = ffxMin(fBaseAccumulation, ffxLerp(fBaseAccumulation, fUpsampledWeight * 10.0f, ffxMax(FfxFloat32(bInMotionLastFrame), ffxSaturate(params.fHrVelocity * FfxFloat32(10)))));

    fBaseAccumulation = ffxMin(fBaseAccumulation, ffxLerp(fBaseAccumulation, fUpsampledWeight, ffxSaturate(params.fHrVelocity / FfxFloat32(20))));

    return fBaseAccumulation.xxx;
}

#if !FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
#if FFXM_HALF
FfxFloat32 ComputeLumaInstabilityFactor(const AccumulationPassCommonParams params, RectificationBoxMin16 clippingBox, FfxFloat32 fThisFrameReactiveFactor, FfxFloat32 fLuminanceDiff, FFXM_PARAMETER_INOUT AccumulateOutputs result)
#else
FfxFloat32 ComputeLumaInstabilityFactor(const AccumulationPassCommonParams params, RectificationBox clippingBox, FfxFloat32 fThisFrameReactiveFactor, FfxFloat32 fLuminanceDiff, FFXM_PARAMETER_INOUT AccumulateOutputs result)
#endif
{
    const FfxFloat32 fUnormThreshold = 1.0f / 255.0f;
    const FfxInt32 N_MINUS_1 = 0;
    const FfxInt32 N_MINUS_2 = 1;
    const FfxInt32 N_MINUS_3 = 2;
    const FfxInt32 N_MINUS_4 = 3;

    FfxFloat32 fCurrentFrameLuma = clippingBox.boxCenter.x;

#if FFXM_FSR2_OPTION_HDR_COLOR_INPUT
    fCurrentFrameLuma = fCurrentFrameLuma / (1.0f + ffxMax(0.0f, fCurrentFrameLuma));
#endif

    fCurrentFrameLuma = round(fCurrentFrameLuma * 255.0f) / 255.0f;

    const FfxBoolean bSampleLumaHistory = (ffxMax(ffxMax(params.fDepthClipFactor, params.fAccumulationMask), fLuminanceDiff) < 0.1f) && (params.bIsNewSample == false);
    FfxFloat32x4 fCurrentFrameLumaHistory = bSampleLumaHistory ? SampleLumaHistory(params.fReprojectedHrUv) : FFXM_BROADCAST_FLOAT32X4(0.0f);

    FfxFloat32 fLumaInstability = 0.0f;
    FfxFloat32 fDiffs0 = (fCurrentFrameLuma - fCurrentFrameLumaHistory[N_MINUS_1]);

    FfxFloat32 fMin = abs(fDiffs0);

    if (fMin >= fUnormThreshold) {
        for (int i = N_MINUS_2; i <= N_MINUS_4; i++) {
            FfxFloat32 fDiffs1 = (fCurrentFrameLuma - fCurrentFrameLumaHistory[i]);

            if (sign(fDiffs0) == sign(fDiffs1)) {

                // Scale difference to protect historically similar values
                const FfxFloat32 fMinBias = 1.0f;
                fMin = ffxMin(fMin, abs(fDiffs1) * fMinBias);
            }
        }

        const FfxFloat32 fBoxSize       = clippingBox.boxVec.x;
        const FfxFloat32 fBoxSizeFactor = ffxPow(ffxSaturate(fBoxSize / 0.1f), 6.0f);

        fLumaInstability = FfxFloat32(fMin != abs(fDiffs0)) * fBoxSizeFactor;
        fLumaInstability = FfxFloat32(fLumaInstability > fUnormThreshold);

        fLumaInstability *= 1.0f - ffxMax(params.fAccumulationMask, ffxPow(fThisFrameReactiveFactor, 1.0f / 6.0f));
    }

    //shift history
    fCurrentFrameLumaHistory[N_MINUS_4] = fCurrentFrameLumaHistory[N_MINUS_3];
    fCurrentFrameLumaHistory[N_MINUS_3] = fCurrentFrameLumaHistory[N_MINUS_2];
    fCurrentFrameLumaHistory[N_MINUS_2] = fCurrentFrameLumaHistory[N_MINUS_1];
    fCurrentFrameLumaHistory[N_MINUS_1] = fCurrentFrameLuma;

    result.fLumaHistory = fCurrentFrameLumaHistory;

    return fLumaInstability * FfxFloat32(fCurrentFrameLumaHistory[N_MINUS_4] != 0);
}
#endif

FfxFloat32 ComputeTemporalReactiveFactor(const AccumulationPassCommonParams params, FfxFloat32 fTemporalReactiveFactor)
{
    FfxFloat32 fNewFactor = ffxMin(0.99f, fTemporalReactiveFactor);

    fNewFactor = ffxMax(fNewFactor, ffxLerp(fNewFactor, 0.4f, ffxSaturate(params.fHrVelocity)));

    fNewFactor = ffxMax(fNewFactor * fNewFactor, ffxMax(params.fDepthClipFactor * 0.1f, params.fDilatedReactiveFactor));

    // Force reactive factor for new samples
    fNewFactor = params.bIsNewSample ? 1.0f : fNewFactor;

    if (ffxSaturate(params.fHrVelocity * 10.0f) >= 1.0f) {
        fNewFactor = ffxMax(FSR2_EPSILON, fNewFactor) * -1.0f;
    }

    return fNewFactor;
}

void initReactiveMaskFactors(FFXM_PARAMETER_INOUT AccumulationPassCommonParams params)
{
    const FFXM_MIN16_F2 fDilatedReactiveMasks = FFXM_MIN16_F2(SampleDilatedReactiveMasks(params.fLrUv_HwSampler));
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    params.fDilatedReactiveFactor = 0.0;
#else
    params.fDilatedReactiveFactor = fDilatedReactiveMasks.x;
#endif
    params.fAccumulationMask = fDilatedReactiveMasks.y;
}

void initDepthClipFactors(FFXM_PARAMETER_INOUT AccumulationPassCommonParams params)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    const FFXM_MIN16_F2 fDilatedReactiveMasks = FFXM_MIN16_F2(SampleDilatedReactiveMasks(params.fLrUv_HwSampler));
    params.fDepthClipFactor = fDilatedReactiveMasks.x;
#else
    params.fDepthClipFactor = FFXM_MIN16_F(ffxSaturate(SampleDepthClip(params.fLrUv_HwSampler)));
#endif
}

void initIsNewSample(FFXM_PARAMETER_INOUT AccumulationPassCommonParams params)
{
    const FfxBoolean bIsResetFrame = (0 == FrameIndex());
    params.bIsNewSample = (params.bIsExistingSample == false || bIsResetFrame);
}


AccumulationPassCommonParams InitParams(FfxInt32x2 iPxHrPos)
{
    AccumulationPassCommonParams params;

    params.iPxHrPos = iPxHrPos;
    const FfxFloat32x2 fHrUv = (iPxHrPos + 0.5f) / DisplaySize();
    params.fHrUv = fHrUv;

    const FfxFloat32x2 fLrUvJittered = fHrUv + Jitter() / RenderSize();
    params.fLrUv_HwSampler = ClampUv(fLrUvJittered, RenderSize(), MaxRenderSize());

    params.fMotionVector = GetMotionVector(iPxHrPos, fHrUv);
    params.fHrVelocity = GetPxHrVelocity(params.fMotionVector);

    ComputeReprojectedUVs(params, params.fReprojectedHrUv, params.bIsExistingSample);

    return params;
}

AccumulateOutputs Accumulate(FfxInt32x2 iPxHrPos)
{
    AccumulationPassCommonParams params = InitParams(iPxHrPos);

    FfxFloat32x3 fHistoryColor = FfxFloat32x3(0, 0, 0);
    FFXM_MIN16_F2 fLockStatus;
    InitializeNewLockSample(fLockStatus);

    FFXM_MIN16_F fTemporalReactiveFactor = FFXM_MIN16_F(0.0f);
    FfxBoolean bInMotionLastFrame = FFXM_FALSE;
    LockState lockState = { FFXM_FALSE , FFXM_FALSE };
    const FfxBoolean bIsResetFrame = (0 == FrameIndex());
    if (params.bIsExistingSample && !bIsResetFrame) {
        ReprojectHistoryColor(params, fHistoryColor, fTemporalReactiveFactor, bInMotionLastFrame);
        lockState = ReprojectHistoryLockStatus(params, fLockStatus);
    }

    initReactiveMaskFactors(params);
    initDepthClipFactors(params);

    FfxFloat32 fThisFrameReactiveFactor = ffxMax(params.fDilatedReactiveFactor, fTemporalReactiveFactor);

    FfxFloat32 fLuminanceDiff = 0.0f;
    FfxFloat32 fLockContributionThisFrame = 0.0f;
    FfxFloat32x2 fLockStatus32 = {fLockStatus.x, fLockStatus.y};
    UpdateLockStatus(params, fThisFrameReactiveFactor, lockState, fLockStatus32, fLockContributionThisFrame, fLuminanceDiff);
    fLockStatus = FFXM_MIN16_F2(fLockStatus32);

#ifdef FFXM_HLSL
    AccumulateOutputs results = (AccumulateOutputs)0;
#else
    AccumulateOutputs results;
#endif

    // Load upsampled input color
#if FFXM_HALF
#ifdef FFXM_HLSL
    RectificationBoxMin16 clippingBox = (RectificationBoxMin16)0;
#else
    RectificationBoxMin16 clippingBox;
#endif
#else
#ifdef FFXM_HLSL
    RectificationBox clippingBox = (RectificationBox)0;
#else
    RectificationBox clippingBox;
#endif
#endif

    initIsNewSample(params);

    FfxFloat32x4 fUpsampledColorAndWeight = ComputeUpsampledColorAndWeight(params, clippingBox, fThisFrameReactiveFactor);

    FinalizeLockStatus(params, fLockStatus, fUpsampledColorAndWeight.w, results);

#if FFXM_SHADER_QUALITY_OPT_DISABLE_LUMA_INSTABILITY || FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    const FfxFloat32 fLumaInstabilityFactor = 0.0f;
#else
    const FfxFloat32 fLumaInstabilityFactor = ComputeLumaInstabilityFactor(params, clippingBox, fThisFrameReactiveFactor, fLuminanceDiff, results);
#endif

    FfxFloat32x3 fAccumulation = ComputeBaseAccumulationWeight(params, fThisFrameReactiveFactor, bInMotionLastFrame, fUpsampledColorAndWeight.w, lockState);

    if (params.bIsNewSample) {
#if FFXM_SHADER_QUALITY_OPT_TONEMAPPED_RGB_PREPARED_INPUT_COLOR
        fHistoryColor = InverseTonemap(fUpsampledColorAndWeight.xyz);
#else
        fHistoryColor = YCoCgToRGB(fUpsampledColorAndWeight.xyz);
#endif
    }
    else {
        RectifyHistory(params, clippingBox, fHistoryColor, fAccumulation, fLockContributionThisFrame, fThisFrameReactiveFactor, fLumaInstabilityFactor);

        Accumulate(params, fHistoryColor, fAccumulation, fUpsampledColorAndWeight);
    }

    fHistoryColor = UnprepareRgb(fHistoryColor, Exposure());

    // Get new temporal reactive factor
    fTemporalReactiveFactor = FFXM_MIN16_F(ComputeTemporalReactiveFactor(params, fThisFrameReactiveFactor));

#if !FFXM_SHADER_QUALITY_OPT_SEPARATE_TEMPORAL_REACTIVE || FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    results.fColorAndWeight = FfxFloat32x4(fHistoryColor, fTemporalReactiveFactor);
#else
    // Output the upscaled color and the temporal reactive factor if these are contained in separate textures
    results.fUpscaledColor = fHistoryColor;
    results.fTemporalReactive = fTemporalReactiveFactor;
#endif
    // Output final color when RCAS is disabled
#if FFXM_FSR2_OPTION_APPLY_SHARPENING == 0
    results.fColor = fHistoryColor;
#endif
    StoreNewLocks(iPxHrPos, 0);

    return results;
}

#endif // FFXM_FSR2_ACCUMULATE_H
