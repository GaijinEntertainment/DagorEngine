////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////
#include <debug/dag_debug.h>
#include <vecmath/dag_vecMath.h>
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include <math/dag_declAlign.h>
#include <math/dag_occlusionTest.h>

#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#if OCCLUSION_BUFFER == OCCLUSION_Z_BUFFER
#define DEPTH_VMIN _mmw_max_ps
#elif OCCLUSION_BUFFER == OCCLUSION_WBUFFER
#define DEPTH_VMIN _mmw_min_ps
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
#define DEPTH_VMIN _mmw_max_ps
#endif

#if OCCLUSION_BUFFER == OCCLUSION_WBUFFER
#define CONVERT_MASKED_TO_DEPTH(a) v_rcp(a)
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
#define CONVERT_MASKED_TO_DEPTH(a) (a)
#else
#error unselected occlusion type
#endif

#include <3d/dag_maskedOcclusionCulling.h>
#if DAGOR_DBGLEVEL > 0
#define OCCLUSION_ASSERT assert
#else
#define OCCLUSION_ASSERT(COND) ((void)0)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compiler specific functions: currently only MSC and Intel compiler should work.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FORCE_INLINE __forceinline

static FORCE_INLINE unsigned long find_clear_lsb(unsigned int *mask)
{
  // unsigned long idx;
  //_BitScanForward(&idx, *mask);
  unsigned long idx = __bsf(*mask);
  *mask &= *mask - 1;
  return idx;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common AVX2 defines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES        8
#define TILE_HEIGHT_SHIFT 3

#define SIMD_LANE_IDX _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)

#define SIMD_SUB_TILE_COL_OFFSET                                                                                      \
  _mm256_setr_epi32(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, \
    SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET _mm256_setr_epi32(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT)
#define SIMD_SUB_TILE_COL_OFFSET_F \
  _mm256_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F _mm256_setr_ps(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT)

#define SIMD_SHUFFLE_SCANLINE_TO_SUBTILES                                                                                             \
  _mm256_setr_epi8(0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF, 0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, \
    0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF)

#define SIMD_LANE_YCOORD_I _mm256_setr_epi32(128, 384, 640, 896, 1152, 1408, 1664, 1920)
#define SIMD_LANE_YCOORD_F _mm256_setr_ps(128.0f, 384.0f, 640.0f, 896.0f, 1152.0f, 1408.0f, 1664.0f, 1920.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common AVX2 functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m256 __mw;
typedef __m256i __mwi;

#define _mmw_set1_ps               _mm256_set1_ps
#define _mmw_setzero_ps            _mm256_setzero_ps
#define _mmw_and_ps                _mm256_and_ps
#define _mmw_or_ps                 _mm256_or_ps
#define _mmw_xor_ps                _mm256_xor_ps
#define _mmw_not_ps(a)             _mm256_xor_ps((a), _mm256_castsi256_ps(_mm256_set1_epi32(~0)))
#define _mmw_andnot_ps             _mm256_andnot_ps
#define _mmw_neg_ps(a)             _mm256_xor_ps((a), _mm256_set1_ps(-0.0f))
#define _mmw_abs_ps(a)             _mm256_and_ps((a), _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps                _mm256_add_ps
#define _mmw_sub_ps                _mm256_sub_ps
#define _mmw_mul_ps                _mm256_mul_ps
#define _mmw_div_ps                _mm256_div_ps
#define _mmw_min_ps                _mm256_min_ps
#define _mmw_max_ps                _mm256_max_ps
#define _mmw_fmadd_ps              _mm256_fmadd_ps
#define _mmw_fmsub_ps              _mm256_fmsub_ps
#define _mmw_movemask_ps           _mm256_movemask_ps
#define _mmw_blendv_ps             _mm256_blendv_ps
#define _mmw_cmpge_ps(a, b)        _mm256_cmp_ps(a, b, _CMP_GE_OQ)
#define _mmw_cmpgt_ps(a, b)        _mm256_cmp_ps(a, b, _CMP_GT_OQ)
#define _mmw_cmpeq_ps(a, b)        _mm256_cmp_ps(a, b, _CMP_EQ_OQ)
#define _mmw_floor_ps(x)           _mm256_round_ps(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)
#define _mmw_ceil_ps(x)            _mm256_round_ps(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)
#define _mmw_shuffle_ps_1010(a, b) _mm256_shuffle_ps(a, b, 0x44)
#define _mmw_shuffle_ps_3232(a, b) _mm256_shuffle_ps(a, b, 0xee)
#define _mmw_shuffle_ps_2020(a, b) _mm256_shuffle_ps(a, b, 0x88)
#define _mmw_shuffle_ps_3131(a, b) _mm256_shuffle_ps(a, b, 0xdd)

#define _mmw_shuffle_ps(a, b, imm) _mm256_shuffle_ps(a, b, imm)

#define _mmw_insertf32x4_ps        _mm256_insertf128_ps
#define _mmw_cvtepi32_ps           _mm256_cvtepi32_ps
#define _mmw_blendv_epi32(a, b, c) simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32        _mm256_set1_epi32
#define _mmw_setzero_epi32     _mm256_setzero_si256
#define _mmw_and_epi32         _mm256_and_si256
#define _mmw_or_epi32          _mm256_or_si256
#define _mmw_xor_epi32         _mm256_xor_si256
#define _mmw_not_epi32(a)      _mm256_xor_si256((a), _mm256_set1_epi32(~0))
#define _mmw_andnot_epi32      _mm256_andnot_si256
#define _mmw_neg_epi32(a)      _mm256_sub_epi32(_mm256_set1_epi32(0), (a))
#define _mmw_add_epi32         _mm256_add_epi32
#define _mmw_sub_epi32         _mm256_sub_epi32
#define _mmw_min_epi32         _mm256_min_epi32
#define _mmw_max_epi32         _mm256_max_epi32
#define _mmw_subs_epu16        _mm256_subs_epu16
#define _mmw_mullo_epi32       _mm256_mullo_epi32
#define _mmw_cmpeq_epi32       _mm256_cmpeq_epi32
#define _mmw_testz_epi32       _mm256_testz_si256
#define _mmw_cmpgt_epi32       _mm256_cmpgt_epi32
#define _mmw_srai_epi32        _mm256_srai_epi32
#define _mmw_srli_epi32        _mm256_srli_epi32
#define _mmw_slli_epi32        _mm256_slli_epi32
#define _mmw_sllv_ones(x)      _mm256_sllv_epi32(SIMD_BITS_ONE, x)
#define _mmw_transpose_epi8(x) _mm256_shuffle_epi8(x, SIMD_SHUFFLE_SCANLINE_TO_SUBTILES)
#define _mmw_abs_epi32         _mm256_abs_epi32
#define _mmw_cvtps_epi32       _mm256_cvtps_epi32
#define _mmw_cvttps_epi32      _mm256_cvttps_epi32

#define _mmx_dp4_ps(a, b) _mm_dp_ps(a, b, 0xFF)
#define _mmx_fmadd_ps     _mm_fmadd_ps
#define _mmx_max_epi32    _mm_max_epi32
#define _mmx_min_epi32    _mm_min_epi32

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Y>
FORCE_INLINE T simd_cast(Y A);
template <>
FORCE_INLINE __m128 simd_cast<__m128>(float A)
{
  return _mm_set1_ps(A);
}
template <>
FORCE_INLINE __m128 simd_cast<__m128>(__m128i A)
{
  return _mm_castsi128_ps(A);
}
template <>
FORCE_INLINE __m128 simd_cast<__m128>(__m128 A)
{
  return A;
}
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(int A)
{
  return _mm_set1_epi32(A);
}
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(__m128 A)
{
  return _mm_castps_si128(A);
}
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(__m128i A)
{
  return A;
}
template <>
FORCE_INLINE __m256 simd_cast<__m256>(float A)
{
  return _mm256_set1_ps(A);
}
template <>
FORCE_INLINE __m256 simd_cast<__m256>(__m256i A)
{
  return _mm256_castsi256_ps(A);
}
template <>
FORCE_INLINE __m256 simd_cast<__m256>(__m256 A)
{
  return A;
}
template <>
FORCE_INLINE __m256i simd_cast<__m256i>(int A)
{
  return _mm256_set1_epi32(A);
}
template <>
FORCE_INLINE __m256i simd_cast<__m256i>(__m256 A)
{
  return _mm256_castps_si256(A);
}
template <>
FORCE_INLINE __m256i simd_cast<__m256i>(__m256i A)
{
  return A;
}

#define MAKE_ACCESSOR(name, simd_type, base_type, is_const, elements)   \
  FORCE_INLINE is_const base_type *name(is_const simd_type &a)          \
  {                                                                     \
    union accessor                                                      \
    {                                                                   \
      simd_type m_native;                                               \
      base_type m_array[elements];                                      \
    };                                                                  \
    is_const accessor *acs = reinterpret_cast<is_const accessor *>(&a); \
    return acs->m_array;                                                \
  }

MAKE_ACCESSOR(simd_f32, __m128, float, , 4)
MAKE_ACCESSOR(simd_f32, __m128, float, const, 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, , 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, const, 4)

MAKE_ACCESSOR(simd_f32, __m256, float, , 8)
MAKE_ACCESSOR(simd_f32, __m256, float, const, 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, , 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, const, 8)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized AVX input assembly function for general vertex gather
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::AVX2;

namespace masked_occlusion_culling_avx2
{
#undef FP_BITS
#include "../MaskedOcclusionCullingCommon.inl"

typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;

MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
{
  auto *object = (MaskedOcclusionCullingPrivate *)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
  new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
  return object;
}
} // namespace masked_occlusion_culling_avx2

unsigned long long get_xcr_feature_mask() { return _xgetbv(0); }

#ifdef MOC_SINGLE_IMPLEMENTATION

#define MOC_STATIC_UPCAST(x)       static_cast<masked_occlusion_culling_avx2::MaskedOcclusionCullingPrivate *>(x)
#define MOC_STATIC_UPCAST_CONST(x) static_cast<const masked_occlusion_culling_avx2::MaskedOcclusionCullingPrivate *>(x)

void MaskedOcclusionCulling::SetResolution(unsigned int width, unsigned int height)
{
  MOC_STATIC_UPCAST(this)->SetResolution(width, height);
}

void MaskedOcclusionCulling::GetResolution(unsigned int &width, unsigned int &height) const
{
  MOC_STATIC_UPCAST_CONST(this)->GetResolution(width, height);
}

void MaskedOcclusionCulling::ComputeBinWidthHeight(unsigned int nBinsW, unsigned int nBinsH, unsigned int &outBinWidth,
  unsigned int &outBinHeight)
{
  MOC_STATIC_UPCAST(this)->ComputeBinWidthHeight(nBinsW, nBinsH, outBinWidth, outBinHeight);
}

void MaskedOcclusionCulling::SetNearClipPlane(float nearDist) { MOC_STATIC_UPCAST(this)->SetNearClipPlane(nearDist); }

float MaskedOcclusionCulling::GetNearClipPlane() const { return MOC_STATIC_UPCAST_CONST(this)->GetNearClipPlane(); }

void MaskedOcclusionCulling::ClearBuffer() { MOC_STATIC_UPCAST(this)->ClearBuffer(); }

void MaskedOcclusionCulling::MergeBuffer(MaskedOcclusionCulling *BufferB) { MOC_STATIC_UPCAST(this)->MergeBuffer(BufferB); }

MaskedOcclusionCulling::CullingResult MaskedOcclusionCulling::RenderTriangles(const float *inVtx, const unsigned short *inTris,
  int nTris, const float *modelToClipMatrix, BackfaceWinding bfWinding, ClipPlanes clipPlaneMask)
{
  return MOC_STATIC_UPCAST(this)->RenderTriangles(inVtx, inTris, nTris, modelToClipMatrix, bfWinding, clipPlaneMask);
}

MaskedOcclusionCulling::CullingResult MaskedOcclusionCulling::TestRect(float xmin, float ymin, float xmax, float ymax,
  float wmin) const
{
  return MOC_STATIC_UPCAST_CONST(this)->TestRect(xmin, ymin, xmax, ymax, wmin);
}

MaskedOcclusionCulling::CullingResult MaskedOcclusionCulling::TestTriangles(const float *inVtx, const unsigned short *inTris,
  int nTris, const float *modelToClipMatrix, BackfaceWinding bfWinding, ClipPlanes clipPlaneMask)
{
  return MOC_STATIC_UPCAST(this)->TestTriangles(inVtx, inTris, nTris, modelToClipMatrix, bfWinding, clipPlaneMask);
}

void MaskedOcclusionCulling::BinTriangles(const float *inVtx, const unsigned short *inTris, int nTris, TriList *triLists,
  unsigned int nBinsW, unsigned int nBinsH, const float *modelToClipMatrix, BackfaceWinding bfWinding, ClipPlanes clipPlaneMask)
{
  MOC_STATIC_UPCAST(this)->BinTriangles(inVtx, inTris, nTris, triLists, nBinsW, nBinsH, modelToClipMatrix, bfWinding, clipPlaneMask);
}

void MaskedOcclusionCulling::RenderTrilist(const TriList &triList, const ScissorRect *scissor)
{
  MOC_STATIC_UPCAST(this)->RenderTrilist(triList, scissor);
}

void MaskedOcclusionCulling::ComputePixelDepthBuffer(float *depthData, bool flipY)
{
  MOC_STATIC_UPCAST(this)->ComputePixelDepthBuffer(depthData, flipY);
}

MaskedOcclusionCulling::OcclusionCullingStatistics MaskedOcclusionCulling::GetStatistics()
{
  return MOC_STATIC_UPCAST(this)->GetStatistics();
}

MaskedOcclusionCulling::Implementation MaskedOcclusionCulling::GetImplementation()
{
  return MOC_STATIC_UPCAST(this)->GetImplementation();
}

void MaskedOcclusionCulling::CombinePixelDepthBuffer2W(float *depthData, int w, int h)
{
  MOC_STATIC_UPCAST(this)->CombinePixelDepthBuffer2W(depthData, w, h);
}

void MaskedOcclusionCulling::DecodePixelDepthBuffer2W(float *depthData, int w, int h)
{
  MOC_STATIC_UPCAST(this)->DecodePixelDepthBuffer2W(depthData, w, h);
}

void MaskedOcclusionCulling::mergeOcclusions(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
  uint32_t last_tile)
{
  MOC_STATIC_UPCAST(this)->mergeOcclusions(another_occl, occl_count, first_tile, last_tile);
}

void MaskedOcclusionCulling::mergeOcclusionsZmin(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
  uint32_t last_tile)
{
  MOC_STATIC_UPCAST(this)->mergeOcclusionsZmin(another_occl, occl_count, first_tile, last_tile);
}

uint32_t MaskedOcclusionCulling::getTilesCount() const { return MOC_STATIC_UPCAST_CONST(this)->getTilesCount(); }

void MaskedOcclusionCulling::Destroy(MaskedOcclusionCulling *moc)
{
  if (!moc)
    return;
  pfnAlignedFree alignedFreeCallback = moc->mAlignedFreeCallback;
  MOC_STATIC_UPCAST(moc)->~MaskedOcclusionCullingPrivate();
  alignedFreeCallback(moc);
}

#endif