/*
 * Copyright (c) 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Intel Corporation nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <debug/dag_debug.h>
#include <vecmath/dag_vecMath.h>
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <malloc.h>
#endif

#include <math/dag_occlusionTest.h>
#include <3d/dag_maskedOcclusionCulling.h>

#ifndef MOC_SINGLE_IMPLEMENTATION

#ifdef __linux__
#include <cpuid.h>
inline void cpuid(int info[4], int InfoType) { __cpuid_count(InfoType, 0, info[0], info[1], info[2], info[3]); }
#endif

#ifdef _WIN32
static void cpuid(int *info, int x) { __cpuidex(info, x, 0); }
#endif

unsigned long long get_xcr_feature_mask();

static MaskedOcclusionCulling::Implementation DetectCPUFeatures()
{
  int cpu_info[4];
  cpuid(cpu_info, 0);
  int ids_count = cpu_info[0];

  cpuid(cpu_info, 0x80000000u);
  // unsigned ex_ids_count = cpu_info[0];

  //  Detect Features
  if (ids_count >= 0x00000001)
  {
    cpuid(cpu_info, 0x00000001);
    const bool sse41_supported = (cpu_info[2] & ((int)1 << 19)) != 0;

    int os_uses_XSAVE_XRSTORE = (cpu_info[2] & (1 << 27)) != 0;
    int os_saves_YMM = 0;
    if (os_uses_XSAVE_XRSTORE)
    {
      // Check if the OS will save the YMM registers
      // _XCR_XFEATURE_ENABLED_MASK = 0
      unsigned long long xcr_feature_mask = get_xcr_feature_mask();
      os_saves_YMM = (xcr_feature_mask & 0x6) != 0;
    }

    int cpu_FMA_support = (cpu_info[2] & ((int)1 << 12)) != 0;

    bool avx2_supported = false;
    if (ids_count >= 0x00000007)
    {
      cpuid(cpu_info, 0x00000007);

      int cpu_AVX2_support = (cpu_info[1] & (1 << 5)) != 0;
      // use fma in conjunction with avx2 support (like microsoft compiler does)
      avx2_supported = os_saves_YMM && cpu_AVX2_support && cpu_FMA_support;
    }

    if (avx2_supported)
      return MaskedOcclusionCulling::Implementation::AVX2;
    else if (sse41_supported)
      return MaskedOcclusionCulling::Implementation::SSE41;
  }

  return MaskedOcclusionCulling::Implementation::SSE2;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Object construction and allocation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace masked_occlusion_culling_neon
{
typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;

MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);
} // namespace masked_occlusion_culling_neon

namespace masked_occlusion_culling_sse2_or_41
{
typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;

MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);
} // namespace masked_occlusion_culling_sse2_or_41

namespace masked_occlusion_culling_avx2
{
typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;

MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);
} // namespace masked_occlusion_culling_avx2

MaskedOcclusionCulling *MaskedOcclusionCulling::Create()
{
  auto aligned_alloc = [](size_t alignment, size_t size) { return defaultmem->allocAligned(size, alignment); };

  auto aligned_free = [](void *p) { defaultmem->freeAligned(p); };

#if _TARGET_SIMD_NEON
  return masked_occlusion_culling_neon::CreateMaskedOcclusionCulling(aligned_alloc, aligned_free);
#elif defined(__APPLE__) || !defined(__x86_64__) || _TARGET_C1 || _TARGET_XBOXONE || _TARGET_SIMD_SSE < 4
  return masked_occlusion_culling_sse2_or_41::CreateMaskedOcclusionCulling(aligned_alloc, aligned_free);
#elif _TARGET_C2 || _TARGET_SCARLETT
  return masked_occlusion_culling_avx2::CreateMaskedOcclusionCulling(aligned_alloc, aligned_free);
#else
  const Implementation supportedImplementation = DetectCPUFeatures();
  if (supportedImplementation == Implementation::AVX2)
  {
    return masked_occlusion_culling_avx2::CreateMaskedOcclusionCulling(aligned_alloc, aligned_free);
  }
  else
  {
    return masked_occlusion_culling_sse2_or_41::CreateMaskedOcclusionCulling(aligned_alloc, aligned_free);
  }
#endif
}

#ifndef MOC_SINGLE_IMPLEMENTATION
void MaskedOcclusionCulling::Destroy(MaskedOcclusionCulling *moc)
{
  if (!moc)
    return;
  pfnAlignedFree alignedFreeCallback = moc->mAlignedFreeCallback;
  moc->~MaskedOcclusionCulling();
  alignedFreeCallback(moc);
}
#endif