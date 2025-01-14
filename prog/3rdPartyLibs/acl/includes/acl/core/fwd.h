#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2022 Nicholas Frechette & Animation Compression Library contributors
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

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
// This header provides forward declarations for all public ACL types.
// Forward declaring symbols from a 3rd party library is a bad idea, use this
// header instead.
// See also: https://blog.libtorrent.org/2017/12/forward-declarations-and-abi/
////////////////////////////////////////////////////////////////////////////////

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	enum class additive_clip_format8 : uint8_t;
	enum class algorithm_type8 : uint8_t;
	enum class buffer_tag32 : uint32_t;
	enum class compressed_tracks_version16 : uint16_t;
	enum class range_reduction_flags8 : uint8_t;
	enum class rotation_format8 : uint8_t;
	enum class vector_format8 : uint8_t;
	union track_format8;
	enum class animation_track_type8 : uint8_t;
	enum class rotation_variant8 : uint8_t;

	enum class quality_tier;
	enum class sample_looping_policy;
	enum class sample_rounding_policy;

	class iallocator;
	class ansi_allocator;

	class bitset_description;
	struct bitset_index_ref;

	class compressed_database;
	class compressed_tracks;

	class error_result;
	class runtime_assert;

	struct fp_environment;
	class scope_enable_fp_exceptions;
	class scope_disable_fp_exceptions;

	namespace hash_impl
	{
		template <typename ResultType, ResultType OffsetBasis, ResultType Prime>
		class fnv1a_impl;
	}

	using fnv1a_32 = hash_impl::fnv1a_impl<uint32_t, 2166136261U, 16777619U>;
	using fnv1a_64 = hash_impl::fnv1a_impl<uint64_t, 14695981039346656037ULL, 1099511628211ULL>;

	namespace acl_impl
	{
		template <class item_type, bool is_const>
		class array_iterator_impl;

		template <class item_type, bool is_const>
		class array_reverse_iterator_impl;
	}

	//ACL_DEPRECATED("Renamed to array_iterator, to be removed in v3.0")
	template <class item_type>
	using iterator = acl_impl::array_iterator_impl<item_type, false>;

	template <class item_type>
	using array_iterator = acl_impl::array_iterator_impl<item_type, false>;

	template <class item_type>
	using array_reverse_iterator = acl_impl::array_reverse_iterator_impl<item_type, false>;

	//ACL_DEPRECATED("Renamed to const_array_iterator, to be removed in v3.0")
	template <class item_type>
	using const_iterator = acl_impl::array_iterator_impl<item_type, true>;

	template <class item_type>
	using const_array_iterator = acl_impl::array_iterator_impl<item_type, true>;

	template <class item_type>
	using const_array_reverse_iterator = acl_impl::array_reverse_iterator_impl<item_type, true>;

	struct invalid_ptr_offset;
	template<typename data_type, typename offset_type> class ptr_offset;
	template<typename data_type> using ptr_offset16 = ptr_offset<data_type, uint16_t>;
	template<typename data_type> using ptr_offset32 = ptr_offset<data_type, uint32_t>;

	class scope_profiler;

	class string;

	enum class default_sub_track_mode;
	struct track_writer;
	struct track_desc_scalarf;
	struct track_desc_transformf;

	template<typename allocated_type> class deleter;

	struct BoneBitRate; // TODO: Should be in impl namespace in ACL 3.0

	ACL_IMPL_VERSION_NAMESPACE_END
}
