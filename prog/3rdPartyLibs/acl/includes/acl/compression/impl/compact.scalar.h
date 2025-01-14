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
#include "acl/core/bitset.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/iallocator.h"
#include "acl/core/track_desc.h"
#include "acl/compression/impl/track_list_context.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline bool is_scalarf_track_constant(const track& track_, const track_range& range)
		{
			const track_desc_scalarf& desc = track_.get_description<track_desc_scalarf>();
			return range.is_constant(desc.precision);
		}

		inline void extract_constant_tracks(track_list_context& context)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			const bitset_description bitset_desc = bitset_description::make_from_num_bits(context.num_tracks);

			context.constant_tracks_bitset = allocate_type_array<uint32_t>(*context.allocator, bitset_desc.get_size());
			bitset_reset(context.constant_tracks_bitset, bitset_desc, false);

			for (uint32_t track_index = 0; track_index < context.num_tracks; ++track_index)
			{
				const track& mut_track = context.track_list[track_index];
				const track_range& range = context.range_list[track_index];

				bool is_constant = false;
				switch (range.category)
				{
				case track_category8::scalarf:
					is_constant = is_scalarf_track_constant(mut_track, range);
					break;
				case track_category8::scalard:
				case track_category8::transformf:
				case track_category8::transformd:
				default:
					ACL_ASSERT(false, "Invalid track category");
					break;
				}

				bitset_set(context.constant_tracks_bitset, bitset_desc, track_index, is_constant);
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
