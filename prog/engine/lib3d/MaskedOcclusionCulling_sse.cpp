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

#define DEPTH_VMIN occlusion_depth_vmin

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
// Common SSE2 defines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES        4
#define TILE_HEIGHT_SHIFT 2

#define SIMD_LANE_IDX _mm_setr_epi32(0, 1, 2, 3)

#define SIMD_SUB_TILE_COL_OFFSET   v_make_vec4i(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET   _mmw_setzero_epi32()
#define SIMD_SUB_TILE_COL_OFFSET_F v_make_vec4f(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F v_zero()

#define SIMD_LANE_YCOORD_I v_make_vec4i(128, 384, 640, 896)
#define SIMD_LANE_YCOORD_F v_make_vec4f(128.0f, 384.0f, 640.0f, 896.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common SSE2 functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m128 __mw;
typedef __m128i __mwi;

#define _mmw_set1_ps               _mm_set1_ps
#define _mmw_setzero_ps            _mm_setzero_ps
#define _mmw_and_ps                _mm_and_ps
#define _mmw_or_ps                 _mm_or_ps
#define _mmw_xor_ps                _mm_xor_ps
#define _mmw_not_ps(a)             _mm_xor_ps((a), _mm_castsi128_ps(_mm_set1_epi32(~0)))
#define _mmw_andnot_ps             _mm_andnot_ps
#define _mmw_neg_ps(a)             _mm_xor_ps((a), _mm_set1_ps(-0.0f))
#define _mmw_abs_ps(a)             _mm_and_ps((a), _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps                _mm_add_ps
#define _mmw_sub_ps                _mm_sub_ps
#define _mmw_mul_ps                _mm_mul_ps
#define _mmw_div_ps                _mm_div_ps
#define _mmw_min_ps                _mm_min_ps
#define _mmw_max_ps                _mm_max_ps
#define _mmw_movemask_ps           _mm_movemask_ps
#define _mmw_cmpge_ps(a, b)        _mm_cmpge_ps(a, b)
#define _mmw_cmpgt_ps(a, b)        _mm_cmpgt_ps(a, b)
#define _mmw_cmpeq_ps(a, b)        _mm_cmpeq_ps(a, b)
#define _mmw_fmadd_ps(a, b, c)     _mm_add_ps(_mm_mul_ps(a, b), c)
#define _mmw_fmsub_ps(a, b, c)     _mm_sub_ps(_mm_mul_ps(a, b), c)
#define _mmw_shuffle_ps_1010(a, b) _mm_shuffle_ps(a, b, 0x44)
#define _mmw_shuffle_ps_3232(a, b) _mm_shuffle_ps(a, b, 0xee)
#define _mmw_shuffle_ps_2020(a, b) _mm_shuffle_ps(a, b, 0x88)
#define _mmw_shuffle_ps_3131(a, b) _mm_shuffle_ps(a, b, 0xdd)

#define _mmw_shuffle_ps_1100(a, b) _mm_shuffle_ps(a, b, 0x50)
#define _mmw_shuffle_ps_3322(a, b) _mm_shuffle_ps(a, b, 0xfa)

#define _mmw_insertf32x4_ps(a, b, c) (b)
#define _mmw_cvtepi32_ps             _mm_cvtepi32_ps
#define _mmw_blendv_epi32(a, b, c)   simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32    _mm_set1_epi32
#define _mmw_setzero_epi32 _mm_setzero_si128
#define _mmw_and_epi32     _mm_and_si128
#define _mmw_or_epi32      _mm_or_si128
#define _mmw_xor_epi32     _mm_xor_si128
#define _mmw_not_epi32(a)  _mm_xor_si128((a), _mm_set1_epi32(~0))
#define _mmw_andnot_epi32  _mm_andnot_si128
#define _mmw_neg_epi32(a)  _mm_sub_epi32(_mm_set1_epi32(0), (a))
#define _mmw_add_epi32     _mm_add_epi32
#define _mmw_sub_epi32     _mm_sub_epi32
#define _mmw_subs_epu16    _mm_subs_epu16
#define _mmw_cmpeq_epi32   _mm_cmpeq_epi32
#define _mmw_cmpgt_epi32   _mm_cmpgt_epi32
#define _mmw_srai_epi32    _mm_srai_epi32
#define _mmw_srli_epi32    _mm_srli_epi32
#define _mmw_slli_epi32    _mm_slli_epi32
#define _mmw_cvtps_epi32   _mm_cvtps_epi32
#define _mmw_cvttps_epi32  _mm_cvttps_epi32

#define _mmx_fmadd_ps  _mmw_fmadd_ps
#define _mmx_max_epi32 _mmw_max_epi32
#define _mmx_min_epi32 _mmw_min_epi32

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized SSE input assembly function for general vertex gather
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if MASKED_TARGET_SIMD_SSE < 4
static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::SSE2;

FORCE_INLINE __m128i _mmw_mullo_epi32(const __m128i &a, const __m128i &b)
{
  // Do products for even / odd lanes & merge the result
  __m128i even = _mm_and_si128(_mm_mul_epu32(a, b), _mm_setr_epi32(~0, 0, ~0, 0));
  __m128i odd = _mm_slli_epi64(_mm_mul_epu32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32)), 32);
  return _mm_or_si128(even, odd);
}
FORCE_INLINE __m128i _mmw_min_epi32(const __m128i &a, const __m128i &b)
{
  __m128i cond = _mm_cmpgt_epi32(a, b);
  return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
}
FORCE_INLINE __m128i _mmw_max_epi32(const __m128i &a, const __m128i &b)
{
  __m128i cond = _mm_cmpgt_epi32(b, a);
  return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
}
FORCE_INLINE __m128i _mmw_abs_epi32(const __m128i &a)
{
  __m128i mask = _mm_cmplt_epi32(a, _mm_setzero_si128());
  return _mm_add_epi32(_mm_xor_si128(a, mask), _mm_srli_epi32(mask, 31));
}
FORCE_INLINE int _mmw_testz_epi32(const __m128i &a, const __m128i &b)
{
  return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(a, b), _mm_setzero_si128())) == 0xFFFF;
}
FORCE_INLINE __m128 _mmw_blendv_ps(const __m128 &a, const __m128 &b, const __m128 &c)
{
  __m128 cond = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(c), 31));
  return _mm_or_ps(_mm_andnot_ps(cond, a), _mm_and_ps(cond, b));
}
FORCE_INLINE __m128 _mmx_dp4_ps(const __m128 &a, const __m128 &b)
{
  // Product and two shuffle/adds pairs (similar to hadd_ps)
  __m128 prod = _mm_mul_ps(a, b);
  __m128 dp = _mm_add_ps(prod, _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1)));
  dp = _mm_add_ps(dp, _mm_shuffle_ps(dp, dp, _MM_SHUFFLE(0, 1, 2, 3)));
  return dp;
}
FORCE_INLINE __m128 _mmw_floor_ps(const __m128 &a)
{
  int originalMode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
  __m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
  _MM_SET_ROUNDING_MODE(originalMode);
  return rounded;
}
FORCE_INLINE __m128 _mmw_ceil_ps(const __m128 &a)
{
  int originalMode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
  __m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
  _MM_SET_ROUNDING_MODE(originalMode);
  return rounded;
}
FORCE_INLINE __m128i _mmw_transpose_epi8(const __m128i &a)
{
  // Perform transpose through two 16->8 bit pack and byte shifts
  __m128i res = a;
  const __m128i mask = _mm_setr_epi8(~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0);
  res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
  res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
  return res;
}
FORCE_INLINE __m128i _mmw_sllv_ones(const __m128i &ishift)
{
  __m128i shift = _mmw_min_epi32(ishift, _mm_set1_epi32(32));

  // Uses scalar approach to perform _mm_sllv_epi32(~0, shift)
  static const unsigned int maskLUT[33] = {~0U << 0, ~0U << 1, ~0U << 2, ~0U << 3, ~0U << 4, ~0U << 5, ~0U << 6, ~0U << 7, ~0U << 8,
    ~0U << 9, ~0U << 10, ~0U << 11, ~0U << 12, ~0U << 13, ~0U << 14, ~0U << 15, ~0U << 16, ~0U << 17, ~0U << 18, ~0U << 19, ~0U << 20,
    ~0U << 21, ~0U << 22, ~0U << 23, ~0U << 24, ~0U << 25, ~0U << 26, ~0U << 27, ~0U << 28, ~0U << 29, ~0U << 30, ~0U << 31, 0U};

  __m128i retMask;
  simd_i32(retMask)[0] = (int)maskLUT[simd_i32(shift)[0]];
  simd_i32(retMask)[1] = (int)maskLUT[simd_i32(shift)[1]];
  simd_i32(retMask)[2] = (int)maskLUT[simd_i32(shift)[2]];
  simd_i32(retMask)[3] = (int)maskLUT[simd_i32(shift)[3]];
  return retMask;
}

#else // MASKED_TARGET_SIMD_SSE < 4
static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::SSE41;

#define _mmw_mullo_epi32(a, b)  _mm_mullo_epi32(a, b)
#define _mmw_min_epi32(a, b)    _mm_min_epi32(a, b)
#define _mmw_max_epi32(a, b)    _mm_max_epi32(a, b)
#define _mmw_abs_epi32(a)       _mm_abs_epi32(a)
#define _mmw_blendv_ps(a, b, c) _mm_blendv_ps(a, b, c)
#define _mmw_testz_epi32(a, b)  _mm_testz_si128(a, b)
#define _mmx_dp4_ps(a, b)       _mm_dp_ps(a, b, 0xFF)
#define _mmw_floor_ps(a)        _mm_round_ps(a, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)
#define _mmw_ceil_ps(a)         _mm_round_ps(a, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)
FORCE_INLINE __m128i _mmw_transpose_epi8(const __m128i &a)
{
  const __m128i shuff = _mm_setr_epi8(0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF);
  return _mm_shuffle_epi8(a, shuff);
}
FORCE_INLINE __m128i _mmw_sllv_ones(const __m128i &ishift)
{
  __m128i shift = _mm_min_epi32(ishift, _mm_set1_epi32(32));

  // Uses lookup tables and _mm_shuffle_epi8 to perform _mm_sllv_epi32(~0, shift)
  const __m128i byteShiftLUT = _mm_setr_epi8((char)0xFF, (char)0xFE, (char)0xFC, (char)0xF8, (char)0xF0, (char)0xE0, (char)0xC0,
    (char)0x80, 0, 0, 0, 0, 0, 0, 0, 0);
  const __m128i byteShiftOffset = _mm_setr_epi8(0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24);
  const __m128i byteShiftShuffle = _mm_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x4, 0x4, 0x4, 0x4, 0x8, 0x8, 0x8, 0x8, 0xC, 0xC, 0xC, 0xC);

  __m128i byteShift = _mm_shuffle_epi8(shift, byteShiftShuffle);
  byteShift = _mm_min_epi8(_mm_subs_epu8(byteShift, byteShiftOffset), _mm_set1_epi8(8));
  __m128i retMask = _mm_shuffle_epi8(byteShiftLUT, byteShift);

  return retMask;
}

#endif // MASKED_TARGET_SIMD_SSE < 4

namespace masked_occlusion_culling_sse2_or_41
{
#undef FP_BITS
#include "MaskedOcclusionCullingCommon.inl"

typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;

MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
{
  auto *object = (MaskedOcclusionCullingPrivate *)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
  new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
  return object;
}
} // namespace masked_occlusion_culling_sse2_or_41

#ifdef MOC_SINGLE_IMPLEMENTATION

#define MOC_STATIC_UPCAST(x)       static_cast<masked_occlusion_culling_sse2_or_41::MaskedOcclusionCullingPrivate *>(x)
#define MOC_STATIC_UPCAST_CONST(x) static_cast<const masked_occlusion_culling_sse2_or_41::MaskedOcclusionCullingPrivate *>(x)

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