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
#include "acl/core/iallocator.h"
#include "acl/core/error.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/compression/impl/animated_track_utils.h"
#include "acl/compression/impl/clip_context.h"

#include "acl/core/impl/compressed_headers.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t get_constant_data_size(const clip_context& clip)
		{
			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t constant_data_size = 0;

			for (uint32_t bone_index = 0; bone_index < clip.num_bones; ++bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (bone_stream.output_index == k_invalid_track_index)
					continue;	// Stripped

				if (!bone_stream.is_rotation_default && bone_stream.is_rotation_constant)
					constant_data_size += bone_stream.rotations.get_packed_sample_size();

				if (!bone_stream.is_translation_default && bone_stream.is_translation_constant)
					constant_data_size += bone_stream.translations.get_packed_sample_size();

				if (clip.has_scale && !bone_stream.is_scale_default && bone_stream.is_scale_constant)
					constant_data_size += bone_stream.scales.get_packed_sample_size();
			}

			return constant_data_size;
		}

		inline void get_num_constant_samples(const clip_context& clip, uint32_t& out_num_constant_rotation_samples, uint32_t& out_num_constant_translation_samples, uint32_t& out_num_constant_scale_samples)
		{
			uint32_t num_constant_rotation_samples = 0;
			uint32_t num_constant_translation_samples = 0;
			uint32_t num_constant_scale_samples = 0;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			for (uint32_t bone_index = 0; bone_index < clip.num_bones; ++bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (bone_stream.output_index == k_invalid_track_index)
					continue;	// Stripped

				if (!bone_stream.is_rotation_default && bone_stream.is_rotation_constant)
					num_constant_rotation_samples++;

				if (!bone_stream.is_translation_default && bone_stream.is_translation_constant)
					num_constant_translation_samples++;

				if (clip.has_scale && !bone_stream.is_scale_default && bone_stream.is_scale_constant)
					num_constant_scale_samples++;
			}

			out_num_constant_rotation_samples = num_constant_rotation_samples;
			out_num_constant_translation_samples = num_constant_translation_samples;
			out_num_constant_scale_samples = num_constant_scale_samples;
		}

		inline void get_animated_variable_bit_rate_data_size(const track_stream& track_stream, uint32_t& out_num_animated_pose_bits)
		{
			const uint8_t bit_rate = track_stream.get_bit_rate();
			const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate) * 3;	// 3 components
			out_num_animated_pose_bits += num_bits_at_bit_rate;
		}

		inline void calculate_animated_data_size(const track_stream& track_stream, uint32_t& num_animated_pose_bits)
		{
			if (track_stream.is_bit_rate_variable())
			{
				get_animated_variable_bit_rate_data_size(track_stream, num_animated_pose_bits);
			}
			else
			{
				const uint32_t sample_size = track_stream.get_packed_sample_size();
				num_animated_pose_bits += sample_size * 8;
			}
		}

		inline void calculate_animated_data_size(clip_context& clip, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			for (segment_context& segment : clip.segment_iterator())
			{
				uint32_t num_animated_pose_rotation_data_bits = 0;
				uint32_t num_animated_pose_translation_data_bits = 0;
				uint32_t num_animated_pose_scale_data_bits = 0;

				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					const uint32_t bone_index = output_bone_mapping[output_index];
					const transform_streams& bone_stream = segment.bone_streams[bone_index];

					if (!bone_stream.is_rotation_constant)
						calculate_animated_data_size(bone_stream.rotations, num_animated_pose_rotation_data_bits);

					if (!bone_stream.is_translation_constant)
						calculate_animated_data_size(bone_stream.translations, num_animated_pose_translation_data_bits);

					if (!bone_stream.is_scale_constant)
						calculate_animated_data_size(bone_stream.scales, num_animated_pose_scale_data_bits);
				}

				const uint32_t num_animated_pose_bits = num_animated_pose_rotation_data_bits + num_animated_pose_translation_data_bits + num_animated_pose_scale_data_bits;
				const uint32_t num_animated_data_bits = num_animated_pose_bits * segment.num_samples;

				segment.animated_rotation_bit_size = num_animated_pose_rotation_data_bits;
				segment.animated_translation_bit_size = num_animated_pose_translation_data_bits;
				segment.animated_scale_bit_size = num_animated_pose_scale_data_bits;
				segment.animated_data_size = align_to(num_animated_data_bits, 8) / 8;
				segment.animated_pose_bit_size = num_animated_pose_bits;
			}
		}

		inline uint32_t get_format_per_track_data_size(const clip_context& clip, rotation_format8 rotation_format, vector_format8 translation_format, vector_format8 scale_format, uint32_t* out_num_animated_variable_sub_tracks_padded = nullptr)
		{
			const bool is_rotation_variable = is_rotation_format_variable(rotation_format);
			const bool is_translation_variable = is_vector_format_variable(translation_format);
			const bool is_scale_variable = is_vector_format_variable(scale_format);

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t format_per_track_data_size = 0;
			uint32_t num_animated_variable_rotations = 0;

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (bone_stream.is_stripped_from_output())
					continue;

				if (!bone_stream.is_rotation_constant && is_rotation_variable)
				{
					format_per_track_data_size++;
					num_animated_variable_rotations++;
				}

				if (!bone_stream.is_translation_constant && is_translation_variable)
					format_per_track_data_size++;

				if (!bone_stream.is_scale_constant && is_scale_variable)
					format_per_track_data_size++;
			}

			// Rotations are padded for alignment
			const uint32_t num_partial_rotations = num_animated_variable_rotations % 4;
			if (num_partial_rotations != 0)
				format_per_track_data_size += 4 - num_partial_rotations;

			if (out_num_animated_variable_sub_tracks_padded != nullptr)
				*out_num_animated_variable_sub_tracks_padded = format_per_track_data_size;	// 1 byte per sub-track

			return format_per_track_data_size;
		}

		inline uint32_t write_constant_track_data(const clip_context& clip, rotation_format8 rotation_format, uint8_t* constant_data, uint32_t constant_data_size, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			ACL_ASSERT(constant_data != nullptr, "'constant_data' cannot be null!");
			(void)constant_data_size;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

#if defined(ACL_HAS_ASSERT_CHECKS)
			const uint8_t* constant_data_end = add_offset_to_ptr<uint8_t>(constant_data, constant_data_size);
#endif

			const uint8_t* constant_data_start = constant_data;

			// If our rotation format drops the W component, we swizzle the data to store XXXX, YYYY, ZZZZ
			const bool swizzle_rotations = get_rotation_variant(rotation_format) == rotation_variant8::quat_drop_w;
			float xxxx[4];
			float yyyy[4];
			float zzzz[4];
			uint32_t num_swizzle_written = 0;

			// Write rotations first
			for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
			{
				const uint32_t bone_index = output_bone_mapping[output_index];
				const transform_streams& bone_stream = segment.bone_streams[bone_index];

				if (!bone_stream.is_rotation_default && bone_stream.is_rotation_constant)
				{
					if (swizzle_rotations)
					{
						const rtm::vector4f rotation = bone_stream.rotations.get_raw_sample<rtm::vector4f>(0);
						xxxx[num_swizzle_written] = rtm::vector_get_x(rotation);
						yyyy[num_swizzle_written] = rtm::vector_get_y(rotation);
						zzzz[num_swizzle_written] = rtm::vector_get_z(rotation);
						num_swizzle_written++;

						if (num_swizzle_written >= 4)
						{
							std::memcpy(constant_data, &xxxx[0], sizeof(xxxx));
							constant_data += sizeof(xxxx);
							std::memcpy(constant_data, &yyyy[0], sizeof(yyyy));
							constant_data += sizeof(yyyy);
							std::memcpy(constant_data, &zzzz[0], sizeof(zzzz));
							constant_data += sizeof(zzzz);
							num_swizzle_written = 0;
						}
					}
					else
					{
						const uint8_t* rotation_ptr = bone_stream.rotations.get_raw_sample_ptr(0);
						uint32_t sample_size = bone_stream.rotations.get_sample_size();
						std::memcpy(constant_data, rotation_ptr, sample_size);
						constant_data += sample_size;
					}

					ACL_ASSERT(constant_data <= constant_data_end, "Invalid constant data offset. Wrote too much data.");
				}
			}

			if (swizzle_rotations && num_swizzle_written != 0)
			{
				std::memcpy(constant_data, &xxxx[0], num_swizzle_written * sizeof(float));
				constant_data += num_swizzle_written * sizeof(float);
				std::memcpy(constant_data, &yyyy[0], num_swizzle_written * sizeof(float));
				constant_data += num_swizzle_written * sizeof(float);
				std::memcpy(constant_data, &zzzz[0], num_swizzle_written * sizeof(float));
				constant_data += num_swizzle_written * sizeof(float);

				ACL_ASSERT(constant_data <= constant_data_end, "Invalid constant data offset. Wrote too much data.");
			}

			// Next, write translations
			for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
			{
				const uint32_t bone_index = output_bone_mapping[output_index];
				const transform_streams& bone_stream = segment.bone_streams[bone_index];

				if (!bone_stream.is_translation_default && bone_stream.is_translation_constant)
				{
					const uint8_t* translation_ptr = bone_stream.translations.get_raw_sample_ptr(0);
					uint32_t sample_size = bone_stream.translations.get_sample_size();
					std::memcpy(constant_data, translation_ptr, sample_size);
					constant_data += sample_size;

					ACL_ASSERT(constant_data <= constant_data_end, "Invalid constant data offset. Wrote too much data.");
				}
			}

			// Finally, write scales
			if (clip.has_scale)
			{
				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					const uint32_t bone_index = output_bone_mapping[output_index];
					const transform_streams& bone_stream = segment.bone_streams[bone_index];

					if (!bone_stream.is_scale_default && bone_stream.is_scale_constant)
					{
						const uint8_t* scale_ptr = bone_stream.scales.get_raw_sample_ptr(0);
						uint32_t sample_size = bone_stream.scales.get_sample_size();
						std::memcpy(constant_data, scale_ptr, sample_size);
						constant_data += sample_size;

						ACL_ASSERT(constant_data <= constant_data_end, "Invalid constant data offset. Wrote too much data.");
					}
				}
			}

			ACL_ASSERT(constant_data == constant_data_end, "Invalid constant data offset. Wrote too little data.");
			return safe_static_cast<uint32_t>(constant_data - constant_data_start);
		}

		inline void write_animated_sample(const track_stream& track_stream, uint32_t sample_index, uint8_t* animated_track_data_begin, uint64_t& out_bit_offset)
		{
			const uint8_t* raw_sample_ptr = track_stream.get_raw_sample_ptr(sample_index);

			if (track_stream.is_bit_rate_variable())
			{
				const uint8_t bit_rate = track_stream.get_bit_rate();
				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate) * 3;	// 3 components

				// Track is constant, our constant sample is stored in the range information
				ACL_ASSERT(!is_constant_bit_rate(bit_rate), "Cannot write constant variable track data");

				if (is_raw_bit_rate(bit_rate))
				{
					const uint32_t* raw_sample_u32 = safe_ptr_cast<const uint32_t>(raw_sample_ptr);

					const uint32_t x = byte_swap(raw_sample_u32[0]);
					memcpy_bits(animated_track_data_begin, out_bit_offset + 0, &x, 0, 32);
					const uint32_t y = byte_swap(raw_sample_u32[1]);
					memcpy_bits(animated_track_data_begin, out_bit_offset + 32, &y, 0, 32);
					const uint32_t z = byte_swap(raw_sample_u32[2]);
					memcpy_bits(animated_track_data_begin, out_bit_offset + 64, &z, 0, 32);
				}
				else
				{
					const uint64_t* raw_sample_u64 = safe_ptr_cast<const uint64_t>(raw_sample_ptr);
					memcpy_bits(animated_track_data_begin, out_bit_offset, raw_sample_u64, 0, num_bits_at_bit_rate);
				}

				out_bit_offset += num_bits_at_bit_rate;
			}
			else
			{
				const uint32_t* raw_sample_u32 = safe_ptr_cast<const uint32_t>(raw_sample_ptr);

				const uint32_t x = byte_swap(raw_sample_u32[0]);
				memcpy_bits(animated_track_data_begin, out_bit_offset + 0, &x, 0, 32);
				const uint32_t y = byte_swap(raw_sample_u32[1]);
				memcpy_bits(animated_track_data_begin, out_bit_offset + 32, &y, 0, 32);
				const uint32_t z = byte_swap(raw_sample_u32[2]);
				memcpy_bits(animated_track_data_begin, out_bit_offset + 64, &z, 0, 32);

				const uint32_t sample_size = track_stream.get_packed_sample_size();
				const bool has_w_component = sample_size == (sizeof(float) * 4);
				if (has_w_component)
				{
					const uint32_t w = byte_swap(raw_sample_u32[3]);
					memcpy_bits(animated_track_data_begin, out_bit_offset + 96, &w, 0, 32);
				}

				out_bit_offset += has_w_component ? 128 : 96;
			}
		}

		inline uint32_t write_animated_track_data(const segment_context& segment, uint8_t* animated_track_data, uint32_t animated_data_size, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			ACL_ASSERT(animated_track_data != nullptr, "'animated_track_data' cannot be null!");
			(void)animated_data_size;

			uint8_t* animated_track_data_begin = animated_track_data;

			const uint8_t* animated_track_data_start = animated_track_data;
			const uint8_t* animated_track_data_end = add_offset_to_ptr<uint8_t>(animated_track_data, animated_data_size);
			const bool has_stripped_keyframes = segment.clip->has_stripped_keyframes;
			const bitset_description hard_keyframes_desc = bitset_description::make_from_num_bits<32>();

			uint64_t bit_offset = 0;

			// Data is sorted first by time, second by bone.
			// This ensures that all bones are contiguous in memory when we sample a particular time.

			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Groups are sorted per sub-track type. All rotation groups come first followed by translations then scales.
			// The last group of each sub-track may or may not have padding. The last group might be less than 4 sub-tracks.

			// For animated samples, when we have a constant bit rate (bit rate 0), we do not store samples
			// and as such the group that contains that sub-track won't contain 4 samples.
			// The largest sample is a full precision vector4f, we can contain at most 4 samples
			alignas(16) uint8_t group_animated_track_data[sizeof(rtm::vector4f) * 4];
			uint64_t group_bit_offset = 0;

			auto group_filter_action = [](animation_track_type8 group_type, uint32_t bone_index)
			{
				(void)group_type;
				(void)bone_index;

				// We want a group of every animated track
				// If a track is variable with a constant bit rate (bit rate 0), the group will have fewer entries
				return true;
			};

			auto group_flush_action = [animated_track_data_begin, &bit_offset, &group_animated_track_data, &group_bit_offset, &animated_track_data, animated_track_data_end](animation_track_type8 group_type, uint32_t group_size)
			{
				(void)group_type;
				(void)group_size;

				memcpy_bits(animated_track_data_begin, bit_offset, &group_animated_track_data[0], 0, group_bit_offset);

				bit_offset += group_bit_offset;
				group_bit_offset = 0;

				animated_track_data = animated_track_data_begin + (bit_offset / 8);

				ACL_ASSERT(animated_track_data <= animated_track_data_end, "Invalid animated track data offset. Wrote too much data."); (void)animated_track_data_end;
			};

			for (uint32_t sample_index = 0; sample_index < segment.num_samples; ++sample_index)
			{
				if (has_stripped_keyframes && !bitset_test(&segment.hard_keyframes, hard_keyframes_desc, sample_index))
					continue;	// This keyframe has been stripped, skip it

				auto group_entry_action = [&segment, sample_index, &group_animated_track_data, &group_bit_offset](animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)
				{
					(void)group_size;

					const transform_streams& bone_stream = segment.bone_streams[bone_index];
					if (group_type == animation_track_type8::rotation)
					{
						if (!is_constant_bit_rate(bone_stream.rotations.get_bit_rate()))
							write_animated_sample(bone_stream.rotations, sample_index, group_animated_track_data, group_bit_offset);
					}
					else if (group_type == animation_track_type8::translation)
					{
						if (!is_constant_bit_rate(bone_stream.translations.get_bit_rate()))
							write_animated_sample(bone_stream.translations, sample_index, group_animated_track_data, group_bit_offset);
					}
					else
					{
						if (!is_constant_bit_rate(bone_stream.scales.get_bit_rate()))
							write_animated_sample(bone_stream.scales, sample_index, group_animated_track_data, group_bit_offset);
					}
				};

				animated_group_writer(segment, output_bone_mapping, num_output_bones, group_filter_action, group_entry_action, group_flush_action);
			}

			if (bit_offset != 0)
				animated_track_data = animated_track_data_begin + ((bit_offset + 7) / 8);

#if defined(ACL_HAS_ASSERT_CHECKS)
			const uint32_t num_stored_samples = has_stripped_keyframes ? bitset_count_set_bits(&segment.hard_keyframes, hard_keyframes_desc) : segment.num_samples;
			ACL_ASSERT((bit_offset == 0 && segment.num_samples == 0) || ((bit_offset / num_stored_samples) == segment.animated_pose_bit_size), "Unexpected number of bits written");
			ACL_ASSERT(animated_track_data == animated_track_data_end, "Invalid animated track data offset. Wrote too little data.");
#endif

			return safe_static_cast<uint32_t>(animated_track_data - animated_track_data_start);
		}

		inline uint32_t write_format_per_track_data(const segment_context& segment, uint8_t* format_per_track_data, uint32_t format_per_track_data_size, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			ACL_ASSERT(format_per_track_data != nullptr, "'format_per_track_data' cannot be null!");
			(void)format_per_track_data_size;

			const uint8_t* format_per_track_data_start = format_per_track_data;
			const uint8_t* format_per_track_data_end = add_offset_to_ptr<uint8_t>(format_per_track_data, format_per_track_data_size);

			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Groups are sorted per sub-track type. All rotation groups come first followed by translations then scales.
			// The last group of each sub-track may or may not have padding. The last group might be less than 4 sub-tracks.

			// To keep decompression simpler, rotations are padded to 4 elements even if the last group is partial
			uint8_t format_per_track_group[4] = { 0 };

			auto group_filter_action = [&segment](animation_track_type8 group_type, uint32_t bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (group_type == animation_track_type8::rotation)
					return bone_stream.rotations.is_bit_rate_variable();
				else if (group_type == animation_track_type8::translation)
					return bone_stream.translations.is_bit_rate_variable();
				else
					return bone_stream.scales.is_bit_rate_variable();
			};

			auto group_entry_action = [&segment, &format_per_track_group](animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];

				uint32_t bit_rate;
				if (group_type == animation_track_type8::rotation)
					bit_rate = bone_stream.rotations.get_bit_rate();
				else if (group_type == animation_track_type8::translation)
					bit_rate = bone_stream.translations.get_bit_rate();
				else
					bit_rate = bone_stream.scales.get_bit_rate();

				const uint32_t num_bits = get_num_bits_at_bit_rate(bit_rate);
				ACL_ASSERT(num_bits <= 32, "Expected 32 bits or less");

				// We only have 25 bit rates and the largest number of bits is 32 (highest bit rate).
				// This would require 6 bits to store in the per sub-track metadata but it would leave
				// most of the entries unused.
				// Instead, we store the number of bits on 5 bits which has a max value of 31.
				// To do so, we remap 32 to 31 since that value is unused.
				// This leaves 3 unused bits in our per sub-track metadata.
				// These will later be needed:
				//    - 1 bit to dictate if rotations contain 3 or 4 components (to allow mixing full quats in with packed quats)
				//    - 2 bits to dictate which rotation component is dropped (to allow the largest component to be dropped over our segment)

				format_per_track_group[group_size] = (bit_rate == k_highest_bit_rate) ? static_cast<uint8_t>(31) : (uint8_t)num_bits;
			};

			auto group_flush_action = [&format_per_track_data, format_per_track_data_end, &format_per_track_group](animation_track_type8 group_type, uint32_t group_size)
			{
				const uint32_t copy_size = group_type == animation_track_type8::rotation ? 4 : group_size;
				std::memcpy(format_per_track_data, &format_per_track_group[0], copy_size);
				format_per_track_data += copy_size;

				// Zero out the temporary buffer for the final group to not contain partial garbage
				std::memset(&format_per_track_group[0], 0, sizeof(format_per_track_group));

				ACL_ASSERT(format_per_track_data <= format_per_track_data_end, "Invalid format per track data offset. Wrote too much data."); (void)format_per_track_data_end;
			};

			animated_group_writer(segment, output_bone_mapping, num_output_bones, group_filter_action, group_entry_action, group_flush_action);

			ACL_ASSERT(format_per_track_data == format_per_track_data_end, "Invalid format per track data offset. Wrote too little data.");

			return safe_static_cast<uint32_t>(format_per_track_data - format_per_track_data_start);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
