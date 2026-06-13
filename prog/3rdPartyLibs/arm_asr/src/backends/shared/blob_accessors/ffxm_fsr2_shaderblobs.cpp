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

#include <host/ffxm_util.h>
#include "ffxm_fsr2_shaderblobs.h"
#include "fsr2/ffxm_fsr2_private.h"

#include <ffxm_fsr2_autogen_reactive_pass_fs_16bit_permutations.h>
#include <ffxm_fsr2_accumulate_pass_fs_16bit_permutations.h>
#include <ffxm_fsr2_compute_luminance_pyramid_pass_16bit_permutations.h>
#include <ffxm_fsr2_depth_clip_pass_fs_16bit_permutations.h>
#include <ffxm_fsr2_lock_pass_16bit_permutations.h>
#include <ffxm_fsr2_reconstruct_previous_depth_pass_fs_16bit_permutations.h>
#include <ffxm_fsr2_rcas_pass_fs_16bit_permutations.h>
#include <ffxm_fsr2_vs_16bit_permutations.h>

#include <string.h> // for memset

namespace arm
{

#if defined(POPULATE_PERMUTATION_KEY)
#undef POPULATE_PERMUTATION_KEY
#endif // #if defined(POPULATE_PERMUTATION_KEY)
#define POPULATE_PERMUTATION_KEY(options, key)                                                                \
key.index = 0;                                                                                                \
key.FFXM_FSR2_OPTION_HDR_COLOR_INPUT = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);                 \
key.FFXM_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);   \
key.FFXM_FSR2_OPTION_JITTERED_MOTION_VECTORS = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);   \
key.FFXM_FSR2_OPTION_INVERTED_DEPTH = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);                   \
key.FFXM_FSR2_OPTION_APPLY_SHARPENING = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING); \
key.FFXM_FSR2_OPTION_SHADER_OPT_BALANCED = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_APPLY_BALANCED_OPT); \
key.FFXM_FSR2_OPTION_SHADER_OPT_PERFORMANCE = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_APPLY_PERFORMANCE_OPT); \
key.FFXM_FSR2_OPTION_SHADER_OPT_ULTRA_PERFORMANCE = FFXM_CONTAINS_FLAG(options, FSR2_SHADER_PERMUTATION_APPLY_ULTRA_PERFORMANCE_OPT);

static FfxmShaderBlob fsr2GetDepthClipPassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_depth_clip_pass_fs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_depth_clip_pass_fs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_depth_clip_pass_fs_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetReconstructPreviousDepthPassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_reconstruct_previous_depth_pass_fs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_reconstruct_previous_depth_pass_fs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_reconstruct_previous_depth_pass_fs_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetLockPassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_lock_pass_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_lock_pass_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_lock_pass_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetAccumulatePassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_accumulate_pass_fs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_accumulate_pass_fs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_accumulate_pass_fs_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetRCASPassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_rcas_pass_fs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_rcas_pass_fs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_rcas_pass_fs_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetComputeLuminancePyramidPassPermutationBlobByIndex(uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_compute_luminance_pyramid_pass_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_compute_luminance_pyramid_pass_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_compute_luminance_pyramid_pass_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetAutogenReactivePassPermutationBlobByIndex(
    uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_autogen_reactive_pass_fs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);

    const int32_t tableIndex = g_ffxm_fsr2_autogen_reactive_pass_fs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_autogen_reactive_pass_fs_16bit_PermutationInfo, tableIndex);
}

static FfxmShaderBlob fsr2GetGeneralVertexPermutationBlobByIndex(
    uint32_t permutationOptions, [[maybe_unused]] bool isWave64, [[maybe_unused]] bool is16bit)
{

    ffxm_fsr2_vs_16bit_PermutationKey key;

    POPULATE_PERMUTATION_KEY(permutationOptions, key);


    const int32_t tableIndex = g_ffxm_fsr2_vs_16bit_IndirectionTable[key.index];
    return POPULATE_SHADER_BLOB_FFX(g_ffxm_fsr2_vs_16bit_PermutationInfo, tableIndex);
}

FfxmErrorCode fsr2GetPermutationBlobByIndex(
    FfxmFsr2Pass passId,
    uint32_t permutationOptions,
    FfxmShaderBlob* outBlob,
    FfxmShaderBlob* outVertBlob) {

    bool isWave64 = FFXM_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit = FFXM_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ALLOW_FP16);

    // Currently all passes share the same vertex shader
    if (outVertBlob)
    {
        FfxmShaderBlob blob = fsr2GetGeneralVertexPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
        memcpy(outVertBlob, &blob, sizeof(FfxmShaderBlob));
    }

    switch (passId) {

        case FFXM_FSR2_PASS_DEPTH_CLIP:
        {
            FfxmShaderBlob blob = fsr2GetDepthClipPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH:
        {
            FfxmShaderBlob blob = fsr2GetReconstructPreviousDepthPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_LOCK:
        {
            FfxmShaderBlob blob = fsr2GetLockPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_ACCUMULATE:
        case FFXM_FSR2_PASS_ACCUMULATE_SHARPEN:
        {
            FfxmShaderBlob blob = fsr2GetAccumulatePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_RCAS:
        {
            FfxmShaderBlob blob = fsr2GetRCASPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID:
        {
            FfxmShaderBlob blob = fsr2GetComputeLuminancePyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        case FFXM_FSR2_PASS_GENERATE_REACTIVE:
        {
            FfxmShaderBlob blob = fsr2GetAutogenReactivePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxmShaderBlob));
            return FFXM_OK;
        }

        default:
            FFXM_ASSERT_FAIL("Should never reach here.");
            break;
    }

    // return an empty blob
    memset(outBlob, 0, sizeof(FfxmShaderBlob));
    return FFXM_OK;
}

FfxmErrorCode fsr2IsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFXM_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFXM_OK;
}

} // namespace arm
