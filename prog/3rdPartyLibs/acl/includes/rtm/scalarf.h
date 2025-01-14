#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
// Copyright (c) 2018 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/constants.h"
#include "rtm/macros.h"
#include "rtm/math.h"
#include "rtm/version.h"
#include "rtm/impl/cmath.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/scalar_common.h"

#include <algorithm>
#include <limits>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Creates a scalar from a floating point value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_set(float xyzw) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_set_ps1(xyzw) };
#else
		return xyzw;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Writes a scalar to memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL scalar_store(scalarf_arg0 input, float* output) RTM_NO_EXCEPT
	{
		_mm_store_ss(output, input.value);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Writes a scalar to memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void scalar_store(float input, float* output) RTM_NO_EXCEPT
	{
		*output = input;
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a scalar into a floating point value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float RTM_SIMD_CALL scalar_cast(scalarf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtss_f32(input.value);
#else
		return input;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the largest integer value not greater than the input (round towards minus infinity).
	// scalar_floor(1.8) = 1.0
	// scalar_floor(-1.8) = -2.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_floor(scalarf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalarf{ _mm_round_ss(input.value, input.value, 0x9) };
#else
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input.value, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ss(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ss(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvtps_epi32(input.value));

		// Test if the returned value is greater than the original.
		// A negative input will round towards zero and be greater when we need it to be smaller.
		__m128 is_negative = _mm_cmpgt_ss(integer_part, input.value);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Positive values will yield a 0.0 bias
		__m128 bias = _mm_cvtepi32_ps(_mm_castps_si128(is_negative));

		// Add our bias to properly handle negative values
		integer_part = _mm_add_ss(integer_part, bias);

		__m128 result = _mm_or_ps(_mm_and_ps(use_original_input, input.value), _mm_andnot_ps(use_original_input, integer_part));
		return scalarf{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest integer value not greater than the input (round towards negative infinity).
	// scalar_floor(1.8) = 1.0
	// scalar_floor(-1.8) = -2.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_floor(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_floor(scalar_set(input)));
#else
		return std::floor(input);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest integer value not less than the input (round towards positive infinity).
	// scalar_ceil(1.8) = 2.0
	// scalar_ceil(-1.8) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_ceil(scalarf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalarf{ _mm_round_ss(input.value, input.value, 0xA) };
#else
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input.value, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ss(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ss(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvtps_epi32(input.value));

		// Test if the returned value is smaller than the original.
		// A positive input will round towards zero and be lower when we need it to be greater.
		__m128 is_positive = _mm_cmplt_ss(integer_part, input.value);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Negative values will yield a 0.0 bias
		__m128 bias = _mm_cvtepi32_ps(_mm_castps_si128(is_positive));

		// Subtract our bias to properly handle positive values
		integer_part = _mm_sub_ss(integer_part, bias);

		__m128 result = _mm_or_ps(_mm_and_ps(use_original_input, input.value), _mm_andnot_ps(use_original_input, integer_part));
		return scalarf{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest integer value not less than the input (round towards positive infinity).
	// scalar_ceil(1.8) = 2.0
	// scalar_ceil(-1.8) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_ceil(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_ceil(scalar_set(input)));
#else
		return std::ceil(input);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the input if it is within the min/max values otherwise the
	// exceeded boundary is returned.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_clamp(scalarf_arg0 input, scalarf_arg1 min, scalarf_arg2 max) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_min_ss(_mm_max_ss(input.value, min.value), max.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the input if it is within the min/max values otherwise the
	// exceeded boundary is returned.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_clamp(float input, float min, float max) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtss_f32(_mm_min_ss(_mm_max_ss(_mm_set_ps1(input), _mm_set_ps1(min)), _mm_set_ps1(max)));
#else
		return (std::min)((std::max)(input, min), max);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the absolute value of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_abs(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		return scalarf{ _mm_and_ps(input.value, _mm_castsi128_ps(abs_mask)) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the absolute value of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_abs(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		return _mm_cvtss_f32(_mm_and_ps(_mm_set_ps1(input), _mm_castsi128_ps(abs_mask)));
#else
		return std::fabs(input);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_sqrt(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_sqrt_ss(input.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float RTM_SIMD_CALL scalar_sqrt(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ps1(input)));
#else
		return std::sqrt(input);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_sqrt_reciprocal(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_div_ss(_mm_set_ps1(1.0F), _mm_sqrt_ss(input.value)) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float RTM_SIMD_CALL scalar_sqrt_reciprocal(float input) RTM_NO_EXCEPT
	{
		// Performance note:
		// With modern out-of-order executing processors, it is typically faster to use
		// a full division/square root instead of a reciprocal estimate + Newton-Raphson iterations
		// because the resulting code is more dense and is more likely to inline and
		// as it uses fewer instructions.
		return 1.0F / scalar_sqrt(input);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_reciprocal(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_div_ss(_mm_set_ps1(1.0F), input.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float RTM_SIMD_CALL scalar_reciprocal(float input) RTM_NO_EXCEPT
	{
		// Performance note:
		// With modern out-of-order executing processors, it is typically faster to use
		// a full division instead of a reciprocal estimate + Newton-Raphson iterations
		// because the resulting code is more dense and is more likely to inline and
		// as it uses fewer instructions.
		return 1.0F / input;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the addition of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_add(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_add_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the addition of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_add(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs + rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the subtraction of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_sub(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_sub_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the subtraction of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_sub(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs - rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_mul(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_mul_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_mul(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs * rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the division of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_div(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_div_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the division of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_div(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs / rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication/addition of the three inputs: s2 + (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_mul_add(scalarf_arg0 s0, scalarf_arg1 s1, scalarf_arg2 s2) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_add_ss(_mm_mul_ss(s0.value, s1.value), s2.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication/addition of the three inputs: s2 + (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_mul_add(float s0, float s1, float s2) RTM_NO_EXCEPT
	{
#if defined(RTM_NEON_INTRINSICS)
		return std::fma(s0, s1, s2);
#else
		return (s0 * s1) + s2;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the negative multiplication/subtraction of the three inputs: -((s0 * s1) - s2)
	// This is mathematically equivalent to: s2 - (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_neg_mul_sub(scalarf_arg0 s0, scalarf_arg1 s1, scalarf_arg2 s2) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_sub_ss(s2.value, _mm_mul_ss(s0.value, s1.value)) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the negative multiplication/subtraction of the three inputs: -((s0 * s1) - s2)
	// This is mathematically equivalent to: s2 - (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_neg_mul_sub(float s0, float s1, float s2) RTM_NO_EXCEPT
	{
		return s2 - (s0 * s1);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_lerp(scalarf_arg0 start, scalarf_arg1 end, scalarf_arg2 alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		return scalar_mul_add(end, alpha, scalar_neg_mul_sub(start, alpha, start));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_lerp(float start, float end, float alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		return scalar_mul_add(end, alpha, scalar_neg_mul_sub(start, alpha, start));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_min(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_min_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_min(float left, float right) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtss_f32(_mm_min_ss(_mm_set_ps1(left), _mm_set_ps1(right)));
#else
		return (std::min)(left, right);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the largest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_max(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalarf{ _mm_max_ss(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_max(float left, float right) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtss_f32(_mm_max_ss(_mm_set_ps1(left), _mm_set_ps1(right)));
#else
		return (std::max)(left, right);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are equal, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_equal(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comieq_ss(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are equal, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool scalar_equal(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs == rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs < rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_lower_than(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comilt_ss(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs < rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool scalar_lower_than(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs < rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs <= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_lower_equal(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comile_ss(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs <= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool scalar_lower_equal(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs <= rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs > rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_greater_than(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comigt_ss(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs > rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool scalar_greater_than(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs > rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs >= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_greater_equal(scalarf_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comige_ss(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs >= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool scalar_greater_equal(float lhs, float rhs) RTM_NO_EXCEPT
	{
		return lhs >= rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are nearly equal, otherwise false: abs(lhs - rhs) <= threshold
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_near_equal(scalarf_arg0 lhs, scalarf_arg1 rhs, scalarf_arg2 threshold) RTM_NO_EXCEPT
	{
		return scalar_lower_equal(scalar_abs(scalar_sub(lhs, rhs)), threshold);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are nearly equal, otherwise false: abs(lhs - rhs) <= threshold
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool scalar_near_equal(float lhs, float rhs, float threshold) RTM_NO_EXCEPT
	{
		return scalar_abs(lhs - rhs) <= threshold;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input is finite (not NaN or Inf), false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_is_finite(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		return check_finite(scalar_cast(input));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input is finite (not NaN or Inf), false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool scalar_is_finite(float input) RTM_NO_EXCEPT
	{
		return check_finite(input);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the rounded input using a symmetric algorithm.
	// scalar_symmetric_round(1.5) = 2.0
	// scalar_symmetric_round(1.2) = 1.0
	// scalar_symmetric_round(-1.5) = -2.0
	// scalar_symmetric_round(-1.2) = -1.0
	// Note: This function relies on the default floating point rounding mode (banker's rounding).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_round_symmetric(scalarf_arg0 input) RTM_NO_EXCEPT
	{
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

#if defined(RTM_SSE4_INTRINSICS)
		__m128 is_positive = _mm_cmpge_ss(input.value, _mm_setzero_ps());

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_andnot_ps(is_positive, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128 bias = _mm_or_ps(sign, _mm_set_ps1(0.5F));
		__m128 biased_input = _mm_add_ss(input.value, bias);

		__m128 floored = _mm_floor_ss(biased_input, biased_input);
		__m128 ceiled = _mm_ceil_ss(biased_input, biased_input);

		__m128 result = RTM_VECTOR4F_SELECT(is_positive, floored, ceiled);
		return scalarf{ result };
#else
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input.value, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ss(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ss(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(input.value, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128 bias = _mm_or_ps(sign, _mm_set_ps1(0.5F));
		__m128 biased_input = _mm_add_ss(input.value, bias);

		// Convert to an integer with truncation and back, this rounds towards zero.
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvttps_epi32(biased_input));

		__m128 result = _mm_or_ps(_mm_and_ps(use_original_input, input.value), _mm_andnot_ps(use_original_input, integer_part));

		return scalarf{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the rounded input using a symmetric algorithm.
	// scalar_round_symmetric(1.5) = 2.0
	// scalar_round_symmetric(1.2) = 1.0
	// scalar_round_symmetric(-1.5) = -2.0
	// scalar_round_symmetric(-1.2) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float scalar_round_symmetric(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_round_symmetric(scalar_set(input)));
#elif defined(RTM_NEON64_INTRINSICS)
		// arm64 has floor/ceil instructions
		return input >= 0.0F ? scalar_floor(input + 0.5F) : scalar_ceil(input - 0.5F);
#else
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const float fractional_limit = 8388608.0F; // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		float abs_input = scalar_abs(input);
		bool is_input_large = abs_input >= fractional_limit;

		// Test if our input is NaN with (value != value), it is only true for NaN
		bool is_nan = input != input;

		// Combine our masks to determine if we should return the original value
		bool use_original_input = is_input_large | is_nan;

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		float bias = input >= 0.0F ? 0.5F : -0.5F;
		float biased_input = input + bias;

		// Convert to an integer with truncation and back, this rounds towards zero.
		float integer_part = static_cast<float>(static_cast<int32_t>(biased_input));

		return use_original_input ? input : integer_part;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the rounded input using banker's rounding (half to even).
	// scalar_round_bankers(2.5) = 2.0
	// scalar_round_bankers(1.5) = 2.0
	// scalar_round_bankers(1.2) = 1.0
	// scalar_round_bankers(-2.5) = -2.0
	// scalar_round_bankers(-1.5) = -2.0
	// scalar_round_bankers(-1.2) = -1.0
	// Note: This function relies on the default floating point rounding mode (banker's rounding).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_round_bankers(scalarf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalarf{ _mm_round_ss(input.value, input.value, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) };
#else
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(input.value, sign_mask);

		// We add the largest integer that a 32 bit floating point number can represent and subtract it afterwards.
		// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
		// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
		// This has the effect of removing the fractional part while simultaneously rounding.
		// Use the same sign as the input value to make sure we handle positive and negative values.
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23
		__m128 truncating_offset = _mm_or_ps(sign, fractional_limit);
		__m128 integer_part = _mm_sub_ss(_mm_add_ss(input.value, truncating_offset), truncating_offset);

		// If our input was so large that it had no fractional part, return it unchanged
		// Otherwise return our integer part
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_input = _mm_and_ps(input.value, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ss(abs_input, fractional_limit);
		__m128 result = _mm_or_ps(_mm_and_ps(is_input_large, input.value), _mm_andnot_ps(is_input_large, integer_part));

		return scalarf{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the rounded input using banker's rounding (half to even).
	// scalar_round_bankers(2.5) = 2.0
	// scalar_round_bankers(1.5) = 2.0
	// scalar_round_bankers(1.2) = 1.0
	// scalar_round_bankers(-2.5) = -2.0
	// scalar_round_bankers(-1.5) = -2.0
	// scalar_round_bankers(-1.2) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_round_bankers(float input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_round_bankers(scalar_set(input)));
#elif defined(RTM_NEON64_INTRINSICS) && defined(RTM_IMPL_VRNDNS_SUPPORTED)
		return vrndns_f32(input);
#else
		if (!scalar_is_finite(input))
			return input;

		int32_t whole = static_cast<int32_t>(input);
		float whole_f = static_cast<float>(whole);
		float remainder = scalar_abs(input - whole_f);
		if (remainder < 0.5F)
			return whole_f;
		if (remainder > 0.5F)
			return input >= 0.0F ? (whole_f + 1.0F) : (whole_f - 1.0F);

		if ((whole % 2) == 0)
			return whole_f;
		else
			return input >= 0.0F ? (whole_f + 1.0F) : (whole_f - 1.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the fractional part of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_fraction(float value) RTM_NO_EXCEPT
	{
		return value - scalar_floor(value);
	}

	//////////////////////////////////////////////////////////////////////////
	// Safely casts an integral input into a float64 output.
	//////////////////////////////////////////////////////////////////////////
	template<typename SrcIntegralType>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float scalar_safe_to_float(SrcIntegralType input) RTM_NO_EXCEPT
	{
		float input_f = float(input);
		RTM_ASSERT(SrcIntegralType(input_f) == input, "Conversion to float would result in truncation");
		return input_f;
	}

	//////////////////////////////////////////////////////////////////////////
	// Trigonometric functions
	//////////////////////////////////////////////////////////////////////////

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_sin(scalarf_arg0 angle) RTM_NO_EXCEPT
	{
		// Use a degree 11 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		__m128 quotient = _mm_mul_ss(angle.value, _mm_set_ps1(constants::one_div_two_pi()));
		quotient = scalar_round_bankers(scalarf{ quotient }).value;
		quotient = _mm_mul_ss(quotient, _mm_set_ps1(constants::two_pi()));
		__m128 x = _mm_sub_ss(angle.value, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(x, sign_mask);
		__m128 reference = _mm_or_ps(sign, _mm_set_ps1(constants::pi()));

		const __m128 reflection = _mm_sub_ss(reference, x);
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));

		__m128 is_less_equal_than_half_pi = _mm_cmple_ss(x_abs, _mm_set_ps1(constants::half_pi()));

		x = RTM_VECTOR4F_SELECT(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		const float x2 = _mm_cvtss_f32(_mm_mul_ss(x, x));
		float result = (x2 * -2.3828544692960918e-8F) + 2.7521557770526783e-6F;
		result = (result * x2) - 1.9840782426250314e-4F;
		result = (result * x2) + 8.3333303183525942e-3F;
		result = (result * x2) - 1.6666666601721269e-1F;
		result = (result * x2) + 1.0F;
		result = result * _mm_cvtss_f32(x);
		return scalar_set(result);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float RTM_SIMD_CALL scalar_sin(float angle) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_sin(scalar_set(angle)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::sin(angle);
#else
		// Use a degree 11 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		float quotient = angle * constants::one_div_two_pi();
		quotient = scalar_round_bankers(quotient);
		quotient = quotient * constants::two_pi();
		float x = angle - quotient;

		// Remap our input in the [-pi/2, pi/2] range
		const float reference = rtm_impl::copysign(constants::pi(), x);
		const float reflection = reference - x;
		const float x_abs = scalar_abs(x);
		x = x_abs <= constants::half_pi() ? x : reflection;

		// Calculate our value
		const float x2 = x * x;
		float result = (x2 * -2.3828544692960918e-8F) + 2.7521557770526783e-6F;
		result = (result * x2) - 1.9840782426250314e-4F;
		result = (result * x2) + 8.3333303183525942e-3F;
		result = (result * x2) - 1.6666666601721269e-1F;
		result = (result * x2) + 1.0F;
		result = result * x;
		return result;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_cos(scalarf_arg0 angle) RTM_NO_EXCEPT
	{
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		__m128 quotient = _mm_mul_ss(angle.value, _mm_set_ps1(constants::one_div_two_pi()));
		quotient = scalar_round_bankers(scalarf{ quotient }).value;
		quotient = _mm_mul_ss(quotient, _mm_set_ps1(constants::two_pi()));
		__m128 x = _mm_sub_ss(angle.value, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 x_sign = _mm_and_ps(x, sign_mask);
		__m128 reference = _mm_or_ps(x_sign, _mm_set_ps1(constants::pi()));
		const __m128 reflection = _mm_sub_ss(reference, x);

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));
		__m128 is_less_equal_than_half_pi = _mm_cmple_ss(x_abs, _mm_set_ps1(constants::half_pi()));

		x = RTM_VECTOR4F_SELECT(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		const float x2 = _mm_cvtss_f32(_mm_mul_ss(x, x));
		float result = (x2 * -2.6051615464872668e-7F) + 2.4760495088926859e-5F;
		result = (result * x2) - 1.3888377661039897e-3F;
		result = (result * x2) + 4.1666638865338612e-2F;
		result = (result * x2) - 4.9999999508695869e-1F;
		result = (result * x2) + 1.0F;

		// Remap into [-pi, pi]
		__m128 result_v = _mm_set_ps1(result);
		__m128 cosine = _mm_or_ps(result_v, _mm_andnot_ps(is_less_equal_than_half_pi, sign_mask));
		return scalarf{ cosine };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float RTM_SIMD_CALL scalar_cos(float angle) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_cos(scalar_set(angle)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::cos(angle);
#else
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		float quotient = angle * constants::one_div_two_pi();
		quotient = scalar_round_bankers(quotient);
		quotient = quotient * constants::two_pi();
		float x = angle - quotient;

		// Remap our input in the [-pi/2, pi/2] range
		const float reference = rtm_impl::copysign(constants::pi(), x);
		const float reflection = reference - x;
		const float x_abs = scalar_abs(x);
		x = x_abs <= constants::half_pi() ? x : reflection;

		// Calculate our value
		const float x2 = x * x;
		float result = (x2 * -2.6051615464872668e-7F) + 2.4760495088926859e-5F;
		result = (result * x2) - 1.3888377661039897e-3F;
		result = (result * x2) + 4.1666638865338612e-2F;
		result = (result * x2) - 4.9999999508695869e-1F;
		result = (result * x2) + 1.0F;

		// Remap into [-pi, pi]
		if (x_abs <= constants::half_pi())
			return result;
		else
			return -result;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	// The result's [x] component contains sin(angle).
	// The result's [y] component contains cos(angle).
	// [zw] are undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL scalar_sincos(scalarf_arg0 angle) RTM_NO_EXCEPT
	{
		scalarf sin_ = scalar_sin(angle);
		scalarf cos_ = scalar_cos(angle);

		return _mm_unpacklo_ps(sin_.value, cos_.value);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	// The result's [x] component contains sin(angle).
	// The result's [y] component contains cos(angle).
	// [zw] are undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL scalar_sincos(float angle) RTM_NO_EXCEPT
	{
		scalarf angle_ = scalar_set(angle);
		scalarf sin_ = scalar_sin(angle_);
		scalarf cos_ = scalar_cos(angle_);

#if defined(RTM_SSE2_INTRINSICS)
		return _mm_unpacklo_ps(sin_.value, cos_.value);
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(cos_, vmovq_n_f32(sin_), 1);
#else
		return vector4f{ sin_, cos_, sin_, cos_ };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void scalar_sincos(float angle, float& out_sin, float& out_cos) RTM_NO_EXCEPT
	{
		out_sin = scalar_sin(angle);
		out_cos = scalar_cos(angle);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_asin(scalarf_arg0 value) RTM_NO_EXCEPT
	{
		// Use a degree 7 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_value = _mm_and_ps(value.value, _mm_castsi128_ps(abs_mask));

		// Calculate our value
		const float x = _mm_cvtss_f32(abs_value);
		float result = (x * -1.2690614339589956e-3F) + 6.7072304676685235e-3F;
		result = (result * x) - 1.7162031184398074e-2F;
		result = (result * x) + 3.0961594977611639e-2F;
		result = (result * x) - 5.0207843052845647e-2F;
		result = (result * x) + 8.8986946573346160e-2F;
		result = (result * x) - 2.1459960076929829e-1F;
		result = (result * x) + 1.5707963267948966F;

		// Scale our result
		const __m128 scale = _mm_sqrt_ss(_mm_sub_ss(_mm_set_ps1(1.0F), abs_value));
		result = result * _mm_cvtss_f32(scale);

		// Handle negative values through reflection
		if (_mm_cvtss_f32(value.value) < 0.0F)
			result = constants::pi() - result;

		// Shift our final result
		const float offset = constants::half_pi();
		result = offset - result;
		return scalarf{ _mm_set_ps1(result) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float scalar_asin(float value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_asin(scalar_set(value)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::asin(value);
#else
		// Use a degree 7 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		const float abs_value = scalar_abs(value);

		// Calculate our value
		float result = (abs_value * -1.2690614339589956e-3F) + 6.7072304676685235e-3F;
		result = (result * abs_value) - 1.7162031184398074e-2F;
		result = (result * abs_value) + 3.0961594977611639e-2F;
		result = (result * abs_value) - 5.0207843052845647e-2F;
		result = (result * abs_value) + 8.8986946573346160e-2F;
		result = (result * abs_value) - 2.1459960076929829e-1F;
		result = (result * abs_value) + 1.5707963267948966F;

		// Scale our result
		const float scale = scalar_sqrt(1.0F - abs_value);
		result = result * scale;

		// Handle negative values through reflection
		if (value < 0.0F)
			result = constants::pi() - result;

		// Shift our final result
		const float offset = constants::half_pi();
		result = offset - result;
		return result;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_acos(scalarf_arg0 value) RTM_NO_EXCEPT
	{
		// Use the identity: acos(value) + asin(value) = PI/2
		// This ends up being: acos(value) = PI/2 - asin(value)
		// Since asin(value) = PI/2 - sqrt(1.0 - polynomial(value))
		// Our end result is acos(value) = sqrt(1.0 - polynomial(value))
		// This means we can re-use the same polynomial as asin()
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_value = _mm_and_ps(value.value, _mm_castsi128_ps(abs_mask));

		// Calculate our value
		const float x = _mm_cvtss_f32(abs_value);
		float result = (x * -1.2690614339589956e-3F) + 6.7072304676685235e-3F;
		result = (result * x) - 1.7162031184398074e-2F;
		result = (result * x) + 3.0961594977611639e-2F;
		result = (result * x) - 5.0207843052845647e-2F;
		result = (result * x) + 8.8986946573346160e-2F;
		result = (result * x) - 2.1459960076929829e-1F;
		result = (result * x) + 1.5707963267948966F;

		// Scale our result
		const __m128 scale = _mm_sqrt_ss(_mm_sub_ss(_mm_set_ps1(1.0F), abs_value));
		result = result * _mm_cvtss_f32(scale);

		// Handle negative values through reflection
		if (_mm_cvtss_f32(value.value) < 0.0F)
			result = constants::pi() - result;

		return scalarf{ _mm_set_ps1(result) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float scalar_acos(float value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_acos(scalar_set(value)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::acos(value);
#else
		// Use the identity: acos(value) + asin(value) = PI/2
		// This ends up being: acos(value) = PI/2 - asin(value)
		// Since asin(value) = PI/2 - sqrt(1.0 - polynomial(value))
		// Our end result is acos(value) = sqrt(1.0 - polynomial(value))
		// This means we can re-use the same polynomial as asin()
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		const float abs_value = scalar_abs(value);

		// Calculate our value
		float result = (abs_value * -1.2690614339589956e-3F) + 6.7072304676685235e-3F;
		result = (result * abs_value) - 1.7162031184398074e-2F;
		result = (result * abs_value) + 3.0961594977611639e-2F;
		result = (result * abs_value) - 5.0207843052845647e-2F;
		result = (result * abs_value) + 8.8986946573346160e-2F;
		result = (result * abs_value) - 2.1459960076929829e-1F;
		result = (result * abs_value) + 1.5707963267948966F;

		// Scale our result
		const float scale = scalar_sqrt(1.0F - abs_value);
		result = result * scale;

		// Handle negative values through reflection
		if (value < 0.0F)
			result = constants::pi() - result;

		return result;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_tan(scalarf_arg0 angle) RTM_NO_EXCEPT
	{
		// Use the identity: tan(angle) = sin(angle) / cos(angle)
		scalarf sin_ = scalar_sin(angle);
		scalarf cos_ = scalar_cos(angle);
		if (scalar_cast(cos_) == 0.0F)
			return scalar_set(rtm_impl::copysign(std::numeric_limits<float>::infinity(), scalar_cast(angle)));

		return scalar_div(sin_, cos_);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float scalar_tan(float angle) RTM_NO_EXCEPT
	{
#if defined(RTM_NEON_INTRINSICS)
		return std::tan(angle);
#else
		// Use the identity: tan(angle) = sin(angle) / cos(angle)
		scalarf angle_ = scalar_set(angle);
		scalarf sin_ = scalar_sin(angle_);
		scalarf cos_ = scalar_cos(angle_);
		if (scalar_cast(cos_) == 0.0F)
			return rtm_impl::copysign(std::numeric_limits<float>::infinity(), angle);

		return scalar_cast(scalar_div(sin_, cos_));
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in. See scalar_atan2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_atan(scalarf_arg0 value) RTM_NO_EXCEPT
	{
		// Use a degree 13 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Discard our sign, we'll restore it later
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_value = _mm_and_ps(value.value, _mm_castsi128_ps(abs_mask));

		// Compute our value
		const __m128 one = _mm_set_ps1(1.0F);
		__m128 is_larger_than_one = _mm_cmpgt_ss(abs_value, one);
		__m128 reciprocal = _mm_div_ss(one, abs_value);

		__m128 x = RTM_VECTOR4F_SELECT(is_larger_than_one, reciprocal, abs_value);

		float x_s = _mm_cvtss_f32(x);
		float x2 = x_s * x_s;

		float result = (x2 * 7.2128853633444123e-3F) - 3.5059680836411644e-2F;
		result = (result * x2) + 8.1675882859940430e-2F;
		result = (result * x2) - 1.3374657325451267e-1F;
		result = (result * x2) + 1.9856563505717162e-1F;
		result = (result * x2) - 3.3324998579202170e-1F;
		result = (result * x2) + 1.0F;
		result = result * x_s;

		__m128 result_s = _mm_set_ps1(result);
		__m128 remapped = _mm_sub_ss(_mm_set_ps1(constants::half_pi()), result_s);

		// pi/2 - result
		result_s = RTM_VECTOR4F_SELECT(is_larger_than_one, remapped, result_s);

		// Keep the original sign
		result_s = _mm_or_ps(result_s, _mm_and_ps(value.value, _mm_set_ps1(-0.0F)));

		return scalarf{ result_s };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in. See scalar_atan2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float RTM_SIMD_CALL scalar_atan(float value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_atan(scalar_set(value)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::atan(value);
#else
		// Use a degree 13 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Discard our sign, we'll restore it later
		float abs_value = scalar_abs(value);

		// Compute our value
		float x = abs_value > 1.0F ? scalar_reciprocal(abs_value) : abs_value;
		float x2 = x * x;

		float result = (x2 * 7.2128853633444123e-3F) - 3.5059680836411644e-2F;
		result = (result * x2) + 8.1675882859940430e-2F;
		result = (result * x2) - 1.3374657325451267e-1F;
		result = (result * x2) + 1.9856563505717162e-1F;
		result = (result * x2) - 3.3324998579202170e-1F;
		result = (result * x2) + 1.0F;
		result = result * x;

		if (abs_value > 1.0f)
			result = constants::half_pi() - result; // pi/2 - result

		// Keep the original sign
		result = value >= 0.0F ? result : -result;

		return result;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalarf RTM_SIMD_CALL scalar_atan2(scalarf y, scalarf x) RTM_NO_EXCEPT
	{
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

		const __m128 y_value = y.value;
		const __m128 x_value = x.value;

		const __m128 zero = _mm_setzero_ps();
		__m128 is_x_zero = _mm_cmpeq_ss(x_value, zero);
		__m128 is_y_zero = _mm_cmpeq_ss(y_value, zero);
		__m128 inputs_are_zero = _mm_and_ps(is_x_zero, is_y_zero);

		__m128 is_x_positive = _mm_cmpgt_ss(x_value, zero);

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 y_sign = _mm_and_ps(y_value, sign_mask);

		// If X == 0.0, our offset is PI/2 otherwise it is PI both with the sign of Y
		__m128 half_pi = _mm_set_ps1(constants::half_pi());
		__m128 pi = _mm_set_ps1(constants::pi());
		__m128 offset = _mm_or_ps(_mm_and_ps(is_x_zero, half_pi), _mm_andnot_ps(is_x_zero, pi));
		offset = _mm_or_ps(offset, y_sign);

		// If X > 0.0, our offset is 0.0
		offset = _mm_andnot_ps(is_x_positive, offset);

		// If X == 0.0 and Y == 0.0, our offset is 0.0
		offset = _mm_andnot_ps(inputs_are_zero, offset);

		__m128 angle = _mm_div_ss(y_value, x_value);
		scalarf angle_s = scalarf{ angle };
		scalarf value_s = scalar_atan(angle_s);
		__m128 value = value_s.value;

		// If X == 0.0, our value is 0.0 otherwise it is atan(y/x)
		value = _mm_or_ps(_mm_and_ps(is_x_zero, zero), _mm_andnot_ps(is_x_zero, value));

		// If X == 0.0 and Y == 0.0, our value is 0.0
		value = _mm_andnot_ps(inputs_are_zero, value);

		__m128 result = _mm_add_ss(value, offset);
		return scalarf{ result };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float scalar_atan2(float y, float x) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_atan2(scalar_set(y), scalar_set(x)));
#elif defined(RTM_NEON_INTRINSICS)
		return std::atan2(y, x);
#else
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

		if (x == 0.0F)
		{
			if (y == 0.0F)
				return 0.0F;

			return rtm_impl::copysign(constants::half_pi(), y);
		}

		float value = scalar_atan(y / x);
		if (x > 0.0F)
			return value;

		float offset = rtm_impl::copysign(constants::pi(), y);
		return value + offset;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts degrees into radians.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_deg_to_rad(float deg) RTM_NO_EXCEPT
	{
		return deg * constants::pi_div_one_eighty();
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts radians into degrees.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float scalar_rad_to_deg(float rad) RTM_NO_EXCEPT
	{
		return rad * constants::one_eighty_div_pi();
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
