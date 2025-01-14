#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2024 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/macros.h"
#include "rtm/math.h"
#include "rtm/scalard.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/error.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed affine 3x4 matrix representing a camera transform.
	// A camera transform multiplied by a point3 transforms a point3 local to the camera
	// into world space.
	// The camera transform is located at the specified position,
	// looking towards the specified direction, and using the specified up direction.
	//
	// The look to direction and up inputs do not need to be normalized.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_look_to(vector4d_arg0 position, vector4d_arg1 direction, vector4d_arg2 up) RTM_NO_EXCEPT
	{
		RTM_ASSERT(vector_is_finite3(position), "Look position must be finite");
		RTM_ASSERT(!vector_all_equal3(direction, vector_zero()) && vector_is_finite3(direction), "Look towards direction must be non-zero and finite");
		RTM_ASSERT(!vector_all_equal3(up, vector_zero()) && vector_is_finite3(up), "Look up direction must be non-zero and finite");

		// Constructs our orientation ortho-normal frame
		vector4d forward_axis = vector_normalize3(direction);
		vector4d right_axis = vector_normalize3(vector_cross3(up, forward_axis));
		vector4d up_axis = vector_cross3(forward_axis, right_axis);	// already normalized since inputs are normalized

		return matrix3x4d{ right_axis, up_axis, forward_axis, position };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed affine 3x4 matrix representing a camera transform.
	// A camera transform multiplied by a point3 transforms a point3 local to the camera
	// into world space.
	// The camera transform is located at the specified position,
	// looking towards the specified location, and using the specified up direction.
	//
	// The up input does not need to be normalized.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_look_at(vector4d_arg0 position, vector4d_arg1 location, vector4d_arg2 up) RTM_NO_EXCEPT
	{
		vector4d look_direction = vector_sub(location, position);
		return matrix_look_to(position, look_direction, up);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed affine 3x4 matrix representing a camera view transform.
	// A camera view transform is the inverse of a camera transform.
	// A camera transform multiplied by a point3 transforms a point3 local to the camera
	// into world space while a camera view transform multiplied by a point3 does the
	// opposite: it transforms a world space point3 into local space of the camera.
	// This is typically used before applying a projection matrix.
	// The camera transform is located at the specified position,
	// looking towards the specified direction, and using the specified up direction.
	// The camera view transform is then constructed from its inverse.
	//
	// The look to direction and up inputs do not need to be normalized.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL view_look_to(vector4d_arg0 position, vector4d_arg1 direction, vector4d_arg2 up) RTM_NO_EXCEPT
	{
		RTM_ASSERT(vector_is_finite3(position), "Look position must be finite");
		RTM_ASSERT(!vector_all_equal3(direction, vector_zero()) && vector_is_finite3(direction), "Look towards direction must be non-zero and finite");
		RTM_ASSERT(!vector_all_equal3(up, vector_zero()) && vector_is_finite3(up), "Look up direction must be non-zero and finite");

		// Constructs our orientation ortho-normal frame
		vector4d forward_axis = vector_normalize3(direction);
		vector4d right_axis = vector_normalize3(vector_cross3(up, forward_axis));
		vector4d up_axis = vector_cross3(forward_axis, right_axis);	// already normalized since inputs are normalized

		// The look-to matrix converts world space inputs into the view space or the look to camera
		// To that end, we thus need the inverse of the world space camera transform
		// Because our affine matrix is ortho-normalized, we can calculate its inverse using a transpose
		// We wish for the translation part to be in the W axis, and so we would have to set the camera
		// position in the W column before we perform a 4x4 transpose and negation of the W axis
		// To avoid this unnecessary 4x4 step, we instead transpose the upper 3x3 portion (in place) and set
		// the W axis directly

		// Negate the position to invert it
		vector4d neg_position = vector_neg(position);

		scalard pos_x = vector_dot3_as_scalar(right_axis, neg_position);
		scalard pos_y = vector_dot3_as_scalar(up_axis, neg_position);
		scalard pos_z = vector_dot3_as_scalar(forward_axis, neg_position);
		vector4d position_axis = vector_set(pos_x, pos_y, pos_z);

		// Transpose in place to invert the rotation
		RTM_MATRIXD_TRANSPOSE_3X3(right_axis, up_axis, forward_axis, right_axis, up_axis, forward_axis);

		return matrix3x4d{ right_axis, up_axis, forward_axis, position_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed affine 3x4 matrix representing a camera view transform.
	// A camera view transform is the inverse of a camera transform.
	// A camera transform multiplied by a point3 transforms a point3 local to the camera
	// into world space while a camera view transform multiplied by a point3 does the
	// opposite: it transforms a world space point3 into local space of the camera.
	// This is typically used before applying a projection matrix.
	// The camera transform is located at the specified position,
	// looking towards the specified location, and using the specified up direction.
	// The camera view transform is then constructed from its inverse.
	//
	// The look to direction and up inputs do not need to be normalized.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL view_look_at(vector4d_arg0 position, vector4d_arg1 location, vector4d_arg2 up) RTM_NO_EXCEPT
	{
		vector4d look_direction = vector_sub(location, position);
		return view_look_to(position, look_direction, up);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed 4x4 perspective projection matrix using the screen dimensions.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix4x4d RTM_SIMD_CALL proj_perspective(double view_width, double view_height, double near_distance, double far_distance) RTM_NO_EXCEPT
	{
		RTM_ASSERT(view_width > 1.0E-8, "View width must be non-zero and positive");
		RTM_ASSERT(view_height > 1.0E-8, "View height must be non-zero and positive");
		RTM_ASSERT(near_distance > 0.0 && far_distance > 0.0, "Near and far distances must be non-zero and positive");
		RTM_ASSERT((far_distance - near_distance) > 1.0E-8, "Near and far distances cannot be equal");

		double double_near_distance = near_distance + near_distance;
		double scaled_far_distance = far_distance / (far_distance - near_distance);

		vector4d x_axis = vector_set(double_near_distance / view_width, 0.0, 0.0, 0.0);
		vector4d y_axis = vector_set(0.0, double_near_distance / view_height, 0.0, 0.0);
		vector4d z_axis = vector_set(0.0, 0.0, scaled_far_distance, 1.0);
		vector4d w_axis = vector_set(0.0, 0.0, -scaled_far_distance * near_distance, 0.0);

		return matrix4x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed 4x4 perspective projection matrix using the screen field of view.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix4x4d RTM_SIMD_CALL proj_perspective_fov(double fov_angle_y, double aspect_ratio, double near_distance, double far_distance) RTM_NO_EXCEPT
	{
		RTM_ASSERT(fov_angle_y > 2.0E-8, "View FoV must be non-zero and positive");
		RTM_ASSERT(aspect_ratio > 1.0E-8, "View aspect ratio must be non-zero and positive");
		RTM_ASSERT(near_distance > 0.0 && far_distance > 0.0, "Near and far distances must be non-zero and positive");
		RTM_ASSERT((far_distance - near_distance) > 1.0E-8, "Near and far distances cannot be equal");

		double sin_fov;
		double cos_fov;
		scalar_sincos(fov_angle_y * 0.5, sin_fov, cos_fov);

		double scaled_view_height = cos_fov / sin_fov;
		double scaled_view_width = scaled_view_height / aspect_ratio;
		double scaled_far_distance = far_distance / (far_distance - near_distance);

		vector4d x_axis = vector_set(scaled_view_width, 0.0, 0.0, 0.0);
		vector4d y_axis = vector_set(0.0, scaled_view_height, 0.0, 0.0);
		vector4d z_axis = vector_set(0.0, 0.0, scaled_far_distance, 1.0);
		vector4d w_axis = vector_set(0.0, 0.0, -scaled_far_distance * near_distance, 0.0);

		return matrix4x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a left-handed 4x4 orthographic projection matrix.
	//
	// In this frame of reference:
	//    - X axis: right direction
	//    - Y axis: up direction
	//    - Z axis: forward/look direction
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix4x4d RTM_SIMD_CALL proj_orthographic(double view_width, double view_height, double near_distance, double far_distance) RTM_NO_EXCEPT
	{
		RTM_ASSERT(view_width > 1.0E-8, "View width must be non-zero and positive");
		RTM_ASSERT(view_height > 1.0E-8, "View height must be non-zero and positive");
		RTM_ASSERT(near_distance > 0.0 && far_distance > 0.0, "Near and far distances must be non-zero and positive");
		RTM_ASSERT((far_distance - near_distance) > 1.0E-8, "Near and far distances cannot be equal");

		double inverse_distance_range = 1.0 / (far_distance - near_distance);

		vector4d x_axis = vector_set(2.0 / view_width, 0.0, 0.0, 0.0);
		vector4d y_axis = vector_set(0.0, 2.0 / view_height, 0.0, 0.0);
		vector4d z_axis = vector_set(0.0, 0.0, inverse_distance_range, 0.0);
		vector4d w_axis = vector_set(0.0, 0.0, -inverse_distance_range * near_distance, 1.0);

		return matrix4x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
