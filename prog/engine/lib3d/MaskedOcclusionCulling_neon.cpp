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
// Common NEON defines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES        4
#define TILE_HEIGHT_SHIFT 2

#define SIMD_LANE_IDX \
  (int32x4_t)         \
  {                   \
    0, 1, 2, 3        \
  }

#define SIMD_SUB_TILE_COL_OFFSET   v_make_vec4i(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET   vdupq_n_s32(0)
#define SIMD_SUB_TILE_COL_OFFSET_F v_make_vec4f(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F v_zero()

#define SIMD_LANE_YCOORD_I v_make_vec4i(128, 384, 640, 896)
#define SIMD_LANE_YCOORD_F v_make_vec4f(128.0f, 384.0f, 640.0f, 896.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common NEON functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef float32x4_t __mw;
typedef int32x4_t __mwi;

#define _mmw_set1_ps         vdupq_n_f32
#define _mmw_setzero_ps()    vdupq_n_f32(0)
#define _mmw_and_ps(a, b)    vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)))
#define _mmw_or_ps(a, b)     vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)))
#define _mmw_xor_ps(a, b)    vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)))
#define _mmw_not_ps(a)       _mm_xor_ps((a), _mm_castsi128_ps(_mm_set1_epi32(~0)))
#define _mmw_andnot_ps(a, b) vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(b), vreinterpretq_s32_f32(a)))
#define _mmw_neg_ps(a)       _mm_xor_ps((a), _mm_set1_ps(-0.0f))
#define _mmw_abs_ps(a)       _mm_and_ps((a), _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps          vaddq_f32
#define _mmw_sub_ps          vsubq_f32
#define _mmw_mul_ps          vmulq_f32
#define _mmw_div_ps          vdivq_f32
#define _mmw_min_ps          vminq_f32
#define _mmw_max_ps          vmaxq_f32

FORCE_INLINE int _mmw_movemask_ps(float32x4_t a)
{
  uint32x4_t input = vreinterpretq_u32_f32(a);
#if defined(__aarch64__)
  static const int32x4_t shift = {0, 1, 2, 3};
  uint32x4_t tmp = vshrq_n_u32(input, 31);
  return vaddvq_u32(vshlq_u32(tmp, shift));
#else
  // Uses the exact same method as _mm_movemask_epi8, see that for details.
  // Shift out everything but the sign bits with a 32-bit unsigned shift
  // right.
  uint64x2_t high_bits = vreinterpretq_u64_u32(vshrq_n_u32(input, 31));
  // Merge the two pairs together with a 64-bit unsigned shift right + add.
  uint8x16_t paired = vreinterpretq_u8_u64(vsraq_n_u64(high_bits, high_bits, 31));
  // Extract the result.
  return vgetq_lane_u8(paired, 0) | (vgetq_lane_u8(paired, 8) << 2);
#endif
}

#define _mmw_cmpge_ps          (int32x4_t) vcgeq_f32
#define _mmw_cmpgt_ps          vcgtq_f32
#define _mmw_cmpeq_ps          vceqq_f32
#define _mmw_fmadd_ps(a, b, c) vaddq_f32(vmulq_f32(a, b), c)
#define _mmw_fmsub_ps(a, b, c) vsubq_f32(vmulq_f32(a, b), c)

template <int i>
FORCE_INLINE float32x4_t _mm_shuffle_ps(float32x4_t a, float32x4_t b)
{
  float32x4_t ret = vmovq_n_f32(vgetq_lane_f32(a, i & 0x3));
  ret = vsetq_lane_f32(vgetq_lane_f32(a, (i >> 2) & 0x3), ret, 1);
  ret = vsetq_lane_f32(vgetq_lane_f32(b, (i >> 4) & 0x3), ret, 2);
  ret = vsetq_lane_f32(vgetq_lane_f32(b, (i >> 6) & 0x3), ret, 3);
  return ret;
}

#define _mmw_shuffle_ps_1010(a, b) _mm_shuffle_ps<0x44>(a, b)
#define _mmw_shuffle_ps_3232(a, b) _mm_shuffle_ps<0xee>(a, b)
#define _mmw_shuffle_ps_2020(a, b) _mm_shuffle_ps<0x88>(a, b)
#define _mmw_shuffle_ps_3131(a, b) _mm_shuffle_ps<0xdd>(a, b)

#define _mmw_shuffle_ps_1100(a, b) vcombine_f32(vdup_n_f32(vgetq_lane_f32(a, 1)), vdup_n_f32(vgetq_lane_f32(b, 0)))
#define _mmw_shuffle_ps_3322(a, b) vcombine_f32(vdup_n_f32(vgetq_lane_f32(a, 3)), vdup_n_f32(vgetq_lane_f32(b, 2)))


#define _mmw_insertf32x4_ps(a, b, c) (b)
#define _mmw_cvtepi32_ps             _mm_cvtepi32_ps
#define _mmw_blendv_epi32(a, b, c)   simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32         vdupq_n_s32
#define _mmw_setzero_epi32()    vdupq_n_s32(0)
#define _mmw_and_epi32          vandq_s32
#define _mmw_or_epi32           vorrq_s32
#define _mmw_xor_epi32          veorq_s32
#define _mmw_not_epi32(a)       veorq_s32((a), vdupq_n_s32(~0))
#define _mmw_andnot_epi32(a, b) vbicq_s32(b, a) // Arguments must be swapped!
#define _mmw_neg_epi32(a)       vsubq_s32(vdupq_n_s32(0), (a))
#define _mmw_add_epi32          vaddq_s32
#define _mmw_sub_epi32          vsubq_s32
#define _mmw_subs_epu16(a, b)   vreinterpretq_s32_u16(vqsubq_u16(vreinterpretq_u16_s32(a), vreinterpretq_u16_s32(b)))
#define _mmw_cmpeq_epi32        (int32x4_t)(vceqq_s32)
#define _mmw_cmpgt_epi32        vcgtq_s32
FORCE_INLINE int32x4_t _mmw_srai_epi32(int32x4_t a, int imm)
{
  int32x4_t ret;
  if (0 < (imm) && (imm) < 32)
  {
    ret = vshlq_s32(a, vdupq_n_s32(-imm));
  }
  else
  {
    ret = vshrq_n_s32(a, 31);
  }
  return ret;
}

FORCE_INLINE int32x4_t _mmw_srli_epi32(int32x4_t a, int imm)
{
  int32x4_t ret;
  if ((imm) & ~31)
  {
    ret = vdupq_n_s32(0);
  }
  else
  {
    ret = vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(-(imm))));
  }
  return ret;
}
#define _mmw_slli_epi32   _mm_slli_epi32
#define _mmw_cvtps_epi32  _mm_cvtps_epi32
#define _mmw_cvttps_epi32 _mm_cvttps_epi32

#define _mmx_fmadd_ps  _mmw_fmadd_ps
#define _mmx_max_epi32 _mmw_max_epi32
#define _mmx_min_epi32 _mmw_min_epi32

/////

FORCE_INLINE float32x4_t _mmw_setr_ps(float w, float z, float y, float x)
{
  float __attribute__((aligned(16))) data[4] = {w, z, y, x};
  return vld1q_f32(data);
}

typedef float32x4_t __m128;
typedef int32x4_t __m128i;

#define _mm_setzero_si128() vdupq_n_s32(0)
#define _mm_setzero_ps      _mmw_setzero_ps
#define _mm_set1_ps         _mmw_set1_ps
#define _mm_set1_epi32      _mmw_set1_epi32

#define _mm_setr_ps    _mmw_setr_ps
#define _mm_setr_epi32 _mmw_setr_epi32

#define _mm_loadu_ps vld1q_f32

#define _mm_movemask_ps _mmw_movemask_ps
#define _mm_xor_ps      _mmw_xor_ps

#define _mm_sub_ps _mmw_sub_ps
#define _mm_div_ps _mmw_div_ps

#define _mm_add_epi32 _mmw_add_epi32

#define _mm_and_si128(a, b) vandq_s32(a, b)

#define _mm_castsi128_ps vcvtq_f32_s32
#define _mm_cvttps_epi32 vcvtq_s32_f32
#define _mm_cvtepi32_ps  vcvtq_f32_s32

#define _mm_slli_epi32(a, imm) vshlq_n_s32(a, imm)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Y>
FORCE_INLINE T simd_cast(Y A);
template <>
FORCE_INLINE float32x4_t simd_cast<float32x4_t>(float A)
{
  return vdupq_n_f32(A);
}
template <>
FORCE_INLINE float32x4_t simd_cast<float32x4_t>(int32x4_t A)
{
  return vreinterpretq_f32_s32(A);
}
template <>
FORCE_INLINE float32x4_t simd_cast<float32x4_t>(float32x4_t A)
{
  return A;
}
template <>
FORCE_INLINE int32x4_t simd_cast<int32x4_t>(int A)
{
  return vdupq_n_s32(A);
}
template <>
FORCE_INLINE int32x4_t simd_cast<int32x4_t>(float32x4_t A)
{
  return vreinterpretq_s32_f32(A);
}
template <>
FORCE_INLINE int32x4_t simd_cast<int32x4_t>(int32x4_t A)
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

MAKE_ACCESSOR(simd_f32, float32x4_t, float, , 4)
MAKE_ACCESSOR(simd_f32, float32x4_t, float, const, 4)
MAKE_ACCESSOR(simd_i32, int32x4_t, int, , 4)
MAKE_ACCESSOR(simd_i32, int32x4_t, int, const, 4)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized SSE input assembly function for general vertex gather
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::NEON;

#define _mmw_mullo_epi32 vmulq_s32
#define _mmw_min_epi32   vminq_s32
#define _mmw_max_epi32   vmaxq_s32

FORCE_INLINE int _mmw_testz_epi32(const int32x4_t &a, const int32x4_t &b)
{
  int64x2_t s64 = vandq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b));
  return !(vgetq_lane_s64(s64, 0) | vgetq_lane_s64(s64, 1));
}
FORCE_INLINE float32x4_t _mmw_blendv_ps(const float32x4_t &a, const float32x4_t &b, const float32x4_t &c)
{
  // Use a signed shift right to create a mask with the sign bit
  uint32x4_t mask = vreinterpretq_u32_s32(vshrq_n_s32(vreinterpretq_s32_f32(c), 31));
  return vbslq_f32(mask, b, a);
}

FORCE_INLINE float32x4_t _mm_shuffle_ps_2301(float32x4_t a, float32x4_t b)
{
  float32x2_t a01 = vrev64_f32(vget_low_f32(a));
  float32x2_t b23 = vrev64_f32(vget_high_f32(b));
  return vcombine_f32(a01, b23);
}

FORCE_INLINE float32x4_t _mmx_dp4_ps(const float32x4_t &a, const float32x4_t &b)
{
  float32x4_t prod = vmulq_f32(a, b);
  float32x4_t dp = vaddq_f32(prod, _mm_shuffle_ps_2301(prod, prod));

  float32x4_t v = vrev64q_f32(dp);
  v = vextq_f32(v, v, 2);

  dp = vaddq_f32(dp, v);
  return dp;
}
#define _mmw_floor_ps vrndmq_f32
#define _mmw_ceil_ps  vrndpq_f32
FORCE_INLINE int32x4_t _mm_srli_epi16(int32x4_t a, int imm)
{
  int32x4_t ret;
  if ((imm) & ~15)
  {
    ret = vdupq_n_s32(0);
  }
  else
  {
    ret = vreinterpretq_s32_u16(vshlq_u16(vreinterpretq_u16_s32(a), vdupq_n_s16(-(imm))));
  }
  return ret;
}
FORCE_INLINE int32x4_t _mmw_transpose_epi8(int32x4_t v)
{
  // Perform transpose through two 16->8 bit pack and byte shifts
  int8_t __attribute__((aligned(16))) data[16] = {~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0};
  const int32x4_t mask = vld1q_s8(data);

#define _mm_packus_epi16(a, b) \
  vreinterpretq_s32_u8(vcombine_u8(vqmovun_s16(vreinterpretq_s16_s32(a)), vqmovun_s16(vreinterpretq_s16_s32(b))));

  v = _mm_packus_epi16(vandq_s32(v, mask), _mm_srli_epi16(v, 8));
  v = _mm_packus_epi16(vandq_s32(v, mask), _mm_srli_epi16(v, 8));

#undef _mm_packus_epi16
#undef _mm_srli_epi16

  return v;
}

FORCE_INLINE int32x4_t _mmw_setr_epi32(int32_t w, int32_t z, int32_t y, int32_t x)
{
  int32_t __attribute__((aligned(16))) data[4] = {w, z, y, x};
  return vld1q_s32(data);
}

FORCE_INLINE int32x4_t _mmw_sllv_ones(const int32x4_t &ishift)
{
  union
  {
    int32x4_t shift_128;
    uint32_t shift_32[4];
  } shift;

  shift.shift_128 = _mmw_min_epi32(ishift, _mmw_set1_epi32(32));

  // Uses scalar approach to perform _mm_sllv_epi32(~0, shift)
  static const unsigned int maskLUT[33] = {~0U << 0, ~0U << 1, ~0U << 2, ~0U << 3, ~0U << 4, ~0U << 5, ~0U << 6, ~0U << 7, ~0U << 8,
    ~0U << 9, ~0U << 10, ~0U << 11, ~0U << 12, ~0U << 13, ~0U << 14, ~0U << 15, ~0U << 16, ~0U << 17, ~0U << 18, ~0U << 19, ~0U << 20,
    ~0U << 21, ~0U << 22, ~0U << 23, ~0U << 24, ~0U << 25, ~0U << 26, ~0U << 27, ~0U << 28, ~0U << 29, ~0U << 30, ~0U << 31, 0U};

  int32x4_t retMask =
    _mmw_setr_epi32(maskLUT[shift.shift_32[0]], maskLUT[shift.shift_32[1]], maskLUT[shift.shift_32[2]], maskLUT[shift.shift_32[3]]);
  return retMask;
}

namespace masked_occlusion_culling_neon
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
} // namespace masked_occlusion_culling_neon

#ifdef MOC_SINGLE_IMPLEMENTATION

#define MOC_STATIC_UPCAST(x)       static_cast<masked_occlusion_culling_neon::MaskedOcclusionCullingPrivate *>(x)
#define MOC_STATIC_UPCAST_CONST(x) static_cast<const masked_occlusion_culling_neon::MaskedOcclusionCullingPrivate *>(x)

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