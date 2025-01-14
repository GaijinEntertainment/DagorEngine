#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

// This macro defines common operators for manipulating bit flags

#define ACL_IMPL_ENUM_FLAGS_OPERATORS(enum_type) \
	constexpr enum_type operator|(enum_type lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		return static_cast<enum_type>(static_cast<raw_type>(lhs) | static_cast<raw_type>(rhs)); \
	} \
	inline void operator|=(enum_type& lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		lhs = static_cast<enum_type>(static_cast<raw_type>(lhs) | static_cast<raw_type>(rhs)); \
	} \
	constexpr enum_type operator&(enum_type lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		return static_cast<enum_type>(static_cast<raw_type>(lhs) & static_cast<raw_type>(rhs)); \
	} \
	inline void operator&=(enum_type& lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		lhs = static_cast<enum_type>(static_cast<raw_type>(lhs) & static_cast<raw_type>(rhs)); \
	} \
	constexpr enum_type operator^(enum_type lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		return static_cast<enum_type>(static_cast<raw_type>(lhs) ^ static_cast<raw_type>(rhs)); \
	} \
	inline void operator^=(enum_type& lhs, enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		lhs = static_cast<enum_type>(static_cast<raw_type>(lhs) ^ static_cast<raw_type>(rhs)); \
	} \
	constexpr enum_type operator~(enum_type rhs) \
	{ \
		using integral_type = typename std::underlying_type<enum_type>::type; \
		using raw_type = typename std::make_unsigned<integral_type>::type; \
		return static_cast<enum_type>(~static_cast<raw_type>(rhs)); \
	}

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// Returns true if any of the requested flags are set.
	template<typename enum_type>
	constexpr bool are_any_enum_flags_set(enum_type flags, enum_type flags_to_test)
	{
		using IntegralType = typename std::underlying_type<enum_type>::type;
		return static_cast<IntegralType>(flags & flags_to_test) != 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns true if all of the requested flags are set.
	template<typename enum_type>
	constexpr bool are_all_enum_flags_set(enum_type flags, enum_type flags_to_test)
	{
		using IntegralType = typename std::underlying_type<enum_type>::type;
		return static_cast<IntegralType>(flags & flags_to_test) == static_cast<IntegralType>(flags_to_test);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
