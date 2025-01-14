#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2018 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/memory_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// Represents an invalid pointer offset, used by 'ptr_offset'.
	////////////////////////////////////////////////////////////////////////////////
	struct invalid_ptr_offset final {};

	////////////////////////////////////////////////////////////////////////////////
	// A type safe pointer offset.
	//
	// This class only wraps an integer of the 'offset_type' type and adds type safety
	// by only casting to 'data_type'.
	////////////////////////////////////////////////////////////////////////////////
	template<typename data_type, typename offset_type>
	class ptr_offset
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Constructs a valid but empty offset.
		constexpr ptr_offset() : m_value(0) {}

		////////////////////////////////////////////////////////////////////////////////
		// Constructs a valid offset with the specified value.
		// Expanded to avoid overflow warnings during assignment.
		constexpr ptr_offset(uint64_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(int64_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(uint32_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(int32_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(uint16_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(int16_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(uint8_t value) : m_value(safe_static_cast<offset_type>(value)) {}
		constexpr ptr_offset(int8_t value) : m_value(safe_static_cast<offset_type>(value)) {}

		//////////////////////////////////////////////////////////////////////////
		// Constructs a valid offset from a base pointer and another pointer part of the same aggregate.
		constexpr ptr_offset(const void* base, const void* ptr) : m_value(safe_static_cast<offset_type>(acl_impl::bit_cast<const uint8_t*>(ptr) - acl_impl::bit_cast<const uint8_t*>(base))) {}

		////////////////////////////////////////////////////////////////////////////////
		// Constructs an invalid offset.
		constexpr ptr_offset(invalid_ptr_offset) : m_value(k_invalid_value) {}

		////////////////////////////////////////////////////////////////////////////////
		// Adds this offset to the provided pointer.
		template<typename BaseType>
		inline data_type* add_to(BaseType* ptr) const
		{
			ACL_ASSERT(is_valid(), "Invalid ptr_offset!");
			return add_offset_to_ptr<data_type>(ptr, m_value);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Adds this offset to the provided pointer.
		template<typename BaseType>
		inline const data_type* add_to(const BaseType* ptr) const
		{
			ACL_ASSERT(is_valid(), "Invalid ptr_offset!");
			return add_offset_to_ptr<const data_type>(ptr, m_value);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Adds this offset to the provided pointer or returns nullptr if the offset is invalid.
		template<typename BaseType>
		inline data_type* safe_add_to(BaseType* ptr) const
		{
			return is_valid() ? add_offset_to_ptr<data_type>(ptr, m_value) : nullptr;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Adds this offset to the provided pointer or returns nullptr if the offset is invalid.
		template<typename BaseType>
		inline const data_type* safe_add_to(const BaseType* ptr) const
		{
			return is_valid() ? add_offset_to_ptr<data_type>(ptr, m_value) : nullptr;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Coercion operator to the underlying 'offset_type'.
		constexpr operator offset_type() const { return m_value; }

		////////////////////////////////////////////////////////////////////////////////
		// Returns true if the offset is valid.
		constexpr bool is_valid() const { return m_value != k_invalid_value; }

	private:
		// Value representing an invalid offset
		static constexpr offset_type k_invalid_value = (std::numeric_limits<offset_type>::max)();

		// Actual offset value.
		offset_type m_value;
	};

	////////////////////////////////////////////////////////////////////////////////
	// A 16 bit offset.
	template<typename data_type>
	using ptr_offset16 = ptr_offset<data_type, uint16_t>;

	////////////////////////////////////////////////////////////////////////////////
	// A 32 bit offset.
	template<typename data_type>
	using ptr_offset32 = ptr_offset<data_type, uint32_t>;

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
