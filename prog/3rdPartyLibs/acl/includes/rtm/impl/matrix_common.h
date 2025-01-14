#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
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
#include "rtm/vector4f.h"
#include "rtm/vector4d.h"
#include "rtm/version.h"
#include "rtm/type_traits.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/matrix_cast.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various matrix types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct matrix_identity_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix3x3d() const RTM_NO_EXCEPT
			{
				return matrix3x3d{ vector_set(1.0, 0.0, 0.0, 0.0), vector_set(0.0, 1.0, 0.0, 0.0), vector_set(0.0, 0.0, 1.0, 0.0) };
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix3x3f() const RTM_NO_EXCEPT
			{
				return matrix3x3f{ vector_set(1.0F, 0.0F, 0.0F, 0.0F), vector_set(0.0F, 1.0F, 0.0F, 0.0F), vector_set(0.0F, 0.0F, 1.0F, 0.0F) };
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix3x4d() const RTM_NO_EXCEPT
			{
				return matrix3x4d{ vector_set(1.0, 0.0, 0.0, 0.0), vector_set(0.0, 1.0, 0.0, 0.0), vector_set(0.0, 0.0, 1.0, 0.0), vector_set(0.0, 0.0, 0.0, 1.0) };
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix3x4f() const RTM_NO_EXCEPT
			{
				return matrix3x4f{ vector_set(1.0F, 0.0F, 0.0F, 0.0F), vector_set(0.0F, 1.0F, 0.0F, 0.0F), vector_set(0.0F, 0.0F, 1.0F, 0.0F), vector_set(0.0F, 0.0F, 0.0F, 1.0F) };
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix4x4d() const RTM_NO_EXCEPT
			{
				return matrix4x4d{ vector_set(1.0, 0.0, 0.0, 0.0), vector_set(0.0, 1.0, 0.0, 0.0), vector_set(0.0, 0.0, 1.0, 0.0), vector_set(0.0, 0.0, 0.0, 1.0) };
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator matrix4x4f() const RTM_NO_EXCEPT
			{
				return matrix4x4f{ vector_set(1.0F, 0.0F, 0.0F, 0.0F), vector_set(0.0F, 1.0F, 0.0F, 0.0F), vector_set(0.0F, 0.0F, 1.0F, 0.0F), vector_set(0.0F, 0.0F, 0.0F, 1.0F) };
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// A helper struct to set matrices with similar width.
		// Note: We use a float type as a template argument because GCC loses alignment
		// attributes on template argument types.
		//////////////////////////////////////////////////////////////////////////
		template<typename float_type>
		struct matrix_setter4x4
		{
			using vector4 = typename related_types<float_type>::vector4;

			//////////////////////////////////////////////////////////////////////////
			// Sets all 4 axes and creates a 3x4 affine matrix.
			//////////////////////////////////////////////////////////////////////////
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator matrix3x4d() const RTM_NO_EXCEPT
			{
				return matrix3x4d{ x_axis, y_axis, z_axis, w_axis };
			}

			//////////////////////////////////////////////////////////////////////////
			// Sets all 4 axes and creates a 3x4 affine matrix.
			//////////////////////////////////////////////////////////////////////////
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator matrix3x4f() const RTM_NO_EXCEPT
			{
				return matrix3x4f{ x_axis, y_axis, z_axis, w_axis };
			}

			//////////////////////////////////////////////////////////////////////////
			// Sets all 4 axes and creates a 4x4 matrix.
			//////////////////////////////////////////////////////////////////////////
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator matrix4x4d() const RTM_NO_EXCEPT
			{
				return matrix4x4d{ x_axis, y_axis, z_axis, w_axis };
			}

			//////////////////////////////////////////////////////////////////////////
			// Sets all 4 axes and creates a 4x4 matrix.
			//////////////////////////////////////////////////////////////////////////
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator matrix4x4f() const RTM_NO_EXCEPT
			{
				return matrix4x4f{ x_axis, y_axis, z_axis, w_axis };
			}

			vector4	x_axis;
			vector4	y_axis;
			vector4	z_axis;
			vector4	w_axis;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets all 3 axes and creates a matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr matrix3x3f RTM_SIMD_CALL matrix_set(vector4f_arg0 x_axis, vector4f_arg1 y_axis, vector4f_arg2 z_axis) RTM_NO_EXCEPT
	{
		return matrix3x3f{ x_axis, y_axis, z_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets all 3 axes and creates a matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr matrix3x3d RTM_SIMD_CALL matrix_set(vector4d_arg0 x_axis, vector4d_arg1 y_axis, vector4d_arg2 z_axis) RTM_NO_EXCEPT
	{
		return matrix3x3d{ x_axis, y_axis, z_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets all 4 axes and creates a matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::matrix_setter4x4<float> RTM_SIMD_CALL matrix_set(vector4f_arg0 x_axis, vector4f_arg1 y_axis, vector4f_arg2 z_axis, vector4f_arg3 w_axis) RTM_NO_EXCEPT
	{
		return rtm_impl::matrix_setter4x4<float>{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets all 4 axes and creates a matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::matrix_setter4x4<double> RTM_SIMD_CALL matrix_set(vector4d_arg0 x_axis, vector4d_arg1 y_axis, vector4d_arg2 z_axis, vector4d_arg3 w_axis) RTM_NO_EXCEPT
	{
		return rtm_impl::matrix_setter4x4<double>{ x_axis, y_axis, z_axis, w_axis };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the identity matrix.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::matrix_identity_impl RTM_SIMD_CALL matrix_identity() RTM_NO_EXCEPT
	{
		return rtm_impl::matrix_identity_impl();
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
