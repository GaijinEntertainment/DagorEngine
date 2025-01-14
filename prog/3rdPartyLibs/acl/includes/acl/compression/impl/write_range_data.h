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
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"
#include "acl/core/enum_utils.h"
#include "acl/core/track_formats.h"
#include "acl/core/track_types.h"
#include "acl/core/range_reduction_types.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/math/quat_packing.h"
#include "acl/math/vector4_packing.h"
#include "acl/compression/impl/animated_track_utils.h"
#include "acl/compression/impl/clip_context.h"

#include "acl/core/impl/compressed_headers.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t get_clip_range_data_size(const clip_context& clip, range_reduction_flags8 range_reduction, rotation_format8 rotation_format)
		{
			const uint32_t rotation_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations) ? get_range_reduction_rotation_size(rotation_format) : 0;
			const uint32_t translation_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations) ? k_clip_range_reduction_vector3_range_size : 0;
			const uint32_t scale_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales) ? k_clip_range_reduction_vector3_range_size : 0;
			uint32_t range_data_size = 0;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];
			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (!bone_stream.is_rotation_constant)
					range_data_size += rotation_size;

				if (!bone_stream.is_translation_constant)
					range_data_size += translation_size;

				if (!bone_stream.is_scale_constant)
					range_data_size += scale_size;
			}

			return range_data_size;
		}

		inline uint32_t write_clip_range_data(const clip_context& clip, range_reduction_flags8 range_reduction, uint8_t* range_data, uint32_t range_data_size, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			(void)range_reduction;
			(void)range_data_size;

			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Groups are sorted per sub-track type. All rotation groups come first followed by translations then scales.
			// The last group of each sub-track may or may not have padding. The last group might be less than 4 sub-tracks.

			const uint8_t* range_data_start = range_data;
			const uint8_t* range_data_end = add_offset_to_ptr<uint8_t>(range_data, range_data_size);

			const rotation_format8 rotation_format = segment.bone_streams[0].rotations.get_rotation_format();	// The same for every track

			// Each range entry is a min/extent at most sizeof(float4f) each, 32 bytes total max per sub-track, 4 sub-tracks per group
			rtm::vector4f range_group_min[4];
			rtm::vector4f range_group_extent[4];

			auto group_filter_action = [range_reduction](animation_track_type8 group_type, uint32_t bone_index)
			{
				(void)bone_index;

				if (group_type == animation_track_type8::rotation)
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations);
				else if (group_type == animation_track_type8::translation)
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations);
				else
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales);
			};

			auto group_entry_action = [&clip, &range_group_min, &range_group_extent](animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)
			{
				if (group_type == animation_track_type8::rotation)
				{
					const transform_range& bone_range = clip.ranges[bone_index];

					const rtm::vector4f range_min = bone_range.rotation.get_min();
					const rtm::vector4f range_extent = bone_range.rotation.get_extent();

					range_group_min[group_size] = range_min;
					range_group_extent[group_size] = range_extent;
				}
				else if (group_type == animation_track_type8::translation)
				{
					const transform_range& bone_range = clip.ranges[bone_index];

					const rtm::vector4f range_min = bone_range.translation.get_min();
					const rtm::vector4f range_extent = bone_range.translation.get_extent();

					range_group_min[group_size] = range_min;
					range_group_extent[group_size] = range_extent;
				}
				else
				{
					const transform_range& bone_range = clip.ranges[bone_index];

					const rtm::vector4f range_min = bone_range.scale.get_min();
					const rtm::vector4f range_extent = bone_range.scale.get_extent();

					range_group_min[group_size] = range_min;
					range_group_extent[group_size] = range_extent;
				}
			};

			auto group_flush_action = [rotation_format, &range_data, range_data_end, &range_group_min, &range_group_extent](animation_track_type8 group_type, uint32_t group_size)
			{
				if (group_type == animation_track_type8::rotation)
				{
					// 2x float3f or float4f, we swizzle into SOA form
					RTM_MATRIXF_TRANSPOSE_4X4(range_group_min[0], range_group_min[1], range_group_min[2], range_group_min[3],
						range_group_min[0], range_group_min[1], range_group_min[2], range_group_min[3]);

					RTM_MATRIXF_TRANSPOSE_4X4(range_group_extent[0], range_group_extent[1], range_group_extent[2], range_group_extent[3],
						range_group_extent[0], range_group_extent[1], range_group_extent[2], range_group_extent[3]);

					if (rotation_format == rotation_format8::quatf_full)
					{
						// min
						std::memcpy(range_data + group_size * sizeof(float) * 0, &range_group_min[0], group_size * sizeof(float));		// xxxx
						std::memcpy(range_data + group_size * sizeof(float) * 1, &range_group_min[1], group_size * sizeof(float));		// yyyy
						std::memcpy(range_data + group_size * sizeof(float) * 2, &range_group_min[2], group_size * sizeof(float));		// zzzz
						std::memcpy(range_data + group_size * sizeof(float) * 3, &range_group_min[3], group_size * sizeof(float));		// wwww

						// extent
						std::memcpy(range_data + group_size * sizeof(float) * 4, &range_group_extent[0], group_size * sizeof(float));	// xxxx
						std::memcpy(range_data + group_size * sizeof(float) * 5, &range_group_extent[1], group_size * sizeof(float));	// yyyy
						std::memcpy(range_data + group_size * sizeof(float) * 6, &range_group_extent[2], group_size * sizeof(float));	// zzzz
						std::memcpy(range_data + group_size * sizeof(float) * 7, &range_group_extent[3], group_size * sizeof(float));	// wwww

						range_data += group_size * sizeof(float) * 8;
					}
					else
					{
						// min
						std::memcpy(range_data + group_size * sizeof(float) * 0, &range_group_min[0], group_size * sizeof(float));		// xxxx
						std::memcpy(range_data + group_size * sizeof(float) * 1, &range_group_min[1], group_size * sizeof(float));		// yyyy
						std::memcpy(range_data + group_size * sizeof(float) * 2, &range_group_min[2], group_size * sizeof(float));		// zzzz

						// extent
						std::memcpy(range_data + group_size * sizeof(float) * 3, &range_group_extent[0], group_size * sizeof(float));	// xxxx
						std::memcpy(range_data + group_size * sizeof(float) * 4, &range_group_extent[1], group_size * sizeof(float));	// yyyy
						std::memcpy(range_data + group_size * sizeof(float) * 5, &range_group_extent[2], group_size * sizeof(float));	// zzzz

						range_data += group_size * sizeof(float) * 6;
					}
				}
				else
				{
					// 2x float3f
					for (uint32_t group_index = 0; group_index < group_size; ++group_index)
					{
						std::memcpy(range_data, &range_group_min[group_index], sizeof(rtm::float3f));
						std::memcpy(range_data + sizeof(rtm::float3f), &range_group_extent[group_index], sizeof(rtm::float3f));
						range_data += sizeof(rtm::float3f) * 2;
					}
				}

				ACL_ASSERT(range_data <= range_data_end, "Invalid range data offset. Wrote too little data."); (void)range_data_end;
			};

			animated_group_writer(segment, output_bone_mapping, num_output_bones, group_filter_action, group_entry_action, group_flush_action);

			ACL_ASSERT(range_data == range_data_end, "Invalid range data offset. Wrote too little data.");

			return safe_static_cast<uint32_t>(range_data - range_data_start);
		}

		inline uint32_t write_segment_range_data(const segment_context& segment, range_reduction_flags8 range_reduction, uint8_t* range_data, uint32_t range_data_size, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			ACL_ASSERT(range_data != nullptr, "'range_data' cannot be null!");
			(void)range_reduction;
			(void)range_data_size;

			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Groups are sorted per sub-track type. All rotation groups come first followed by translations then scales.
			// The last group of each sub-track may or may not have padding. The last group might be less than 4 sub-tracks.

			// normalized value is between [0.0 .. 1.0]
			// value = (normalized value * range extent) + range min
			// normalized value = (value - range min) / range extent

			const uint8_t* range_data_start = range_data;
			const uint8_t* range_data_end = add_offset_to_ptr<uint8_t>(range_data, range_data_size);

			// For rotations contains: min.xxxx, min.yyyy, min.zzzz, extent.xxxx, extent.yyyy, extent.zzzz
			// For trans/scale: min0.xyz, extent0.xyz, min1.xyz, extent1.xyz, min2.xyz, extent2.xyz, min3.xyz, extent3.xyz
			// To keep decompression simpler, rotations are padded to 4 elements even if the last group is partial
			alignas(16) uint8_t range_data_group[6 * 4] = { 0 };

			auto group_filter_action = [range_reduction](animation_track_type8 group_type, uint32_t bone_index)
			{
				(void)bone_index;

				if (group_type == animation_track_type8::rotation)
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations);
				else if (group_type == animation_track_type8::translation)
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations);
				else
					return are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales);
			};

			auto group_entry_action = [&segment, &range_data_group](animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (group_type == animation_track_type8::rotation)
				{
					if (is_constant_bit_rate(bone_stream.rotations.get_bit_rate()))
					{
						const uint8_t* sample = bone_stream.rotations.get_raw_sample_ptr(0);

						// Swizzle into SOA form
						range_data_group[group_size + 0] = sample[1];		// sample.x top 8 bits
						range_data_group[group_size + 4] = sample[0];		// sample.x bottom 8 bits
						range_data_group[group_size + 8] = sample[3];		// sample.y top 8 bits
						range_data_group[group_size + 12] = sample[2];		// sample.y bottom 8 bits
						range_data_group[group_size + 16] = sample[5];		// sample.z top 8 bits
						range_data_group[group_size + 20] = sample[4];		// sample.z bottom 8 bits
					}
					else
					{
						const transform_range& bone_range = segment.ranges[bone_index];

						const rtm::vector4f range_min = bone_range.rotation.get_min();
						const rtm::vector4f range_extent = bone_range.rotation.get_extent();

						// Swizzle into SOA form
						alignas(16) uint8_t range_min_buffer[16];
						alignas(16) uint8_t range_extent_buffer[16];
						pack_vector3_u24_unsafe(range_min, range_min_buffer);
						pack_vector3_u24_unsafe(range_extent, range_extent_buffer);

						range_data_group[group_size + 0] = range_min_buffer[0];
						range_data_group[group_size + 4] = range_min_buffer[1];
						range_data_group[group_size + 8] = range_min_buffer[2];

						range_data_group[group_size + 12] = range_extent_buffer[0];
						range_data_group[group_size + 16] = range_extent_buffer[1];
						range_data_group[group_size + 20] = range_extent_buffer[2];
					}
				}
				else if (group_type == animation_track_type8::translation)
				{
					if (is_constant_bit_rate(bone_stream.translations.get_bit_rate()))
					{
						const uint8_t* sample = bone_stream.translations.get_raw_sample_ptr(0);
						uint8_t* sub_track_range_data = &range_data_group[group_size * 6];
						std::memcpy(sub_track_range_data, sample, 6);
					}
					else
					{
						const transform_range& bone_range = segment.ranges[bone_index];

						const rtm::vector4f range_min = bone_range.translation.get_min();
						const rtm::vector4f range_extent = bone_range.translation.get_extent();

						uint8_t* sub_track_range_data = &range_data_group[group_size * 6];
						pack_vector3_u24_unsafe(range_min, sub_track_range_data);
						pack_vector3_u24_unsafe(range_extent, sub_track_range_data + 3);
					}
				}
				else
				{
					if (is_constant_bit_rate(bone_stream.scales.get_bit_rate()))
					{
						const uint8_t* sample = bone_stream.scales.get_raw_sample_ptr(0);
						uint8_t* sub_track_range_data = &range_data_group[group_size * 6];
						std::memcpy(sub_track_range_data, sample, 6);
					}
					else
					{
						const transform_range& bone_range = segment.ranges[bone_index];

						const rtm::vector4f range_min = bone_range.scale.get_min();
						const rtm::vector4f range_extent = bone_range.scale.get_extent();

						uint8_t* sub_track_range_data = &range_data_group[group_size * 6];
						pack_vector3_u24_unsafe(range_min, sub_track_range_data);
						pack_vector3_u24_unsafe(range_extent, sub_track_range_data + 3);
					}
				}
			};

			auto group_flush_action = [&range_data, range_data_end, &range_data_group](animation_track_type8 group_type, uint32_t group_size)
			{
				const size_t copy_size = group_type == animation_track_type8::rotation ? 4 : group_size;
				std::memcpy(range_data, &range_data_group[0], copy_size * 6);
				range_data += copy_size * 6;

				// Zero out the temporary buffer for the final group to not contain partial garbage
				std::memset(&range_data_group[0], 0, sizeof(range_data_group));

				ACL_ASSERT(range_data <= range_data_end, "Invalid range data offset. Wrote too little data."); (void)range_data_end;
			};

			animated_group_writer(segment, output_bone_mapping, num_output_bones, group_filter_action, group_entry_action, group_flush_action);

			ACL_ASSERT(range_data == range_data_end, "Invalid range data offset. Wrote too little data.");

			return safe_static_cast<uint32_t>(range_data - range_data_start);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
