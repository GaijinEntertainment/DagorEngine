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
#include "acl/core/impl/compiler_utils.h"
#include <acl/core/iallocator.h>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	template<typename allocated_type>
	class deleter
	{
	public:
		deleter() noexcept : m_allocator(nullptr) {}
		explicit deleter(iallocator& allocator) noexcept : m_allocator(&allocator) {}
		deleter(const deleter&) noexcept = default;
		deleter(deleter&&) noexcept = default;
		~deleter() noexcept = default;
		deleter& operator=(const deleter&) noexcept = default;
		deleter& operator=(deleter&&) noexcept = default;

		void operator()(allocated_type* ptr)
		{
			if (ptr == nullptr)
				return;

			if (!std::is_trivially_destructible<allocated_type>::value)
				ptr->~allocated_type();

			m_allocator->deallocate(ptr, sizeof(allocated_type));
		}

	private:
		iallocator* m_allocator;
	};

	template<typename allocated_type, typename... args>
	std::unique_ptr<allocated_type, deleter<allocated_type>> make_unique(iallocator& allocator, args&&... arguments)
	{
		return std::unique_ptr<allocated_type, deleter<allocated_type>>(
			allocate_type<allocated_type>(allocator, std::forward<args>(arguments)...),
			deleter<allocated_type>(allocator));
	}

	template<typename allocated_type, typename... args>
	std::unique_ptr<allocated_type, deleter<allocated_type>> make_unique_aligned(iallocator& allocator, size_t alignment, args&&... arguments)
	{
		return std::unique_ptr<allocated_type, deleter<allocated_type>>(
			allocate_type_aligned<allocated_type>(allocator, alignment, std::forward<args>(arguments)...),
			deleter<allocated_type>(allocator));
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
