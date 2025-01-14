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

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		template<typename cached_type, uint32_t cache_size, uint32_t num_rounding_modes>
		struct track_cache_v0
		{
			// Our cached type
			using type = typename cached_type::type;

			// Our cache size
			static constexpr uint32_t k_cache_size = cache_size;

			// How many rounding modes we cache
			static constexpr uint32_t k_num_rounding_modes = num_rounding_modes;

			// Our cached values
			type			cached_samples[num_rounding_modes][cache_size];

			// The index to write the next cache entry when we unpack
			// Effective index value is modulo k_cache_size what is stored here, guaranteed to never wrap
			uint32_t		cache_write_index = 0;

			// The index to read the next cache entry when we consume
			// Effective index value is modulo k_cache_size what is stored here, guaranteed to never wrap
			uint32_t		cache_read_index = 0;

			// How many we have left to unpack in total
			uint32_t		num_left_to_unpack;

			uint32_t		padding;

			// Returns the number of cached entries
			uint32_t		get_num_cached() const { return cache_write_index - cache_read_index; }
		};

		// Type traits for the track_cache to avoid GCC warning about template argument attributes
		struct track_cache_quatf_trait
		{
			using type = rtm::quatf;
		};

		struct track_cache_vector4f_trait
		{
			using type = rtm::vector4f;
		};

		template<uint32_t cache_size, uint32_t num_rounding_modes>
		using track_cache_quatf_v0 = track_cache_v0<track_cache_quatf_trait, cache_size, num_rounding_modes>;

		template<uint32_t cache_size, uint32_t num_rounding_modes>
		using track_cache_vector4f_v0 = track_cache_v0<track_cache_vector4f_trait, cache_size, num_rounding_modes>;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

ACL_IMPL_FILE_PRAGMA_POP
