#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Realtime Math contributors
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
#include "rtm/matrix3x3d.h"
#include "rtm/matrix3x4d.h"
#include "rtm/quatd.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/matrix_affine_common.h"
#include "rtm/impl/qv_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Casts a QV transform float32 variant to a float64 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_cast(qvf_arg0 input) RTM_NO_EXCEPT
	{
		return qvd{ quat_cast(input.rotation), vector_cast(input.translation) };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a rotation 3x3 matrix into a QV transform.
	// The translation will be the identity.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvd RTM_SIMD_CALL qv_from_matrix(matrix3x3d_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = rtm_impl::quat_from_matrix(input.x_axis, input.y_axis, input.z_axis);
		const vector4d translation = vector_zero();
		return qvd{ rotation, translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a rotation 3x4 affine matrix into a QV transform.
	// If the matrix contains scale, it is stripped and lost.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvd RTM_SIMD_CALL qv_from_matrix(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		// See qvv_from_matrix for details, the process here is very similar
		// We still treat every axis independently as if they have unique scale values
		// to be able to handle ill-formed matrices with non-uniform scale.

		// Translation is always in the W axis
		const vector4d translation = input.w_axis;

		// First, we compute our absolute scale values
		const scalard x_scale = vector_length3_as_scalar(input.x_axis);
		const scalard y_scale = vector_length3_as_scalar(input.y_axis);
		const scalard z_scale = vector_length3_as_scalar(input.z_axis);
		const vector4d scale = vector_set(x_scale, y_scale, z_scale);

		// Grab a pointer to the first scale value, we'll index into it
		const double* scale_ptr = rtm_impl::bit_cast<const double*>(&scale);

		// Next, we find the largest scale components and sort them from largest to smallest
		component3 largest_scale_component;
		component3 second_largest_scale_component;
		component3 third_largest_scale_component;

		if (scalar_greater_equal(x_scale, y_scale))
		{
			// X >= Y
			if (scalar_greater_equal(y_scale, z_scale))
			{
				// X >= Y >= Z
				largest_scale_component = component3::x;
				second_largest_scale_component = component3::y;
				third_largest_scale_component = component3::z;
			}
			else
			{
				if (scalar_greater_equal(x_scale, z_scale))
				{
					// X >= Z >= Y
					largest_scale_component = component3::x;
					second_largest_scale_component = component3::z;
				}
				else
				{
					// Z >= X >= Y
					largest_scale_component = component3::z;
					second_largest_scale_component = component3::x;
				}

				third_largest_scale_component = component3::y;
			}
		}
		else
		{
			if (scalar_greater_equal(x_scale, z_scale))
			{
				// Y > X >= Z
				largest_scale_component = component3::y;
				second_largest_scale_component = component3::x;
				third_largest_scale_component = component3::z;
			}
			else
			{
				if (scalar_greater_equal(y_scale, z_scale))
				{
					// Y >= Z > X
					largest_scale_component = component3::y;
					second_largest_scale_component = component3::z;
				}
				else
				{
					// Z > Y > X
					largest_scale_component = component3::z;
					second_largest_scale_component = component3::y;
				}

				third_largest_scale_component = component3::x;
			}
		}

		// Cast to integer for array indexing
		const int32_t largest_scale_component_i = static_cast<int32_t>(largest_scale_component);
		const int32_t second_largest_scale_component_i = static_cast<int32_t>(second_largest_scale_component);
		const int32_t third_largest_scale_component_i = static_cast<int32_t>(third_largest_scale_component);

		const matrix3x3d identity3x3 = matrix_identity();

		// Copy the rotation part from our input
		matrix3x3d rotation = matrix_cast(input);

		// Grab a pointer to the first axis, we'll index into it
		vector4d* rotation_axes = &rotation.x_axis;

		// We use a threshold to test for zero because division with near zero values is imprecise
		// and might not yield a normalized result
		const double zero_scale_threshold = 1.0E-8;
		double largest_scale = scale_ptr[largest_scale_component_i];
		if (largest_scale < zero_scale_threshold)
		{
			// The largest scale value is zero which means all three are zero
			// We'll return the identity rotation since its value is not recoverable
			const quatd rotation_q = quat_identity();
			return qvd{ rotation_q, translation };
		}

		// Normalize the largest scale axis which is non-zero
		vector4d largest_scale_axis = rotation_axes[largest_scale_component_i];
		largest_scale_axis = vector_div(largest_scale_axis, vector_set(largest_scale));

		vector4d second_largest_scale_axis = rotation_axes[second_largest_scale_component_i];
		double second_largest_scale = scale_ptr[second_largest_scale_component_i];
		if (second_largest_scale < zero_scale_threshold)
		{
			// The second largest scale value is zero which means the two smallest axes are zero
			// We'll use the largest axis and build an orthogonal basis around it
			const vector4d largest_y_dot = vector_dot3_as_vector(largest_scale_axis, identity3x3.y_axis);
			const mask4d is_largest_parallel_to_y = vector_greater_equal(vector_abs(largest_y_dot), vector_set(1.0F - zero_scale_threshold));
			const vector4d orthogonal_axis = vector_select(is_largest_parallel_to_y, identity3x3.z_axis, identity3x3.y_axis);

			second_largest_scale_axis = vector_cross3(largest_scale_axis, orthogonal_axis);

			// Recompute the scale to ensure we properly normalize
			// Here, the second largest axis should alread be normalized
			second_largest_scale = vector_length3(second_largest_scale_axis);
		}

		// Normalize the second largest axis which is non-zero
		second_largest_scale_axis = vector_div(second_largest_scale_axis, vector_set(second_largest_scale));

		vector4d third_largest_scale_axis = rotation_axes[third_largest_scale_component_i];
		double third_largest_scale = scale_ptr[third_largest_scale_component_i];
		if (third_largest_scale < zero_scale_threshold)
		{
			// The third largest scale value is zero
			// We'll use the two larger axes to build an orthogonal basis
			third_largest_scale_axis = vector_cross3(largest_scale_axis, second_largest_scale_axis);

			// Recompute the scale to ensure we properly normalize
			// Here, the third largest axis should alread be normalized
			third_largest_scale = vector_length3(third_largest_scale_axis);
		}

		// Normalize the third largest axis which is non-zero
		third_largest_scale_axis = vector_div(third_largest_scale_axis, vector_set(third_largest_scale));

		// Set our new basis
		rotation_axes[largest_scale_component_i] = largest_scale_axis;
		rotation_axes[second_largest_scale_component_i] = second_largest_scale_axis;
		rotation_axes[third_largest_scale_component_i] = third_largest_scale_axis;

		// Now that we have built the ortho-normal basis part of our rotation, we check its winding
		const double determinant = scalar_cast(matrix_determinant(rotation));
		if (determinant < 0.0)
		{
			// Our winding is reversed meaning one or three scale axes contain reflection
			// We negate one axis to flip the winding to be able to find some valid rotation
			largest_scale_axis = vector_neg(largest_scale_axis);

			// Update our rotation basis
			rotation_axes[largest_scale_component_i] = largest_scale_axis;
		}

		// Build a quaternion from the ortho-normal basis we found
		const quatd rotation_q = rtm_impl::quat_from_matrix(rotation.x_axis, rotation.y_axis, rotation.z_axis);

		return qvd{ rotation_q, translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two QV transforms.
	// Multiplication order is as follow: local_to_world = qv_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvd RTM_SIMD_CALL qv_mul(qvd_arg0 lhs, qvd_arg1 rhs) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_mul(lhs.rotation, rhs.rotation);
		const vector4d translation = vector_add(quat_mul_vector3(lhs.translation, rhs.rotation), rhs.translation);
		return qv_set(rotation, translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a QV transform and a 3D point.
	// Multiplication order is as follow: world_position = qv_mul_point3(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL qv_mul_point3(vector4d_arg0 point, qvd_arg1 qv) RTM_NO_EXCEPT
	{
		return vector_add(quat_mul_vector3(point, qv.rotation), qv.translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QV transform.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvd RTM_SIMD_CALL qv_inverse(qvd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(input.translation, inv_rotation));
		return qv_set(inv_rotation, inv_translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a QV transforms with the rotation part normalized.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_normalize(qvd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_normalize(input.rotation);
		return qv_set(rotation, input.translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_lerp(qvd_arg0 start, qvd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qv_set(rotation, translation);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_lerp(qvd_arg0 start, qvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qv_set(rotation, translation);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_slerp(qvd_arg0 start, qvd_arg0 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qv_set(rotation, translation);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvd RTM_SIMD_CALL qv_slerp(qvd_arg0 start, qvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qv_set(rotation, translation);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input QV does not contain any NaN or Inf, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL qv_is_finite(qvd_arg0 input) RTM_NO_EXCEPT
	{
		return quat_is_finite(input.rotation) && vector_is_finite3(input.translation);
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
