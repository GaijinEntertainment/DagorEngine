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
#include "acl/core/iallocator.h"
#include "acl/compression/impl/track_list_context.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline scalarf_range extract_scalarf_range(const track& track)
		{
			using namespace rtm;

			vector4f min = rtm::vector_set(1e10F);
			vector4f max = rtm::vector_set(-1e10F);

			const uint32_t num_samples = track.get_num_samples();
			const track_vector4f& typed_track = track_cast<const track_vector4f>(track);
			
			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const vector4f sample = typed_track[sample_index];

				min = vector_min(min, sample);
				max = vector_max(max, sample);
			}

			return scalarf_range::from_min_max(min, max);
		}

		inline void extract_track_ranges(track_list_context& context)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			context.range_list = allocate_type_array<track_range>(*context.allocator, context.num_tracks);

			for (uint32_t track_index = 0; track_index < context.num_tracks; ++track_index)
			{
				const track& track = context.track_list[track_index];

				switch (track.get_category())
				{
				case track_category8::scalarf:
					context.range_list[track_index] = track_range(extract_scalarf_range(track));
					break;
				case track_category8::scalard:
				case track_category8::transformf:
				case track_category8::transformd:
				default:
					ACL_ASSERT(false, "Invalid track category");
					break;
				}
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
