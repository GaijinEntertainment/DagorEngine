#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Realtime Math contributors
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
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/macros.mask4.impl.h"
#include "rtm/impl/mask_common.h"

#if !defined(RTM_SSE2_INTRINSICS)
	#include <cstring>
#endif

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4f [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_x(mask4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(input)));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(input, 0);
#else
		return input.x;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4f [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_y(mask4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(_mm_shuffle_ps(input, input, _MM_SHUFFLE(1, 1, 1, 1)))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(input, 1);
#else
		return input.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4f [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_z(mask4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(_mm_shuffle_ps(input, input, _MM_SHUFFLE(2, 2, 2, 2)))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(input, 2);
#else
		return input.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4f [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint32_t RTM_SIMD_CALL mask_get_w(mask4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(_mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 3, 3, 3)))));
#elif defined(RTM_NEON_INTRINSICS)
		return vgetq_lane_u32(input, 3);
#else
		return input.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are true, otherwise false: all(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ALL_TRUE(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are true, otherwise false: all(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true2(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ALL_TRUE2(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are true, otherwise false: all(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true3(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ALL_TRUE3(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are true, otherwise false: any(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ANY_TRUE(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are true, otherwise false: any(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true2(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ANY_TRUE2(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are true, otherwise false: any(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true3(mask4f_arg0 input) RTM_NO_EXCEPT
	{
		bool result;
		RTM_MASK4F_ANY_TRUE3(input, result);
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_ps(_mm_xor_ps(lhs, rhs)) == 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x4_t mask = veorq_u32(lhs, rhs);
		uint16x4_t truncated_mask = vmovn_u32(mask);
		return vget_lane_u64(vreinterpret_u64_u16(truncated_mask), 0) == 0;
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint32_t) * 4) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal2(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_ps(_mm_xor_ps(lhs, rhs)) & 0x03) == 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x2_t mask = veor_u32(vget_low_u32(lhs), vget_low_u32(rhs));
		return vget_lane_u64(vreinterpret_u64_u32(mask), 0) == 0;
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint32_t) * 2) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal3(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_ps(_mm_xor_ps(lhs, rhs)) & 0x07) == 0;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x4_t mask = veorq_u32(lhs, rhs);
		uint16x4_t truncated_mask = vmovn_u32(mask);
		return (vget_lane_u64(vreinterpret_u64_u16(truncated_mask), 0) & 0x0000FFFFFFFFFFFFULL) == 0;
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint32_t) * 3) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_ps(_mm_xor_ps(lhs, rhs)) != 0x0F;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x4_t mask = veorq_u32(lhs, rhs);
		uint16x4_t truncated_mask = vmovn_u32(mask);
		return vget_lane_u64(vreinterpret_u64_u16(truncated_mask), 0) != 0xFFFFFFFFFFFFFFFFULL;
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.z, &rhs.z, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.w, &rhs.w, sizeof(uint32_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal2(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_ps(_mm_xor_ps(lhs, rhs)) & 0x03) != 0x03;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x2_t mask = veor_u32(vget_low_u32(lhs), vget_low_u32(rhs));
		return vget_lane_u64(vreinterpret_u64_u32(mask), 0) != 0xFFFFFFFFFFFFFFFFULL;
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint32_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal3(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
		// Masks are always 0 or ~0, use this to our advantage
		// lhs ^ rhs = 0 if both are equal and != 0 if they are not
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_ps(_mm_xor_ps(lhs, rhs)) & 0x07) != 0x07;
#elif defined(RTM_NEON_INTRINSICS)
		uint32x4_t mask = veorq_u32(lhs, rhs);
		uint16x4_t truncated_mask = vmovn_u32(mask);
		return (vget_lane_u64(vreinterpret_u64_u16(truncated_mask), 0) & 0x0000FFFFFFFFFFFFULL) != 0x0000FFFFFFFFFFFFULL;
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint32_t)) == 0
			|| std::memcmp(&lhs.z, &rhs.z, sizeof(uint32_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: lhs & rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL mask_and(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_and_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vandq_u32(lhs, rhs);
#else
		const uint32_t* lhs_ = rtm_impl::bit_cast<const uint32_t*>(&lhs);
		const uint32_t* rhs_ = rtm_impl::bit_cast<const uint32_t*>(&rhs);

		union
		{
			mask4f vector;
			uint32_t scalar[4];
		} result;

		result.scalar[0] = lhs_[0] & rhs_[0];
		result.scalar[1] = lhs_[1] & rhs_[1];
		result.scalar[2] = lhs_[2] & rhs_[2];
		result.scalar[3] = lhs_[3] & rhs_[3];

		return result.vector;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical OR between the inputs: lhs | rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL mask_or(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_or_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vorrq_u32(lhs, rhs);
#else
		const uint32_t* lhs_ = rtm_impl::bit_cast<const uint32_t*>(&lhs);
		const uint32_t* rhs_ = rtm_impl::bit_cast<const uint32_t*>(&rhs);

		union
		{
			mask4f vector;
			uint32_t scalar[4];
		} result;

		result.scalar[0] = lhs_[0] | rhs_[0];
		result.scalar[1] = lhs_[1] | rhs_[1];
		result.scalar[2] = lhs_[2] | rhs_[2];
		result.scalar[3] = lhs_[3] | rhs_[3];

		return result.vector;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical XOR between the inputs: lhs ^ rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL mask_xor(mask4f_arg0 lhs, mask4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_xor_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return veorq_u32(lhs, rhs);
#else
		const uint32_t* lhs_ = rtm_impl::bit_cast<const uint32_t*>(&lhs);
		const uint32_t* rhs_ = rtm_impl::bit_cast<const uint32_t*>(&rhs);

		union
		{
			mask4f vector;
			uint32_t scalar[4];
		} result;

		result.scalar[0] = lhs_[0] ^ rhs_[0];
		result.scalar[1] = lhs_[1] ^ rhs_[1];
		result.scalar[2] = lhs_[2] ^ rhs_[2];
		result.scalar[3] = lhs_[3] ^ rhs_[3];

		return result.vector;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical NOT of the input: ~input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL mask_not(mask4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i true_mask = _mm_set_epi32(0xFFFFFFFFULL, 0xFFFFFFFFULL, 0xFFFFFFFFULL, 0xFFFFFFFFULL);
		return _mm_andnot_ps(input, _mm_castsi128_ps(true_mask));
#elif defined(RTM_NEON_INTRINSICS)
		return vmvnq_u32(input);
#else
		const uint32_t* input_ = rtm_impl::bit_cast<const uint32_t*>(&input);

		union
		{
			mask4f vector;
			uint32_t scalar[4];
		} result;

		result.scalar[0] = ~input_[0];
		result.scalar[1] = ~input_[1];
		result.scalar[2] = ~input_[2];
		result.scalar[3] = ~input_[3];

		return result.vector;
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
