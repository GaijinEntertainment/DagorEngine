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
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct mask4_bool_set
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4d() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				const uint64_t x_mask = x ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t y_mask = y ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t z_mask = z ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t w_mask = w ? 0xFFFFFFFFFFFFFFFFULL : 0;

				return mask4d{ _mm_castsi128_pd(_mm_set_epi64x(static_cast<int64_t>(y_mask), static_cast<int64_t>(x_mask))), _mm_castsi128_pd(_mm_set_epi64x(static_cast<int64_t>(w_mask), static_cast<int64_t>(z_mask))) };
#else
				const uint64_t x_mask = x ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t y_mask = y ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t z_mask = z ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t w_mask = w ? 0xFFFFFFFFFFFFFFFFULL : 0;

				return mask4d{ x_mask, y_mask, z_mask, w_mask };
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4q() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				const uint64_t x_mask = x ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t y_mask = y ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t z_mask = z ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t w_mask = w ? 0xFFFFFFFFFFFFFFFFULL : 0;

				return mask4q{ _mm_set_epi64x(static_cast<int64_t>(y_mask), static_cast<int64_t>(x_mask)), _mm_set_epi64x(static_cast<int64_t>(w_mask), static_cast<int64_t>(z_mask)) };
#else
				const uint64_t x_mask = x ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t y_mask = y ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t z_mask = z ? 0xFFFFFFFFFFFFFFFFULL : 0;
				const uint64_t w_mask = w ? 0xFFFFFFFFFFFFFFFFULL : 0;

				return mask4q{ x_mask, y_mask, z_mask, w_mask };
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4f() const RTM_NO_EXCEPT
			{
				const uint32_t x_mask = x ? 0xFFFFFFFFU : 0;
				const uint32_t y_mask = y ? 0xFFFFFFFFU : 0;
				const uint32_t z_mask = z ? 0xFFFFFFFFU : 0;
				const uint32_t w_mask = w ? 0xFFFFFFFFU : 0;

#if defined(RTM_SSE2_INTRINSICS)
				return _mm_castsi128_ps(_mm_set_epi32(static_cast<int32_t>(w_mask), static_cast<int32_t>(z_mask), static_cast<int32_t>(y_mask), static_cast<int32_t>(x_mask)));
#elif defined(RTM_NEON_INTRINSICS)
				uint32x2_t V0 = vcreate_u32(((uint64_t)x_mask) | ((uint64_t)(y_mask) << 32));
				uint32x2_t V1 = vcreate_u32(((uint64_t)z_mask) | ((uint64_t)(w_mask) << 32));
				return vcombine_u32(V0, V1);
#else
				return mask4f{ x_mask, y_mask, z_mask, w_mask };
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4i() const RTM_NO_EXCEPT
			{
				const uint32_t x_mask = x ? 0xFFFFFFFFU : 0;
				const uint32_t y_mask = y ? 0xFFFFFFFFU : 0;
				const uint32_t z_mask = z ? 0xFFFFFFFFU : 0;
				const uint32_t w_mask = w ? 0xFFFFFFFFU : 0;

#if defined(RTM_SSE2_INTRINSICS)
				return _mm_set_epi32(static_cast<int32_t>(w_mask), static_cast<int32_t>(z_mask), static_cast<int32_t>(y_mask), static_cast<int32_t>(x_mask));
#elif defined(RTM_NEON_INTRINSICS)
				uint32x2_t V0 = vcreate_u32(((uint64_t)x_mask) | ((uint64_t)(y_mask) << 32));
				uint32x2_t V1 = vcreate_u32(((uint64_t)z_mask) | ((uint64_t)(w_mask) << 32));
				return RTM_IMPL_MASK4i_SET(vcombine_u32(V0, V1));
#else
				return mask4i{ x_mask, y_mask, z_mask, w_mask };
#endif
			}

			bool x;
			bool y;
			bool z;
			bool w;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a mask4 from all 4 bool components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::mask4_bool_set RTM_SIMD_CALL mask_set(bool x, bool y, bool z, bool w) RTM_NO_EXCEPT
	{
		return rtm_impl::mask4_bool_set{ x, y, z, w };
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct mask4_uint32_set
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_castsi128_ps(_mm_set_epi32(static_cast<int32_t>(w), static_cast<int32_t>(z), static_cast<int32_t>(y), static_cast<int32_t>(x)));
#elif defined(RTM_NEON_INTRINSICS)
				uint32x2_t V0 = vcreate_u32(((uint64_t)x) | ((uint64_t)(y) << 32));
				uint32x2_t V1 = vcreate_u32(((uint64_t)z) | ((uint64_t)(w) << 32));
				return vcombine_u32(V0, V1);
#else
				return mask4f{ x, y, z, w };
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4i() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_set_epi32(static_cast<int32_t>(w), static_cast<int32_t>(z), static_cast<int32_t>(y), static_cast<int32_t>(x));
#elif defined(RTM_NEON_INTRINSICS)
				uint32x2_t V0 = vcreate_u32(((uint64_t)x) | ((uint64_t)(y) << 32));
				uint32x2_t V1 = vcreate_u32(((uint64_t)z) | ((uint64_t)(w) << 32));
				return RTM_IMPL_MASK4i_SET(vcombine_u32(V0, V1));
#else
				return mask4i{ x, y, z, w };
#endif
			}

			uint32_t x;
			uint32_t y;
			uint32_t z;
			uint32_t w;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a mask4 from 4 uint32 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::mask4_uint32_set RTM_SIMD_CALL mask_set(uint32_t x, uint32_t y, uint32_t z, uint32_t w) RTM_NO_EXCEPT
	{
		return rtm_impl::mask4_uint32_set{ x, y, z, w };
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct mask4_uint64_set
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4d() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// HACK ALERT!
	// VS2015 and VS2017 crash when compiling with _mm_set_epi64x() here.
	// To work around this, we use alternative code. We assume that the high and low words
	// are identical in the mask, which should be true.
	// See: https://github.com/nfrechette/rtm/issues/84
	//////////////////////////////////////////////////////////////////////////
	#if defined(RTM_COMPILER_MSVC) && RTM_COMPILER_MSVC < RTM_COMPILER_MSVC_2019 && defined(RTM_ARCH_X86) && !defined(NDEBUG)
				const uint32_t x_mask = x ? 0xFFFFFFFFU : 0;
				const uint32_t y_mask = y ? 0xFFFFFFFFU : 0;
				const uint32_t z_mask = z ? 0xFFFFFFFFU : 0;
				const uint32_t w_mask = w ? 0xFFFFFFFFU : 0;

				return mask4d{ _mm_castsi128_pd(_mm_set_epi32(static_cast<int32_t>(y_mask), static_cast<int32_t>(y_mask), static_cast<int32_t>(x_mask), static_cast<int32_t>(x_mask))), _mm_castsi128_pd(_mm_set_epi32(static_cast<int32_t>(w_mask), static_cast<int32_t>(w_mask), static_cast<int32_t>(z_mask), static_cast<int32_t>(z_mask))) };
	#else
				return mask4d{ _mm_castsi128_pd(_mm_set_epi64x(static_cast<int64_t>(y), static_cast<int64_t>(x))), _mm_castsi128_pd(_mm_set_epi64x(static_cast<int64_t>(w), static_cast<int64_t>(z))) };
	#endif
#else
				return mask4d{ x, y, z, w };
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator mask4q() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// HACK ALERT!
	// VS2015 and VS2017 crash when compiling with _mm_set_epi64x() here.
	// To work around this, we use alternative code. We assume that the high and low words
	// are identical in the mask, which should be true.
	// See: https://github.com/nfrechette/rtm/issues/84
	//////////////////////////////////////////////////////////////////////////
	#if defined(RTM_COMPILER_MSVC) && RTM_COMPILER_MSVC < RTM_COMPILER_MSVC_2019 && defined(RTM_ARCH_X86) && !defined(NDEBUG)
				const uint32_t x_mask = x ? 0xFFFFFFFFU : 0;
				const uint32_t y_mask = y ? 0xFFFFFFFFU : 0;
				const uint32_t z_mask = z ? 0xFFFFFFFFU : 0;
				const uint32_t w_mask = w ? 0xFFFFFFFFU : 0;

				return mask4q{ _mm_set_epi32(static_cast<int32_t>(y_mask), static_cast<int32_t>(y_mask), static_cast<int32_t>(x_mask), static_cast<int32_t>(x_mask)), _mm_set_epi32(static_cast<int32_t>(w_mask), static_cast<int32_t>(w_mask), static_cast<int32_t>(z_mask), static_cast<int32_t>(z_mask)) };
	#else
				return mask4q{ _mm_set_epi64x(static_cast<int64_t>(y), static_cast<int64_t>(x)), _mm_set_epi64x(static_cast<int64_t>(w), static_cast<int64_t>(z)) };
	#endif
#else
				return mask4q{ x, y, z, w };
#endif
			}

			uint64_t x;
			uint64_t y;
			uint64_t z;
			uint64_t w;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a mask4 from 4 uint64 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::mask4_uint64_set RTM_SIMD_CALL mask_set(uint64_t x, uint64_t y, uint64_t z, uint64_t w) RTM_NO_EXCEPT
	{
		return rtm_impl::mask4_uint64_set{ x, y, z, w };
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a mask4 with all 4 components set to true.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::mask4_bool_set RTM_SIMD_CALL mask_true() RTM_NO_EXCEPT
	{
		return rtm_impl::mask4_bool_set{ true, true, true, true };
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a mask4 with all 4 components set to false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::mask4_bool_set RTM_SIMD_CALL mask_false() RTM_NO_EXCEPT
	{
		return rtm_impl::mask4_bool_set{ false, false, false, false };
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
