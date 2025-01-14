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

#include <cstdint>
#include <cstring>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace hash_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		// FNV 1a hash implementation for 32/64 bit hashes.
		////////////////////////////////////////////////////////////////////////////////
		template <typename ResultType, ResultType OffsetBasis, ResultType Prime>
		class fnv1a_impl final
		{
		public:
			////////////////////////////////////////////////////////////////////////////////
			// Constructs a hash structures and initializes it with the offset basis.
			constexpr fnv1a_impl()
				: m_state(OffsetBasis)
			{}

			////////////////////////////////////////////////////////////////////////////////
			// Updates the current hash with the new provided data.
			//
			// data: A pointer to the memory buffer to hash.
			// size: The memory buffer size in bytes to hash.
			void update(const void* data, size_t size)
			{
				const uint8_t* cdata = static_cast<const uint8_t*>(data);
				ResultType acc = m_state;
				for (size_t i = 0; i < size; ++i)
				{
					const ResultType next = cdata[i];
					acc = (acc ^ next) * Prime;
				}
				m_state = acc;
			}

			////////////////////////////////////////////////////////////////////////////////
			// Returns the current hash digest value.
			constexpr ResultType digest() const { return m_state; }

		private:
			static_assert(std::is_unsigned<ResultType>::value, "need unsigned integer");

			// The running hash digest value.
			ResultType m_state;
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	// A 32 bit hash instance type
	using fnv1a_32 = hash_impl::fnv1a_impl<uint32_t, 2166136261U, 16777619U>;

	////////////////////////////////////////////////////////////////////////////////
	// A 64 bit hash instance type
	using fnv1a_64 = hash_impl::fnv1a_impl<uint64_t, 14695981039346656037ULL, 1099511628211ULL>;

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 32 bit hash of the provided buffer and size in bytes.
	inline uint32_t hash32(const void* buffer, size_t buffer_size)
	{
		fnv1a_32 hashfn = fnv1a_32();
		hashfn.update(buffer, buffer_size);
		return hashfn.digest();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 32 bit hash of the provided element.
	template<typename ElementType>
	inline uint32_t hash32(const ElementType& element) { return hash32(&element, sizeof(ElementType)); }

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 32 bit hash of the provided string.
	// The null terminator not included in the hash.
	inline uint32_t hash32(const char* str)
	{
		const size_t buffer_size = str != nullptr ? std::strlen(str) : 0;
		return hash32(str, buffer_size);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 64 bit hash of the provided buffer and size in bytes.
	inline uint64_t hash64(const void* buffer, size_t buffer_size)
	{
		fnv1a_64 hashfn = fnv1a_64();
		hashfn.update(buffer, buffer_size);
		return hashfn.digest();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 64 bit hash of the provided element.
	template<typename ElementType>
	inline uint64_t hash64(const ElementType& element) { return hash64(&element, sizeof(ElementType)); }

	////////////////////////////////////////////////////////////////////////////////
	// Returns the 64 bit hash of the provided string.
	// The null terminator not included in the hash.
	inline uint64_t hash64(const char* str)
	{
		const size_t buffer_size = str != nullptr ? std::strlen(str) : 0;
		return hash64(str, buffer_size);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Combines two hashes into a new one.
	inline uint32_t hash_combine(uint32_t hash_a, uint32_t hash_b) { return (hash_a ^ hash_b) * 16777619U; }
	inline uint64_t hash_combine(uint64_t hash_a, uint64_t hash_b) { return (hash_a ^ hash_b) * 1099511628211ULL; }

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
