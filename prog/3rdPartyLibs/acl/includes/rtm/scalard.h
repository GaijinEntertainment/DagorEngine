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
#include "rtm/math.h"
#include "rtm/version.h"
#include "rtm/impl/cmath.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/scalar_common.h"

#if defined(RTM_SSE2_INTRINSICS)
#include "rtm/macros.h"
#endif

#include <algorithm>
#include <limits>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Creates a scalar from a floating point value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_set(double xyzw) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_set1_pd(xyzw) };
#else
		return xyzw;
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Writes a scalar to memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL scalar_store(scalard_arg0 input, double* output) RTM_NO_EXCEPT
	{
		_mm_store_sd(output, input.value);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Writes a scalar to memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL scalar_store(double input, double* output) RTM_NO_EXCEPT
	{
		*output = input;
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a scalar into a floating point value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_cast(scalard_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtsd_f64(input.value);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_floor(scalard_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalard{ _mm_round_sd(input.value, input.value, 0x9) };
#else
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input = _mm_and_pd(input.value, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large = _mm_cmpge_sd(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan = _mm_cmpneq_sd(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input = _mm_or_pd(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128d integer_part = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.value));

		// Test if the returned value is greater than the original.
		// A negative input will round towards zero and be greater when we need it to be smaller.
		__m128d is_negative = _mm_cmpgt_sd(integer_part, input.value);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Positive values will yield a 0.0 bias
		__m128d bias = _mm_cvtepi32_pd(_mm_castpd_si128(is_negative));

		// Add our bias to properly handle negative values
		integer_part = _mm_add_sd(integer_part, bias);

		__m128d result = _mm_or_pd(_mm_and_pd(use_original_input, input.value), _mm_andnot_pd(use_original_input, integer_part));
		return scalard{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest integer value not greater than the input (round towards negative infinity).
	// scalar_floor(1.8) = 1.0
	// scalar_floor(-1.8) = -2.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_floor(double input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_ceil(scalard_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalard{ _mm_round_sd(input.value, input.value, 0xA) };
#else
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input = _mm_and_pd(input.value, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large = _mm_cmpge_sd(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan = _mm_cmpneq_sd(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input = _mm_or_pd(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128d integer_part = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.value));

		// Test if the returned value is smaller than the original.
		// A positive input will round towards zero and be lower when we need it to be greater.
		__m128d is_positive = _mm_cmplt_sd(integer_part, input.value);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Negative values will yield a 0.0 bias
		__m128d bias = _mm_cvtepi32_pd(_mm_castpd_si128(is_positive));

		// Subtract our bias to properly handle positive values
		integer_part = _mm_sub_sd(integer_part, bias);

		__m128d result = _mm_or_pd(_mm_and_pd(use_original_input, input.value), _mm_andnot_pd(use_original_input, integer_part));
		return scalard{ result };
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest integer value not less than the input (round towards positive infinity).
	// scalar_ceil(1.8) = 2.0
	// scalar_ceil(-1.8) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_ceil(double input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_clamp(scalard_arg0 input, scalard_arg1 min, scalard_arg2 max) RTM_NO_EXCEPT
	{
		return scalard{ _mm_min_sd(_mm_max_sd(input.value, min.value), max.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the input if it is within the min/max values otherwise the
	// exceeded boundary is returned.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_clamp(double input, double min, double max) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtsd_f64(_mm_min_sd(_mm_max_sd(_mm_set1_pd(input), _mm_set1_pd(min)), _mm_set1_pd(max)));
#else
		return (std::min)((std::max)(input, min), max);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the absolute value of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_abs(scalard_arg0 input) RTM_NO_EXCEPT
	{
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		return scalard{ _mm_and_pd(input.value, _mm_castsi128_pd(abs_mask)) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the absolute value of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_abs(double input) RTM_NO_EXCEPT
	{
		return std::fabs(input);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_sqrt(scalard_arg0 input) RTM_NO_EXCEPT
	{
		return scalard{ _mm_sqrt_sd(input.value, input.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_sqrt(double input) RTM_NO_EXCEPT
	{
		return std::sqrt(input);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_sqrt_reciprocal(scalard_arg0 input) RTM_NO_EXCEPT
	{
		const __m128d input_sqrt = _mm_sqrt_sd(input.value, input.value);
		const __m128d result = _mm_div_sd(_mm_set_sd(1.0), input_sqrt);
		return scalard{ result };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal square root of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_sqrt_reciprocal(double input) RTM_NO_EXCEPT
	{
		return 1.0 / scalar_sqrt(input);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_reciprocal(scalard_arg0 input) RTM_NO_EXCEPT
	{
		return scalard{ _mm_div_sd(_mm_set1_pd(1.0), input.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_reciprocal(double input) RTM_NO_EXCEPT
	{
		return 1.0 / input;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the addition of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_add(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_add_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the addition of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_add(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs + rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the subtraction of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_sub(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_sub_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the subtraction of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_sub(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs - rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_mul(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_mul_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_mul(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs * rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the division of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_div(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_div_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the division of the two scalar inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_div(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs / rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication/addition of the three inputs: s2 + (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_mul_add(scalard_arg0 s0, scalard_arg1 s1, scalard_arg2 s2) RTM_NO_EXCEPT
	{
		return scalard{ _mm_add_sd(_mm_mul_sd(s0.value, s1.value), s2.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the multiplication/addition of the three inputs: s2 + (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_mul_add(double s0, double s1, double s2) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_neg_mul_sub(scalard_arg0 s0, scalard_arg1 s1, scalard_arg2 s2) RTM_NO_EXCEPT
	{
		return scalard{ _mm_sub_sd(s2.value, _mm_mul_sd(s0.value, s1.value)) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the negative multiplication/subtraction of the three inputs: -((s0 * s1) - s2)
	// This is mathematically equivalent to: s2 - (s0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_neg_mul_sub(double s0, double s1, double s2) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_lerp(scalard_arg0 start, scalard_arg1 end, scalard_arg2 alpha) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_lerp(double start, double end, double alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		return scalar_mul_add(end, alpha, scalar_neg_mul_sub(start, alpha, start));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_min(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_min_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_min(double left, double right) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtsd_f64(_mm_min_sd(_mm_set1_pd(left), _mm_set1_pd(right)));
#else
		return (std::min)(left, right);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the largest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_max(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return scalard{ _mm_max_sd(lhs.value, rhs.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest of the two inputs.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_max(double left, double right) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cvtsd_f64(_mm_max_sd(_mm_set1_pd(left), _mm_set1_pd(right)));
#else
		return (std::max)(left, right);
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are equal, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_equal(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comieq_sd(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are equal, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool RTM_SIMD_CALL scalar_equal(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs == rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs < rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_lower_than(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comilt_sd(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs < rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool RTM_SIMD_CALL scalar_lower_than(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs < rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs <= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_lower_equal(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comile_sd(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs <= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool RTM_SIMD_CALL scalar_lower_equal(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs <= rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs > rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_greater_than(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comigt_sd(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs > rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool RTM_SIMD_CALL scalar_greater_than(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs > rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs >= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_greater_equal(scalard_arg0 lhs, scalard_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_comige_sd(lhs.value, rhs.value) != 0;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if lhs >= rhs, false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool RTM_SIMD_CALL scalar_greater_equal(double lhs, double rhs) RTM_NO_EXCEPT
	{
		return lhs >= rhs;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are nearly equal, otherwise false: abs(lhs - rhs) <= threshold
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_near_equal(scalard_arg0 lhs, scalard_arg1 rhs, scalard_arg2 threshold) RTM_NO_EXCEPT
	{
		return scalar_lower_equal(scalar_abs(scalar_sub(lhs, rhs)), threshold);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if both inputs are nearly equal, otherwise false: abs(lhs - rhs) <= threshold
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_near_equal(double lhs, double rhs, double threshold) RTM_NO_EXCEPT
	{
		return scalar_abs(lhs - rhs) <= threshold;
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input is finite (not NaN or Inf), false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_is_finite(scalard_arg0 input) RTM_NO_EXCEPT
	{
		return check_finite(scalar_cast(input));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input is finite (not NaN or Inf), false otherwise.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL scalar_is_finite(double input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_round_symmetric(scalard_arg0 input) RTM_NO_EXCEPT
	{
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

#if defined(RTM_SSE4_INTRINSICS)
		__m128d is_positive = _mm_cmpge_sd(input.value, _mm_setzero_pd());

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign = _mm_andnot_pd(is_positive, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128d bias = _mm_or_pd(sign, _mm_set1_pd(0.5));
		__m128d biased_input = _mm_add_sd(input.value, bias);

		__m128d floored = _mm_floor_sd(biased_input, biased_input);
		__m128d ceiled = _mm_ceil_sd(biased_input, biased_input);

		__m128d result = RTM_VECTOR2D_SELECT(is_positive, floored, ceiled);
		return scalard{ result };
#else
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input = _mm_and_pd(input.value, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large = _mm_cmpge_sd(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan = _mm_cmpneq_sd(input.value, input.value);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input = _mm_or_pd(is_input_large, is_nan);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign = _mm_and_pd(input.value, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128d bias = _mm_or_pd(sign, _mm_set1_pd(0.5));
		__m128d biased_input = _mm_add_sd(input.value, bias);

		// Convert to an integer with truncation and back, this rounds towards zero.
		__m128d integer_part = _mm_cvtepi32_pd(_mm_cvttpd_epi32(biased_input));

		__m128d result = _mm_or_pd(_mm_and_pd(use_original_input, input.value), _mm_andnot_pd(use_original_input, integer_part));

		return scalard{ result };
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_round_symmetric(double input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_round_symmetric(scalar_set(input)));
#else
		return input >= 0.0 ? scalar_floor(input + 0.5) : scalar_ceil(input - 0.5);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_round_bankers(scalard_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return scalard{ _mm_round_sd(input.value, input.value, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) };
#else
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign = _mm_and_pd(input.value, sign_mask);

		// We add the largest integer that a 64 bit floating point number can represent and subtract it afterwards.
		// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
		// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
		// This has the effect of removing the fractional part while simultaneously rounding.
		// Use the same sign as the input value to make sure we handle positive and negative values.
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52
		__m128d truncating_offset = _mm_or_pd(sign, fractional_limit);
		__m128d integer_part = _mm_sub_sd(_mm_add_sd(input.value, truncating_offset), truncating_offset);

		__m128d abs_input = _mm_and_pd(input.value, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large = _mm_cmpge_sd(abs_input, fractional_limit);
		__m128d result = _mm_or_pd(_mm_and_pd(is_input_large, input.value), _mm_andnot_pd(is_input_large, integer_part));

		return scalard{ result };
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_round_bankers(double input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalar_cast(scalar_round_bankers(scalar_set(input)));
#else
		if (!scalar_is_finite(input))
			return input;

		int64_t whole = static_cast<int64_t>(input);
		double whole_f = static_cast<double>(whole);
		double remainder = scalar_abs(input - whole_f);
		if (remainder < 0.5)
			return whole_f;
		if (remainder > 0.5)
			return input >= 0.0 ? (whole_f + 1.0) : (whole_f - 1.0);

		if ((whole % 2) == 0)
			return whole_f;
		else
			return input >= 0.0 ? (whole_f + 1.0) : (whole_f - 1.0);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the fractional part of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_fraction(double value) RTM_NO_EXCEPT
	{
		return value - scalar_floor(value);
	}

	//////////////////////////////////////////////////////////////////////////
	// Safely casts an integral input into a float64 output.
	//////////////////////////////////////////////////////////////////////////
	template<typename SrcIntegralType>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE double RTM_SIMD_CALL scalar_safe_to_double(SrcIntegralType input) RTM_NO_EXCEPT
	{
		double input_f = double(input);
		RTM_ASSERT(SrcIntegralType(input_f) == input, "Conversion to double would result in truncation");
		return input_f;
	}

	//////////////////////////////////////////////////////////////////////////
	// Trigonometric functions
	//////////////////////////////////////////////////////////////////////////

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_sin(scalard_arg0 angle) RTM_NO_EXCEPT
	{
		return scalar_set(std::sin(scalar_cast(angle)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_sin(double angle) RTM_NO_EXCEPT
	{
		return std::sin(angle);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_cos(scalard_arg0 angle) RTM_NO_EXCEPT
	{
		return scalar_set(std::cos(scalar_cast(angle)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_cos(double angle) RTM_NO_EXCEPT
	{
		return std::cos(angle);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL scalar_sincos(scalard_arg0 angle) RTM_NO_EXCEPT
	{
		scalard sin_ = scalar_sin(angle);
		scalard cos_ = scalar_cos(angle);

		__m128d xy = _mm_unpacklo_pd(sin_.value, cos_.value);
		return vector4d{ xy, xy };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	// The result's [x] component contains sin(angle).
	// The result's [y] component contains cos(angle).
	// [zw] are undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL scalar_sincos(double angle) RTM_NO_EXCEPT
	{
		scalard angle_ = scalar_set(angle);
		scalard sin_ = scalar_sin(angle_);
		scalard cos_ = scalar_cos(angle_);

#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_unpacklo_pd(sin_.value, cos_.value);
		return vector4d{ xy, xy };
#else
		return vector4d{ sin_, cos_, sin_, cos_ };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns both sine and cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void RTM_SIMD_CALL scalar_sincos(double angle, double& out_sin, double& out_cos) RTM_NO_EXCEPT
	{
		out_sin = scalar_sin(angle);
		out_cos = scalar_cos(angle);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_asin(scalard_arg0 value) RTM_NO_EXCEPT
	{
		return scalar_set(std::asin(scalar_cast(value)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_asin(double value) RTM_NO_EXCEPT
	{
		return std::asin(value);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_acos(scalard_arg0 value) RTM_NO_EXCEPT
	{
		return scalar_set(std::acos(scalar_cast(value)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_acos(double value) RTM_NO_EXCEPT
	{
		return std::acos(value);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_tan(scalard_arg0 angle) RTM_NO_EXCEPT
	{
		return scalar_set(std::tan(scalar_cast(angle)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_tan(double angle) RTM_NO_EXCEPT
	{
		return std::tan(angle);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in. See scalar_atan2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_atan(scalard_arg0 value) RTM_NO_EXCEPT
	{
		return scalar_set(std::atan(scalar_cast(value)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in. See scalar_atan2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_atan(double value) RTM_NO_EXCEPT
	{
		return std::atan(value);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL scalar_atan2(scalard_arg0 y, scalard_arg1 x) RTM_NO_EXCEPT
	{
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

		const __m128d zero = _mm_setzero_pd();
		__m128d is_x_zero = _mm_cmpeq_sd(x.value, zero);
		__m128d is_y_zero = _mm_cmpeq_sd(y.value, zero);
		__m128d inputs_are_zero = _mm_and_pd(is_x_zero, is_y_zero);

		__m128d is_x_positive = _mm_cmpgt_sd(x.value, zero);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d y_sign = _mm_and_pd(y.value, sign_mask);

		// If X == 0.0, our offset is PI/2 otherwise it is PI both with the sign of Y
		__m128d half_pi = _mm_set1_pd(constants::half_pi());
		__m128d pi = _mm_set1_pd(constants::pi());
		__m128d offset = _mm_or_pd(_mm_and_pd(is_x_zero, half_pi), _mm_andnot_pd(is_x_zero, pi));
		offset = _mm_or_pd(offset, y_sign);

		// If X > 0.0, our offset is 0.0
		offset = _mm_andnot_pd(is_x_positive, offset);

		// If X == 0.0 and Y == 0.0, our offset is 0.0
		offset = _mm_andnot_pd(inputs_are_zero, offset);

		__m128d angle = _mm_div_sd(y.value, x.value);
		__m128d value = scalar_atan(scalard{ angle }).value;

		// If X == 0.0, our value is 0.0 otherwise it is atan(y/x)
		value = _mm_or_pd(_mm_and_pd(is_x_zero, zero), _mm_andnot_pd(is_x_zero, value));

		// If X == 0.0 and Y == 0.0, our value is 0.0
		value = _mm_andnot_pd(inputs_are_zero, value);

		__m128d result = _mm_add_sd(value, offset);
		return scalard{ result };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL scalar_atan2(double y, double x) RTM_NO_EXCEPT
	{
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

		if (x == 0.0)
		{
			if (y == 0.0)
				return 0.0;

			return rtm_impl::copysign(constants::half_pi(), y);
		}

		double value = scalar_atan(y / x);
		if (x > 0.0)
			return value;

		double offset = rtm_impl::copysign(constants::pi(), y);
		return value + offset;
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts degrees into radians.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_deg_to_rad(double deg) RTM_NO_EXCEPT
	{
		return deg * constants::pi_div_one_eighty();
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts radians into degrees.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL scalar_rad_to_deg(double rad) RTM_NO_EXCEPT
	{
		return rad * constants::one_eighty_div_pi();
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
