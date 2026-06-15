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

#ifndef FFXM_FSR2_LOCK_H
#define FFXM_FSR2_LOCK_H

void ClearResourcesForNextFrame(in FfxInt32x2 iPxHrPos)
{
    if (all(FFXM_LESS_THAN(iPxHrPos, FfxInt32x2(RenderSize()))))
    {
#if FFXM_FSR2_OPTION_INVERTED_DEPTH
        const FfxUInt32 farZ = 0x0;
#else
        const FfxUInt32 farZ = 0x3f800000;
#endif
        SetReconstructedDepth(iPxHrPos, farZ);
    }
}

FfxBoolean ComputeThinFeatureConfidence(FfxInt32x2 pos)
{
    const FfxInt32 RADIUS = 1;

    FFXM_MIN16_F fNucleus = LoadLockInputLuma(pos);

    FFXM_MIN16_F similar_threshold = FFXM_MIN16_F(1.05f);
    FFXM_MIN16_F dissimilarLumaMin = FFXM_MIN16_F(FSR2_FP16_MAX);
    FFXM_MIN16_F dissimilarLumaMax = FFXM_MIN16_F(0);

    /*
     0 1 2
     3 4 5
     6 7 8
    */

    #define SETBIT(x) (1U << x)

    FfxUInt32 mask = SETBIT(4); //flag fNucleus as similar

    const FfxUInt32 uNumRejectionMasks = 4;
    const FfxUInt32 uRejectionMasks[uNumRejectionMasks] = {
        SETBIT(0) | SETBIT(1) | SETBIT(3) | SETBIT(4), //Upper left
        SETBIT(1) | SETBIT(2) | SETBIT(4) | SETBIT(5), //Upper right
        SETBIT(3) | SETBIT(4) | SETBIT(6) | SETBIT(7), //Lower left
        SETBIT(4) | SETBIT(5) | SETBIT(7) | SETBIT(8), //Lower right
    };

    FFXM_MIN16_F lumaSamples [9];
    FFXM_MIN16_F fTmpDummy = FFXM_MIN16_F(0.0f);
    const FfxFloat32x2 fInputLumaSize = FfxFloat32x2(RenderSize());
    const FfxFloat32x2 fPxBaseUv = FfxFloat32x2(pos) / fInputLumaSize;
    const FfxFloat32x2 fUnitUv = FfxFloat32x2(1.0f, 1.0f) / fInputLumaSize;

    // Gather samples
    GatherLockInputLumaRQuad(fPxBaseUv,
        lumaSamples[0], lumaSamples[1],
        lumaSamples[3], lumaSamples[4]);
    GatherLockInputLumaRQuad(fUnitUv + fPxBaseUv,
        fTmpDummy, lumaSamples[5],
        lumaSamples[7], lumaSamples[8]);
    lumaSamples[2] = LoadLockInputLuma(pos + FfxInt32x2(1, -1));
    lumaSamples[6] = LoadLockInputLuma(pos + FfxInt32x2(-1, 1));

    FfxInt32 idx = 0;
    FFXM_UNROLL
    for (FfxInt32 y = -RADIUS; y <= RADIUS; y++) {
        FFXM_UNROLL
        for (FfxInt32 x = -RADIUS; x <= RADIUS; x++, idx++) {
            if (x == 0 && y == 0) continue;

            FfxInt32 sampleIdx = (y + 1) * 3 + x + 1;
            FFXM_MIN16_F sampleLuma = lumaSamples[sampleIdx];

            FFXM_MIN16_F difference = ffxMax(sampleLuma, fNucleus) / ffxMin(sampleLuma, fNucleus);

            if (difference > FFXM_MIN16_F(0) && (difference < similar_threshold)) {
                mask |= SETBIT(idx);
            } else {
                dissimilarLumaMin = ffxMin(dissimilarLumaMin, sampleLuma);
                dissimilarLumaMax = ffxMax(dissimilarLumaMax, sampleLuma);
            }
        }
    }

    FfxBoolean isRidge = fNucleus > dissimilarLumaMax || fNucleus < dissimilarLumaMin;

    if (FFXM_FALSE == isRidge) {

        return false;
    }

    FFXM_UNROLL
    for (FfxInt32 i = 0; i < 4; i++) {

        if ((mask & uRejectionMasks[i]) == uRejectionMasks[i]) {
            return false;
        }
    }

    return true;
}

void ComputeLock(FfxInt32x2 iPxLrPos)
{
    if (ComputeThinFeatureConfidence(iPxLrPos))
    {
        StoreNewLocks(ComputeHrPosFromLrPos(iPxLrPos), 1.f);
    }

    ClearResourcesForNextFrame(iPxLrPos);
}

#endif // FFXM_FSR2_LOCK_H
