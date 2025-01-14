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
#include "rtm/impl/mask_common.h"

#if !defined(RTM_SSE2_INTRINSICS)
	#include <cstring>
#endif

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4d [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_x(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_castpd_si128(input.xy)));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_castpd_si128(input.xy)));
#endif
#else
		return input.x;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4d [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_y(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_castpd_si128(_mm_shuffle_pd(input.xy, input.xy, 1))));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_castpd_si128(_mm_shuffle_pd(input.xy, input.xy, 1))));
#endif
#else
		return input.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4d [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_z(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_castpd_si128(input.zw)));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_castpd_si128(input.zw)));
#endif
#else
		return input.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the mask4d [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE uint64_t RTM_SIMD_CALL mask_get_w(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
#if defined(RTM_ARCH_X64)
		return static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_castpd_si128(_mm_shuffle_pd(input.zw, input.zw, 1))));
#else
		// Just sign extend on 32bit systems
		return static_cast<uint64_t>(_mm_cvtsi128_si32(_mm_castpd_si128(_mm_shuffle_pd(input.zw, input.zw, 1))));
#endif
#else
		return input.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are true, otherwise false: all(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_pd(input.xy) & _mm_movemask_pd(input.zw)) == 3;
#else
		return input.x != 0 && input.y != 0 && input.z != 0 && input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are true, otherwise false: all(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true2(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_pd(input.xy) == 3;
#else
		return input.x != 0 && input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are true, otherwise false: all(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_true3(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_pd(input.xy) == 3 && (_mm_movemask_pd(input.zw) & 1) != 0;
#else
		return input.x != 0 && input.y != 0 && input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are true, otherwise false: any(input.xyzw != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return (_mm_movemask_pd(input.xy) | _mm_movemask_pd(input.zw)) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0 || input.w != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are true, otherwise false: any(input.xy != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true2(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_pd(input.xy) != 0;
#else
		return input.x != 0 || input.y != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are true, otherwise false: any(input.xyz != 0)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_true3(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_movemask_pd(input.xy) != 0 || (_mm_movemask_pd(input.zw) & 1) != 0;
#else
		return input.x != 0 || input.y != 0 || input.z != 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i lhs_zw = _mm_castpd_si128(lhs.zw);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		__m128i rhs_zw = _mm_castpd_si128(rhs.zw);
		__m128i xy_eq = _mm_cmpeq_epi32(lhs_xy, rhs_xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs_zw, rhs_zw);
		return (_mm_movemask_epi8(xy_eq) & _mm_movemask_epi8(zw_eq)) == 0xFFFF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint64_t) * 4) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal2(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs_xy, rhs_xy)) == 0xFFFF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint64_t) * 2) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_all_equal3(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i lhs_zw = _mm_castpd_si128(lhs.zw);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		__m128i rhs_zw = _mm_castpd_si128(rhs.zw);
		__m128i xy_eq = _mm_cmpeq_epi32(lhs_xy, rhs_xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs_zw, rhs_zw);
		return _mm_movemask_epi8(xy_eq) == 0xFFFF && (_mm_movemask_epi8(zw_eq) & 0x00FF) == 0x00FF;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs, &rhs, sizeof(uint64_t) * 3) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i lhs_zw = _mm_castpd_si128(lhs.zw);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		__m128i rhs_zw = _mm_castpd_si128(rhs.zw);
		__m128i xy_eq = _mm_cmpeq_epi32(lhs_xy, rhs_xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs_zw, rhs_zw);
		return (_mm_movemask_epi8(xy_eq) | _mm_movemask_epi8(zw_eq)) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.z, &rhs.z, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.w, &rhs.w, sizeof(uint64_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal2(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs_xy, rhs_xy)) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint64_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL mask_any_equal3(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// Cannot use == and != with NaN floats
#if defined(RTM_SSE2_INTRINSICS)
		// WARNING: SSE2 doesn't have a 64 bit int compare, we use 32 bit here and we assume
		// that in a mask all bits are equal
		__m128i lhs_xy = _mm_castpd_si128(lhs.xy);
		__m128i lhs_zw = _mm_castpd_si128(lhs.zw);
		__m128i rhs_xy = _mm_castpd_si128(rhs.xy);
		__m128i rhs_zw = _mm_castpd_si128(rhs.zw);
		__m128i xy_eq = _mm_cmpeq_epi32(lhs_xy, rhs_xy);
		__m128i zw_eq = _mm_cmpeq_epi32(lhs_zw, rhs_zw);
		return _mm_movemask_epi8(xy_eq) != 0 || (_mm_movemask_epi8(zw_eq) & 0x00FF) != 0;
#elif defined(RTM_SSE4_INTRINSICS) && 0
		// TODO
#else
		return std::memcmp(&lhs.x, &rhs.x, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.y, &rhs.y, sizeof(uint64_t)) == 0
			|| std::memcmp(&lhs.z, &rhs.z, sizeof(uint64_t)) == 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: lhs & rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL mask_and(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_and_pd(lhs.xy, rhs.xy);
		__m128d zw = _mm_and_pd(lhs.zw, rhs.zw);
		return mask4d{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4d vector;
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL mask_or(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_or_pd(lhs.xy, rhs.xy);
		__m128d zw = _mm_or_pd(lhs.zw, rhs.zw);
		return mask4d{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4d vector;
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL mask_xor(mask4d_arg0 lhs, mask4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_xor_pd(lhs.xy, rhs.xy);
		__m128d zw = _mm_xor_pd(lhs.zw, rhs.zw);
		return mask4d{ xy, zw };
#else
		const uint64_t* lhs_ = rtm_impl::bit_cast<const uint64_t*>(&lhs);
		const uint64_t* rhs_ = rtm_impl::bit_cast<const uint64_t*>(&rhs);

		union
		{
			mask4d vector;
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL mask_not(mask4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i true_mask = _mm_set_epi64x(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
		__m128d xy = _mm_andnot_pd(input.xy, _mm_castsi128_pd(true_mask));
		__m128d zw = _mm_andnot_pd(input.zw, _mm_castsi128_pd(true_mask));
		return mask4d{ xy, zw };
#else
		const uint64_t* input_ = rtm_impl::bit_cast<const uint64_t*>(&input);

		union
		{
			mask4d vector;
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
