#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Realtime Math contributors
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
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion on the hypersphere with a positive [w] component
	// that represents the same 3D rotation as the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd quat_ensure_positive_w(const quatd& input) RTM_NO_EXCEPT
	{
		return quat_get_w(input) >= 0.0 ? input : quat_neg(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a quaternion constructed from a vector3 representing the [xyz]
	// components while reconstructing the [w] component by assuming it is positive.
	// Note: When the squared length of [xyz] is very small, the square-root might not
	// be very accurate when [w] is reconstructed. As such, the resulting quaternion
	// might be near but not quite normalized. If high accuracy is required, make
	// sure to normalize explicitly afterwards.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd quat_from_positive_w(const vector4d& input) RTM_NO_EXCEPT
	{
		const double input_x = vector_get_x(input);
		const double input_y = vector_get_y(input);
		const double input_z = vector_get_z(input);

		// Operation order is important here, due to rounding, ((1.0 - (X*X)) - Y*Y) - Z*Z is more accurate than 1.0 - dot3(xyz, xyz)
		const double w_squared = ((1.0 - (input_x * input_x)) - (input_y * input_y)) - (input_z * input_z);

		// w_squared can be negative either due to rounding or due to quantization imprecision, we take the absolute value
		// to ensure the resulting quaternion is always normalized with a positive W component
		const double w = scalar_sqrt(scalar_abs(w_squared));
		return quat_set_w(vector_to_quat(input), w);
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
