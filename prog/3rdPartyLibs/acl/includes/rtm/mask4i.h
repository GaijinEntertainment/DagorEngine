#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Realtime Math contributors
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
////////////////////////////////////////////////////////////////////////////////

#include "rtm/math.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/mask_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4i [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_x(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(input));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(RTM_IMPL_MASK4i_GET(input), 0);
#else
		return input.x;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4i [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_y(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(1, 1, 1, 1))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(RTM_IMPL_MASK4i_GET(input), 1);
#else
		return input.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4i [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_z(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(2, 2, 2, 2))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(RTM_IMPL_MASK4i_GET(input), 2);
#else
		return input.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4i [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_w(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(3, 3, 3, 3))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(RTM_IMPL_MASK4i_GET(input), 3);
#else
		return input.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are true, otherwise false: all(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input) == 0xFFFF;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(RTM_IMPL_MASK4i_GET(input));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) == 0xFFFFFFFFU;
#else
		return input.x != 0 && input.y != 0 && input.z != 0 && input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are true, otherwise false: all(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true2(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input) & 0x00FF) == 0x00FF;
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u64(vreinterpretq_u64_u32(RTM_IMPL_MASK4i_GET(input)), 0) == 0xFFFFFFFFFFFFFFFFULL;
#else
		return input.x != 0 && input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are true, otherwise false: all(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true3(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input) & 0x0FFF) == 0x0FFF;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(RTM_IMPL_MASK4i_GET(input));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return (vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) & 0x00FFFFFFU) == 0x00FFFFFFU;
#else
		return input.x != 0 && input.y != 0 && input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are true, otherwise false: any(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(RTM_IMPL_MASK4i_GET(input));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0 || input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are true, otherwise false: any(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true2(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input) & 0x00FF) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u64(vreinterpretq_u64_u32(RTM_IMPL_MASK4i_GET(input)), 0) != 0;
#else
		return input.x != 0 || input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are true, otherwise false: any(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true3(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input) & 0x0FFF) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(RTM_IMPL_MASK4i_GET(input));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return (vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) & 0x00FFFFFFU) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) == 0xFFFF;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(vceqq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) == 0xFFFFFFFFU;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal2(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) & 0x00FF) == 0x00FF;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x2_t mask = vceq_u32(vget_low_u32(RTM_IMPL_MASK4i_GET(lhs)), vget_low_u32(RTM_IMPL_MASK4i_GET(rhs)));
		return vget_lane_u64(vreinterpret_u64_u32(mask), 0) == 0xFFFFFFFFFFFFFFFFu;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal3(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) & 0x0FFF) == 0x0FFF;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(vceqq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return (vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) & 0x00FFFFFFU) == 0x00FFFFFFU;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(vceqq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z || lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal2(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) & 0x00FF) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x2_t mask = vceq_u32(vget_low_u32(RTM_IMPL_MASK4i_GET(lhs)), vget_low_u32(RTM_IMPL_MASK4i_GET(rhs)));
		return vget_lane_u64(vreinterpret_u64_u32(mask), 0) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal3(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)) & 0x0FFF) != 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint8x16_t mask = vreinterpretq_u8_u32(vceqq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
		uint8x8x2_t mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15 = vzip_u8(vget_low_u8(mask), vget_high_u8(mask));
		uint16x4x2_t mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15 = vzip_u16(vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[0]), vreinterpret_u16_u8(mask_0_8_1_9_2_10_3_11_4_12_5_13_6_14_7_15.val[1]));
		return (vget_lane_u32(vreinterpret_u32_u16(mask_0_8_4_12_1_9_5_13_2_10_6_14_3_11_7_15.val[0]), 0) & 0x00FFFFFFU) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: lhs & rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4i RTM_SIMD_CALL mask_and(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_and_si128(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return RTM_IMPL_MASK4i_SET(vandq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
#else
		return mask4i{ lhs.x & rhs.x, lhs.y & rhs.y, lhs.z & rhs.z, lhs.w & rhs.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical OR between the inputs: lhs | rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4i RTM_SIMD_CALL mask_or(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_or_si128(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return RTM_IMPL_MASK4i_SET(vorrq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
#else
		return mask4i{ lhs.x | rhs.x, lhs.y | rhs.y, lhs.z | rhs.z, lhs.w | rhs.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical XOR between the inputs: lhs ^ rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4i RTM_SIMD_CALL mask_xor(mask4i_arg0 lhs, mask4i_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_xor_si128(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return RTM_IMPL_MASK4i_SET(veorq_u32(RTM_IMPL_MASK4i_GET(lhs), RTM_IMPL_MASK4i_GET(rhs)));
#else
		return mask4i{ lhs.x ^ rhs.x, lhs.y ^ rhs.y, lhs.z ^ rhs.z, lhs.w ^ rhs.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical NOT of the input: ~input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4i RTM_SIMD_CALL mask_not(mask4i_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i true_mask = _mm_set_epi32(0xFFFFFFFFULL, 0xFFFFFFFFULL, 0xFFFFFFFFULL, 0xFFFFFFFFULL);
		return _mm_andnot_si128(input, true_mask);
#elif defined(RTM_NEON_INTRINSICS)
		return RTM_IMPL_MASK4i_SET(vmvnq_u32(RTM_IMPL_MASK4i_GET(input)));
#else
		return mask4i{ ~input.x, ~input.y, ~input.z, ~input.w };
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
