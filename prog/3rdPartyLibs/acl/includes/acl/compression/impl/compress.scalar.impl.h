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

// Included only once from compress.h

#include "acl/version.h"
#include "acl/core/buffer_tag.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/error.h"
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/scope_profiler.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/output_stats.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/track_list_context.h"
#include "acl/compression/impl/compact.scalar.h"
#include "acl/compression/impl/normalize.scalar.h"
#include "acl/compression/impl/optimize_looping.scalar.h"
#include "acl/compression/impl/quantize.scalar.h"
#include "acl/compression/impl/track_range_impl.h"
#include "acl/compression/impl/write_compression_stats_impl.h"
#include "acl/compression/impl/write_track_data_impl.h"
#include "acl/compression/impl/write_track_metadata.h"

#if defined(ACL_USE_SJSON)
#include <sjson/writer.h>
#endif

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline error_result compress_scalar_track_list(iallocator& allocator, const track_array& track_list, const compression_settings& settings, compressed_tracks*& out_compressed_tracks, output_stats& out_stats)
		{
			(void)out_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler compression_time;
#endif

			track_list_context context;
			if (!initialize_context(allocator, track_list, context))
				return error_result("Some samples are not finite");

			// Wrap instead of clamp if we loop
			optimize_looping(context, settings);

			// Extract our ranges now, we need it for compacting the constant tracks
			extract_track_ranges(context);

			// Compact and collapse the constant tracks
			extract_constant_tracks(context);

			// Normalize our samples into the track wide ranges per track
			normalize_tracks(context);

			// Find how many bits we need per track and quantize everything
			quantize_tracks(context);

			// Done transforming our input tracks, time to pack them into their final form
			const uint32_t per_track_metadata_size = write_track_metadata(context, nullptr);
			const uint32_t constant_values_size = write_track_constant_values(context, nullptr);
			const uint32_t range_values_size = write_track_range_values(context, nullptr);
			const uint32_t animated_num_bits = write_track_animated_values(context, nullptr);
			const uint32_t animated_values_size = (animated_num_bits + 7) / 8;		// Round up to nearest byte
			const uint32_t num_bits_per_frame = context.num_samples != 0 ? (animated_num_bits / context.num_samples) : 0;

			uint32_t buffer_size = 0;
			buffer_size += sizeof(raw_buffer_header);								// Header
			buffer_size += sizeof(tracks_header);									// Header
			buffer_size += sizeof(scalar_tracks_header);							// Header
			ACL_ASSERT(is_aligned_to(buffer_size, alignof(track_metadata)), "Invalid alignment");
			buffer_size += per_track_metadata_size;									// Per track metadata
			buffer_size = align_to(buffer_size, 4);									// Align constant values
			buffer_size += constant_values_size;									// Constant values
			ACL_ASSERT(is_aligned_to(buffer_size, 4), "Invalid alignment");
			buffer_size += range_values_size;										// Range values
			ACL_ASSERT(is_aligned_to(buffer_size, 4), "Invalid alignment");
			buffer_size += animated_values_size;									// Animated values

			// Optional metadata
			const uint32_t metadata_start_offset = align_to(buffer_size, 4);
			const uint32_t metadata_track_list_name_size = settings.metadata.include_track_list_name ? write_track_list_name(track_list, nullptr) : 0;
			const uint32_t metadata_track_names_size = settings.metadata.include_track_names ? write_track_names(track_list, context.track_output_indices, context.num_output_tracks, nullptr) : 0;
			const uint32_t metadata_track_descriptions_size = settings.metadata.include_track_descriptions ? write_track_descriptions(track_list, context.track_output_indices, context.num_output_tracks, nullptr) : 0;

			uint32_t metadata_size = 0;
			metadata_size += metadata_track_list_name_size;
			metadata_size = align_to(metadata_size, 4);
			metadata_size += metadata_track_names_size;
			metadata_size = align_to(metadata_size, 4);
			metadata_size += metadata_track_descriptions_size;

			if (metadata_size != 0)
			{
				buffer_size = align_to(buffer_size, 4);
				buffer_size += metadata_size;

				buffer_size = align_to(buffer_size, 4);
				buffer_size += sizeof(optional_metadata_header);
			}
			else
				buffer_size += 15;	// Ensure we have sufficient padding for unaligned 16 byte loads

			uint8_t* buffer = allocate_type_array_aligned<uint8_t>(allocator, buffer_size, alignof(compressed_tracks));
			std::memset(buffer, 0, buffer_size);

			uint8_t* buffer_start = buffer;
			out_compressed_tracks = bit_cast<compressed_tracks*>(buffer);

			raw_buffer_header* buffer_header = safe_ptr_cast<raw_buffer_header>(buffer);
			buffer += sizeof(raw_buffer_header);

			tracks_header* header = safe_ptr_cast<tracks_header>(buffer);
			buffer += sizeof(tracks_header);

			// Write our primary header
			header->tag = static_cast<uint32_t>(buffer_tag32::compressed_tracks);
			header->version = compressed_tracks_version16::latest;
			header->algorithm_type = algorithm_type8::uniformly_sampled;
			header->track_type = track_list.get_track_type();
			header->num_tracks = context.num_output_tracks;
			header->num_samples = context.num_output_tracks != 0 ? context.num_samples : 0;
			header->sample_rate = context.num_output_tracks != 0 ? context.sample_rate : 0.0F;
			header->set_is_wrap_optimized(context.looping_policy == sample_looping_policy::wrap);
			header->set_has_metadata(metadata_size != 0);

			// Write our scalar tracks header
			scalar_tracks_header* scalars_header = safe_ptr_cast<scalar_tracks_header>(buffer);
			buffer += sizeof(scalar_tracks_header);

			scalars_header->num_bits_per_frame = num_bits_per_frame;

			const uint8_t* packed_data_start_offset = buffer - sizeof(scalar_tracks_header);	// Relative to our header
			scalars_header->metadata_per_track = uint32_t(buffer - packed_data_start_offset);
			buffer += per_track_metadata_size;
			buffer = align_to(buffer, 4);
			scalars_header->track_constant_values = uint32_t(buffer - packed_data_start_offset);
			buffer += constant_values_size;
			scalars_header->track_range_values = uint32_t(buffer - packed_data_start_offset);
			buffer += range_values_size;
			scalars_header->track_animated_values = uint32_t(buffer - packed_data_start_offset);
			buffer += animated_values_size;

			if (metadata_size != 0)
			{
				buffer = align_to(buffer, 4);
				buffer += metadata_size;

				buffer = align_to(buffer, 4);
				buffer += sizeof(optional_metadata_header);
			}
			else
				buffer += 15;

			(void)buffer_start;	// Avoid VS2017 bug, it falsely reports this variable as unused even when asserts are enabled
			ACL_ASSERT((buffer_start + buffer_size) == buffer, "Buffer size and pointer mismatch");

			// Write our compressed data
			track_metadata* per_track_metadata = scalars_header->get_track_metadata();
			write_track_metadata(context, per_track_metadata);

			float* constant_values = scalars_header->get_track_constant_values();
			write_track_constant_values(context, constant_values);

			float* range_values = scalars_header->get_track_range_values();
			write_track_range_values(context, range_values);

			uint8_t* animated_values = scalars_header->get_track_animated_values();
			write_track_animated_values(context, animated_values);

			// Optional metadata header is last
			uint32_t writter_metadata_track_list_name_size = 0;
			uint32_t written_metadata_track_names_size = 0;
			uint32_t written_metadata_track_descriptions_size = 0;
			if (metadata_size != 0)
			{
				optional_metadata_header* metadada_header = bit_cast<optional_metadata_header*>(buffer_start + buffer_size - sizeof(optional_metadata_header));
				uint32_t metadata_offset = metadata_start_offset;

				if (settings.metadata.include_track_list_name)
				{
					metadada_header->track_list_name = metadata_offset;
					writter_metadata_track_list_name_size = write_track_list_name(track_list, metadada_header->get_track_list_name(*out_compressed_tracks));
					metadata_offset += writter_metadata_track_list_name_size;
				}
				else
					metadada_header->track_list_name = invalid_ptr_offset();

				if (settings.metadata.include_track_names)
				{
					metadata_offset = align_to(metadata_offset, 4);
					metadada_header->track_name_offsets = metadata_offset;
					written_metadata_track_names_size = write_track_names(track_list, context.track_output_indices, context.num_output_tracks, metadada_header->get_track_name_offsets(*out_compressed_tracks));
					metadata_offset += written_metadata_track_names_size;
				}
				else
					metadada_header->track_name_offsets = invalid_ptr_offset();

				metadada_header->parent_track_indices = invalid_ptr_offset();	// Not supported for scalar tracks

				if (settings.metadata.include_track_descriptions)
				{
					metadata_offset = align_to(metadata_offset, 4);
					metadada_header->track_descriptions = metadata_offset;
					written_metadata_track_descriptions_size = write_track_descriptions(track_list, context.track_output_indices, context.num_output_tracks, metadada_header->get_track_descriptions(*out_compressed_tracks));
					metadata_offset += written_metadata_track_descriptions_size;
				}
				else
					metadada_header->track_descriptions = invalid_ptr_offset();
			}

			ACL_ASSERT(writter_metadata_track_list_name_size == metadata_track_list_name_size, "Wrote too little or too much data");
			ACL_ASSERT(written_metadata_track_names_size == metadata_track_names_size, "Wrote too little or too much data");
			ACL_ASSERT(written_metadata_track_descriptions_size == metadata_track_descriptions_size, "Wrote too little or too much data");

			// Finish the raw buffer header
			buffer_header->size = buffer_size;
			buffer_header->hash = hash32(safe_ptr_cast<const uint8_t>(header), buffer_size - sizeof(raw_buffer_header));	// Hash everything but the raw buffer header

#if defined(ACL_HAS_ASSERT_CHECKS)
			if (metadata_size == 0)
			{
				for (const uint8_t* padding = buffer - 15; padding < buffer; ++padding)
					ACL_ASSERT(*padding == 0, "Padding was overwritten");
			}
#endif

#if defined(ACL_USE_SJSON)
			compression_time.stop();

			if (out_stats.logging != stat_logging::none)
				write_compression_stats(context, *out_compressed_tracks, compression_time, out_stats);
#endif

			return error_result();
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
