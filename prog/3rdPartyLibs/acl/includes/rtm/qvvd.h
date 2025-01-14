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
#include "rtm/quatd.h"
#include "rtm/vector4d.h"
#include "rtm/matrix3x4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/qvv_common.h"
#include "rtm/impl/matrix_affine_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Casts a QVV transform float32 variant to a float64 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_cast(qvvf_arg0 input) RTM_NO_EXCEPT
	{
		return qvvd{ quat_cast(input.rotation), vector_cast(input.translation), vector_cast(input.scale) };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a pure rotation 3x3 matrix into a QVV transform.
	// The translation/scale will be the identity.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_from_matrix(matrix3x3d_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = rtm_impl::quat_from_matrix(input.x_axis, input.y_axis, input.z_axis);
		const vector4d translation = vector_zero();
		const vector4d scale = vector_set(1.0);
		return qvvd{ rotation, translation, scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a 3x4 affine matrix into a QVV transform.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_from_matrix(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		// Let us define the following:
		// A = S * R * T
		// A is an affine matrix that scales, rotates, then translates (an SRT transform)
		// The translation part lives in the W row and is easily recovered as the scale/rotation
		// components are stored in the XYZ rows. For the remainder of the discussion, we'll omit
		// translation and only consider scale/rotation.
		//
		// For scale and rotation, recovering the original values S and R is more complicated
		// if we allow scale to be zero or negative.
		//
		// Let us first consider the simple case where scale is always non-zero and positive.
		// When this is the case, we can recover the matrix S by computing the length of each
		// of the XYZ rows (R is ortho-normalized and its rows thus have unit length). R can
		// then be recovered by normalizing the XYZ rows of A.
		//
		// If scale can be zero, we can recover S in the same way but R requires a bit more care.
		// With a zero row, we cannot normalize it (due to division by zero). To retain orthogonality,
		// we must use the cross product to recover our missing values where possible. To that end,
		// the scale axes are sorted from largest to smallest and we process them starting with the largest.
		//     - If the largest scale axis is below our precision threshold, we cannot normalize it. It also means
		//       that the remaining two axes are also below our precision threshold. As such, we will return a zero
		//       scale matrix and the identity rotation matrix. The rotation matrix doesn't matter when we have zero scale.
		//     - If the second largest scale axis is below our precision threshold, we cannot normalize it. It also
		//       means that the third axis is also below our precision threshold. We thus need to find two axes orthogonal
		//       to the first. To that end, we test the dot product of the first axis with the identity Y axis. If
		//       the two are parallel, we will use the identity Z axis for the first cross product, otherwise we use the Y axis.
		//       This yields our second axis that has zero scale (the axis doesn't matter since we have zero scale).
		//       We find our third axis by taking the cross product of the first two, which also has zero scale.
		//     - If the third largest scale axis is below our precision threshold, we cannot normalize it. Since we already
		//       have two valid axes, we can find our third using the cross product and it will have zero scale.
		// As such, if one or more axes are zero, we can recover the original S matrix but not the original R matrix.
		// However, when the new rotation matrix R' is multiplied with the S matrix, it yields the same affine
		// matrix A because anything multiplied with zero yields zero:
		// A = S * R'
		//
		// Negative scale throws a wrench in things. Negative scaling along one axis corresponds to
		// performing a reflection along that axis. When a single axis is reflected, the matrix A
		// will no longer have the same winding (it flips with every reflected axis). As such, if we
		// extract S by using the row lengths, we cannot recover R as the resulting rotation matrix
		// will be ill formed due to its winding.
		// As such, when negative scale is present, we will not be able to recover the original S and R
		// matrices. Let us look at every possible scenario:
		//     - If two axes are reflected, they are combined into a new rotation matrix. This happens because
		//       two successive reflections (F1 and F2) can be represented by a single rotation (RF) like so:
		//       RF = F1 * F2
		//       And two rotations (R1 and R2) can be combined into a single rotation R' with: R' = R1 * R2
		//       Let us express everything now:
		//       S = S' * F1 * F2 (some scaling with 2 reflections)
		//       A = S' * F1 * F2 * R
		//       A = S' * R'
		//       And so while we cannot recover the original S and R matrices, we can find two matrices
		//       that yield the same resulting affine transform.
		//     - If three axes are reflected, the winding is reversed and we have the following:
		//       S = S' * F1 * F2 * F3
		//       A = S' * F1 * F2 * F3 * R
		//       A = S' * F1 * R'
		//       And so we see that the case of three reflected axes is equivalent to the single reflection case.
		//     - If a single axis is reflected, the winding is reversed and we have the following:
		//       S = S' * F1
		//       A = S' * F1 * R
		//       When this occurs, we cannot tell which axis was originally reflected. But thankfully, it does
		//       not matter. To find a solution, we negate one axis (any) in both the scale matrix
		//       and the rotation matrix. When the two will be multiplied, the two negations will cancel out.
		//       By negating an axis on the rotation matrix, we will reverse the winding back into the one we
		//       want to find the rotation we are looking for. We negate the equivalent scale axis only to be
		//       able to recover the original matrix. And so we have:
		//       A = S' * R'
		//       The axis we choose to negate doesn't matter as long as it doesn't have zero scale (zero cannot be negated).
		//       To that end, we negate the largest scale axis.
		//
		// Taking everything together, we first handle positive and zero scale together by finding our
		// orthogonal basis and positive (or zero) scale values. Then, if the determinant is negative, the winding
		// is reversed and we must apply our reflection fixup as described above by negating the largest scale axis
		// in both the orthogonal basis (to restore the correct winding) and on our scale to cancel it out.
		// If all three axes have zero scale, we must early out and skip this last step as we know there is no reflection.
		// If one or two axes have zero scale, they will be reconstructed using the cross product which means that the
		// winding will never be reversed and no reflection fixup will be performed.

		// Translation is always in the W axis
		const vector4d translation = input.w_axis;

		// First, we compute our absolute scale values
		const scalard x_scale = vector_length3_as_scalar(input.x_axis);
		const scalard y_scale = vector_length3_as_scalar(input.y_axis);
		const scalard z_scale = vector_length3_as_scalar(input.z_axis);
		vector4d scale = vector_set(x_scale, y_scale, z_scale);

		// Grab a pointer to the first scale value, we'll index into it
		double* scale_ptr = rtm_impl::bit_cast<double*>(&scale);

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
			return qvvd{ rotation_q, translation, scale };
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
			largest_scale_axis = vector_neg(largest_scale_axis);
			largest_scale = -largest_scale;

			// Update our scale output and rotation basis
			rotation_axes[largest_scale_component_i] = largest_scale_axis;
			scale_ptr[largest_scale_component_i] = largest_scale;
		}

		// Build a quaternion from the ortho-normal basis we found
		const quatd rotation_q = rtm_impl::quat_from_matrix(rotation.x_axis, rotation.y_axis, rotation.z_axis);

		return qvvd{ rotation_q, translation, scale };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two QVV transforms.
	// Multiplication order is as follow: local_to_world = qvv_mul(local_to_object, object_to_world)
	// NOTE: When scale is present, multiplication will not properly handle skew/shear,
	// use affine matrices if you have issues.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_mul(qvvd_arg0 lhs, qvvd_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4d min_scale = vector_min(lhs.scale, rhs.scale);
		const vector4d scale = vector_mul(lhs.scale, rhs.scale);

		if (vector_any_less_than3(min_scale, vector_zero()))
		{
			// If we have negative scale, we go through a matrix
			const matrix3x4d lhs_mtx = matrix_from_qvv(lhs);
			const matrix3x4d rhs_mtx = matrix_from_qvv(rhs);
			matrix3x4d result_mtx = matrix_mul(lhs_mtx, rhs_mtx);
			result_mtx = matrix_remove_scale(result_mtx);

			const vector4d sign = vector_sign(scale);
			result_mtx.x_axis = vector_mul(result_mtx.x_axis, vector_dup_x(sign));
			result_mtx.y_axis = vector_mul(result_mtx.y_axis, vector_dup_y(sign));
			result_mtx.z_axis = vector_mul(result_mtx.z_axis, vector_dup_z(sign));

			const quatd rotation = quat_from_matrix(result_mtx);
			const vector4d translation = result_mtx.w_axis;
			return qvv_set(rotation, translation, scale);
		}
		else
		{
			const quatd rotation = quat_mul(lhs.rotation, rhs.rotation);
			const vector4d translation = vector_add(quat_mul_vector3(vector_mul(lhs.translation, rhs.scale), rhs.rotation), rhs.translation);
			return qvv_set(rotation, translation, scale);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two QVV transforms ignoring 3D scale.
	// The resulting QVV transform will have the LHS scale.
	// Multiplication order is as follow: local_to_world = qvv_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_mul_no_scale(qvvd_arg0 lhs, qvvd_arg1 rhs) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_mul(lhs.rotation, rhs.rotation);
		const vector4d translation = vector_add(quat_mul_vector3(lhs.translation, rhs.rotation), rhs.translation);
		return qvv_set(rotation, translation, lhs.scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a QVV transform and a 3D point.
	// Multiplication order is as follow: world_position = qvv_mul_point3(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL qvv_mul_point3(vector4d_arg0 point, qvvd_arg1 qvv) RTM_NO_EXCEPT
	{
		return vector_add(quat_mul_vector3(vector_mul(point, qvv.scale), qvv.rotation), qvv.translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a QVV transform and a 3D point ignoring 3D scale.
	// Multiplication order is as follow: world_position = qvv_mul_point3_no_scale(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL qvv_mul_point3_no_scale(vector4d_arg0 point, qvvd_arg1 qvv) RTM_NO_EXCEPT
	{
		return vector_add(quat_mul_vector3(point, qvv.rotation), qvv.translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVV transform.
	// If zero scale is contained, the result is undefined.
	// For a safe alternative, supply a fallback scale value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_inverse(qvvd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const vector4d inv_scale = vector_reciprocal(input.scale);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(vector_mul(input.translation, inv_scale), inv_rotation));
		return qvv_set(inv_rotation, inv_translation, inv_scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVV transform.
	// If the input scale has an absolute value below the supplied threshold, the
	// fallback value is used instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_inverse(qvvd_arg0 input, vector4d_arg3 fallback_scale, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const mask4d is_scale_zero = vector_less_equal(vector_abs(input.scale), vector_set(threshold));
		const vector4d scale = vector_select(is_scale_zero, fallback_scale, input.scale);
		const vector4d inv_scale = vector_reciprocal(scale);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(vector_mul(input.translation, inv_scale), inv_rotation));
		return qvv_set(inv_rotation, inv_translation, inv_scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the inverse of the input QVV transform ignoring 3D scale.
	// The resulting QVV transform will have the input scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline qvvd RTM_SIMD_CALL qvv_inverse_no_scale(qvvd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd inv_rotation = quat_conjugate(input.rotation);
		const vector4d inv_translation = vector_neg(quat_mul_vector3(input.translation, inv_rotation));
		return qvv_set(inv_rotation, inv_translation, input.scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a QVV transforms with the rotation part normalized.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_normalize(qvvd_arg0 input) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_normalize(input.rotation);
		return qvv_set(rotation, input.translation, input.scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_lerp(qvvd_arg0 start, qvvd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		const vector4d scale = vector_lerp(start.scale, end.scale, alpha);
		return qvv_set(rotation, translation, scale);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_lerp(qvvd_arg0 start, qvvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		const vector4d scale = vector_lerp(start.scale, end.scale, alpha);
		return qvv_set(rotation, translation, scale);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_lerp_no_scale(qvvd_arg0 start, qvvd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qvv_set(rotation, translation, start.scale);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_lerp_no_scale(qvvd_arg0 start, qvvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_lerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qvv_set(rotation, translation, start.scale);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_slerp(qvvd_arg0 start, qvvd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		const vector4d scale = vector_lerp(start.scale, end.scale, alpha);
		return qvv_set(rotation, translation, scale);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_slerp(qvvd_arg0 start, qvvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		const vector4d scale = vector_lerp(start.scale, end.scale, alpha);
		return qvv_set(rotation, translation, scale);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_slerp_no_scale(qvvd_arg0 start, qvvd_arg1 end, double alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qvv_set(rotation, translation, start.scale);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component spherical interpolation of the two inputs at the specified alpha.
	// See quat_slerp(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE qvvd RTM_SIMD_CALL qvv_slerp_no_scale(qvvd_arg0 start, qvvd_arg1 end, scalard_argn alpha) RTM_NO_EXCEPT
	{
		const quatd rotation = quat_slerp(start.rotation, end.rotation, alpha);
		const vector4d translation = vector_lerp(start.translation, end.translation, alpha);
		return qvv_set(rotation, translation, start.scale);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input QVV does not contain any NaN or Inf, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL qvv_is_finite(qvvd_arg0 input) RTM_NO_EXCEPT
	{
		return quat_is_finite(input.rotation) && vector_is_finite3(input.translation) && vector_is_finite3(input.scale);
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
