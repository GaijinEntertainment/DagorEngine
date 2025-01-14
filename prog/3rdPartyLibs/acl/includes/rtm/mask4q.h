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
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/mask_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4q [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_x(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(input.xy));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(input.xy));
#endif
#else
		return input.x;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4q [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_y(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_shuffle_epi32(input.xy, _MM_SHUFFLE(3, 2, 3, 2))));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_shuffle_epi32(input.xy, _MM_SHUFFLE(3, 2, 3, 2))));
#endif
#else
		return input.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4q [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_z(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(input.zw));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(input.zw));
#endif
#else
		return input.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4q [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_w(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_shuffle_epi32(input.zw, _MM_SHUFFLE(3, 2, 3, 2))));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_shuffle_epi32(input.zw, _MM_SHUFFLE(3, 2, 3, 2))));
#endif
#else
		return input.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are true, otherwise false: all(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input.xy) & _mm_movemask_epi8(input.zw)) == 0xFFFF;
#else
		return input.x != 0 && input.y != 0 && input.z != 0 && input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are true, otherwise false: all(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true2(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input.xy) == 0xFFFF;
#else
		return input.x != 0 && input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are true, otherwise false: all(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true3(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input.xy) == 0xFFFF && (_mm_movemask_epi8(input.zw) & 0x00FF) == 0x00FF;
#else
		return input.x != 0 && input.y != 0 && input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are true, otherwise false: any(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_epi8(input.xy) | _mm_movemask_epi8(input.zw)) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0 || input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are true, otherwise false: any(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true2(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input.xy) != 0;
#else
		return input.x != 0 || input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are true, otherwise false: any(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true3(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_epi8(input.xy) != 0 || (_mm_movemask_epi8(input.zw) & 0x00FF) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i xy_eq = _mm_cmpeq_epi32(lhs.xy, rhs.xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs.zw, rhs.zw);
		return (_mm_movemask_epi8(xy_eq) & _mm_movemask_epi8(zw_eq)) == 0xFFFF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal2(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs.xy, rhs.xy)) == 0xFFFF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x && lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal3(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i xy_eq = _mm_cmpeq_epi32(lhs.xy, rhs.xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs.zw, rhs.zw);
		return _mm_movemask_epi8(xy_eq) == 0xFFFF && (_mm_movemask_epi8(zw_eq) & 0x00FF) == 0x00FF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i xy_eq = _mm_cmpeq_epi32(lhs.xy, rhs.xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs.zw, rhs.zw);
		return (_mm_movemask_epi8(xy_eq) | _mm_movemask_epi8(zw_eq)) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z || lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal2(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs.xy, rhs.xy)) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x || lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal3(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i xy_eq = _mm_cmpeq_epi32(lhs.xy, rhs.xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs.zw, rhs.zw);
		return _mm_movemask_epi8(xy_eq) != 0 || (_mm_movemask_epi8(zw_eq) & 0x00FF) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: lhs & rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4q RTM_SIMD_CALL mask_and(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128i xy = _mm_and_si128(lhs.xy, rhs.xy);
		__m128i zw = _mm_and_si128(lhs.zw, rhs.zw);
		return mask4q{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4q vector;
			uint64_t scalar[4];
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4q RTM_SIMD_CALL mask_or(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128i xy = _mm_or_si128(lhs.xy, rhs.xy);
		__m128i zw = _mm_or_si128(lhs.zw, rhs.zw);
		return mask4q{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4q vector;
			uint64_t scalar[4];
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4q RTM_SIMD_CALL mask_xor(mask4q_arg0 lhs, mask4q_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128i xy = _mm_xor_si128(lhs.xy, rhs.xy);
		__m128i zw = _mm_xor_si128(lhs.zw, rhs.zw);
		return mask4q{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4q vector;
			uint64_t scalar[4];
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4q RTM_SIMD_CALL mask_not(mask4q_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i true_mask = _mm_set_epi64x(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
		__m128i xy = _mm_andnot_si128(input.xy, true_mask);
		__m128i zw = _mm_andnot_si128(input.zw, true_mask);
		return mask4q{ xy, zw };
#else
		const uint64_t* input_ = rtm_impl::bit_cast<const uint64_t*>(&input);

		union
		{
			mask4q vector;
			uint64_t scalar[4];
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
