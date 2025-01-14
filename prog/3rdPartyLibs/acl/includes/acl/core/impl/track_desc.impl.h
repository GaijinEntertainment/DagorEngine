#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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

// Included only once from track_desc.h

#include "acl/version.h"
#include "acl/core/error_result.h"
#include "acl/core/track_types.h"

#include <rtm/qvvf.h>
#include <rtm/scalarf.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline error_result track_desc_scalarf::is_valid() const
	{
		if (precision < 0.0F || !rtm::scalar_is_finite(precision))
			return error_result("Invalid precision");

		return error_result();
	}

	inline error_result track_desc_transformf::is_valid() const
	{
		if (precision < 0.0F || !rtm::scalar_is_finite(precision))
			return error_result("Invalid precision");

		if (shell_distance < 0.0F || !rtm::scalar_is_finite(shell_distance))
			return error_result("Invalid shell_distance");

		if (!rtm::qvv_is_finite(default_value))
			return error_result("Invalid default_value must be finite");

		if (!rtm::quat_is_normalized(default_value.rotation))
			return error_result("Description default_value rotation is not normalized");

		return error_result();
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
