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
#include "acl/core/bitset.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/segment_context.h"
#include "acl/compression/impl/write_range_data.h"
#include "acl/compression/impl/write_stream_data.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t write_segment_start_indices(const clip_context& clip, uint32_t* segment_start_indices)
		{
			uint32_t size_written = 0;

			const uint32_t num_segments = clip.num_segments;
			for (uint32_t segment_index = 0; segment_index < num_segments; ++segment_index)
			{
				const segment_context& segment = clip.segments[segment_index];
				segment_start_indices[segment_index] = segment.clip_sample_offset;
				size_written += sizeof(uint32_t);
			}

			// Write our sentinel value
			segment_start_indices[clip.num_segments] = 0xFFFFFFFFU;
			size_written += sizeof(uint32_t);

			return size_written;
		}

		inline uint32_t write_segment_headers(const clip_context& clip, const compression_settings& settings, segment_header* segment_headers, uint32_t segment_data_start_offset)
		{
			uint32_t size_written = 0;

			const uint32_t format_per_track_data_size = get_format_per_track_data_size(clip, settings.rotation_format, settings.translation_format, settings.scale_format);
			const bool has_stripped_keyframes = clip.has_stripped_keyframes;
			stripped_segment_header_t* stripped_segment_headers = static_cast<stripped_segment_header_t*>(segment_headers);

			uint32_t segment_data_offset = segment_data_start_offset;
			for (uint32_t segment_index = 0; segment_index < clip.num_segments; ++segment_index)
			{
				const segment_context& segment = clip.segments[segment_index];

				if (has_stripped_keyframes)
				{
					stripped_segment_header_t& header = stripped_segment_headers[segment_index];

					ACL_ASSERT(header.animated_pose_bit_size == 0, "Buffer overrun detected");

					header.animated_pose_bit_size = segment.animated_pose_bit_size;
					header.animated_rotation_bit_size = segment.animated_rotation_bit_size;
					header.animated_translation_bit_size = segment.animated_translation_bit_size;
					header.segment_data = segment_data_offset;
					header.sample_indices = segment.hard_keyframes;

					segment_data_offset = align_to(segment_data_offset + format_per_track_data_size, 2);		// Aligned to 2 bytes
					segment_data_offset = align_to(segment_data_offset + segment.range_data_size, 4);			// Aligned to 4 bytes
					segment_data_offset = segment_data_offset + segment.animated_data_size;

					size_written += sizeof(stripped_segment_header_t);

					ACL_ASSERT((segment_data_offset - (uint32_t)header.segment_data) == segment.segment_data_size, "Unexpected segment size");
				}
				else
				{
					segment_header& header = segment_headers[segment_index];

					ACL_ASSERT(header.animated_pose_bit_size == 0, "Buffer overrun detected");

					header.animated_pose_bit_size = segment.animated_pose_bit_size;
					header.animated_rotation_bit_size = segment.animated_rotation_bit_size;
					header.animated_translation_bit_size = segment.animated_translation_bit_size;
					header.segment_data = segment_data_offset;

					segment_data_offset = align_to(segment_data_offset + format_per_track_data_size, 2);		// Aligned to 2 bytes
					segment_data_offset = align_to(segment_data_offset + segment.range_data_size, 4);			// Aligned to 4 bytes
					segment_data_offset = segment_data_offset + segment.animated_data_size;

					size_written += sizeof(segment_header);

					ACL_ASSERT((segment_data_offset - (uint32_t)header.segment_data) == segment.segment_data_size, "Unexpected segment size");
				}
			}

			return size_written;
		}

		inline uint32_t write_segment_data(const clip_context& clip, const compression_settings& settings, range_reduction_flags8 range_reduction, segment_header* segment_headers, transform_tracks_header& header, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			const uint32_t format_per_track_data_size = get_format_per_track_data_size(clip, settings.rotation_format, settings.translation_format, settings.scale_format);
			const bool has_stripped_keyframes = clip.has_stripped_keyframes;
			stripped_segment_header_t* stripped_segment_headers = static_cast<stripped_segment_header_t*>(segment_headers);

			uint32_t size_written = 0;

			const uint32_t num_segments = clip.num_segments;
			for (uint32_t segment_index = 0; segment_index < num_segments; ++segment_index)
			{
				const segment_context& segment = clip.segments[segment_index];

				uint8_t* format_per_track_data = nullptr;
				uint8_t* range_data = nullptr;
				uint8_t* animated_data = nullptr;

				if (has_stripped_keyframes)
				{
					stripped_segment_header_t& segment_header_ = stripped_segment_headers[segment_index];
					header.get_segment_data(segment_header_, format_per_track_data, range_data, animated_data);
				}
				else
				{
					segment_header& segment_header_ = segment_headers[segment_index];
					header.get_segment_data(segment_header_, format_per_track_data, range_data, animated_data);
				}

				ACL_ASSERT(format_per_track_data[0] == 0, "Buffer overrun detected");
				ACL_ASSERT(range_data[0] == 0, "Buffer overrun detected");
				ACL_ASSERT(animated_data[0] == 0, "Buffer overrun detected");

				if (format_per_track_data_size != 0)
				{
					const uint32_t size = write_format_per_track_data(segment, format_per_track_data, format_per_track_data_size, output_bone_mapping, num_output_bones);
					ACL_ASSERT(size == format_per_track_data_size, "Unexpected format per track data size"); (void)size;
				}

				if (segment.range_data_size != 0)
				{
					const uint32_t size = write_segment_range_data(segment, range_reduction, range_data, segment.range_data_size, output_bone_mapping, num_output_bones);
					ACL_ASSERT(size == segment.range_data_size, "Unexpected range data size"); (void)size;
				}

				if (segment.animated_data_size != 0)
				{
					const uint32_t size = write_animated_track_data(segment, animated_data, segment.animated_data_size, output_bone_mapping, num_output_bones);
					ACL_ASSERT(size == segment.animated_data_size, "Unexpected animated data size"); (void)size;
				}

				size_written += segment.segment_data_size;
			}

			return size_written;
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
