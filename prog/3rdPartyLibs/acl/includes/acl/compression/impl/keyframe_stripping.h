#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/scope_profiler.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/impl/compression_stats.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline void strip_keyframes(clip_context& lossy_clip_context, const compression_settings& settings, compression_stats_t& compression_stats)
		{
			if (!settings.keyframe_stripping.is_enabled())
				return;	// We don't want to strip keyframes, nothing to do

			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler keyframe_stripping_time;
#endif

			const bitset_description hard_keyframes_desc = bitset_description::make_from_num_bits<32>();
			const uint32_t num_keyframes = lossy_clip_context.num_samples;

			const keyframe_stripping_metadata_t* contributing_error_per_keyframe = lossy_clip_context.contributing_error;

			// Initialize which keyframes are retained, we'll strip them later
			for (segment_context& segment : lossy_clip_context.segment_iterator())
				bitset_set_range(&segment.hard_keyframes, hard_keyframes_desc, 0, segment.num_samples, true);

			// First determine how many we wish to strip based on proportion
			// A frame is movable if it isn't the first or last frame of a segment
			// If we have more than 1 frame, we can remove 2 frames per segment
			// We know that the only way to get a segment with 1 frame is if the whole clip contains
			// a single frame and thus has one segment
			// If we have 0 or 1 frame, none are movable
			const uint32_t num_movable_frames = num_keyframes >= 2 ? (num_keyframes - (lossy_clip_context.num_segments * 2)) : 0;

			uint32_t num_keyframes_to_strip = 0;

			// First, we strip the trivial keyframes
			if (settings.keyframe_stripping.strip_trivial)
			{
				// We only strip the ones that have the lowest error
				// Since the error depends on the order in which keyframes are removed,
				// a trivial keyframe might only become trivial after another keyframe that
				// isn't trivial is removed.
				for (; num_keyframes_to_strip < num_movable_frames; ++num_keyframes_to_strip)
				{
					if (!contributing_error_per_keyframe[num_keyframes_to_strip].is_keyframe_trivial)
						break;	// Found the first non-trivial keyframe
				}
			}

			// Then scan starting until we find our threshold if its above the proportion
			// We removed trivial keyframes above, meaning the error should be below our desired
			// threshold or it's low enough not to matter
			// Since the error depends on the order in which keyframes are removed, we process
			// the threshold first as it could decrease
			for (; num_keyframes_to_strip < num_movable_frames; ++num_keyframes_to_strip)
			{
				if (contributing_error_per_keyframe[num_keyframes_to_strip].stripping_error > settings.keyframe_stripping.threshold)
					break;	// The error exceeds our threshold, stop stripping keyframes
			}

			// Make sure to at least remove as many as the proportion we want
			const uint32_t num_proportion_keyframes_to_strip = std::min<uint32_t>(num_movable_frames, uint32_t(settings.keyframe_stripping.proportion * float(num_keyframes)));
			num_keyframes_to_strip = std::max<uint32_t>(num_keyframes_to_strip, num_proportion_keyframes_to_strip);

			ACL_ASSERT(num_keyframes_to_strip <= num_movable_frames, "Cannot strip more than the number of movable keyframes");

			// Check if stripping our keyframes actually saves any memory
			// Stripping requires us to add a small header per segment, we need to offset that cost
			uint32_t num_bits_saved = 0;
			for (uint32_t index = 0; index < num_keyframes_to_strip; ++index)
			{
				const keyframe_stripping_metadata_t& contributing_error = contributing_error_per_keyframe[index];
				const segment_context& segment = lossy_clip_context.segments[contributing_error.segment_index];

				num_bits_saved += segment.animated_pose_bit_size;
			}

			const uint32_t num_bytes_segment_overhead = sizeof(stripped_segment_header_t) - sizeof(segment_header);
			const uint32_t num_bytes_stripping_overhead = lossy_clip_context.num_segments * num_bytes_segment_overhead;
			const uint32_t num_bytes_saved = num_bits_saved / 8;	// Round down to a full byte to be conservative

			// If we don't save enough memory, don't strip anything
			if (num_bytes_saved <= num_bytes_stripping_overhead)
				num_keyframes_to_strip = 0;

			// Now that we know how many keyframes to strip, remove them
			for (uint32_t index = 0; index < num_keyframes_to_strip; ++index)
			{
				const keyframe_stripping_metadata_t& contributing_error = contributing_error_per_keyframe[index];
				segment_context& segment = lossy_clip_context.segments[contributing_error.segment_index];

				const uint32_t segment_keyframe_index = contributing_error.keyframe_index - segment.clip_sample_offset;
				ACL_ASSERT(segment_keyframe_index != 0 && segment_keyframe_index < (segment.num_samples - 1), "Cannot strip the first and last sample of a segment");

				bitset_set(&segment.hard_keyframes, hard_keyframes_desc, segment_keyframe_index, false);

				// Update how much animated we have
				const uint32_t num_stored_samples = bitset_count_set_bits(&segment.hard_keyframes, hard_keyframes_desc);
				const uint32_t num_animated_data_bits = segment.animated_pose_bit_size * num_stored_samples;
				segment.animated_data_size = align_to(num_animated_data_bits, 8) / 8;
			}

			lossy_clip_context.has_stripped_keyframes = num_keyframes_to_strip != 0;

#if defined(ACL_USE_SJSON)
			compression_stats.keyframe_stripping_elapsed_seconds = keyframe_stripping_time.get_elapsed_seconds();
#endif
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
