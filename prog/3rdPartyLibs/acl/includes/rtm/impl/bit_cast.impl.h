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

#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/detect_cpp_version.h"

#if RTM_CPP_VERSION >= RTM_CPP_VERSION_20
	#if __has_include(<bit>)
		#include <bit>
	#endif
#endif
#if defined(__clang__) && !defined(__cpp_lib_bit_cast)
	#include <type_traits>
#endif

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// C++20 introduced std::bit_cast which is safer than reinterpret_cast
		//////////////////////////////////////////////////////////////////////////

	#if RTM_CPP_VERSION >= RTM_CPP_VERSION_20 && defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
		using std::bit_cast;
	#elif defined(__clang__)
		template <class dest_type_t, class src_type_t,
		std::enable_if_t<std::conjunction_v<std::bool_constant<sizeof(dest_type_t) == sizeof(src_type_t)>,
				std::is_trivially_copyable<dest_type_t>, std::is_trivially_copyable<src_type_t>>, int> = 0>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr dest_type_t bit_cast(src_type_t input) noexcept
		{
			return __builtin_bit_cast(dest_type_t, input);
		}
  #else
		template<class dest_type_t, class src_type_t>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr dest_type_t bit_cast(src_type_t input) noexcept
		{
			return reinterpret_cast<dest_type_t>(input);
		}
	#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
