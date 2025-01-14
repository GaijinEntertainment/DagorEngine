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

#include "rtm/math.h"
#include "rtm/scalard.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/memory_utils.h"
#include "rtm/impl/quat_common.h"

#include <limits>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Setters, getters, and casts
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned quaternion from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_load(const double* input) RTM_NO_EXCEPT
	{
		return quat_set(input[0], input[1], input[2], input[3]);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned quat from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_load(const float4d* input) RTM_NO_EXCEPT
	{
		return quat_set(input->x, input->y, input->z, input->w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a vector4 to a quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL vector_to_quat(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ input.xy, input.zw };
#else
		return quatd{ input.x, input.y, input.z, input.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a quaternion float32 variant to a float64 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_cast(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ _mm_cvtps_pd(input), _mm_cvtps_pd(_mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 2, 3, 2))) };
#elif defined(RTM_NEON_INTRINSICS)
		return quatd{ double(vgetq_lane_f32(input, 0)), double(vgetq_lane_f32(input, 1)), double(vgetq_lane_f32(input, 2)), double(vgetq_lane_f32(input, 3)) };
#else
		return quatd{ double(input.x), double(input.y), double(input.z), double(input.w) };
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_get_x
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(input.xy);
#else
				return input.x;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ input.xy };
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [x] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_get_x RTM_SIMD_CALL quat_get_x(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_get_x{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [x] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_get_x_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ input.xy };
#else
		return quat_get_x(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_get_y
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(_mm_shuffle_pd(input.xy, input.xy, 1));
#else
				return input.y;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ _mm_shuffle_pd(input.xy, input.xy, 1) };
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [y] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_get_y RTM_SIMD_CALL quat_get_y(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_get_y{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [y] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_get_y_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_shuffle_pd(input.xy, input.xy, 1) };
#else
		return quat_get_y(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_get_z
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(input.zw);
#else
				return input.z;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ input.zw };
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [z] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_get_z RTM_SIMD_CALL quat_get_z(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_get_z{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [z] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_get_z_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ input.zw };
#else
		return quat_get_z(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_get_w
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(_mm_shuffle_pd(input.zw, input.zw, 1));
#else
				return input.w;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ _mm_shuffle_pd(input.zw, input.zw, 1) };
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [w] component (imaginary part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_get_w RTM_SIMD_CALL quat_get_w(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_get_w{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [w] component (imaginary part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_get_w_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_shuffle_pd(input.zw, input.zw, 1) };
#else
		return quat_get_w(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [x] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_x(quatd_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ _mm_move_sd(input.xy, _mm_set_sd(lane_value)), input.zw };
#else
		return quatd{ lane_value, input.y, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [x] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_x(quatd_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return quatd{ _mm_move_sd(input.xy, lane_value.value), input.zw };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [y] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_y(quatd_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ _mm_shuffle_pd(input.xy, _mm_set_sd(lane_value), 0), input.zw };
#else
		return quatd{ input.x, lane_value, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [y] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_y(quatd_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return quatd{ _mm_shuffle_pd(input.xy, lane_value.value, 0), input.zw };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [z] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_z(quatd_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ input.xy, _mm_move_sd(input.zw, _mm_set_sd(lane_value)) };
#else
		return quatd{ input.x, input.y, lane_value, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [z] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_z(quatd_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return quatd{ input.xy, _mm_move_sd(input.zw, lane_value.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [w] component (imaginary part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_w(quatd_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ input.xy, _mm_shuffle_pd(input.zw, _mm_set_sd(lane_value), 0) };
#else
		return quatd{ input.x, input.y, input.z, lane_value };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [w] component (imaginary part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set_w(quatd_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return quatd{ input.xy, _mm_shuffle_pd(input.zw, lane_value.value, 0) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Writes a quaternion to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL quat_store(quatd_arg0 input, double* output) RTM_NO_EXCEPT
	{
		output[0] = quat_get_x(input);
		output[1] = quat_get_y(input);
		output[2] = quat_get_z(input);
		output[3] = quat_get_w(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a quaternion to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL quat_store(quatd_arg0 input, float4d* output) RTM_NO_EXCEPT
	{
		output->x = quat_get_x(input);
		output->y = quat_get_y(input);
		output->z = quat_get_z(input);
		output->w = quat_get_w(input);
	}



	//////////////////////////////////////////////////////////////////////////
	// Arithmetic
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion conjugate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_conjugate(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return quat_set(-quat_get_x(input), -quat_get_y(input), -quat_get_z(input), quat_get_w(input));
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two quaternions.
	// Note that due to floating point rounding, the result might not be perfectly normalized.
	// Multiplication order is as follow: local_to_world = quat_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_mul(quatd_arg0 lhs, quatd_arg1 rhs) RTM_NO_EXCEPT
	{
		double lhs_x = quat_get_x(lhs);
		double lhs_y = quat_get_y(lhs);
		double lhs_z = quat_get_z(lhs);
		double lhs_w = quat_get_w(lhs);

		double rhs_x = quat_get_x(rhs);
		double rhs_y = quat_get_y(rhs);
		double rhs_z = quat_get_z(rhs);
		double rhs_w = quat_get_w(rhs);

		double x = (rhs_w * lhs_x) + (rhs_x * lhs_w) + (rhs_y * lhs_z) - (rhs_z * lhs_y);
		double y = (rhs_w * lhs_y) - (rhs_x * lhs_z) + (rhs_y * lhs_w) + (rhs_z * lhs_x);
		double z = (rhs_w * lhs_z) + (rhs_x * lhs_y) - (rhs_y * lhs_x) + (rhs_z * lhs_w);
		double w = (rhs_w * lhs_w) - (rhs_x * lhs_x) - (rhs_y * lhs_y) - (rhs_z * lhs_z);

		return quat_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a quaternion and a 3D vector, rotating it.
	// Multiplication order is as follow: world_position = quat_mul_vector3(local_vector, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL quat_mul_vector3(vector4d_arg0 vector, quatd_arg1 rotation) RTM_NO_EXCEPT
	{
		quatd vector_quat = quat_set_w(vector_to_quat(vector), 0.0);
		quatd inv_rotation = quat_conjugate(rotation);
		return quat_to_vector(quat_mul(quat_mul(inv_rotation, vector_quat), rotation));
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_dot
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK inline RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard lhs_x = quat_get_x_as_scalar(lhs);
				const scalard lhs_y = quat_get_y_as_scalar(lhs);
				const scalard lhs_z = quat_get_z_as_scalar(lhs);
				const scalard lhs_w = quat_get_w_as_scalar(lhs);
				const scalard rhs_x = quat_get_x_as_scalar(rhs);
				const scalard rhs_y = quat_get_y_as_scalar(rhs);
				const scalard rhs_z = quat_get_z_as_scalar(rhs);
				const scalard rhs_w = quat_get_w_as_scalar(rhs);
				const scalard xx = scalar_mul(lhs_x, rhs_x);
				const scalard yy = scalar_mul(lhs_y, rhs_y);
				const scalard zz = scalar_mul(lhs_z, rhs_z);
				const scalard ww = scalar_mul(lhs_w, rhs_w);
				return scalar_cast(scalar_add(scalar_add(xx, yy), scalar_add(zz, ww)));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK inline RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard lhs_x = quat_get_x_as_scalar(lhs);
				const scalard lhs_y = quat_get_y_as_scalar(lhs);
				const scalard lhs_z = quat_get_z_as_scalar(lhs);
				const scalard lhs_w = quat_get_w_as_scalar(lhs);
				const scalard rhs_x = quat_get_x_as_scalar(rhs);
				const scalard rhs_y = quat_get_y_as_scalar(rhs);
				const scalard rhs_z = quat_get_z_as_scalar(rhs);
				const scalard rhs_w = quat_get_w_as_scalar(rhs);
				const scalard xx = scalar_mul(lhs_x, rhs_x);
				const scalard yy = scalar_mul(lhs_y, rhs_y);
				const scalard zz = scalar_mul(lhs_z, rhs_z);
				const scalard ww = scalar_mul(lhs_w, rhs_w);
				return scalar_add(scalar_add(xx, yy), scalar_add(zz, ww));
			}
#endif

			quatd lhs;
			quatd rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Quaternion dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK constexpr rtm_impl::quatd_quat_dot RTM_SIMD_CALL quat_dot(quatd_arg0 lhs, quatd_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_dot{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// Quaternion dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL quat_dot_as_scalar(quatd_arg0 lhs, quatd_arg1 rhs) RTM_NO_EXCEPT
	{
		const scalard lhs_x = quat_get_x_as_scalar(lhs);
		const scalard lhs_y = quat_get_y_as_scalar(lhs);
		const scalard lhs_z = quat_get_z_as_scalar(lhs);
		const scalard lhs_w = quat_get_w_as_scalar(lhs);
		const scalard rhs_x = quat_get_x_as_scalar(rhs);
		const scalard rhs_y = quat_get_y_as_scalar(rhs);
		const scalard rhs_z = quat_get_z_as_scalar(rhs);
		const scalard rhs_w = quat_get_w_as_scalar(rhs);
		const scalard xx = scalar_mul(lhs_x, rhs_x);
		const scalard yy = scalar_mul(lhs_y, rhs_y);
		const scalard zz = scalar_mul(lhs_z, rhs_z);
		const scalard ww = scalar_mul(lhs_w, rhs_w);
		return scalar_add(scalar_add(xx, yy), scalar_add(zz, ww));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK constexpr rtm_impl::quatd_quat_dot RTM_SIMD_CALL quat_length_squared(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_dot{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL quat_length_squared_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return quat_dot_as_scalar(input, input);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_length
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = quat_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = quat_length_squared_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_length RTM_SIMD_CALL quat_length(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_length{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_length_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = quat_length_squared_as_scalar(input);
		return scalar_sqrt(len_sq);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatd_quat_length_reciprocal
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = quat_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = quat_length_squared_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			quatd input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatd_quat_length_reciprocal RTM_SIMD_CALL quat_length_reciprocal(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatd_quat_length_reciprocal{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL quat_length_reciprocal_as_scalar(quatd_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = quat_length_squared_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized quaternion.
	// Note that if the input quaternion is invalid (pure zero or with NaN/Inf),
	// the result is undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_normalize(quatd_arg0 input) RTM_NO_EXCEPT
	{
		// TODO: Use high precision recip sqrt function and vector_mul
		double length = quat_length(input);
		//float length_recip = quat_length_reciprocal(input);
		vector4d input_vector = quat_to_vector(input);
		//return vector_to_quat(vector_mul(input_vector, length_recip));
		return vector_to_quat(vector_div(input_vector, vector_set(length)));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized quaternion using a deterministic algorithm.
	// This ensures that for a given input, the output will be identical on all
	// platforms that implement IEEE-754. This can be slower than `quat_normalize`.
	// Note that if the input quaternion is invalid (pure zero or with NaN/Inf),
	// the result is undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_normalize_deterministic(quatd_arg0 input) RTM_NO_EXCEPT
	{
		vector4d inputv = quat_to_vector(input);

		// Multiply once and retrieve floats, can't use scalarf because we need to use volatile
		// volatile will force a roundtrip to memory and should prevent re-ordering as well as
		// other optimizations such as FMA.
		vector4d input_sq = vector_mul(inputv, inputv);
		volatile double x_sq = vector_get_x(input_sq);
		volatile double y_sq = vector_get_y(input_sq);
		volatile double z_sq = vector_get_z(input_sq);
		volatile double w_sq = vector_get_w(input_sq);
		volatile double sum_xy_sq = x_sq + y_sq;
		volatile double sum_zw_sq = z_sq + w_sq;
		double len_sq = sum_xy_sq + sum_zw_sq;

		// We add volatile to ensure rsqrt or similar isn't used
		volatile double len = scalar_sqrt(len_sq);
		return vector_to_quat(vector_div(inputv, vector_set(len)));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation between start and end for a given alpha value.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when 'alpha' is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// Note that if 'start' and 'end' are on the opposite ends of the hypersphere, 'end' is negated
	// before we interpolate. As such, when 'alpha' is 1.0, either 'end' or its negated equivalent
	// is returned. Furthermore, if 'start' and 'end' aren't exactly normalized, the result might
	// not match exactly when 'alpha' is 0.0 or 1.0 because we normalize the resulting quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_lerp(quatd_arg0 start, quatd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		// To ensure we take the shortest path, we apply a bias if the dot product is negative
		vector4d start_vector = quat_to_vector(start);
		vector4d end_vector = quat_to_vector(end);
		double dot = vector_dot(start_vector, end_vector);
		double bias = dot >= 0.0 ? 1.0 : -1.0;
		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4d value = vector_mul_add(vector_mul(end_vector, bias), alpha, vector_neg_mul_sub(start_vector, alpha, start_vector));
		return quat_normalize(vector_to_quat(value));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation between start and end for a given alpha value.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when 'alpha' is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// Note that if 'start' and 'end' are on the opposite ends of the hypersphere, 'end' is negated
	// before we interpolate. As such, when 'alpha' is 1.0, either 'end' or its negated equivalent
	// is returned. Furthermore, if 'start' and 'end' aren't exactly normalized, the result might
	// not match exactly when 'alpha' is 0.0 or 1.0 because we normalize the resulting quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_lerp(quatd_arg0 start, quatd_arg1 end, scalard_arg4 alpha) RTM_NO_EXCEPT
	{
		// To ensure we take the shortest path, we apply a bias if the dot product is negative
		vector4d start_vector = quat_to_vector(start);
		vector4d end_vector = quat_to_vector(end);
		double dot = vector_dot(start_vector, end_vector);
		double bias = dot >= 0.0 ? 1.0 : -1.0;
		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4d alpha_v = vector_set(alpha);
		vector4d value = vector_mul_add(vector_mul(end_vector, bias), alpha_v, vector_neg_mul_sub(start_vector, alpha_v, start_vector));
		return quat_normalize(vector_to_quat(value));
	}
#endif

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the spherical interpolation between start and end for a given alpha value.
	// See: https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	// Perhaps try this someday: http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_slerp(quatd_arg0 start, quatd_arg1 end, scalard_arg4 alpha) RTM_NO_EXCEPT
	{
		vector4d start_v = quat_to_vector(start);
		vector4d end_v = quat_to_vector(end);

		vector4d cos_half_angle_v = vector_dot_as_vector(start_v, end_v);
		mask4d is_angle_negative = vector_less_than(cos_half_angle_v, vector_zero());

		// If the two input quaternions aren't on the same half of the hypersphere, flip one and the angle sign
		end_v = vector_select(is_angle_negative, vector_neg(end_v), end_v);
		cos_half_angle_v = vector_select(is_angle_negative, vector_neg(cos_half_angle_v), cos_half_angle_v);

		// Clamp our half angle cosine
		cos_half_angle_v = vector_clamp(cos_half_angle_v, vector_set(-1.0), vector_set(1.0));

		scalard cos_half_angle = vector_get_x_as_scalar(cos_half_angle_v);
		scalard half_angle = scalar_acos(cos_half_angle);
		scalard sin_half_angle = scalar_sqrt(scalar_sub(scalar_set(1.0), scalar_mul(cos_half_angle, cos_half_angle)));
		scalard inv_sin_half_angle = scalar_reciprocal(sin_half_angle);

		scalard start_contribution_angle = scalar_mul(scalar_sub(scalar_set(1.0), alpha), half_angle);
		scalard end_contribution_angle = scalar_mul(alpha, half_angle);
		vector4d contribution_angles = vector_set(start_contribution_angle, end_contribution_angle, start_contribution_angle, end_contribution_angle);
		vector4d contributions = vector_mul(vector_sin(contribution_angles), inv_sin_half_angle);
		vector4d start_contribution = vector_dup_x(contributions);
		vector4d end_contribution = vector_dup_y(contributions);

		vector4d result = vector_add(vector_mul(start_v, start_contribution), vector_mul(end_v, end_contribution));
		return vector_to_quat(result);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the spherical interpolation between start and end for a given alpha value.
	// See: https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	// Perhaps try this someday: http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_slerp(quatd_arg0 start, quatd_arg0 end, double alpha) RTM_NO_EXCEPT
	{
		vector4d start_v = quat_to_vector(start);
		vector4d end_v = quat_to_vector(end);
		scalard alpha_s = scalar_set(alpha);

		vector4d cos_half_angle_v = vector_dot_as_vector(start_v, end_v);
		mask4d is_angle_negative = vector_less_than(cos_half_angle_v, vector_zero());

		// If the two input quaternions aren't on the same half of the hypersphere, flip one and the angle sign
		end_v = vector_select(is_angle_negative, vector_neg(end_v), end_v);
		cos_half_angle_v = vector_select(is_angle_negative, vector_neg(cos_half_angle_v), cos_half_angle_v);

		// Clamp our half angle cosine
		cos_half_angle_v = vector_clamp(cos_half_angle_v, vector_set(-1.0), vector_set(1.0));

		scalard cos_half_angle = vector_get_x_as_scalar(cos_half_angle_v);
		scalard half_angle = scalar_acos(cos_half_angle);
		scalard sin_half_angle = scalar_sqrt(scalar_sub(scalar_set(1.0), scalar_mul(cos_half_angle, cos_half_angle)));
		scalard inv_sin_half_angle = scalar_reciprocal(sin_half_angle);

		scalard start_contribution_angle = scalar_mul(scalar_sub(scalar_set(1.0), alpha_s), half_angle);
		scalard end_contribution_angle = scalar_mul(alpha_s, half_angle);
		vector4d contribution_angles = vector_set(start_contribution_angle, end_contribution_angle, start_contribution_angle, end_contribution_angle);
		vector4d contributions = vector_mul(vector_sin(contribution_angles), inv_sin_half_angle);
		vector4d start_contribution = vector_dup_x(contributions);
		vector4d end_contribution = vector_dup_y(contributions);

		vector4d result = vector_add(vector_mul(start_v, start_contribution), vector_mul(end_v, end_contribution));
		return vector_to_quat(result);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a component wise negated quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_neg(quatd_arg0 input) RTM_NO_EXCEPT
	{
		return vector_to_quat(vector_mul(quat_to_vector(input), -1.0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion logarithm for a 3D rotation.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_rotation_log(quatd_arg0 input) RTM_NO_EXCEPT
	{
		// The logarithm equation is as follow:
		// log(q).xyz = (q.xyz / ||q.xyz||) * acos(q.w / ||q||)
		// log(q).w = ln(||q||)
		//
		// If our quaternion is normalized (a rotation), our output W is always 0.0 since ln(1) = 0.0
		// Our XYZ simplifies as well down to: normalize(q.xyz) * acos(q.w)
		// The length of our input XYZ can be calculated either as sqrt(dot(q.xyz, q.xyz)) or
		// by taking the sine of the quaternion half angle with sin(acos(q.w))
		//
		// If our input rotation is near the identity, we cannot calculate the output XYZ since we would
		// divide by zero. If this happens we return a quaternion near zero which should reconstruct as the
		// identity with quat_rotation_exp(..).
		//
		// If our quaternion isn't normalized, more math is required

		const vector4d input_v = quat_to_vector(input);
		const scalard input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0), scalar_set(1.0));
		const scalard half_angle = scalar_acos(input_w);
		const scalard xyz_inv_len = vector_length_reciprocal3_as_scalar(input_v);
		vector4d result_xyz = vector_mul(input_v, scalar_mul(xyz_inv_len, half_angle));

		// If we are near the identity, xyz will be set to our input xyz which should be near zero
		// For a true identity input, we'll output zero
		const mask4d is_input_near_identity = vector_greater_than(vector_set(input_w), vector_set(1.0 - 1.0E-8));
		result_xyz = vector_select(is_input_near_identity, input_v, result_xyz);

		return vector_to_quat(vector_set_w(result_xyz, 0.0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion exponential for a 3D rotation.
	// The result might not be fully normalized if the input is near zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_rotation_exp(quatd_arg0 input) RTM_NO_EXCEPT
	{
		// The exponential equation is as follow:
		// exp(q).xyz = exp(q.w) * (q.xyz / ||q.xyz||) * sin(||q.xyz||)
		// exp(q).w = exp(q.w) * cos(||q.xyz||)
		//
		// If our output (normalized) quaternion is to represent a rotation, its logarithm passed as input has a W component equal to 0.0
		// Furthermore our input XYZ length is equal to our rotation half angle
		// Since exp(0.0) = 1.0, our equation simplifies as follow:
		// exp(q).xyz = (q.xyz / ||q.xyz||) * sin(||q.xyz||) = normalize(q.xyz) * sin(||q.xyz||)
		// exp(q).w = cos(||q.xyz||)
		//
		// If our half angle is 0.0, the whole input is 0,0,0,0 and the result is undefined since we can use any rotation axis.
		// When this happens, we return the approximated identity.
		// This is easily achieved because cos(||q.xyz||) = cos(0.0) = 1.0
		//
		// If our output quaternion does not represent a rotation, more math is required

		const vector4d input_v = quat_to_vector(input);
		const scalard input_len = vector_length3_as_scalar(input_v);
		const vector4d input_len_v = vector_set(input_len);
		const vector4d input_normalized = vector_div(input_v, input_len_v);
		const vector4d sincos = scalar_sincos(input_len);

		vector4d result_xyz = vector_mul(input_normalized, vector_dup_x(sincos));

		// If we are near zero, xyz will be set to our input xyz which should be near zero
		// For a true zero input, we'll output the identity
		const mask4d is_input_near_zero = vector_less_than(input_len_v, vector_set(1.0E-8));
		result_xyz = vector_select(is_input_near_zero, input_v, result_xyz);

		const vector4d result_w = vector_dup_y(sincos);
		const vector4d result = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(result_xyz, result_w);
		return vector_to_quat(result);
	}



	//////////////////////////////////////////////////////////////////////////
	// Conversion to/from axis/angle/euler
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation axis and rotation angle that make up the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void RTM_SIMD_CALL quat_to_axis_angle(quatd_arg0 input, vector4d& out_axis, double& out_angle) RTM_NO_EXCEPT
	{
		constexpr double epsilon = 1.0E-8;
		constexpr double epsilon_squared = epsilon * epsilon;

		const scalard input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0), scalar_set(1.0));
		out_angle = scalar_cast(scalar_acos(input_w)) * 2.0;

		const double scale_sq = scalar_max(1.0 - quat_get_w(input) * quat_get_w(input), 0.0);
		out_axis = scale_sq >= epsilon_squared ? vector_mul(quat_to_vector(input), vector_set(scalar_sqrt_reciprocal(scale_sq))) : vector_set(1.0, 0.0, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation axis part of the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL quat_get_axis(quatd_arg0 input) RTM_NO_EXCEPT
	{
		constexpr double epsilon = 1.0E-8;
		constexpr double epsilon_squared = epsilon * epsilon;

		const double scale_sq = scalar_max(1.0 - quat_get_w(input) * quat_get_w(input), 0.0);
		return scale_sq >= epsilon_squared ? vector_mul(quat_to_vector(input), vector_set(scalar_sqrt_reciprocal(scale_sq))) : vector_set(1.0, 0.0, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation angle part of the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline double RTM_SIMD_CALL quat_get_angle(quatd_arg0 input) RTM_NO_EXCEPT
	{
		const scalard input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0), scalar_set(1.0));
		return scalar_cast(scalar_acos(input_w)) * 2.0;
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from a rotation axis and a rotation angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_from_axis_angle(vector4d_arg0 axis, double angle) RTM_NO_EXCEPT
	{
		vector4d sincos_ = scalar_sincos(0.5 * angle);
		vector4d sin_ = vector_dup_x(sincos_);
		scalard cos_ = vector_get_y_as_scalar(sincos_);

		return vector_to_quat(vector_set_w(vector_mul(sin_, axis), cos_));
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from Euler Pitch/Yaw/Roll angles.
	// Pitch is around the Y axis (right)
	// Yaw is around the Z axis (up)
	// Roll is around the X axis (forward)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_from_euler(double pitch, double yaw, double roll) RTM_NO_EXCEPT
	{
		double sp;
		double sy;
		double sr;
		double cp;
		double cy;
		double cr;

		scalar_sincos(pitch * 0.5, sp, cp);
		scalar_sincos(yaw * 0.5, sy, cy);
		scalar_sincos(roll * 0.5, sr, cr);

		return quat_set(cr * sp * sy - sr * cp * cy,
			-cr * sp * cy - sr * cp * sy,
			cr * cp * sy - sr * sp * cy,
			cr * cp * cy + sr * sp * sy);
	}



	//////////////////////////////////////////////////////////////////////////
	// Comparisons and masking
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion does not contain any NaN or Inf, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline bool RTM_SIMD_CALL quat_is_finite(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));

		const __m128d infinity = _mm_set1_pd(std::numeric_limits<double>::infinity());
		__m128d is_infinity_xy = _mm_cmpeq_pd(abs_input_xy, infinity);
		__m128d is_infinity_zw = _mm_cmpeq_pd(abs_input_zw, infinity);

		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		__m128d is_not_finite_xy = _mm_or_pd(is_infinity_xy, is_nan_xy);
		__m128d is_not_finite_zw = _mm_or_pd(is_infinity_zw, is_nan_zw);
		__m128d is_not_finite = _mm_or_pd(is_not_finite_xy, is_not_finite_zw);
		return _mm_movemask_pd(is_not_finite) == 0;
#else
		return scalar_is_finite(quat_get_x(input)) && scalar_is_finite(quat_get_y(input)) && scalar_is_finite(quat_get_z(input)) && scalar_is_finite(quat_get_w(input));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion is normalized, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_is_normalized(quatd_arg0 input, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		double length_squared = quat_length_squared(input);
		return scalar_abs(length_squared - 1.0) <= threshold;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the two quaternions are equal component wise, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_are_equal(quatd_arg0 lhs, quatd_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_eq_pd) & _mm_movemask_pd(zw_eq_pd)) == 3;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the two quaternions are nearly equal component wise, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_near_equal(quatd_arg0 lhs, quatd_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_all_near_equal(quat_to_vector(lhs), quat_to_vector(rhs), threshold);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion is nearly equal to the identity quaternion
	// by comparing its rotation angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline bool RTM_SIMD_CALL quat_near_identity(quatd_arg0 input, double threshold_angle = 0.00284714461) RTM_NO_EXCEPT
	{
		// See the quatf version of quat_near_identity for details.
		const scalard input_w = quat_get_w_as_scalar(input);
		const scalard input_abs_w = scalar_min(scalar_abs(input_w), scalar_set(1.0));
		const double positive_w_angle = scalar_acos(scalar_cast(input_abs_w)) * 2.0;
		return positive_w_angle <= threshold_angle;
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	// Note that if the mask lanes are not all identical, the resulting quaternion
	// may not be normalized.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_select(mask4d_arg0 mask, quatd_arg1 if_true, quatd_arg2 if_false) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = RTM_VECTOR2D_SELECT(mask.xy, if_true.xy, if_false.xy);
		__m128d zw = RTM_VECTOR2D_SELECT(mask.zw, if_true.zw, if_false.zw);
		return quatd{ xy, zw };
#else
		return quatd{ rtm_impl::select(mask.x, if_true.x, if_false.x), rtm_impl::select(mask.y, if_true.y, if_false.y), rtm_impl::select(mask.z, if_true.z, if_false.z), rtm_impl::select(mask.w, if_true.w, if_false.w) };
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
