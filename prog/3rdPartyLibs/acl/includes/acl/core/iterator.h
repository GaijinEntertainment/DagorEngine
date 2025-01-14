#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Animation Compression Library contributors
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

#include <cstdint>
#include <iterator>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		template <class item_type, bool is_const>
		class array_iterator_impl
		{
		public:
			using item_ptr_type = typename std::conditional<is_const, const item_type*, item_type*>::type;

			constexpr array_iterator_impl(item_ptr_type items, size_t num_items) : m_items(items), m_num_items(num_items) {}

			constexpr item_ptr_type begin() const { return m_items; }
			constexpr item_ptr_type end() const { return m_items + m_num_items; }

		private:
			item_ptr_type	m_items;
			size_t			m_num_items;
		};

		template <class item_type, bool is_const>
		class array_reverse_iterator_impl
		{
		public:
			using item_ptr_type = typename std::conditional<is_const, const item_type*, item_type*>::type;

			constexpr array_reverse_iterator_impl(item_ptr_type items, size_t num_items) : m_items(items), m_num_items(num_items) {}

			constexpr std::reverse_iterator<item_ptr_type> begin() const { return std::reverse_iterator<item_ptr_type>(m_items + m_num_items); }
			constexpr std::reverse_iterator<item_ptr_type> end() const { return std::reverse_iterator<item_ptr_type>(m_items); }

		private:
			item_ptr_type	m_items;
			size_t			m_num_items;
		};
	}

	//ACL_DEPRECATED("Renamed to array_iterator, to be removed in v3.0")
	template <class item_type>
	using iterator = acl_impl::array_iterator_impl<item_type, false>;

	template <class item_type>
	using array_iterator = acl_impl::array_iterator_impl<item_type, false>;

	//ACL_DEPRECATED("Renamed to const_array_iterator, to be removed in v3.0")
	template <class item_type>
	using const_iterator = acl_impl::array_iterator_impl<item_type, true>;

	template <class item_type>
	using const_array_iterator = acl_impl::array_iterator_impl<item_type, true>;

	template <class item_type, size_t num_items>
	array_iterator<item_type> make_iterator(item_type (&items)[num_items])
	{
		return array_iterator<item_type>(items, num_items);
	}

	template <class item_type, size_t num_items>
	const_array_iterator<item_type> make_iterator(item_type const (&items)[num_items])
	{
		return const_array_iterator<item_type>(items, num_items);
	}

	template <class item_type>
	array_iterator<item_type> make_iterator(item_type* items, size_t num_items)
	{
		return array_iterator<item_type>(items, num_items);
	}

	template <class item_type>
	const_array_iterator<item_type> make_iterator(const item_type* items, size_t num_items)
	{
		return const_array_iterator<item_type>(items, num_items);
	}

	template <class item_type>
	using array_reverse_iterator = acl_impl::array_reverse_iterator_impl<item_type, false>;

	template <class item_type>
	using const_array_reverse_iterator = acl_impl::array_reverse_iterator_impl<item_type, true>;

	template <class item_type, size_t num_items>
	array_reverse_iterator<item_type> make_reverse_iterator(item_type(&items)[num_items])
	{
		return array_reverse_iterator<item_type>(items, num_items);
	}

	template <class item_type, size_t num_items>
	const_array_reverse_iterator<item_type> make_reverse_iterator(item_type const (&items)[num_items])
	{
		return const_array_reverse_iterator<item_type>(items, num_items);
	}

	template <class item_type>
	array_reverse_iterator<item_type> make_reverse_iterator(item_type* items, size_t num_items)
	{
		return array_reverse_iterator<item_type>(items, num_items);
	}

	template <class item_type>
	const_array_reverse_iterator<item_type> make_reverse_iterator(const item_type* items, size_t num_items)
	{
		return const_array_reverse_iterator<item_type>(items, num_items);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
