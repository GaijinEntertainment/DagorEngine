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

#if !defined(FFXM_FSR2_COMMON_H)
#define FFXM_FSR2_COMMON_H

#if defined(FFXM_CPU) || defined(FFXM_GPU)
//Locks
#define LOCK_LIFETIME_REMAINING 0
#define LOCK_TEMPORAL_LUMA 1
#endif // #if defined(FFXM_CPU) || defined(FFXM_GPU)

#if defined(FFXM_GPU)
FFXM_STATIC const FfxFloat32 FSR2_FP16_MIN = 6.10e-05f;
FFXM_STATIC const FfxFloat32 FSR2_FP16_MAX = 65504.0f;
FFXM_STATIC const FfxFloat32 FSR2_EPSILON = 1e-03f;
FFXM_STATIC const FfxFloat32 FSR2_TONEMAP_EPSILON = 1.0f / FSR2_FP16_MAX;
FFXM_STATIC const FfxFloat32 FSR2_FLT_MAX = 3.402823466e+38f;
FFXM_STATIC const FfxFloat32 FSR2_FLT_MIN = 1.175494351e-38f;

// treat vector truncation warnings as errors
#pragma warning(error: 3206)

// suppress warnings
#pragma warning(disable: 3205)  // conversion from larger type to smaller
#pragma warning(disable: 3571)  // in ffxPow(f, e), f could be negative

// Reconstructed depth usage
FFXM_STATIC const FfxFloat32 fReconstructedDepthBilinearWeightThreshold = 0.01f;

// Accumulation
#if !FFXM_SHADER_QUALITY_OPT_UPSCALING_LANCZOS_5TAP
FFXM_STATIC const FfxFloat32 fUpsampleLanczosWeightScale = 1.0f / 9.0f;
#else
FFXM_STATIC const FfxFloat32 fUpsampleLanczosWeightScale = 1.0f / 5.0f;
#endif
FFXM_STATIC const FfxFloat32 fMaxAccumulationLanczosWeight = 1.0f;
FFXM_STATIC const FfxFloat32 fAverageLanczosWeightPerFrame = 0.74f * fUpsampleLanczosWeightScale; // Average lanczos weight for jitter accumulated samples
FFXM_STATIC const FfxFloat32 fAccumulationMaxOnMotion = 3.0f * fUpsampleLanczosWeightScale;

// Auto exposure
FFXM_STATIC const FfxFloat32 resetAutoExposureAverageSmoothing = 65504.0f;

// Optimizations defines
#ifndef FFXM_OPT_USE_GATHER_OPS
#define FFXM_OPT_USE_GATHER_OPS 0
#endif

struct AccumulationPassCommonParams
{
    FfxInt32x2 iPxHrPos;
    FfxFloat32x2 fHrUv;
    FfxFloat32x2 fLrUv_HwSampler;
    FfxFloat32x2 fMotionVector;
    FfxFloat32x2 fReprojectedHrUv;
    FfxFloat32 fHrVelocity;
    FFXM_MIN16_F fDepthClipFactor;
    FFXM_MIN16_F fDilatedReactiveFactor;
    FFXM_MIN16_F fAccumulationMask;

    //FfxBoolean bIsResetFrame;
    FfxBoolean bIsExistingSample;
    FfxBoolean bIsNewSample;
};

struct LockState
{
    FfxBoolean NewLock; //Set for both unique new and re-locked new
    FfxBoolean WasLockedPrevFrame; //Set to identify if the pixel was already locked (relock)
};

void InitializeNewLockSample(FFXM_PARAMETER_OUT FfxFloat32x2 fLockStatus)
{
    fLockStatus = FfxFloat32x2(0, 0);
}

#if FFXM_HALF
void InitializeNewLockSample(FFXM_PARAMETER_OUT FFXM_MIN16_F2 fLockStatus)
{
    fLockStatus = FFXM_MIN16_F2(0, 0);
}
#endif


void KillLock(FFXM_PARAMETER_INOUT FfxFloat32x2 fLockStatus)
{
    fLockStatus[LOCK_LIFETIME_REMAINING] = 0;
}

#if FFXM_HALF
void KillLock(FFXM_PARAMETER_INOUT FFXM_MIN16_F2 fLockStatus)
{
    fLockStatus[LOCK_LIFETIME_REMAINING] = FFXM_MIN16_F(0);
}
#endif

struct RectificationBox
{
    FfxFloat32x3 boxCenter;
    FfxFloat32x3 boxVec;
    FfxFloat32x3 aabbMin;
    FfxFloat32x3 aabbMax;
    FfxFloat32 fBoxCenterWeight;
};
#if FFXM_HALF
struct RectificationBoxMin16
{
    FFXM_MIN16_F3 boxCenter;
    FFXM_MIN16_F3 boxVec;
    FFXM_MIN16_F3 aabbMin;
    FFXM_MIN16_F3 aabbMax;
    FFXM_MIN16_F fBoxCenterWeight;
};
#endif

void RectificationBoxReset(FFXM_PARAMETER_INOUT RectificationBox rectificationBox)
{
    rectificationBox.fBoxCenterWeight = FfxFloat32(0);

    rectificationBox.boxCenter = FfxFloat32x3(0, 0, 0);
    rectificationBox.boxVec = FfxFloat32x3(0, 0, 0);
    rectificationBox.aabbMin = FfxFloat32x3(FSR2_FLT_MAX, FSR2_FLT_MAX, FSR2_FLT_MAX);
    rectificationBox.aabbMax = -FfxFloat32x3(FSR2_FLT_MAX, FSR2_FLT_MAX, FSR2_FLT_MAX);
}
#if FFXM_HALF
void RectificationBoxReset(FFXM_PARAMETER_INOUT RectificationBoxMin16 rectificationBox)
{
    rectificationBox.fBoxCenterWeight = FFXM_MIN16_F(0);

    rectificationBox.boxCenter = FFXM_MIN16_F3(0, 0, 0);
    rectificationBox.boxVec = FFXM_MIN16_F3(0, 0, 0);
    rectificationBox.aabbMin = FFXM_MIN16_F3(FSR2_FP16_MAX, FSR2_FP16_MAX, FSR2_FP16_MAX);
    rectificationBox.aabbMax = -FFXM_MIN16_F3(FSR2_FP16_MAX, FSR2_FP16_MAX, FSR2_FP16_MAX);
}
#endif

void RectificationBoxAddInitialSample(FFXM_PARAMETER_INOUT RectificationBox rectificationBox, const FfxFloat32x3 colorSample, const FfxFloat32 fSampleWeight)
{
    rectificationBox.aabbMin = colorSample;
    rectificationBox.aabbMax = colorSample;

    FfxFloat32x3 weightedSample = colorSample * fSampleWeight;
    rectificationBox.boxCenter = weightedSample;
    rectificationBox.boxVec = colorSample * weightedSample;
    rectificationBox.fBoxCenterWeight = fSampleWeight;
}

void RectificationBoxAddSample(FfxBoolean bInitialSample, FFXM_PARAMETER_INOUT RectificationBox rectificationBox, const FfxFloat32x3 colorSample, const FfxFloat32 fSampleWeight)
{
    if (bInitialSample) {
        RectificationBoxAddInitialSample(rectificationBox, colorSample, fSampleWeight);
    } else {
        rectificationBox.aabbMin = ffxMin(rectificationBox.aabbMin, colorSample);
        rectificationBox.aabbMax = ffxMax(rectificationBox.aabbMax, colorSample);

        FfxFloat32x3 weightedSample = colorSample * fSampleWeight;
        rectificationBox.boxCenter += weightedSample;
        rectificationBox.boxVec += colorSample * weightedSample;
        rectificationBox.fBoxCenterWeight += fSampleWeight;
    }
}
#if FFXM_HALF
void RectificationBoxAddInitialSample(FFXM_PARAMETER_INOUT RectificationBoxMin16 rectificationBox, const FFXM_MIN16_F3 colorSample, const FFXM_MIN16_F fSampleWeight)
{
    rectificationBox.aabbMin = colorSample;
    rectificationBox.aabbMax = colorSample;

    FFXM_MIN16_F3 weightedSample = colorSample * fSampleWeight;
    rectificationBox.boxCenter = weightedSample;
    rectificationBox.boxVec = colorSample * weightedSample;
    rectificationBox.fBoxCenterWeight = fSampleWeight;
}

void RectificationBoxAddSample(FfxBoolean bInitialSample, FFXM_PARAMETER_INOUT RectificationBoxMin16 rectificationBox, const FFXM_MIN16_F3 colorSample, const FFXM_MIN16_F fSampleWeight)
{
    if (bInitialSample) {
        RectificationBoxAddInitialSample(rectificationBox, colorSample, fSampleWeight);
    } else {
        rectificationBox.aabbMin = ffxMin(rectificationBox.aabbMin, colorSample);
        rectificationBox.aabbMax = ffxMax(rectificationBox.aabbMax, colorSample);

        FFXM_MIN16_F3 weightedSample = colorSample * fSampleWeight;
        rectificationBox.boxCenter += weightedSample;
        rectificationBox.boxVec += colorSample * weightedSample;
        rectificationBox.fBoxCenterWeight += fSampleWeight;
    }
}
#endif

void RectificationBoxComputeVarianceBoxData(FFXM_PARAMETER_INOUT RectificationBox rectificationBox)
{
    rectificationBox.fBoxCenterWeight = (abs(rectificationBox.fBoxCenterWeight) > FfxFloat32(FSR2_EPSILON) ? rectificationBox.fBoxCenterWeight : FfxFloat32(1.f));
    rectificationBox.boxCenter /= rectificationBox.fBoxCenterWeight;
    rectificationBox.boxVec /= rectificationBox.fBoxCenterWeight;
    FfxFloat32x3 stdDev = sqrt(abs(rectificationBox.boxVec - rectificationBox.boxCenter * rectificationBox.boxCenter));
    rectificationBox.boxVec = stdDev;
}
#if FFXM_HALF
void RectificationBoxComputeVarianceBoxData(FFXM_PARAMETER_INOUT RectificationBoxMin16 rectificationBox)
{
    rectificationBox.fBoxCenterWeight = (abs(rectificationBox.fBoxCenterWeight) > FFXM_MIN16_F(FSR2_EPSILON) ? rectificationBox.fBoxCenterWeight : FFXM_MIN16_F(1.f));
    rectificationBox.boxCenter /= rectificationBox.fBoxCenterWeight;
    rectificationBox.boxVec /= rectificationBox.fBoxCenterWeight;
    FFXM_MIN16_F3 stdDev = sqrt(abs(rectificationBox.boxVec - rectificationBox.boxCenter * rectificationBox.boxCenter));
    rectificationBox.boxVec = stdDev;
}
#endif

FfxFloat32x3 SafeRcp3(FfxFloat32x3 v)
{
    return (all(FFXM_NOT_EQUAL(v, FfxFloat32x3(0, 0, 0)))) ? (FfxFloat32x3(1, 1, 1) / v) : FfxFloat32x3(0, 0, 0);
}
#if FFXM_HALF
FFXM_MIN16_F3 SafeRcp3(FFXM_MIN16_F3 v)
{
    return (all(FFXM_NOT_EQUAL(v, FFXM_MIN16_F3(0, 0, 0)))) ? (FFXM_MIN16_F3(1, 1, 1) / v) : FFXM_MIN16_F3(0, 0, 0);
}
#endif

FfxFloat32 MinDividedByMax(const FfxFloat32 v0, const FfxFloat32 v1)
{
    const FfxFloat32 m = ffxMax(v0, v1);
    return m != 0 ? ffxMin(v0, v1) / m : 0;
}

#if FFXM_HALF
FFXM_MIN16_F MinDividedByMax(const FFXM_MIN16_F v0, const FFXM_MIN16_F v1)
{
    const FFXM_MIN16_F m = ffxMax(v0, v1);
    return m != FFXM_MIN16_F(0) ? ffxMin(v0, v1) / m : FFXM_MIN16_F(0);
}
#endif

FfxFloat32x3 YCoCgToRGB(FfxFloat32x3 fYCoCg)
{
    FfxFloat32x3 fRgb;

    fRgb = FfxFloat32x3(
        fYCoCg.x + fYCoCg.y - fYCoCg.z,
        fYCoCg.x + fYCoCg.z,
        fYCoCg.x - fYCoCg.y - fYCoCg.z);

    return fRgb;
}
#if FFXM_HALF
FFXM_MIN16_F3 YCoCgToRGB(FFXM_MIN16_F3 fYCoCg)
{
    FFXM_MIN16_F3 fRgb;

    fRgb = FFXM_MIN16_F3(
        fYCoCg.x + fYCoCg.y - fYCoCg.z,
        fYCoCg.x + fYCoCg.z,
        fYCoCg.x - fYCoCg.y - fYCoCg.z);

    return fRgb;
}
#endif

FfxFloat32x3 RGBToYCoCg(FfxFloat32x3 fRgb)
{
    FfxFloat32x3 fYCoCg;

    fYCoCg = FfxFloat32x3(
        0.25f * fRgb.r + 0.5f * fRgb.g + 0.25f * fRgb.b,
        0.5f * fRgb.r - 0.5f * fRgb.b,
        -0.25f * fRgb.r + 0.5f * fRgb.g - 0.25f * fRgb.b);

    return fYCoCg;
}
#if FFXM_HALF
FFXM_MIN16_F3 RGBToYCoCg(FFXM_MIN16_F3 fRgb)
{
    FFXM_MIN16_F3 fYCoCg;

    fYCoCg = FFXM_MIN16_F3(
        0.25 * fRgb.r + 0.5 * fRgb.g + 0.25 * fRgb.b,
        0.5 * fRgb.r - 0.5 * fRgb.b,
        -0.25 * fRgb.r + 0.5 * fRgb.g - 0.25 * fRgb.b);

    return fYCoCg;
}
#endif

FfxFloat32 RGBToLuma(FfxFloat32x3 fLinearRgb)
{
    return dot(fLinearRgb, FfxFloat32x3(0.2126f, 0.7152f, 0.0722f));
}
#if FFXM_HALF
FFXM_MIN16_F RGBToLuma(FFXM_MIN16_F3 fLinearRgb)
{
    return dot(fLinearRgb, FFXM_MIN16_F3(0.2126f, 0.7152f, 0.0722f));
}
#endif

FfxFloat32 RGBToPerceivedLuma(FfxFloat32x3 fLinearRgb)
{
    FfxFloat32 fLuminance = RGBToLuma(fLinearRgb);

    FfxFloat32 fPercievedLuminance = 0;
    if (fLuminance <= 216.0f / 24389.0f) {
        fPercievedLuminance = fLuminance * (24389.0f / 27.0f);
    }
    else {
        fPercievedLuminance = ffxPow(fLuminance, 1.0f / 3.0f) * 116.0f - 16.0f;
    }

    return fPercievedLuminance * 0.01f;
}
#if FFXM_HALF
FFXM_MIN16_F RGBToPerceivedLuma(FFXM_MIN16_F3 fLinearRgb)
{
    FFXM_MIN16_F fLuminance = RGBToLuma(fLinearRgb);

    FFXM_MIN16_F fPercievedLuminance = FFXM_MIN16_F(0);
    if (fLuminance <= FFXM_MIN16_F(216.0f / 24389.0f)) {
        fPercievedLuminance = fLuminance * FFXM_MIN16_F(24389.0f / 27.0f);
    }
    else {
        fPercievedLuminance = ffxPow(fLuminance, FFXM_MIN16_F(1.0f / 3.0f)) * FFXM_MIN16_F(116.0f) - FFXM_MIN16_F(16.0f);
    }

    return fPercievedLuminance * FFXM_MIN16_F(0.01f);
}
#endif

FfxFloat32x3 Tonemap(FfxFloat32x3 fRgb)
{
    return fRgb / (ffxMax(ffxMax(0.f, fRgb.r), ffxMax(fRgb.g, fRgb.b)) + 1.f).xxx;
}

FfxFloat32x3 InverseTonemap(FfxFloat32x3 fRgb)
{
    return fRgb / ffxMax(FSR2_TONEMAP_EPSILON, 1.f - ffxMax(fRgb.r, ffxMax(fRgb.g, fRgb.b))).xxx;
}

#if FFXM_HALF
FFXM_MIN16_F3 Tonemap(FFXM_MIN16_F3 fRgb)
{
    return fRgb / (ffxMax(ffxMax(FFXM_MIN16_F(0.f), fRgb.r), ffxMax(fRgb.g, fRgb.b)) + FFXM_MIN16_F(1.f)).xxx;
}

FFXM_MIN16_F3 InverseTonemap(FFXM_MIN16_F3 fRgb)
{
    return fRgb / ffxMax(FFXM_MIN16_F(FSR2_TONEMAP_EPSILON), FFXM_MIN16_F(1.f) - ffxMax(fRgb.r, ffxMax(fRgb.g, fRgb.b))).xxx;
}
#endif

FfxInt32x2 ClampLoad(FfxInt32x2 iPxSample, FfxInt32x2 iPxOffset, FfxInt32x2 iTextureSize)
{
    FfxInt32x2 result = iPxSample + iPxOffset;
    result.x = (iPxOffset.x < 0) ? ffxMax(result.x, 0) : result.x;
    result.x = (iPxOffset.x > 0) ? ffxMin(result.x, iTextureSize.x - 1) : result.x;
    result.y = (iPxOffset.y < 0) ? ffxMax(result.y, 0) : result.y;
    result.y = (iPxOffset.y > 0) ? ffxMin(result.y, iTextureSize.y - 1) : result.y;
    return result;

    // return ffxMed3(iPxSample + iPxOffset, FfxInt32x2(0, 0), iTextureSize - FfxInt32x2(1, 1));
}
#if FFXM_HALF
FFXM_MIN16_I2 ClampLoad(FFXM_MIN16_I2 iPxSample, FFXM_MIN16_I2 iPxOffset, FFXM_MIN16_I2 iTextureSize)
{
    FFXM_MIN16_I2 result = iPxSample + iPxOffset;
    result.x = (iPxOffset.x < 0) ? ffxMax(result.x, FFXM_MIN16_I(0)) : result.x;
    result.x = (iPxOffset.x > 0) ? ffxMin(result.x, iTextureSize.x - FFXM_MIN16_I(1)) : result.x;
    result.y = (iPxOffset.y < 0) ? ffxMax(result.y, FFXM_MIN16_I(0)) : result.y;
    result.y = (iPxOffset.y > 0) ? ffxMin(result.y, iTextureSize.y - FFXM_MIN16_I(1)) : result.y;
    return result;

    // return ffxMed3Half(iPxSample + iPxOffset, FFXM_MIN16_I2(0, 0), iTextureSize - FFXM_MIN16_I2(1, 1));
}
#endif

FfxFloat32x2 ClampUv(FfxFloat32x2 fUv, FfxInt32x2 iTextureSize, FfxInt32x2 iResourceSize)
{
    const FfxFloat32x2 fSampleLocation = fUv * iTextureSize;
    const FfxFloat32x2 fClampedLocation = ffxMax(FfxFloat32x2(0.5f, 0.5f), ffxMin(fSampleLocation, FfxFloat32x2(iTextureSize) - FfxFloat32x2(0.5f, 0.5f)));
    const FfxFloat32x2 fClampedUv = fClampedLocation / FfxFloat32x2(iResourceSize);

    return fClampedUv;
}

FfxBoolean IsOnScreen(FfxInt32x2 pos, FfxInt32x2 size)
{
    return all(FFXM_LESS_THAN(FfxUInt32x2(pos), FfxUInt32x2(size)));
}
#if FFXM_HALF
FfxBoolean IsOnScreen(FFXM_MIN16_I2 pos, FFXM_MIN16_I2 size)
{
    return all(FFXM_LESS_THAN(FFXM_MIN16_U2(pos), FFXM_MIN16_U2(size)));
}
#endif

FfxFloat32 ComputeAutoExposureFromLavg(FfxFloat32 Lavg)
{
    Lavg = exp(Lavg);

    const FfxFloat32 S = 100.0f; //ISO arithmetic speed
    const FfxFloat32 K = 12.5f;
    FfxFloat32 ExposureISO100 = log2((Lavg * S) / K);

    const FfxFloat32 q = 0.65f;
    FfxFloat32 Lmax = (78.0f / (q * S)) * ffxPow(2.0f, ExposureISO100);

    return 1 / Lmax;
}
#if FFXM_HALF
FFXM_MIN16_F ComputeAutoExposureFromLavg(FFXM_MIN16_F Lavg)
{
    Lavg = exp(Lavg);

    const FFXM_MIN16_F S = FFXM_MIN16_F(100.0f); //ISO arithmetic speed
    const FFXM_MIN16_F K = FFXM_MIN16_F(12.5f);
    const FFXM_MIN16_F ExposureISO100 = log2((Lavg * S) / K);

    const FFXM_MIN16_F q = FFXM_MIN16_F(0.65f);
    const FFXM_MIN16_F Lmax = (FFXM_MIN16_F(78.0f) / (q * S)) * ffxPow(FFXM_MIN16_F(2.0f), ExposureISO100);

    return FFXM_MIN16_F(1) / Lmax;
}
#endif

FfxInt32x2 ComputeHrPosFromLrPos(FfxInt32x2 iPxLrPos)
{
    FfxFloat32x2 fSrcJitteredPos = FfxFloat32x2(iPxLrPos) + 0.5f - Jitter();
    FfxFloat32x2 fLrPosInHr = (fSrcJitteredPos / RenderSize()) * DisplaySize();
    FfxInt32x2 iPxHrPos = FfxInt32x2(floor(fLrPosInHr));
    return iPxHrPos;
}
#if FFXM_HALF
FFXM_MIN16_I2 ComputeHrPosFromLrPos(FFXM_MIN16_I2 iPxLrPos)
{
    FFXM_MIN16_F2 fSrcJitteredPos = FFXM_MIN16_F2(iPxLrPos) + FFXM_MIN16_F(0.5f) - FFXM_MIN16_F2(Jitter());
    FFXM_MIN16_F2 fLrPosInHr = (fSrcJitteredPos / FFXM_MIN16_F2(RenderSize())) * FFXM_MIN16_F2(DisplaySize());
    FFXM_MIN16_I2 iPxHrPos = FFXM_MIN16_I2(floor(fLrPosInHr));
    return iPxHrPos;
}
#endif

FfxFloat32x2 ComputeNdc(FfxFloat32x2 fPxPos, FfxInt32x2 iSize)
{
    return fPxPos / FfxFloat32x2(iSize) * FfxFloat32x2(2.0f, -2.0f) + FfxFloat32x2(-1.0f, 1.0f);
}

FfxFloat32 GetViewSpaceDepth(FfxFloat32 fDeviceDepth)
{
    const FfxFloat32x4 fDeviceToViewDepth = DeviceToViewSpaceTransformFactors();

    // fDeviceToViewDepth details found in ffx_fsr2.cpp
    return (fDeviceToViewDepth[1] / (fDeviceDepth - fDeviceToViewDepth[0]));
}

FfxFloat32 GetViewSpaceDepthInMeters(FfxFloat32 fDeviceDepth)
{
    return GetViewSpaceDepth(fDeviceDepth) * ViewSpaceToMetersFactor();
}

FfxFloat32x3 GetViewSpacePosition(FfxInt32x2 iViewportPos, FfxInt32x2 iViewportSize, FfxFloat32 fDeviceDepth)
{
    const FfxFloat32x4 fDeviceToViewDepth = DeviceToViewSpaceTransformFactors();

    const FfxFloat32 Z = GetViewSpaceDepth(fDeviceDepth);

    const FfxFloat32x2 fNdcPos = ComputeNdc(iViewportPos, iViewportSize);
    const FfxFloat32 X = fDeviceToViewDepth[2] * fNdcPos.x * Z;
    const FfxFloat32 Y = fDeviceToViewDepth[3] * fNdcPos.y * Z;

    return FfxFloat32x3(X, Y, Z);
}

FfxFloat32x3 GetViewSpacePositionInMeters(FfxInt32x2 iViewportPos, FfxInt32x2 iViewportSize, FfxFloat32 fDeviceDepth)
{
    return GetViewSpacePosition(iViewportPos, iViewportSize, fDeviceDepth) * ViewSpaceToMetersFactor();
}

FfxFloat32 GetMaxDistanceInMeters()
{
#if FFXM_FSR2_OPTION_INVERTED_DEPTH
    return GetViewSpaceDepth(0.0f) * ViewSpaceToMetersFactor();
#else
    return GetViewSpaceDepth(1.0f) * ViewSpaceToMetersFactor();
#endif
}

FfxFloat32x3 PrepareRgb(FfxFloat32x3 fRgb, FfxFloat32 fExposure, FfxFloat32 fPreExposure)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    return fRgb;
#else
    fRgb /= fPreExposure;
    fRgb *= fExposure;

    fRgb = clamp(fRgb, 0.0f, FSR2_FP16_MAX);

    return fRgb;
#endif
}

FfxFloat32x3 UnprepareRgb(FfxFloat32x3 fRgb, FfxFloat32 fExposure)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    return fRgb;
#else
    fRgb /= fExposure;
    fRgb *= PreExposure();

    return fRgb;
#endif
}

#if FFXM_HALF
FfxFloat16x3 PrepareRgb(FfxFloat16x3 fRgb, FfxFloat16 fExposure, FfxFloat16 fPreExposure)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    return fRgb;
#else
    fRgb /= fPreExposure;
    fRgb *= fExposure;

    fRgb = clamp(fRgb, FfxFloat16(0.0f), FfxFloat16(FSR2_FP16_MAX));

    return fRgb;
#endif
}

FfxFloat16x3 UnprepareRgb(FfxFloat16x3 fRgb, FfxFloat16 fExposure)
{
#if FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE
    return fRgb;
#else
    fRgb /= fExposure;
    fRgb *= FfxFloat16(PreExposure());

    return fRgb;
#endif
}
#endif

struct BilinearSamplingData
{
    FfxInt32x2 iOffsets[4];
    FfxFloat32 fWeights[4];
    FfxInt32x2 iBasePos;
    FfxFloat32x2 fQuadCenterUv;
};

BilinearSamplingData GetBilinearSamplingData(FfxFloat32x2 fUv, FfxInt32x2 iSize)
{
    BilinearSamplingData data;

    FfxFloat32x2 fPxSample = (fUv * iSize) - FfxFloat32x2(0.5f, 0.5f);
    data.iBasePos = FfxInt32x2(floor(fPxSample));
    data.fQuadCenterUv = fPxSample / FfxFloat32x2(iSize);
    FfxFloat32x2 fPxFrac = ffxFract(fPxSample);

    data.iOffsets[0] = FfxInt32x2(0, 0);
    data.iOffsets[1] = FfxInt32x2(1, 0);
    data.iOffsets[2] = FfxInt32x2(0, 1);
    data.iOffsets[3] = FfxInt32x2(1, 1);

    data.fWeights[0] = (1 - fPxFrac.x) * (1 - fPxFrac.y);
    data.fWeights[1] = (fPxFrac.x) * (1 - fPxFrac.y);
    data.fWeights[2] = (1 - fPxFrac.x) * (fPxFrac.y);
    data.fWeights[3] = (fPxFrac.x) * (fPxFrac.y);

    return data;
}

struct PlaneData
{
    FfxFloat32x3 fNormal;
    FfxFloat32 fDistanceFromOrigin;
};

PlaneData GetPlaneFromPoints(FfxFloat32x3 fP0, FfxFloat32x3 fP1, FfxFloat32x3 fP2)
{
    PlaneData plane;

    FfxFloat32x3 v0 = fP0 - fP1;
    FfxFloat32x3 v1 = fP0 - fP2;
    plane.fNormal = normalize(cross(v0, v1));
    plane.fDistanceFromOrigin = -dot(fP0, plane.fNormal);

    return plane;
}

FfxFloat32 PointToPlaneDistance(PlaneData plane, FfxFloat32x3 fPoint)
{
    return abs(dot(plane.fNormal, fPoint) + plane.fDistanceFromOrigin);
}

#endif // #if defined(FFXM_GPU)

#endif //!defined(FFXM_FSR2_COMMON_H)
