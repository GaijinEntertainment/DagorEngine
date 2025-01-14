#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <rtm/quatf.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	RTM_DISABLE_SECURITY_COOKIE_CHECK inline rtm::quatf RTM_SIMD_CALL quat_lerp_no_normalization(rtm::quatf_arg0 start, rtm::quatf_arg1 end, float alpha) RTM_NO_EXCEPT
	{
		using namespace rtm;

#if defined(RTM_SSE2_INTRINSICS)
		// Calculate the vector4 dot product: dot(start, end)
		__m128 dot;
#if defined(RTM_SSE4_INTRINSICS)
		// The dpps instruction isn't as accurate but we don't care here, we only need the sign of the
		// dot product. If both rotations are on opposite ends of the hypersphere, the result will be
		// very negative. If we are on the edge, the rotations are nearly opposite but not quite which
		// means that the linear interpolation here will have terrible accuracy to begin with. It is designed
		// for interpolating rotations that are reasonably close together. The bias check is mainly necessary
		// because the W component is often kept positive which flips the sign.
		// Using the dpps instruction reduces the number of registers that we need and helps the function get
		// inlined.
		dot = _mm_dp_ps(start, end, 0xFF);
#else
		{
			__m128 x2_y2_z2_w2 = _mm_mul_ps(start, end);
			__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
			__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
			__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
			// Shuffle the dot product to all SIMD lanes, there is no _mm_and_ss and loading
			// the constant from memory with the 'and' instruction is faster, it uses fewer registers
			// and fewer instructions
			dot = _mm_shuffle_ps(x2y2z2w2_0_0_0, x2y2z2w2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
		}
#endif

		// Calculate the bias, if the dot product is positive or zero, there is no bias
		// but if it is negative, we want to flip the 'end' rotation XYZW components
		__m128 bias = _mm_and_ps(dot, _mm_set_ps1(-0.0F));

		// Lerp the rotation after applying the bias
		// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
		__m128 alpha_ = _mm_set_ps1(alpha);
		__m128 interpolated_rotation = _mm_add_ps(_mm_sub_ps(start, _mm_mul_ps(alpha_, start)), _mm_mul_ps(alpha_, _mm_xor_ps(end, bias)));

		// Due to the interpolation, the result might not be anywhere near normalized!
		// Make sure to normalize afterwards before using
		return interpolated_rotation;
#elif defined (RTM_NEON64_INTRINSICS)
		// On ARM64 with NEON, we load 1.0 once and use it twice which is faster than
		// using a AND/XOR with the bias (same number of instructions)
		float dot = vector_dot(start, end);
		float bias = dot >= 0.0F ? 1.0F : -1.0F;

		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4f interpolated_rotation = vector_mul_add(vector_mul(end, bias), alpha, vector_neg_mul_sub(start, alpha, start));

		// Due to the interpolation, the result might not be anywhere near normalized!
		// Make sure to normalize afterwards before using
		return interpolated_rotation;
#elif defined(RTM_NEON_INTRINSICS)
		// Calculate the vector4 dot product: dot(start, end)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(start, end);
		float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
		float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
		float32x2_t x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
		float32x2_t x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);

		// Calculate the bias, if the dot product is positive or zero, there is no bias
		// but if it is negative, we want to flip the 'end' rotation XYZW components
		// On ARM-v7-A, the AND/XOR trick is faster than the cmp/fsel
		uint32x2_t bias = vand_u32(vreinterpret_u32_f32(x2y2z2w2), vdup_n_u32(0x80000000));

		// Lerp the rotation after applying the bias
		// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
		float32x4_t end_biased = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(end), vcombine_u32(bias, bias)));
		float32x4_t interpolated_rotation = vmlaq_n_f32(vmlsq_n_f32(start, start, alpha), end_biased, alpha);

		// Due to the interpolation, the result might not be anywhere near normalized!
		// Make sure to normalize afterwards before using
		return interpolated_rotation;
#else
		// To ensure we take the shortest path, we apply a bias if the dot product is negative
		vector4f start_vector = quat_to_vector(start);
		vector4f end_vector = quat_to_vector(end);
		float dot = vector_dot(start_vector, end_vector);
		float bias = dot >= 0.0F ? 1.0F : -1.0F;
		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4f interpolated_rotation = vector_mul_add(vector_mul(end_vector, bias), alpha, vector_neg_mul_sub(start_vector, alpha, start_vector));

		// Due to the interpolation, the result might not be anywhere near normalized!
		// Make sure to normalize afterwards before using
		return vector_to_quat(interpolated_rotation);
#endif
	}

	namespace acl_impl
	{
		// About 31 cycles with AVX on Skylake
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL quat_from_positive_w4(rtm::vector4f_arg0 xxxx, rtm::vector4f_arg1 yyyy, rtm::vector4f_arg2 zzzz)
		{
			// 1.0 - (x * x)
			rtm::vector4f result = rtm::vector_neg_mul_sub(xxxx, xxxx, rtm::vector_set(1.0F));
			// result - (y * y)
			result = rtm::vector_neg_mul_sub(yyyy, yyyy, result);
			// result - (z * z)
			const rtm::vector4f wwww_squared = rtm::vector_neg_mul_sub(zzzz, zzzz, result);

			// w_squared can be negative either due to rounding or due to quantization imprecision, we take the absolute value
			// to ensure the resulting quaternion is always normalized with a positive W component
			return rtm::vector_sqrt(rtm::vector_abs(wwww_squared));
		}

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK __m256 RTM_SIMD_CALL quat_from_positive_w_avx8(__m256 xxxx0_xxxx1, __m256 yyyy0_yyyy1, __m256 zzzz0_zzzz1)
		{
			const __m256 one_v = _mm256_set1_ps(1.0F);

			const __m256 xxxx0_xxxx1_squared = _mm256_mul_ps(xxxx0_xxxx1, xxxx0_xxxx1);
			const __m256 yyyy0_yyyy1_squared = _mm256_mul_ps(yyyy0_yyyy1, yyyy0_yyyy1);
			const __m256 zzzz0_zzzz1_squared = _mm256_mul_ps(zzzz0_zzzz1, zzzz0_zzzz1);

			const __m256 wwww0_wwww1_squared = _mm256_sub_ps(_mm256_sub_ps(_mm256_sub_ps(one_v, xxxx0_xxxx1_squared), yyyy0_yyyy1_squared), zzzz0_zzzz1_squared);

			const __m256i abs_mask = _mm256_set1_epi32(0x7FFFFFFFULL);
			const __m256 wwww0_wwww1_squared_abs = _mm256_and_ps(wwww0_wwww1_squared, _mm256_castsi256_ps(abs_mask));

			return _mm256_sqrt_ps(wwww0_wwww1_squared_abs);
		}
#endif

		// About 28 cycles with AVX on Skylake
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL quat_lerp_no_normalization4(
			rtm::vector4f_arg0 xxxx0, rtm::vector4f_arg1 yyyy0, rtm::vector4f_arg2 zzzz0, rtm::vector4f_arg3 wwww0,
			rtm::vector4f_arg4 xxxx1, rtm::vector4f_arg5 yyyy1, rtm::vector4f_arg6 zzzz1, rtm::vector4f_arg7 wwww1,
			rtm::vector4f_argn interpolation_alpha,
			rtm::vector4f& interp_xxxx, rtm::vector4f& interp_yyyy, rtm::vector4f& interp_zzzz, rtm::vector4f& interp_wwww)
		{
			// Calculate the vector4 dot product: dot(start, end)
			const rtm::vector4f dot4 = rtm::vector_mul_add(wwww0, wwww1, rtm::vector_mul_add(zzzz0, zzzz1, rtm::vector_mul_add(yyyy0, yyyy1, rtm::vector_mul(xxxx0, xxxx1))));

			// Calculate the bias, if the dot product is positive or zero, there is no bias
			// but if it is negative, we want to flip the 'end' rotation XYZW components
			const rtm::vector4f neg_zero = rtm::vector_set(-0.0F);
			const rtm::vector4f bias = rtm::vector_and(dot4, neg_zero);

			// Apply our bias to the 'end'
			const rtm::vector4f xxxx1_with_bias = rtm::vector_xor(xxxx1, bias);
			const rtm::vector4f yyyy1_with_bias = rtm::vector_xor(yyyy1, bias);
			const rtm::vector4f zzzz1_with_bias = rtm::vector_xor(zzzz1, bias);
			const rtm::vector4f wwww1_with_bias = rtm::vector_xor(wwww1, bias);

			// Lerp the rotation after applying the bias
			// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
			interp_xxxx = rtm::vector_mul_add(xxxx1_with_bias, interpolation_alpha, rtm::vector_neg_mul_sub(xxxx0, interpolation_alpha, xxxx0));
			interp_yyyy = rtm::vector_mul_add(yyyy1_with_bias, interpolation_alpha, rtm::vector_neg_mul_sub(yyyy0, interpolation_alpha, yyyy0));
			interp_zzzz = rtm::vector_mul_add(zzzz1_with_bias, interpolation_alpha, rtm::vector_neg_mul_sub(zzzz0, interpolation_alpha, zzzz0));
			interp_wwww = rtm::vector_mul_add(wwww1_with_bias, interpolation_alpha, rtm::vector_neg_mul_sub(wwww0, interpolation_alpha, wwww0));
		}

		// About 9 cycles with AVX on Skylake
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL quat_normalize4(rtm::vector4f& xxxx, rtm::vector4f& yyyy, rtm::vector4f& zzzz, rtm::vector4f& wwww)
		{
			const rtm::vector4f dot4 = rtm::vector_mul_add(wwww, wwww, rtm::vector_mul_add(zzzz, zzzz, rtm::vector_mul_add(yyyy, yyyy, rtm::vector_mul(xxxx, xxxx))));

			const rtm::vector4f len4 = rtm::vector_sqrt(dot4);
			const rtm::vector4f inv_len4 = rtm::vector_div(rtm::vector_set(1.0F), len4);

			xxxx = rtm::vector_mul(xxxx, inv_len4);
			yyyy = rtm::vector_mul(yyyy, inv_len4);
			zzzz = rtm::vector_mul(zzzz, inv_len4);
			wwww = rtm::vector_mul(wwww, inv_len4);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
