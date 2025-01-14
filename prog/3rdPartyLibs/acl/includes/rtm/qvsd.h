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
#include "rtm/quatd.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/qvs_common.h"
#include "rtm/impl/matrix_affine_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Casts a QVS transform float32 variant to a float64 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_cast(qvsf_arg0 input) RTM_NO_EXCEPT
	{
		return qvsd{ quat_cast(input.rotation), vector_cast(input.translation_scale) };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a rotation 3x3 matrix into a QVS transform.
	// The translation/scale will be the identity.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_from_matrix(matrix3x3d_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = rtm_impl::quat_from_matrix(input.x_axis, input.y_axis, input.z_axis);
		return qvsd{ rotation, vector_set(0.0, 0.0, 0.0, 1.0) };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a rotation 3x4 affine matrix into a QVS transform.
	// If the matrix scale is non-uniform, the scale of the largest axis will be returned.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_from_matrix(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		// See qvv_from_matrix for details, the process here is very similar
		// We still treat every axis independently as if they have unique scale values
		// to be able to handle ill-formed matrices with non-uniform scale.
		// Unlike with QVV transforms, if the scale is uniform and negative, we know
		// that all three axes have reflection and we can recover the original rotation.

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
			const vector4d translation_scale = vector_set_w(translation, largest_scale);
			return qvsd{ rotation_q, translation_scale };
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
			// Our winding is reversed meaning one or three of the scale axes contains reflection
			// We negate an axis to flip the winding and negate the scale to match
			rotation.x_axis = vector_neg(rotation.x_axis);
			rotation.y_axis = vector_neg(rotation.y_axis);
			rotation.z_axis = vector_neg(rotation.z_axis);
			largest_scale = -largest_scale;
		}

		// Build a quaternion from the ortho-normal basis we found
		const quatd rotation_q = rtm_impl::quat_from_matrix(rotation.x_axis, rotation.y_axis, rotation.z_axis);

		return qvsd{ rotation_q, vector_set_w(translation, largest_scale) };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation part of a QVS transform.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL qvs_get_rotation(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		return input.rotation;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the translation part of a QVS transform.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL qvs_get_translation(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		return input.translation_scale;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the scale part of a QVS transform.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_w RTM_SIMD_CALL qvs_get_scale(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_w{ input.translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two QVS transforms.
	// Multiplication order is as follow: local_to_world = qvs_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_mul(qvsd_arg0 lhs, qvsd_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4d rhs_scale = vector_dup_w(rhs.translation_scale);

		const quatd rotation = quat_mul(lhs.rotation, rhs.rotation);
		const vector4d translation = vector_add(quat_mul_vector3(vector_mul(lhs.translation_scale, rhs_scale), rhs.rotation), rhs.translation_scale);
		const vector4d scale_w = vector_mul(lhs.translation_scale, rhs.translation_scale);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation, scale_w);

		return qvsd{ rotation, translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two QVS transforms ignoring scale.
	// The resulting QVS transform will have the LHS scale.
	// Multiplication order is as follow: local_to_world = qvs_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_mul_no_scale(qvsd_arg0 lhs, qvsd_arg1 rhs) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_mul(lhs.rotation, rhs.rotation);
		const vector4d translation = vector_add(quat_mul_vector3(lhs.translation_scale, rhs.rotation), rhs.translation_scale);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation, lhs.translation_scale);

		return qvsd{ rotation, translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a QVS transform and a 3D point.
	// Multiplication order is as follow: world_position = qvs_mul_point3(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL qvs_mul_point3(vector4d_arg0 point, qvsd_arg1 qvs) RTM_NO_EXCEPT
	{
		const vector4d scale = vector_dup_w(qvs.translation_scale);
		return vector_add(quat_mul_vector3(vector_mul(scale, point), qvs.rotation), qvs.translation_scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a QVS transform and a 3D point ignoring 3D scale.
	// Multiplication order is as follow: world_position = qvs_mul_point3_no_scale(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL qvs_mul_point3_no_scale(vector4d_arg0 point, qvsd_arg1 qvs) RTM_NO_EXCEPT
	{
		return vector_add(quat_mul_vector3(point, qvs.rotation), qvs.translation_scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVS transform.
	// If zero scale is contained, the result is undefined.
	// For a safe alternative, supply a fallback scale value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_inverse(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const vector4d inv_scale_w = vector_reciprocal(input.translation_scale);
		const vector4d inv_scale = vector_dup_w(inv_scale_w);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(vector_mul(input.translation_scale, inv_scale), inv_rotation));
		const vector4d inv_translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(inv_translation, inv_scale_w);

		return qvsd{ inv_rotation, inv_translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVS transform.
	// If the input scale has an absolute value below the supplied threshold, the
	// fallback value is used instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_inverse(qvsd_arg0 input, double fallback_scale, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const mask4d is_scale_w_zero = vector_less_equal(vector_abs(input.translation_scale), vector_set(threshold));
		const vector4d scale_w = vector_select(is_scale_w_zero, vector_set(fallback_scale), input.translation_scale);
		const vector4d inv_scale_w = vector_reciprocal(scale_w);
		const vector4d inv_scale = vector_dup_w(inv_scale_w);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(vector_mul(input.translation_scale, inv_scale), inv_rotation));
		const vector4d inv_translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(inv_translation, inv_scale_w);

		return qvsd{ inv_rotation, inv_translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVS transform ignoring 3D scale.
	// The resulting QVS transform will have the input scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvsd RTM_SIMD_CALL qvs_inverse_no_scale(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(input.translation_scale, inv_rotation));
		const vector4d inv_translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(inv_translation, input.translation_scale);

		return qvsd{ inv_rotation, inv_translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a QVS transforms with the rotation part normalized.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_normalize(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_normalize(input.rotation);
		return qvsd{ rotation, input.translation_scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_lerp(qvsd_arg0 start, qvsd_arg0 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		return qvsd{ rotation, translation_scale };
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_lerp(qvsd_arg0 start, qvsd_arg0 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		return qvsd{ rotation, translation_scale };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// The resulting QVV transform will have the start scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_lerp_no_scale(qvsd_arg0 start, qvsd_arg0 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale_lerp = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation_scale_lerp, start.translation_scale);
		return qvsd{ rotation, translation_scale };
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// The resulting QVV transform will have the start scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_lerp_no_scale(qvsd_arg0 start, qvsd_arg0 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale_lerp = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation_scale_lerp, start.translation_scale);
		return qvsd{ rotation, translation_scale };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_slerp(qvsd_arg0 start, qvsd_arg0 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		return qvsd{ rotation, translation_scale };
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_slerp(qvsd_arg0 start, qvsd_arg0 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		return qvsd{ rotation, translation_scale };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_slerp_no_scale(qvsd_arg0 start, qvsd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale_lerp = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation_scale_lerp, start.translation_scale);
		return qvsd{ rotation, translation_scale };
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvsd RTM_SIMD_CALL qvs_slerp_no_scale(qvsd_arg0 start, qvsd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation_scale_lerp = vector_lerp(start.translation_scale, end.translation_scale, alpha);
		const vector4d translation_scale = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(translation_scale_lerp, start.translation_scale);
		return qvsd{ rotation, translation_scale };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input QVS does not contain any NaN or Inf, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL qvs_is_finite(qvsd_arg0 input) RTM_NO_EXCEPT
	{
		return quat_is_finite(input.rotation) && vector_is_finite(input.translation_scale);
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
