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

#define FSR2_BIND_SRV_INPUT_COLOR                     0

#define FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC               1
#define FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE        2
#define FSR2_BIND_UAV_EXPOSURE_MIP_5                  3
#define FSR2_BIND_UAV_AUTO_EXPOSURE                   4

#define FSR2_BIND_CB_FSR2                             0
#define FSR2_BIND_CB_SPD                              1

// Global mandatory defines
#if !defined(FFXM_HALF)
#define FFXM_HALF 1
#endif
#if !defined(FFXM_GPU)
#define FFXM_GPU 1
#endif
#if !defined(FFXM_HLSL)
#define FFXM_HLSL 1
#endif

#include "fsr2/ffxm_fsr2_callbacks_hlsl.h"
#include "fsr2/ffxm_fsr2_common.h"
#include "fsr2/ffxm_fsr2_compute_luminance_pyramid.h"

#ifndef FFXM_FSR2_THREAD_GROUP_WIDTH
#define FFXM_FSR2_THREAD_GROUP_WIDTH 256
#endif // #ifndef FFXM_FSR2_THREAD_GROUP_WIDTH
#ifndef FFXM_FSR2_THREAD_GROUP_HEIGHT
#define FFXM_FSR2_THREAD_GROUP_HEIGHT 1
#endif // #ifndef FFXM_FSR2_THREAD_GROUP_HEIGHT
#ifndef FFXM_FSR2_THREAD_GROUP_DEPTH
#define FFXM_FSR2_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFXM_FSR2_THREAD_GROUP_DEPTH
#ifndef FFXM_FSR2_NUM_THREADS
#define FFXM_FSR2_NUM_THREADS [numthreads(FFXM_FSR2_THREAD_GROUP_WIDTH, FFXM_FSR2_THREAD_GROUP_HEIGHT, FFXM_FSR2_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFXM_FSR2_NUM_THREADS

FFXM_PREFER_WAVE64
FFXM_FSR2_NUM_THREADS
FFXM_FSR2_EMBED_CB2_ROOTSIG_CONTENT
void main(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex)
{
    ComputeAutoExposure(WorkGroupId, LocalThreadIndex);
}
