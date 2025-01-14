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
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various scalar types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct scalar_loaderf
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				return *ptr;
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ _mm_load_ss(ptr) };
			}
#endif

			const float* ptr;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various scalar types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct scalar_loaderd
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				return *ptr;
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ _mm_load_sd(ptr) };
			}
#endif

			const double* ptr;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads a scalar from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::scalar_loaderf RTM_SIMD_CALL scalar_load(const float* input) RTM_NO_EXCEPT
	{
		return rtm_impl::scalar_loaderf{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads a scalar from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL scalar_load_as_scalar(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_load_ss(input) };
#else
		return scalar_load(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads a scalar from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::scalar_loaderd RTM_SIMD_CALL scalar_load(const double* input) RTM_NO_EXCEPT
	{
		return rtm_impl::scalar_loaderd{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads a scalar from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL scalar_load_as_scalar(const double* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_load_sd(input) };
#else
		return scalar_load(input);
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
