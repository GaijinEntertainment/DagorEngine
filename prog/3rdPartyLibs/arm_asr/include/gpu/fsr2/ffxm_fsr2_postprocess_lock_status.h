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

#ifndef FFXM_FSR2_POSTPROCESS_LOCK_STATUS_H
#define FFXM_FSR2_POSTPROCESS_LOCK_STATUS_H

FfxFloat32x4 WrapShadingChangeLuma(FfxInt32x2 iPxSample)
{
    return FfxFloat32x4(LoadMipLuma(iPxSample, LumaMipLevelToUse()), 0, 0, 0);
}

#if FFXM_HALF
FFXM_MIN16_F4 WrapShadingChangeLuma(FFXM_MIN16_I2 iPxSample)
{
    return FFXM_MIN16_F4(LoadMipLuma(iPxSample, LumaMipLevelToUse()), 0, 0, 0);
}
#endif

#if FFXM_HALF
DeclareCustomFetchBilinearSamplesMin16(FetchShadingChangeLumaSamples, WrapShadingChangeLuma)
DeclareCustomTextureSampleMin16(ShadingChangeLumaSample, Bilinear, FetchShadingChangeLumaSamples)
#else
DeclareCustomFetchBicubicSamples(FetchShadingChangeLumaSamples, WrapShadingChangeLuma)
DeclareCustomTextureSample(ShadingChangeLumaSample, Lanczos2, FetchShadingChangeLumaSamples)
#endif

FfxFloat32 GetShadingChangeLuma(FfxInt32x2 iPxHrPos, FfxFloat32x2 fUvCoord)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    return 1.0;
#else
    FfxFloat32 fShadingChangeLuma = 0;
    const FfxFloat32 fDiv = FfxFloat32(FfxInt32(2) << LumaMipLevelToUse());
    FfxInt32x2 iMipRenderSize = FfxInt32x2(RenderSize() / fDiv);

    fUvCoord = ClampUv(fUvCoord, iMipRenderSize, LumaMipDimensions());
    fShadingChangeLuma = Exposure() * exp(FfxFloat32(SampleMipLuma(fUvCoord, LumaMipLevelToUse())));

    fShadingChangeLuma = ffxPow(fShadingChangeLuma, 1.0f / 6.0f);

    return fShadingChangeLuma;
#endif
}

void UpdateLockStatus(AccumulationPassCommonParams params,
    FFXM_PARAMETER_INOUT FfxFloat32 fReactiveFactor, LockState state,
    FFXM_PARAMETER_INOUT FfxFloat32x2 fLockStatus,
    FFXM_PARAMETER_OUT FfxFloat32 fLockContributionThisFrame,
    FFXM_PARAMETER_OUT FfxFloat32 fLuminanceDiff) {

    const FfxFloat32 fShadingChangeLuma = GetShadingChangeLuma(params.iPxHrPos, params.fHrUv);

    //init temporal shading change factor, init to -1 or so in reproject to know if "true new"?
    fLockStatus[LOCK_TEMPORAL_LUMA] = (fLockStatus[LOCK_TEMPORAL_LUMA] == FfxFloat32(0.0f)) ? fShadingChangeLuma : fLockStatus[LOCK_TEMPORAL_LUMA];

    FfxFloat32 fPreviousShadingChangeLuma = fLockStatus[LOCK_TEMPORAL_LUMA];

    fLuminanceDiff = 1.0f - MinDividedByMax(fPreviousShadingChangeLuma, fShadingChangeLuma);

    if (state.NewLock) {
        fLockStatus[LOCK_TEMPORAL_LUMA] = fShadingChangeLuma;

        fLockStatus[LOCK_LIFETIME_REMAINING] = (fLockStatus[LOCK_LIFETIME_REMAINING] != 0.0f) ? 2.0f : 1.0f;
    }
    else if(fLockStatus[LOCK_LIFETIME_REMAINING] <= 1.0f) {
        fLockStatus[LOCK_TEMPORAL_LUMA] = ffxLerp(fLockStatus[LOCK_TEMPORAL_LUMA], FfxFloat32(fShadingChangeLuma), 0.5f);
    }
    else {
        if (fLuminanceDiff > 0.1f) {
            KillLock(fLockStatus);
        }
    }

    fReactiveFactor = ffxMax(fReactiveFactor, ffxSaturate((fLuminanceDiff - 0.1f) * 10.0f));
    fLockStatus[LOCK_LIFETIME_REMAINING] *= (1.0f - fReactiveFactor);

    fLockStatus[LOCK_LIFETIME_REMAINING] *= ffxSaturate(1.0f - params.fAccumulationMask);
    fLockStatus[LOCK_LIFETIME_REMAINING] *= FfxFloat32(params.fDepthClipFactor < 0.1f);

    // Compute this frame lock contribution
    const FfxFloat32 fLifetimeContribution = ffxSaturate(fLockStatus[LOCK_LIFETIME_REMAINING] - 1.0f);
    const FfxFloat32 fShadingChangeContribution = ffxSaturate(MinDividedByMax(fLockStatus[LOCK_TEMPORAL_LUMA], fShadingChangeLuma));

    fLockContributionThisFrame = ffxSaturate(ffxSaturate(fLifetimeContribution * 4.0f) * fShadingChangeContribution);
}

#endif //!defined( FFXM_FSR2_POSTPROCESS_LOCK_STATUS_H )
