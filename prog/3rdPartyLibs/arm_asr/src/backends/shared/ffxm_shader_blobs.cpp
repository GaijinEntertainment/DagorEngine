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

#include "ffxm_shader_blobs.h"

#if defined(FFXM_FSR) || defined(FFXM_ALL)
#include "blob_accessors/ffxm_fsr2_shaderblobs.h"
#endif // #if defined(FFXM_FSR) || defined(FFXM_ALL)

#include <string.h> // for memset

namespace arm
{

FfxmErrorCode ffxmGetPermutationBlobByIndex(
    FfxmEffect effectId,
    FfxmPass passId,
    uint32_t permutationOptions,
    FfxmShaderBlob* outBlob,
    FfxmShaderBlob* outVertBlob)
{

    switch (effectId)
    {
#if defined(FFXM_FSR) || defined(FFXM_ALL)
    case FFXM_EFFECT_FSR2:
        return fsr2GetPermutationBlobByIndex((FfxmFsr2Pass)passId, permutationOptions, outBlob, outVertBlob);
#endif // #if defined(FFXM_FSR) || defined(FFXM_ALL)

    default:
        FFXM_ASSERT_MESSAGE(false, "Not implemented");
        break;
    }

    // return an empty blob
    memset(outBlob, 0, sizeof(FfxmShaderBlob));
    return FFXM_OK;
}

FfxmErrorCode ffxmIsWave64(FfxmEffect effectId, uint32_t permutationOptions, bool& isWave64)
{
switch (effectId)
    {
#if defined(FFXM_FSR) || defined(FFXM_ALL)
    case FFXM_EFFECT_FSR2:
        return fsr2IsWave64(permutationOptions, isWave64);
#endif // #if defined(FFXM_FSR) || defined(FFXM_ALL)

    default:
        FFXM_ASSERT_MESSAGE(false, "Not implemented");
        isWave64 = false;
        break;
    }

    return FFXM_ERROR_BACKEND_API_ERROR;
}

} // namespace arm
