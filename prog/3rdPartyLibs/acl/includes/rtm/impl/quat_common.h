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
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set(float x, float y, float z, float w) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_set_ps(w, z, y, x);
#elif defined(RTM_NEON_INTRINSICS)
		float32x2_t V0 = vset_lane_f32(y, vmov_n_f32(x), 1);
		float32x2_t V1 = vset_lane_f32(w, vmov_n_f32(z), 1);
		return vcombine_f32(V0, V1);
#else
		return quatf{ x, y, z, w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatd RTM_SIMD_CALL quat_set(double x, double y, double z, double w) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return quatd{ _mm_set_pd(y, x), _mm_set_pd(w, z) };
#else
		return quatd{ x, y, z, w };
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various quaternion types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quat_identity_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator quatd() const RTM_NO_EXCEPT
			{
				return quat_set(0.0, 0.0, 0.0, 1.0);
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator quatf() const RTM_NO_EXCEPT
			{
				return quat_set(0.0F, 0.0F, 0.0F, 1.0F);
			}
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the identity quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quat_identity_impl RTM_SIMD_CALL quat_identity() RTM_NO_EXCEPT
	{
		return rtm_impl::quat_identity_impl();
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
