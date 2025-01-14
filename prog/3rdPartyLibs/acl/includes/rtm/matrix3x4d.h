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
#include "rtm/matrix3x3d.h"
#include "rtm/quatd.h"
#include "rtm/qvsd.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/matrix_common.h"
#include "rtm/impl/matrix_affine_common.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Converts a rotation 3x3 matrix into a 3x4 affine matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE matrix3x4d RTM_SIMD_CALL matrix_from_rotation(matrix3x3d_arg0 rotation) RTM_NO_EXCEPT
	{
		return matrix3x4d{ rotation.x_axis, rotation.y_axis, rotation.z_axis, (vector4d)vector_zero() };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a translation vector into a 3x4 affine matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE matrix3x4d RTM_SIMD_CALL matrix_from_translation(vector4d_arg0 translation) RTM_NO_EXCEPT
	{
		return matrix3x4d{ vector_set(1.0, 0.0, 0.0, 0.0), vector_set(0.0, 1.0, 0.0, 0.0), vector_set(0.0, 0.0, 1.0, 0.0), translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets a 3x4 affine matrix from a rotation quaternion and translation.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qv(quatd_arg0 quat, vector4d_arg1 translation) RTM_NO_EXCEPT
	{
		RTM_ASSERT(quat_is_normalized(quat), "Quaternion is not normalized");

		const double x2 = quat_get_x(quat) + quat_get_x(quat);
		const double y2 = quat_get_y(quat) + quat_get_y(quat);
		const double z2 = quat_get_z(quat) + quat_get_z(quat);
		const double xx = quat_get_x(quat) * x2;
		const double xy = quat_get_x(quat) * y2;
		const double xz = quat_get_x(quat) * z2;
		const double yy = quat_get_y(quat) * y2;
		const double yz = quat_get_y(quat) * z2;
		const double zz = quat_get_z(quat) * z2;
		const double wx = quat_get_w(quat) * x2;
		const double wy = quat_get_w(quat) * y2;
		const double wz = quat_get_w(quat) * z2;

		const vector4d x_axis = vector_set(1.0 - (yy + zz), xy + wz, xz - wy, 0.0);
		const vector4d y_axis = vector_set(xy - wz, 1.0 - (xx + zz), yz + wx, 0.0);
		const vector4d z_axis = vector_set(xz + wy, yz - wx, 1.0 - (xx + yy), 0.0);
		return matrix3x4d{ x_axis, y_axis, z_axis, translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a QV transform into a 3x4 affine matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qv(qvd_arg0 transform) RTM_NO_EXCEPT
	{
		return matrix_from_qv(transform.rotation, transform.translation);
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets a 3x4 affine matrix from a rotation quaternion, translation, and scalar scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qvs(quatd_arg0 quat, vector4d_arg1 translation, double scale) RTM_NO_EXCEPT
	{
		RTM_ASSERT(quat_is_normalized(quat), "Quaternion is not normalized");

		const double x2 = quat_get_x(quat) + quat_get_x(quat);
		const double y2 = quat_get_y(quat) + quat_get_y(quat);
		const double z2 = quat_get_z(quat) + quat_get_z(quat);
		const double xx = quat_get_x(quat) * x2;
		const double xy = quat_get_x(quat) * y2;
		const double xz = quat_get_x(quat) * z2;
		const double yy = quat_get_y(quat) * y2;
		const double yz = quat_get_y(quat) * z2;
		const double zz = quat_get_z(quat) * z2;
		const double wx = quat_get_w(quat) * x2;
		const double wy = quat_get_w(quat) * y2;
		const double wz = quat_get_w(quat) * z2;

		const scalard scale_s = scalar_set(scale);

		const vector4d x_axis = vector_mul(vector_set(1.0 - (yy + zz), xy + wz, xz - wy, 0.0), scale_s);
		const vector4d y_axis = vector_mul(vector_set(xy - wz, 1.0 - (xx + zz), yz + wx, 0.0), scale_s);
		const vector4d z_axis = vector_mul(vector_set(xz + wy, yz - wx, 1.0 - (xx + yy), 0.0), scale_s);
		return matrix3x4d{ x_axis, y_axis, z_axis, translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a QVS transform into a 3x4 affine matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qvs(qvsd_arg0 transform) RTM_NO_EXCEPT
	{
		return matrix_from_qvs(transform.rotation, transform.translation_scale, qvs_get_scale(transform));
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets a 3x4 affine matrix from a rotation quaternion, translation, and 3D scale.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qvv(quatd_arg0 quat, vector4d_arg1 translation, vector4d_arg2 scale) RTM_NO_EXCEPT
	{
		RTM_ASSERT(quat_is_normalized(quat), "Quaternion is not normalized");

		const double x2 = quat_get_x(quat) + quat_get_x(quat);
		const double y2 = quat_get_y(quat) + quat_get_y(quat);
		const double z2 = quat_get_z(quat) + quat_get_z(quat);
		const double xx = quat_get_x(quat) * x2;
		const double xy = quat_get_x(quat) * y2;
		const double xz = quat_get_x(quat) * z2;
		const double yy = quat_get_y(quat) * y2;
		const double yz = quat_get_y(quat) * z2;
		const double zz = quat_get_z(quat) * z2;
		const double wx = quat_get_w(quat) * x2;
		const double wy = quat_get_w(quat) * y2;
		const double wz = quat_get_w(quat) * z2;

		const scalard scale_x = vector_get_x_as_scalar(scale);
		const scalard scale_y = vector_get_y_as_scalar(scale);
		const scalard scale_z = vector_get_z_as_scalar(scale);

		const vector4d x_axis = vector_mul(vector_set(1.0 - (yy + zz), xy + wz, xz - wy, 0.0), scale_x);
		const vector4d y_axis = vector_mul(vector_set(xy - wz, 1.0 - (xx + zz), yz + wx, 0.0), scale_y);
		const vector4d z_axis = vector_mul(vector_set(xz + wy, yz - wx, 1.0 - (xx + yy), 0.0), scale_z);
		return matrix3x4d{ x_axis, y_axis, z_axis, translation };
	}

	//////////////////////////////////////////////////////////////////////////
	// Converts a QVV transform into a 3x4 affine matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_from_qvv(qvvd_arg0 transform) RTM_NO_EXCEPT
	{
		return matrix_from_qvv(transform.rotation, transform.translation, transform.scale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the axis pointing in the forward direction of the default coordinate system (Z+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr vector4d RTM_SIMD_CALL matrix_get_coord_forward(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return input.z_axis;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the axis pointing in the up direction of the default coordinate system (Y+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr vector4d RTM_SIMD_CALL matrix_get_coord_up(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return input.y_axis;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the axis pointing in the cross direction of the default coordinate system (X+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr vector4d RTM_SIMD_CALL matrix_get_coord_cross(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return input.x_axis;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the axis holding the position of the default coordinate system (W+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr vector4d RTM_SIMD_CALL matrix_get_coord_position(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return input.w_axis;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the desired 3x4 affine matrix axis.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr vector4d RTM_SIMD_CALL matrix_get_axis(matrix3x4d_arg0 input, axis4 axis) RTM_NO_EXCEPT
	{
		return axis == axis4::x ? input.x_axis : (axis == axis4::y ? input.y_axis : (axis == axis4::z ? input.z_axis : input.w_axis));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a new 3x4 matrix where the specified axis has been replaced on the input matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_set_axis(matrix3x4d_arg0 input, vector4d_arg4 axis_value, axis4 axis) RTM_NO_EXCEPT
	{
		switch (axis)
		{
			default:
			case axis4::x:	return matrix3x4d{ axis_value, input.y_axis, input.z_axis, input.w_axis };
			case axis4::y:	return matrix3x4d{ input.x_axis, axis_value, input.z_axis, input.w_axis };
			case axis4::z:	return matrix3x4d{ input.x_axis, input.y_axis, axis_value, input.w_axis };
			case axis4::w:	return matrix3x4d{ input.x_axis, input.y_axis, input.z_axis, axis_value };
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the desired 3x4 matrix component from the specified axis.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline rtm_impl::vector4d_vector_get_component RTM_SIMD_CALL matrix_get_component(matrix3x4d_arg0 input, axis4 axis, component3 component) RTM_NO_EXCEPT
	{
		switch (axis)
		{
			default:
			case axis4::x:	return vector_get_component3(input.x_axis, component);
			case axis4::y:	return vector_get_component3(input.y_axis, component);
			case axis4::z:	return vector_get_component3(input.z_axis, component);
			case axis4::w:	return vector_get_component3(input.w_axis, component);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the desired 3x4 matrix component from the specified axis.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL matrix_get_component_as_scalar(matrix3x4d_arg0 input, axis4 axis, component3 component) RTM_NO_EXCEPT
	{
		switch (axis)
		{
			default:
			case axis4::x:	return vector_get_component3_as_scalar(input.x_axis, component);
			case axis4::y:	return vector_get_component3_as_scalar(input.y_axis, component);
			case axis4::z:	return vector_get_component3_as_scalar(input.z_axis, component);
			case axis4::w:	return vector_get_component3_as_scalar(input.w_axis, component);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a new 3x4 matrix where the specified axis/component has been replaced on the input matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_set_component(matrix3x4d_arg0 input, double component_value, axis4 axis, component3 component) RTM_NO_EXCEPT
	{
		switch (axis)
		{
			default:
			case axis4::x:	return matrix3x4d{ vector_set_component3(input.x_axis, component_value, component), input.y_axis, input.z_axis, input.w_axis };
			case axis4::y:	return matrix3x4d{ input.x_axis, vector_set_component3(input.y_axis, component_value, component), input.z_axis, input.w_axis };
			case axis4::z:	return matrix3x4d{ input.x_axis, input.y_axis, vector_set_component3(input.z_axis, component_value, component), input.w_axis };
			case axis4::w:	return matrix3x4d{ input.x_axis, input.y_axis, input.z_axis, vector_set_component3(input.w_axis, component_value, component) };
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns a new 3x4 matrix where the specified axis/component has been replaced on the input matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_set_component(matrix3x4d_arg0 input, scalard_arg4 component_value, axis4 axis, component3 component) RTM_NO_EXCEPT
	{
		switch (axis)
		{
			default:
			case axis4::x:	return matrix3x4d{ vector_set_component3(input.x_axis, component_value, component), input.y_axis, input.z_axis, input.w_axis };
			case axis4::y:	return matrix3x4d{ input.x_axis, vector_set_component3(input.y_axis, component_value, component), input.z_axis, input.w_axis };
			case axis4::z:	return matrix3x4d{ input.x_axis, input.y_axis, vector_set_component3(input.z_axis, component_value, component), input.w_axis };
			case axis4::w:	return matrix3x4d{ input.x_axis, input.y_axis, input.z_axis, vector_set_component3(input.w_axis, component_value, component) };
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Converts a 3x4 affine matrix into a rotation quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatd RTM_SIMD_CALL quat_from_matrix(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quat_from_matrix(input.x_axis, input.y_axis, input.z_axis);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two 3x4 affine matrices.
	// Multiplication order is as follow: local_to_world = matrix_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_mul(matrix3x4d_arg0 lhs, matrix3x4d_arg1 rhs) RTM_NO_EXCEPT
	{
		vector4d tmp = vector_mul(vector_dup_x(lhs.x_axis), rhs.x_axis);
		tmp = vector_mul_add(vector_dup_y(lhs.x_axis), rhs.y_axis, tmp);
		tmp = vector_mul_add(vector_dup_z(lhs.x_axis), rhs.z_axis, tmp);
		vector4d x_axis = tmp;

		tmp = vector_mul(vector_dup_x(lhs.y_axis), rhs.x_axis);
		tmp = vector_mul_add(vector_dup_y(lhs.y_axis), rhs.y_axis, tmp);
		tmp = vector_mul_add(vector_dup_z(lhs.y_axis), rhs.z_axis, tmp);
		vector4d y_axis = tmp;

		tmp = vector_mul(vector_dup_x(lhs.z_axis), rhs.x_axis);
		tmp = vector_mul_add(vector_dup_y(lhs.z_axis), rhs.y_axis, tmp);
		tmp = vector_mul_add(vector_dup_z(lhs.z_axis), rhs.z_axis, tmp);
		vector4d z_axis = tmp;

		tmp = vector_mul(vector_dup_x(lhs.w_axis), rhs.x_axis);
		tmp = vector_mul_add(vector_dup_y(lhs.w_axis), rhs.y_axis, tmp);
		tmp = vector_mul_add(vector_dup_z(lhs.w_axis), rhs.z_axis, tmp);
		vector4d w_axis = vector_add(rhs.w_axis, tmp);
		return matrix3x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a 3x4 affine matrix and a 3D point.
	// Multiplication order is as follow: world_position = matrix_mul(local_position, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL matrix_mul_point3(vector4d_arg0 point, matrix3x4d_argn mtx) RTM_NO_EXCEPT
	{
		vector4d tmp0;
		vector4d tmp1;

		tmp0 = vector_mul(vector_dup_x(point), mtx.x_axis);
		tmp0 = vector_mul_add(vector_dup_y(point), mtx.y_axis, tmp0);
		tmp1 = vector_mul_add(vector_dup_z(point), mtx.z_axis, mtx.w_axis);

		return vector_add(tmp0, tmp1);
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a 3x4 affine matrix and a 3D vector.
	// Multiplication order is as follow: world_position = matrix_mul(local_vector, local_to_world)
	// Note: The proper way to transform a normal by an affine matrix with non-uniform scale
	// is to multiply the normal with the cofactor matrix of the 3x3 rotation/scale part.
	// See: https://github.com/graphitemaster/normals_revisited
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL matrix_mul_vector3(vector4d_arg0 vec3, matrix3x4d_argn mtx) RTM_NO_EXCEPT
	{
		vector4d tmp;

		tmp = vector_mul(vector_dup_x(vec3), mtx.x_axis);
		tmp = vector_mul_add(vector_dup_y(vec3), mtx.y_axis, tmp);
		tmp = vector_mul_add(vector_dup_z(vec3), mtx.z_axis, tmp);

		return tmp;
	}

	//////////////////////////////////////////////////////////////////////////
	// Transposes a 3x4 affine matrix.
	// Note: This transposes the upper 3x3 rotation/scale part of the matrix
	// and it discards the translation. This is because a transposed 4x4 affine
	// matrix is no longer affine due to its last row no longer being [0, 0, 0, 1].
	// The most common usage of an affine transpose operation is to construct the
	// inverse transpose used to transform normal bi-vectors.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE matrix3x3d RTM_SIMD_CALL matrix_transpose(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		vector4d x_axis;
		vector4d y_axis;
		vector4d z_axis;
		RTM_MATRIXD_TRANSPOSE_3X3(input.x_axis, input.y_axis, input.z_axis, x_axis, y_axis, z_axis);
		return matrix3x3d{ x_axis, y_axis, z_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Inverses a 3x4 affine matrix.
	// If the input matrix is not invertible, the result is undefined.
	// For a safe alternative, supply a fallback value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_inverse(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		// Invert the 3x3 portion of the matrix that contains the rotation and 3D scale
		const vector4d v00_v01_v10_v11 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::b>(input.x_axis, input.y_axis);
		const vector4d v02_v03_v12_v13 = vector_mix<mix4::z, mix4::w, mix4::c, mix4::d>(input.x_axis, input.y_axis);

		const vector4d v00_v10_v20_v22 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::c>(v00_v01_v10_v11, input.z_axis);
		const vector4d v01_v11_v21_v23 = vector_mix<mix4::y, mix4::w, mix4::b, mix4::d>(v00_v01_v10_v11, input.z_axis);
		const vector4d v02_v12_v22_v22 = vector_mix<mix4::x, mix4::z, mix4::c, mix4::c>(v02_v03_v12_v13, input.z_axis);

		const vector4d v11_v21_v01 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(v01_v11_v21_v23, input.x_axis);
		const vector4d v22_v02_v12_v10 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::a>(v02_v12_v22_v22, input.y_axis);
		const vector4d v11v22_v21v02_v01v12 = vector_mul(v11_v21_v01, v22_v02_v12_v10);

		const vector4d v01_v02_v11_v12 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.x_axis, input.y_axis);

		const vector4d v12_v01_v11 = vector_mix<mix4::w, mix4::x, mix4::b, mix4::c>(v01_v02_v11_v12, v01_v11_v21_v23);
		const vector4d v21_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.z_axis, v22_v02_v12_v10);

		vector4d x_axis = vector_neg_mul_sub(v12_v01_v11, v21_v22_v02, v11v22_v21v02_v01v12);

		const vector4d v20_v00_v10_v22 = vector_mix<mix4::z, mix4::x, mix4::d, mix4::a>(v00_v10_v20_v22, v22_v02_v12_v10);
		const vector4d v12_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v02_v12_v22_v22, v21_v22_v02);
		const vector4d v20v12_v00v22_v10v02 = vector_mul(v20_v00_v10_v22, v12_v22_v02);

		const vector4d v10_v02_v00 = vector_mix<mix4::w, mix4::y, mix4::b, mix4::c>(v22_v02_v12_v10, v20_v00_v10_v22);
		const vector4d v22_v20_v12 = vector_mix<mix4::w, mix4::x, mix4::a, mix4::c>(v20_v00_v10_v22, v12_v22_v02);

		vector4d y_axis = vector_neg_mul_sub(v10_v02_v00, v22_v20_v12, v20v12_v00v22_v10v02);

		const vector4d v10_v20_v00 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::c>(v20_v00_v10_v22, v10_v02_v00);
		const vector4d v21_v01_v11 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v11_v21_v01, v12_v01_v11);
		const vector4d v10v21_v20v01_v00v11 = vector_mul(v10_v20_v00, v21_v01_v11);

		const vector4d v20_v00_v01 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v10_v20_v00, v11_v21_v01);
		const vector4d v11_v21_v10 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::c>(v11_v21_v01, v10_v20_v00);

		vector4d z_axis = vector_neg_mul_sub(v20_v00_v01, v11_v21_v10, v10v21_v20v01_v00v11);

		const vector4d o00_o00_o10_o10 = vector_mix<mix4::x, mix4::x, mix4::a, mix4::a>(x_axis, y_axis);
		const vector4d o00_o10_o20 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::a>(o00_o00_o10_o10, z_axis);

		const scalard det = vector_dot3_as_scalar(o00_o10_o20, input.x_axis);
		const scalard inv_det_s = scalar_reciprocal(det);
		const vector4d inv_det = vector_set(inv_det_s);

		x_axis = vector_mul(x_axis, inv_det);
		y_axis = vector_mul(y_axis, inv_det);
		z_axis = vector_mul(z_axis, inv_det);

		// Invert the translation
		const vector4d tmp0 = vector_mul(vector_dup_z(input.w_axis), z_axis);
		const vector4d tmp1 = vector_mul_add(vector_dup_y(input.w_axis), y_axis, tmp0);
		vector4d w_axis = vector_neg(vector_mul_add(vector_dup_x(input.w_axis), x_axis, tmp1));

		return matrix3x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Inverses a 3x4 affine matrix.
	// If the input matrix has a determinant whose absolute value is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_inverse(matrix3x4d_arg0 input, matrix3x4d_argn fallback, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		// Invert the 3x3 portion of the matrix that contains the rotation and 3D scale
		const vector4d v00_v01_v10_v11 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::b>(input.x_axis, input.y_axis);
		const vector4d v02_v03_v12_v13 = vector_mix<mix4::z, mix4::w, mix4::c, mix4::d>(input.x_axis, input.y_axis);

		const vector4d v00_v10_v20_v22 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::c>(v00_v01_v10_v11, input.z_axis);
		const vector4d v01_v11_v21_v23 = vector_mix<mix4::y, mix4::w, mix4::b, mix4::d>(v00_v01_v10_v11, input.z_axis);
		const vector4d v02_v12_v22_v22 = vector_mix<mix4::x, mix4::z, mix4::c, mix4::c>(v02_v03_v12_v13, input.z_axis);

		const vector4d v11_v21_v01 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(v01_v11_v21_v23, input.x_axis);
		const vector4d v22_v02_v12_v10 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::a>(v02_v12_v22_v22, input.y_axis);
		const vector4d v11v22_v21v02_v01v12 = vector_mul(v11_v21_v01, v22_v02_v12_v10);

		const vector4d v01_v02_v11_v12 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.x_axis, input.y_axis);

		const vector4d v12_v01_v11 = vector_mix<mix4::w, mix4::x, mix4::b, mix4::c>(v01_v02_v11_v12, v01_v11_v21_v23);
		const vector4d v21_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.z_axis, v22_v02_v12_v10);

		vector4d x_axis = vector_neg_mul_sub(v12_v01_v11, v21_v22_v02, v11v22_v21v02_v01v12);

		const vector4d v20_v00_v10_v22 = vector_mix<mix4::z, mix4::x, mix4::d, mix4::a>(v00_v10_v20_v22, v22_v02_v12_v10);
		const vector4d v12_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v02_v12_v22_v22, v21_v22_v02);
		const vector4d v20v12_v00v22_v10v02 = vector_mul(v20_v00_v10_v22, v12_v22_v02);

		const vector4d v10_v02_v00 = vector_mix<mix4::w, mix4::y, mix4::b, mix4::c>(v22_v02_v12_v10, v20_v00_v10_v22);
		const vector4d v22_v20_v12 = vector_mix<mix4::w, mix4::x, mix4::a, mix4::c>(v20_v00_v10_v22, v12_v22_v02);

		vector4d y_axis = vector_neg_mul_sub(v10_v02_v00, v22_v20_v12, v20v12_v00v22_v10v02);

		const vector4d v10_v20_v00 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::c>(v20_v00_v10_v22, v10_v02_v00);
		const vector4d v21_v01_v11 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v11_v21_v01, v12_v01_v11);
		const vector4d v10v21_v20v01_v00v11 = vector_mul(v10_v20_v00, v21_v01_v11);

		const vector4d v20_v00_v01 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v10_v20_v00, v11_v21_v01);
		const vector4d v11_v21_v10 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::c>(v11_v21_v01, v10_v20_v00);

		vector4d z_axis = vector_neg_mul_sub(v20_v00_v01, v11_v21_v10, v10v21_v20v01_v00v11);

		const vector4d o00_o00_o10_o10 = vector_mix<mix4::x, mix4::x, mix4::a, mix4::a>(x_axis, y_axis);
		const vector4d o00_o10_o20 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::a>(o00_o00_o10_o10, z_axis);

		const scalard det = vector_dot3_as_scalar(o00_o10_o20, input.x_axis);
		if (scalar_cast(scalar_abs(det)) < threshold)
			return fallback;

		const scalard inv_det_s = scalar_reciprocal(det);
		const vector4d inv_det = vector_set(inv_det_s);

		x_axis = vector_mul(x_axis, inv_det);
		y_axis = vector_mul(y_axis, inv_det);
		z_axis = vector_mul(z_axis, inv_det);

		// Invert the translation
		const vector4d tmp0 = vector_mul(vector_dup_z(input.w_axis), z_axis);
		const vector4d tmp1 = vector_mul_add(vector_dup_y(input.w_axis), y_axis, tmp0);
		vector4d w_axis = vector_neg(vector_mul_add(vector_dup_x(input.w_axis), x_axis, tmp1));

		return matrix3x4d{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the determinant of the 3x3 rotation/scale part of the input 3x4 matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL matrix_determinant(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		const vector4d v00_v01_v10_v11 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::b>(input.x_axis, input.y_axis);
		const vector4d v02_v03_v12_v13 = vector_mix<mix4::z, mix4::w, mix4::c, mix4::d>(input.x_axis, input.y_axis);

		const vector4d v00_v10_v20_v22 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::c>(v00_v01_v10_v11, input.z_axis);
		const vector4d v01_v11_v21_v23 = vector_mix<mix4::y, mix4::w, mix4::b, mix4::d>(v00_v01_v10_v11, input.z_axis);
		const vector4d v02_v12_v22_v22 = vector_mix<mix4::x, mix4::z, mix4::c, mix4::c>(v02_v03_v12_v13, input.z_axis);

		const vector4d v11_v21_v01 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(v01_v11_v21_v23, input.x_axis);
		const vector4d v22_v02_v12_v10 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::a>(v02_v12_v22_v22, input.y_axis);
		const vector4d v11v22_v21v02_v01v12 = vector_mul(v11_v21_v01, v22_v02_v12_v10);

		const vector4d v01_v02_v11_v12 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.x_axis, input.y_axis);

		const vector4d v12_v01_v11 = vector_mix<mix4::w, mix4::x, mix4::b, mix4::c>(v01_v02_v11_v12, v01_v11_v21_v23);
		const vector4d v21_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::b, mix4::c>(input.z_axis, v22_v02_v12_v10);

		vector4d x_axis = vector_neg_mul_sub(v12_v01_v11, v21_v22_v02, v11v22_v21v02_v01v12);

		const vector4d v20_v00_v10_v22 = vector_mix<mix4::z, mix4::x, mix4::d, mix4::a>(v00_v10_v20_v22, v22_v02_v12_v10);
		const vector4d v12_v22_v02 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v02_v12_v22_v22, v21_v22_v02);
		const vector4d v20v12_v00v22_v10v02 = vector_mul(v20_v00_v10_v22, v12_v22_v02);

		const vector4d v10_v02_v00 = vector_mix<mix4::w, mix4::y, mix4::b, mix4::c>(v22_v02_v12_v10, v20_v00_v10_v22);
		const vector4d v22_v20_v12 = vector_mix<mix4::w, mix4::x, mix4::a, mix4::c>(v20_v00_v10_v22, v12_v22_v02);

		vector4d y_axis = vector_neg_mul_sub(v10_v02_v00, v22_v20_v12, v20v12_v00v22_v10v02);

		const vector4d v10_v20_v00 = vector_mix<mix4::z, mix4::x, mix4::c, mix4::c>(v20_v00_v10_v22, v10_v02_v00);
		const vector4d v21_v01_v11 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v11_v21_v01, v12_v01_v11);
		const vector4d v10v21_v20v01_v00v11 = vector_mul(v10_v20_v00, v21_v01_v11);

		const vector4d v20_v00_v01 = vector_mix<mix4::y, mix4::z, mix4::c, mix4::c>(v10_v20_v00, v11_v21_v01);
		const vector4d v11_v21_v10 = vector_mix<mix4::x, mix4::y, mix4::a, mix4::c>(v11_v21_v01, v10_v20_v00);

		vector4d z_axis = vector_neg_mul_sub(v20_v00_v01, v11_v21_v10, v10v21_v20v01_v00v11);

		const vector4d o00_o00_o10_o10 = vector_mix<mix4::x, mix4::x, mix4::a, mix4::a>(x_axis, y_axis);
		const vector4d o00_o10_o20 = vector_mix<mix4::x, mix4::z, mix4::a, mix4::a>(o00_o00_o10_o10, z_axis);

		return vector_dot3_as_scalar(o00_o10_o20, input.x_axis);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the minor of the 3x3 rotation/scale part of the input 3x4 matrix.
	// See: https://en.wikipedia.org/wiki/Minor_(linear_algebra)
	// The minor is the determinant of the sub-matrix input when the specified
	// row and column are removed.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline scalard RTM_SIMD_CALL matrix_minor(matrix3x4d_arg0 input, axis3 row, axis3 column) RTM_NO_EXCEPT
	{
		// The minor boils down to calculating the determinant of a 2x2 matrix.
		// det([a, b], [c, d]) = (a * d) - (b * c)

		// Find which two rows we need.
		vector4d row0;
		vector4d row1;
		if (row == axis3::x)
		{
			row0 = input.y_axis;
			row1 = input.z_axis;
		}
		else if (row == axis3::y)
		{
			row0 = input.x_axis;
			row1 = input.z_axis;
		}
		else
		{
			row0 = input.x_axis;
			row1 = input.y_axis;
		}

		// Because our input is a 3x3 matrix, there are only a few possibilities for the 2x2 part:
		// row0 = [x0, y0, z0]
		// row1 = [x1, y1, z1]
		// det([x0, y0], [x1, y1]) = (x0 * y1) - (y0 * x1) (z removed)
		// det([x0, z0], [x1, z1]) = (x0 * z1) - (z0 * x1) (y removed)
		// det([y0, z0], [y1, z1]) = (y0 * z1) - (z0 * y1) (x removed)
		// det([column0, column1], [column2, column3]) = (column0 * column3) - (column1 * column2)

		// For performance reasons, we can compute all three determinants at the same time.
		const vector4d column0 = vector_mix<mix4::x, mix4::x, mix4::y, mix4::y>(row0, row0);
		const vector4d column1 = vector_mix<mix4::y, mix4::z, mix4::z, mix4::z>(row0, row0);
		const vector4d column2 = vector_mix<mix4::x, mix4::x, mix4::y, mix4::y>(row1, row1);
		const vector4d column3 = vector_mix<mix4::y, mix4::z, mix4::z, mix4::z>(row1, row1);

		const vector4d determinants = vector_neg_mul_sub(column1, column2, vector_mul(column0, column3));

		// Extract the one we need
		if (column == axis3::x)
			return vector_get_z_as_scalar(determinants);
		else if (column == axis3::y)
			return vector_get_y_as_scalar(determinants);
		else
			return vector_get_x_as_scalar(determinants);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the cofactor matrix of the 3x3 rotation/scale part of the input 3x4 matrix.
	// See: https://en.wikipedia.org/wiki/Minor_(linear_algebra)#Cofactor_expansion_of_the_determinant
	// Note: The proper way to transform a normal by an affine matrix with non-uniform scale
	// is to multiply the normal with the cofactor matrix of the 3x3 rotation/scale part.
	// See: https://github.com/graphitemaster/normals_revisited
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x3d RTM_SIMD_CALL matrix_cofactor(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		const vector4d x_axis = vector_cross3(input.y_axis, input.z_axis);
		const vector4d y_axis = vector_cross3(input.z_axis, input.x_axis);
		const vector4d z_axis = vector_cross3(input.x_axis, input.y_axis);
		return matrix3x3d{ x_axis, y_axis, z_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the adjugate of the input matrix.
	// See: https://en.wikipedia.org/wiki/Adjugate_matrix
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x3d RTM_SIMD_CALL matrix_adjugate(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		return matrix_transpose(matrix_cofactor(input));
	}

	//////////////////////////////////////////////////////////////////////////
	// Removes the 3D scale from a 3x4 affine matrix.
	// Note that if the scaling is 0.0 for a particular axis, the original rotation axis cannot
	// be recovered trivially and no attempt is done to do so. In theory, we could use the other axes
	// to try and recover it.
	// TODO: Implement rotation recovering, perhaps in a separate function and rename this
	// one to matrix_remove_non_zero_scale(..)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline matrix3x4d RTM_SIMD_CALL matrix_remove_scale(matrix3x4d_arg0 input) RTM_NO_EXCEPT
	{
		matrix3x4d result;
		result.x_axis = vector_normalize3(input.x_axis, input.x_axis);
		result.y_axis = vector_normalize3(input.y_axis, input.y_axis);
		result.z_axis = vector_normalize3(input.z_axis, input.z_axis);
		result.w_axis = input.w_axis;
		return result;
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
