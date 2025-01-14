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
#include "rtm/quatf.h"
#include "rtm/vector4f.h"
#include "rtm/version.h"
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion on the hypersphere with a positive [w] component
	// that represents the same 3D rotation as the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_ensure_positive_w(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 sign_bit = RTM_VECTOR4F_MAKE(-0.0F, -0.0F, -0.0F, -0.0F);
		const __m128 input_sign = _mm_and_ps(input, sign_bit);
		const __m128 bias = _mm_shuffle_ps(input_sign, input_sign, _MM_SHUFFLE(3, 3, 3, 3));
		return _mm_xor_ps(input, bias);
#elif defined(RTM_NEON_INTRINSICS)
		alignas(16) constexpr uint32_t sign_bit_i[4] = { 0x80000000U, 0x80000000U, 0x80000000U, 0x80000000U };
		const uint32x4_t sign_bit = *rtm_impl::bit_cast<const uint32x4_t*>(&sign_bit_i[0]);
		const uint32x4_t input_u32 = vreinterpretq_u32_f32(input);
		const uint32x4_t input_sign = vandq_u32(input_u32, sign_bit);
		const uint32_t input_sign_w = vgetq_lane_u32(input_sign, 3);
#if defined(RTM_COMPILER_MSVC)
		// MSVC's intrinsic is an alias to the unsigned variant
		const uint32x4_t bias = vmovq_n_u32(static_cast<int32_t>(input_sign_w));
#else
		const uint32x4_t bias = vmovq_n_u32(input_sign_w);
#endif
		return vreinterpretq_f32_u32(veorq_u32(input_u32, bias));
#else
		return quat_get_w(input) >= 0.f ? input : quat_neg(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a quaternion constructed from a vector3 representing the [xyz]
	// components while reconstructing the [w] component by assuming it is positive.
	// Note: When the squared length of [xyz] is very small, the square-root might not
	// be very accurate when [w] is reconstructed. As such, the resulting quaternion
	// might be near but not quite normalized. If high accuracy is required, make
	// sure to normalize explicitly afterwards.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_from_positive_w(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// w_squared can be negative either due to rounding or due to quantization imprecision, we take the absolute value
		// to ensure the resulting quaternion is always normalized with a positive W component

#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);

		__m128 x2y2z2 = _mm_mul_ps(input, input);
		__m128 one = _mm_set_ss(1.0F);
		__m128 w_squared = _mm_sub_ss(_mm_sub_ss(_mm_sub_ss(one, x2y2z2), _mm_shuffle_ps(x2y2z2, x2y2z2, _MM_SHUFFLE(1, 1, 1, 1))), _mm_shuffle_ps(x2y2z2, x2y2z2, _MM_SHUFFLE(2, 2, 2, 2)));
		w_squared = _mm_and_ps(w_squared, _mm_castsi128_ps(abs_mask));
		__m128 w = _mm_sqrt_ss(w_squared);

#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, w, 0x30);
#else
		__m128 input_wyzx = _mm_shuffle_ps(input, input, _MM_SHUFFLE(0, 2, 1, 3));
		__m128 result_wyzx = _mm_move_ss(input_wyzx, w);
		return _mm_shuffle_ps(result_wyzx, result_wyzx, _MM_SHUFFLE(0, 2, 1, 3));
#endif
#elif defined(RTM_NEON64_INTRINSICS) && defined(RTM_IMPL_VFMSS_SUPPORTED)
		// 1.0 - (x * x)
		float result = vfmss_laneq_f32(1.0F, vgetq_lane_f32(input, 0), input, 0);
		// result - (y * y)
		result = vfmss_laneq_f32(result, vgetq_lane_f32(input, 1), input, 1);
		// result - (z * z)
		float w_squared = vfmss_laneq_f32(result, vgetq_lane_f32(input, 2), input, 2);
		float w = scalar_sqrt(scalar_abs(w_squared));
		return vsetq_lane_f32(w, input, 3);
#else
		// Operation order is important here, due to rounding, ((1.0 - (X*X)) - Y*Y) - Z*Z is more accurate than 1.0 - dot3(xyz, xyz)
		float w_squared = ((1.0F - vector_get_x(input) * vector_get_x(input)) - vector_get_y(input) * vector_get_y(input)) - vector_get_z(input) * vector_get_z(input);
		// w_squared can be negative either due to rounding or due to quantization imprecision, we take the absolute value
		// to ensure the resulting quaternion is always normalized with a positive W component
		float w = scalar_sqrt(scalar_abs(w_squared));
		return quat_set_w(vector_to_quat(input), w);
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
