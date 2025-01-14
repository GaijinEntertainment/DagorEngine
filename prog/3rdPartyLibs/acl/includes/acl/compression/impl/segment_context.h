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
#include "acl/core/error.h"
#include "acl/core/hash.h"
#include "acl/core/iallocator.h"
#include "acl/core/iterator.h"
#include "acl/core/quality_tiers.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"
#include "acl/compression/impl/track_stream.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct clip_context;

		//////////////////////////////////////////////////////////////////////////
		// Encapsulates all the compression settings related to segmenting.
		// Segmenting ensures that large clips are split into smaller segments and
		// compressed independently to allow a smaller memory footprint as well as
		// faster compression and decompression.
		// See also: https://nfrechette.github.io/2016/11/10/anim_compression_uniform_segmenting/
		struct compression_segmenting_settings
		{
			//////////////////////////////////////////////////////////////////////////
			// How many samples to try and fit in our segments.
			// Defaults to '16'
			uint32_t ideal_num_samples = 16;

			//////////////////////////////////////////////////////////////////////////
			// Maximum number of samples per segment.
			// Defaults to '31'
			uint32_t max_num_samples = 31;

			//////////////////////////////////////////////////////////////////////////
			// Checks if everything is valid and if it isn't, returns an error string.
			// Returns nullptr if the settings are valid.
			inline error_result is_valid() const
			{
				if (ideal_num_samples < 8)
					return error_result("ideal_num_samples must be greater or equal to 8");

				if (ideal_num_samples > max_num_samples)
					return error_result("ideal_num_samples must be smaller or equal to max_num_samples");

				return error_result();
			}
		};

		struct segment_context
		{
			clip_context* clip								= nullptr;
			transform_streams* bone_streams					= nullptr;
			transform_range* ranges							= nullptr;

			// Optional if we request it in the compression settings
			// Sorted by stripping order within this segment
			keyframe_stripping_metadata_t* contributing_error	= nullptr;

			uint32_t num_samples_allocated					= 0;
			uint32_t num_samples							= 0;
			uint32_t num_bones								= 0;

			uint32_t clip_sample_offset						= 0;
			uint32_t segment_index							= 0;
			uint32_t hard_keyframes							= 0;		// Bit set of which keyframes are hard and retained when keyframe stripping is used

			bool are_rotations_normalized					= false;
			bool are_translations_normalized				= false;
			bool are_scales_normalized						= false;

			// Stat tracking
			uint32_t animated_rotation_bit_size				= 0;		// Tier 0
			uint32_t animated_translation_bit_size			= 0;		// Tier 0
			uint32_t animated_scale_bit_size				= 0;		// Tier 0
			uint32_t animated_pose_bit_size					= 0;		// Tier 0
			uint32_t animated_data_size						= 0;		// Tier 0
			uint32_t range_data_size						= 0;
			uint32_t segment_data_size						= 0;
			uint32_t total_header_size						= 0;

			//////////////////////////////////////////////////////////////////////////
			iterator<transform_streams> bone_iterator() { return iterator<transform_streams>(bone_streams, num_bones); }
			const_iterator<transform_streams> const_bone_iterator() const { return const_iterator<transform_streams>(bone_streams, num_bones); }
		};

		inline void destroy_segment_context(iallocator& allocator, segment_context& segment)
		{
			deallocate_type_array(allocator, segment.bone_streams, segment.num_bones);
			deallocate_type_array(allocator, segment.ranges, segment.num_bones);
			deallocate_type_array(allocator, segment.contributing_error, segment.num_samples_allocated);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
