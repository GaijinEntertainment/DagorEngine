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
#include "acl/core/bitset.h"
#include "acl/core/compressed_database.h"
#include "acl/core/error_result.h"
#include "acl/core/hash.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/bit_cast.impl.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct frame_tier_mapping
		{
			const uint8_t* animated_data;			// Pointer to the segment's animated data which contains this frame

			uint32_t tracks_index;
			uint32_t segment_index;
			uint32_t clip_frame_index;
			uint32_t segment_frame_index;
			uint32_t frame_bit_size;				// Size in bits of this frame

			float contributing_error;
		};

		struct database_tier_mapping
		{
			frame_tier_mapping* frames = nullptr;	// Frames mapped to this tier
			uint32_t num_frames = 0;				// Number of frames mapped to this tier

			quality_tier tier = quality_tier::highest_importance;	// Actual tier

			bool is_empty() const { return num_frames == 0; }
		};

		struct clip_contributing_error_t
		{
			const keyframe_stripping_metadata_t* keyframe_metadata = nullptr;	// List of keyframe stripping metadata, sorted in stripping order

			uint32_t num_assigned = 0;				// Number of keyframes already assigned
			uint32_t num_frames = 0;				// Number of keyframes in clip
		};

		struct frame_assignment_context
		{
			iallocator& allocator;

			const compressed_tracks* const* compressed_tracks_list;
			uint32_t num_compressed_tracks;

			database_tier_mapping mappings[k_num_quality_tiers];		// 0 = high importance, 1 = medium importance, 2 = low importance
			uint32_t num_movable_frames;

			clip_contributing_error_t* contributing_error_per_clip;		// One instance per clip

			frame_assignment_context(iallocator& allocator_, const compressed_tracks* const* compressed_tracks_list_, uint32_t num_compressed_tracks_, uint32_t num_movable_frames_)
				: allocator(allocator_)
				, compressed_tracks_list(compressed_tracks_list_)
				, num_compressed_tracks(num_compressed_tracks_)
				, num_movable_frames(num_movable_frames_)
				, contributing_error_per_clip(allocate_type_array<clip_contributing_error_t>(allocator_, num_compressed_tracks_))
			{
				mappings[0].tier = quality_tier::highest_importance;
				mappings[1].tier = quality_tier::medium_importance;
				mappings[2].tier = quality_tier::lowest_importance;

				// Setup our error metadata to make iterating on it easier and track what has been assigned
				for (uint32_t list_index = 0; list_index < num_compressed_tracks_; ++list_index)
				{
					const compressed_tracks* tracks = compressed_tracks_list_[list_index];
					const tracks_header& header = get_tracks_header(*tracks);
					const transform_tracks_header& transform_header = get_transform_tracks_header(*tracks);
					const optional_metadata_header& metadata_header = get_optional_metadata_header(*tracks);

					clip_contributing_error_t& clip_error = contributing_error_per_clip[list_index];
					clip_error.num_frames = header.num_samples;

					if (tracks->get_version() >= compressed_tracks_version16::v02_01_99_2)
					{
						clip_error.keyframe_metadata = bit_cast<const keyframe_stripping_metadata_t*>(metadata_header.get_contributing_error(*tracks));
					}
					else
					{
						// Allocate and populate
						keyframe_stripping_metadata_t* keyframe_metadata = allocate_type_array<keyframe_stripping_metadata_t>(allocator_, header.num_samples);

						const bool has_multiple_segments = transform_header.has_multiple_segments();
						const uint32_t* segment_start_indices = has_multiple_segments ? transform_header.get_segment_start_indices() : nullptr;
						const uint32_t num_segments = has_multiple_segments ? transform_header.num_segments : 1;	// HACK to avoid static analysis warning

						const frame_contributing_error* contributing_errors = bit_cast<const frame_contributing_error*>(metadata_header.get_contributing_error(*tracks));

						for (uint32_t segment_index = 0; segment_index < num_segments; ++segment_index)
						{
							const uint32_t segment_start_frame_index = has_multiple_segments ? segment_start_indices[segment_index] : 0;

							uint32_t num_segment_frames;
							if (num_segments == 1)
								num_segment_frames = header.num_samples;	// Only one segment, it has every frame
							else if (segment_index + 1 == num_segments)
								num_segment_frames = header.num_samples - segment_start_indices[segment_index];	// Last segment has the remaining frames
							else
								num_segment_frames = segment_start_indices[segment_index + 1] - segment_start_indices[segment_index];

							for (uint32_t segment_keyframe_index = 0; segment_keyframe_index < num_segment_frames; ++segment_keyframe_index)
							{
								const uint32_t clip_keyframe_index = segment_start_frame_index + segment_keyframe_index;

								keyframe_metadata[clip_keyframe_index].keyframe_index = segment_start_frame_index + contributing_errors[clip_keyframe_index].index;
								keyframe_metadata[clip_keyframe_index].stripping_index = 0;			// Calculated below
								keyframe_metadata[clip_keyframe_index].stripping_error = contributing_errors[clip_keyframe_index].error;
								keyframe_metadata[clip_keyframe_index].is_keyframe_trivial = false;	// Not supported
							}
						}

						// Sort by contributing error
						// It isn't fully accurate but the original sort order has been lost and we cannot recover it
						auto sort_predicate = [](const keyframe_stripping_metadata_t& lhs, const keyframe_stripping_metadata_t& rhs) { return lhs.stripping_error < rhs.stripping_error; };
						std::sort(keyframe_metadata, keyframe_metadata + header.num_samples, sort_predicate);

						// Populate our stripping order
						for (uint32_t clip_keyframe_index = 0; clip_keyframe_index < header.num_samples; ++clip_keyframe_index)
							keyframe_metadata[clip_keyframe_index].stripping_index = clip_keyframe_index;
					}
				}
			}

			~frame_assignment_context()
			{
				for (uint32_t tier_index = 0; tier_index < k_num_quality_tiers; ++tier_index)
					deallocate_type_array(allocator, mappings[tier_index].frames, mappings[tier_index].num_frames);

				for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
				{
					const compressed_tracks* tracks = compressed_tracks_list[list_index];
					if (tracks->get_version() < compressed_tracks_version16::v02_01_99_2)
					{
						// Old versions copied the data manually
						keyframe_stripping_metadata_t* keyframe_metadata = const_cast<keyframe_stripping_metadata_t*>(contributing_error_per_clip[list_index].keyframe_metadata);
						deallocate_type_array(allocator, keyframe_metadata, contributing_error_per_clip[list_index].num_frames);
					}
				}

				deallocate_type_array(allocator, contributing_error_per_clip, num_compressed_tracks);
			}

			frame_assignment_context(const frame_assignment_context&) = delete;
			frame_assignment_context& operator=(const frame_assignment_context&) = delete;

			database_tier_mapping& get_tier_mapping(quality_tier tier) { return mappings[(uint32_t)tier]; }
			const database_tier_mapping& get_tier_mapping(quality_tier tier) const { return mappings[(uint32_t)tier]; }

			void set_tier_num_frames(quality_tier tier, uint32_t num_frames)
			{
				database_tier_mapping& mapping = mappings[(uint32_t)tier];
				ACL_ASSERT(mapping.frames == nullptr, "Tier already setup");

				mapping.frames = allocate_type_array<frame_tier_mapping>(allocator, num_frames);
				mapping.num_frames = num_frames;
			}
		};

		inline uint32_t calculate_num_frames(const compressed_tracks* const* compressed_tracks_list, uint32_t num_compressed_tracks)
		{
			uint32_t num_frames = 0;

			for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
			{
				const compressed_tracks* tracks = compressed_tracks_list[list_index];

				const tracks_header& header = get_tracks_header(*tracks);

				num_frames += header.num_samples;
			}

			return num_frames;
		}

		inline uint32_t calculate_num_movable_frames(const compressed_tracks* const* compressed_tracks_list, uint32_t num_compressed_tracks)
		{
			uint32_t num_movable_frames = 0;

			for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
			{
				const compressed_tracks* tracks = compressed_tracks_list[list_index];

				const tracks_header& header = get_tracks_header(*tracks);
				const transform_tracks_header& transform_header = get_transform_tracks_header(*tracks);

				// A frame is movable if it isn't the first or last frame of a segment
				// If we have more than 1 frame, we can remove 2 frames per segment
				// We know that the only way to get a segment with 1 frame is if the whole clip contains
				// a single frame and thus has one segment
				// If we have 0 or 1 frame, none are movable
				if (header.num_samples >= 2)
					num_movable_frames += header.num_samples - (transform_header.num_segments * 2);
			}

			return num_movable_frames;
		}

		inline uint32_t calculate_num_segments(const compressed_tracks* const* compressed_tracks_list, uint32_t num_compressed_tracks)
		{
			uint32_t num_segments = 0;

			for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
			{
				const compressed_tracks* tracks = compressed_tracks_list[list_index];
				const transform_tracks_header& transform_header = get_transform_tracks_header(*tracks);

				num_segments += transform_header.num_segments;
			}

			return num_segments;
		}

		inline void assign_frames_to_tier(frame_assignment_context& context, database_tier_mapping& tier_mapping)
		{
			// Iterate until we've fully assigned every frame we can to this tier
			for (uint32_t assigned_frame_count = 0; assigned_frame_count < tier_mapping.num_frames; ++assigned_frame_count)
			{
				frame_tier_mapping best_mapping{};
				best_mapping.contributing_error = std::numeric_limits<float>::infinity();

				// Iterate over every clip and find the one with the frame that has the lowest contributing error to assign to this tier
				for (uint32_t list_index = 0; list_index < context.num_compressed_tracks; ++list_index)
				{
					const compressed_tracks* tracks = context.compressed_tracks_list[list_index];
					const transform_tracks_header& transforms_header = get_transform_tracks_header(*tracks);
					const bool has_multiple_segments = transforms_header.has_multiple_segments();
					const uint32_t* segment_start_indices = has_multiple_segments ? transforms_header.get_segment_start_indices() : nullptr;
					const segment_header* segment_headers = transforms_header.get_segment_headers();

					const clip_contributing_error_t& clip_error = context.contributing_error_per_clip[list_index];

					if (clip_error.num_assigned >= clip_error.num_frames)
						continue;	// Every keyframe has been stripped from this clip

					const keyframe_stripping_metadata_t& next_keyframe_to_strip = clip_error.keyframe_metadata[clip_error.num_assigned];

					// TODO: When we populate our tiers, we should strip and ignore the trivial keyframes
					// that come first in the strip order

					// High importance frames can always be moved since they end up in our compressed tracks
					if (next_keyframe_to_strip.stripping_error < best_mapping.contributing_error ||
						tier_mapping.tier == quality_tier::highest_importance)
					{
						// This frame has a lower error, use it
						const uint32_t segment_index = next_keyframe_to_strip.segment_index;

						const uint8_t* format_per_track_data;
						const uint8_t* range_data;
						const uint8_t* animated_data;
						transforms_header.get_segment_data(segment_headers[segment_index], format_per_track_data, range_data, animated_data);

						const uint32_t segment_start_frame_index = has_multiple_segments ? segment_start_indices[segment_index] : 0;

						best_mapping.animated_data = animated_data;
						best_mapping.tracks_index = list_index;
						best_mapping.segment_index = segment_index;
						best_mapping.frame_bit_size = segment_headers[segment_index].animated_pose_bit_size;
						best_mapping.clip_frame_index = next_keyframe_to_strip.keyframe_index;
						best_mapping.segment_frame_index = next_keyframe_to_strip.keyframe_index - segment_start_frame_index;
						best_mapping.contributing_error = next_keyframe_to_strip.stripping_error;
					}
				}

				ACL_ASSERT(tier_mapping.tier == quality_tier::highest_importance || rtm::scalar_is_finite(best_mapping.contributing_error), "Error should be finite");

				// Assigned our mapping
				tier_mapping.frames[assigned_frame_count] = best_mapping;

				// Mark it as being assigned so we don't try to use it again
				context.contributing_error_per_clip[best_mapping.tracks_index].num_assigned++;
			}

			// Once we have assigned every frame we could to this tier, sort them by clip, by segment, then by segment frame index
			const auto sort_predicate = [](const frame_tier_mapping& lhs, const frame_tier_mapping& rhs)
			{
				// Sort by clip index first
				if (lhs.tracks_index < rhs.tracks_index)
					return true;
				else if (lhs.tracks_index == rhs.tracks_index)
				{
					// Sort by segment index second
					if (lhs.segment_index < rhs.segment_index)
						return true;
					else if (lhs.segment_index == rhs.segment_index)
					{
						// Sort by frame index third
						if (lhs.segment_frame_index < rhs.segment_frame_index)
							return true;
					}
				}

				return false;
			};

			std::sort(tier_mapping.frames, tier_mapping.frames + tier_mapping.num_frames, sort_predicate);
		}

		inline void assign_frames_to_tiers(frame_assignment_context& context)
		{
			// Assign frames to our lowest importance tier first
			assign_frames_to_tier(context, context.get_tier_mapping(quality_tier::lowest_importance));

			// Assign frames to our medium importance tier next
			assign_frames_to_tier(context, context.get_tier_mapping(quality_tier::medium_importance));

			// Then our high importance tier
			assign_frames_to_tier(context, context.get_tier_mapping(quality_tier::highest_importance));

			// Sanity check that everything has been assigned
#if defined(ACL_HAS_ASSERT_CHECKS)
			for (uint32_t list_index = 0; list_index < context.num_compressed_tracks; ++list_index)
			{
				const clip_contributing_error_t& clip_error = context.contributing_error_per_clip[list_index];
				ACL_ASSERT(clip_error.num_assigned == clip_error.num_frames, "Every frame should have been assigned");
			}
#endif
		}

		inline uint32_t find_first_metadata_offset(const optional_metadata_header& header)
		{
			if (header.track_list_name.is_valid())
				return (uint32_t)header.track_list_name;

			if (header.track_name_offsets.is_valid())
				return (uint32_t)header.track_name_offsets;

			if (header.parent_track_indices.is_valid())
				return (uint32_t)header.parent_track_indices;

			if (header.track_descriptions.is_valid())
				return (uint32_t)header.track_descriptions;

			if (header.contributing_error.is_valid())
				return (uint32_t)header.contributing_error;

			ACL_ASSERT(false, "Expected metadata to be present");
			return ~0U;
		}

		inline uint32_t get_metadata_track_list_name_size(const optional_metadata_header& header)
		{
			if (!header.track_list_name.is_valid())
				return 0;

			if (header.track_name_offsets.is_valid())
				return (uint32_t)header.track_name_offsets - (uint32_t)header.track_list_name;

			if (header.parent_track_indices.is_valid())
				return (uint32_t)header.parent_track_indices - (uint32_t)header.track_list_name;

			if (header.track_descriptions.is_valid())
				return (uint32_t)header.track_descriptions - (uint32_t)header.track_list_name;

			if (header.contributing_error.is_valid())
				return (uint32_t)header.contributing_error - (uint32_t)header.track_list_name;

			ACL_ASSERT(false, "Expected metadata to be present");
			return ~0U;
		}

		inline uint32_t get_metadata_track_names_size(const optional_metadata_header& header)
		{
			if (!header.track_name_offsets.is_valid())
				return 0;

			if (header.parent_track_indices.is_valid())
				return (uint32_t)header.parent_track_indices - (uint32_t)header.track_name_offsets;

			if (header.track_descriptions.is_valid())
				return (uint32_t)header.track_descriptions - (uint32_t)header.track_name_offsets;

			if (header.contributing_error.is_valid())
				return (uint32_t)header.contributing_error - (uint32_t)header.track_name_offsets;

			ACL_ASSERT(false, "Expected metadata to be present");
			return ~0U;
		}

		inline uint32_t get_metadata_parent_track_indices_size(const optional_metadata_header& header)
		{
			if (!header.parent_track_indices.is_valid())
				return 0;

			if (header.track_descriptions.is_valid())
				return (uint32_t)header.track_descriptions - (uint32_t)header.parent_track_indices;

			if (header.contributing_error.is_valid())
				return (uint32_t)header.contributing_error - (uint32_t)header.parent_track_indices;

			ACL_ASSERT(false, "Expected metadata to be present");
			return ~0U;
		}

		inline uint32_t get_metadata_track_descriptions_size(const optional_metadata_header& header)
		{
			if (!header.track_descriptions.is_valid())
				return 0;

			if (header.contributing_error.is_valid())
				return (uint32_t)header.contributing_error - (uint32_t)header.track_descriptions;

			ACL_ASSERT(false, "Expected metadata to be present");
			return ~0U;
		}

		inline uint32_t build_sample_indices(const database_tier_mapping& tier_mapping, uint32_t tracks_index, uint32_t segment_index)
		{
			// TODO: Binary search our first entry and bail out once done?

			const bitset_description desc = bitset_description::make_from_num_bits<32>();

			uint32_t sample_indices = 0;
			for (uint32_t frame_index = 0; frame_index < tier_mapping.num_frames; ++frame_index)
			{
				const frame_tier_mapping& frame = tier_mapping.frames[frame_index];
				if (frame.tracks_index != tracks_index)
					continue;	// This is not the tracks instance we care about

				if (frame.segment_index != segment_index)
					continue;	// This is not the segment we care about

				bitset_set(&sample_indices, desc, frame.segment_frame_index, true);
			}

			return sample_indices;
		}

		inline void rewrite_segment_headers(const database_tier_mapping& tier_mapping, uint32_t tracks_index, const transform_tracks_header& input_transforms_header, const segment_header* headers, uint32_t segment_data_base_offset, stripped_segment_header_t* out_headers)
		{
			const bitset_description desc = bitset_description::make_from_num_bits<32>();

			uint32_t segment_data_offset = segment_data_base_offset;
			for (uint32_t segment_index = 0; segment_index < input_transforms_header.num_segments; ++segment_index)
			{
				const uint32_t animated_pose_bit_size = headers[segment_index].animated_pose_bit_size;

				out_headers[segment_index].animated_pose_bit_size = animated_pose_bit_size;
				out_headers[segment_index].animated_rotation_bit_size = headers[segment_index].animated_rotation_bit_size;
				out_headers[segment_index].animated_translation_bit_size = headers[segment_index].animated_translation_bit_size;
				out_headers[segment_index].segment_data = segment_data_offset;
				out_headers[segment_index].sample_indices = build_sample_indices(tier_mapping, tracks_index, segment_index);

				const uint8_t* format_per_track_data;
				const uint8_t* range_data;
				const uint8_t* animated_data;
				input_transforms_header.get_segment_data(headers[segment_index], format_per_track_data, range_data, animated_data);

				// Range data, whether present or not, follows the per track metadata, use it to calculate our size
				const uint32_t format_per_track_data_size = uint32_t(range_data - format_per_track_data);

				// Animated data follows the range data (present or not), use it to calculate our size
				const uint32_t range_data_size = uint32_t(animated_data - range_data);

				const uint32_t num_animated_frames = bitset_count_set_bits(&out_headers[segment_index].sample_indices, desc);
				const uint32_t animated_data_num_bits = num_animated_frames * animated_pose_bit_size;
				const uint32_t animated_data_size = (animated_data_num_bits + 7) / 8;

				segment_data_offset += format_per_track_data_size;				// Format per track data

				// TODO: Alignment only necessary with 16bit per component (segment constant tracks), need to fix scalar decoding path
				segment_data_offset = align_to(segment_data_offset, 2);			// Align range data
				segment_data_offset += range_data_size;							// Range data

				// TODO: Variable bit rate doesn't need alignment
				segment_data_offset = align_to(segment_data_offset, 4);			// Align animated data
				segment_data_offset += animated_data_size;						// Animated data
			}
		}

		inline void rewrite_segment_data(const database_tier_mapping& tier_mapping, uint32_t tracks_index,
			const transform_tracks_header& input_transforms_header, const segment_header* input_headers,
			transform_tracks_header& output_transforms_header, const stripped_segment_header_t* output_headers)
		{
			for (uint32_t segment_index = 0; segment_index < input_transforms_header.num_segments; ++segment_index)
			{
				const uint8_t* input_format_per_track_data;
				const uint8_t* input_range_data;
				const uint8_t* input_animated_data;
				input_transforms_header.get_segment_data(input_headers[segment_index], input_format_per_track_data, input_range_data, input_animated_data);

				uint8_t* output_format_per_track_data;
				uint8_t* output_range_data;
				uint8_t* output_animated_data;
				output_transforms_header.get_segment_data(output_headers[segment_index], output_format_per_track_data, output_range_data, output_animated_data);

				// Range data, whether present or not, follows the per track metadata, use it to calculate our size
				const uint32_t format_per_track_data_size = uint32_t(input_range_data - input_format_per_track_data);

				// Animated data follows the range data (present or not), use it to calculate our size
				const uint32_t range_data_size = uint32_t(input_animated_data - input_range_data);

				// Copy our per track metadata, it does not change
				std::memcpy(output_format_per_track_data, input_format_per_track_data, format_per_track_data_size);

				// Copy our range data whether it is present or not, it does not change
				std::memcpy(output_range_data, input_range_data, range_data_size);

				// Populate our new animated data from our sorted frame mapping data
				uint64_t output_animated_bit_offset = 0;
				for (uint32_t frame_index = 0; frame_index < tier_mapping.num_frames; ++frame_index)
				{
					const frame_tier_mapping& frame = tier_mapping.frames[frame_index];
					if (frame.tracks_index != tracks_index)
						continue;	// This is not the tracks instance we care about

					if (frame.segment_index != segment_index)
						continue;	// This is not the segment we care about

					// Append this frame
					const uint32_t input_animated_bit_offset = frame.segment_frame_index * frame.frame_bit_size;
					memcpy_bits(output_animated_data, output_animated_bit_offset, input_animated_data, input_animated_bit_offset, frame.frame_bit_size);
					output_animated_bit_offset += frame.frame_bit_size;
				}
			}
		}

		inline void build_compressed_tracks(const frame_assignment_context& context, compressed_tracks** out_compressed_tracks)
		{
			const bitset_description desc = bitset_description::make_from_num_bits<32>();

			const database_tier_mapping& tier_mapping = context.get_tier_mapping(quality_tier::highest_importance);
			uint32_t clip_header_offset = 0;

			for (uint32_t list_index = 0; list_index < context.num_compressed_tracks; ++list_index)
			{
				const compressed_tracks* input_tracks = context.compressed_tracks_list[list_index];

				const tracks_header& input_header = get_tracks_header(*input_tracks);
				const transform_tracks_header& input_transforms_header = get_transform_tracks_header(*input_tracks);
				const segment_header* input_segment_headers = input_transforms_header.get_segment_headers();
				const optional_metadata_header& input_metadata_header = get_optional_metadata_header(*input_tracks);

				const uint32_t num_sub_tracks_per_bone = input_header.get_has_scale() ? 3U : 2U;

				// Calculate how many sub-track packed entries we have
				// Each sub-track is 2 bits packed within a 32 bit entry
				// For each sub-track type, we round up to simplify bookkeeping
				// For example, if we have 3 tracks made up of rotation/translation we'll have one entry for each with unused padding
				// All rotation types come first, followed by all translation types, and with scale types at the end when present
				const uint32_t num_sub_track_entries = ((input_header.num_tracks + k_num_sub_tracks_per_packed_entry - 1) / k_num_sub_tracks_per_packed_entry) * num_sub_tracks_per_bone;
				const uint32_t packed_sub_track_buffer_size = num_sub_track_entries * sizeof(packed_sub_track_types);

				// Adding an extra index at the end to delimit things, the index is always invalid: 0xFFFFFFFF
				const uint32_t segment_start_indices_size = input_transforms_header.num_segments > 1 ? (uint32_t(sizeof(uint32_t)) * (input_transforms_header.num_segments + 1)) : 0;
				const uint32_t segment_headers_size = sizeof(stripped_segment_header_t) * input_transforms_header.num_segments;

				// Range data follows constant data, use that to calculate our size
				const uint32_t constant_data_size = (uint32_t)input_transforms_header.clip_range_data_offset - (uint32_t)input_transforms_header.constant_track_data_offset;

				// The data from our first segment follows the clip range data, use that to calculate our size
				const uint32_t clip_range_data_size = (uint32_t)input_segment_headers[0].segment_data - (uint32_t)input_transforms_header.clip_range_data_offset;

				// Calculate the new size of our clip
				uint32_t buffer_size = 0;

				// Per clip data
				buffer_size += sizeof(raw_buffer_header);							// Header
				buffer_size += sizeof(tracks_header);								// Header
				buffer_size += sizeof(transform_tracks_header);						// Header

				buffer_size = align_to(buffer_size, 4);								// Align segment start indices
				buffer_size += segment_start_indices_size;							// Segment start indices
				buffer_size = align_to(buffer_size, 4);								// Align segment headers
				buffer_size += segment_headers_size;								// Segment headers

				buffer_size = align_to(buffer_size, 4);								// Align database header
				buffer_size += sizeof(tracks_database_header);						// Database header

				buffer_size = align_to(buffer_size, 4);								// Align sub-track types
				buffer_size += packed_sub_track_buffer_size;						// Packed sub-track types sorted by type
				buffer_size = align_to(buffer_size, 4);								// Align constant track data
				buffer_size += constant_data_size;									// Constant track data
				buffer_size = align_to(buffer_size, 4);								// Align range data
				buffer_size += clip_range_data_size;								// Range data

				uint32_t num_remaining_keyframes = 0;

				// Per segment data
				for (uint32_t segment_index = 0; segment_index < input_transforms_header.num_segments; ++segment_index)
				{
					const uint8_t* format_per_track_data;
					const uint8_t* range_data;
					const uint8_t* animated_data;
					input_transforms_header.get_segment_data(input_segment_headers[segment_index], format_per_track_data, range_data, animated_data);

					// Range data, whether present or not, follows the per track metadata, use it to calculate our size
					const uint32_t format_per_track_data_size = uint32_t(range_data - format_per_track_data);

					// Animated data follows the range data (present or not), use it to calculate our size
					const uint32_t range_data_size = uint32_t(animated_data - range_data);

					buffer_size += format_per_track_data_size;				// Format per track data

					// TODO: Alignment only necessary with 16bit per component (segment constant tracks), need to fix scalar decoding path
					buffer_size = align_to(buffer_size, 2);					// Align range data
					buffer_size += range_data_size;							// Range data

					// Check our data mapping to find our how many frames we'll retain
					const uint32_t sample_indices = build_sample_indices(tier_mapping, list_index, segment_index);
					const uint32_t num_animated_frames = bitset_count_set_bits(&sample_indices, desc);
					const uint32_t animated_data_size = ((num_animated_frames * input_segment_headers[segment_index].animated_pose_bit_size) + 7) / 8;

					num_remaining_keyframes += num_animated_frames;

					// TODO: Variable bit rate doesn't need alignment
					buffer_size = align_to(buffer_size, 4);					// Align animated data
					buffer_size += animated_data_size;						// Animated track data
				}

				const uint32_t num_stripped_keyframes = input_header.num_samples - num_remaining_keyframes;

				// Optional metadata
				const uint32_t metadata_start_offset = align_to(buffer_size, 4);
				const uint32_t metadata_track_list_name_size = get_metadata_track_list_name_size(input_metadata_header);
				const uint32_t metadata_track_names_size = get_metadata_track_names_size(input_metadata_header);
				const uint32_t metadata_parent_track_indices_size = get_metadata_parent_track_indices_size(input_metadata_header);
				const uint32_t metadata_track_descriptions_size = get_metadata_track_descriptions_size(input_metadata_header);
				const uint32_t metadata_contributing_error_size = 0;	// We'll strip it!

				uint32_t metadata_size = 0;
				metadata_size += metadata_track_list_name_size;
				metadata_size = align_to(metadata_size, 4);
				metadata_size += metadata_track_names_size;
				metadata_size = align_to(metadata_size, 4);
				metadata_size += metadata_parent_track_indices_size;
				metadata_size = align_to(metadata_size, 4);
				metadata_size += metadata_track_descriptions_size;
				metadata_size = align_to(metadata_size, 4);
				metadata_size += metadata_contributing_error_size;

				if (metadata_size != 0)
				{
					buffer_size = align_to(buffer_size, 4);
					buffer_size += metadata_size;

					buffer_size = align_to(buffer_size, 4);
					buffer_size += sizeof(optional_metadata_header);
				}
				else
					buffer_size += 15;	// Ensure we have sufficient padding for unaligned 16 byte loads

				// Allocate our new buffer
				uint8_t* buffer = allocate_type_array_aligned<uint8_t>(context.allocator, buffer_size, alignof(compressed_tracks));
				std::memset(buffer, 0, buffer_size);

				uint8_t* buffer_start = buffer;
				out_compressed_tracks[list_index] = bit_cast<compressed_tracks*>(buffer);

				raw_buffer_header* buffer_header = safe_ptr_cast<raw_buffer_header>(buffer);
				buffer += sizeof(raw_buffer_header);

				tracks_header* header = safe_ptr_cast<tracks_header>(buffer);
				buffer += sizeof(tracks_header);

				// Copy our header and update the parts that change
				std::memcpy(header, &input_header, sizeof(tracks_header));

				header->set_has_database(true);
				header->set_has_stripped_keyframes(num_stripped_keyframes != 0);
				header->set_has_metadata(metadata_size != 0);

				transform_tracks_header* transforms_header = safe_ptr_cast<transform_tracks_header>(buffer);
				buffer += sizeof(transform_tracks_header);

				// Copy our header and update the parts that change
				std::memcpy(transforms_header, &input_transforms_header, sizeof(transform_tracks_header));

				const uint32_t segment_start_indices_offset = align_to<uint32_t>(sizeof(transform_tracks_header), 4);	// Relative to the start of our transform_tracks_header
				transforms_header->database_header_offset = align_to(segment_start_indices_offset + segment_start_indices_size, 4);
				transforms_header->segment_headers_offset = align_to(transforms_header->database_header_offset + uint32_t(sizeof(tracks_database_header)), 4);
				transforms_header->sub_track_types_offset = align_to(transforms_header->segment_headers_offset + segment_headers_size, 4);
				transforms_header->constant_track_data_offset = align_to(transforms_header->sub_track_types_offset + packed_sub_track_buffer_size, 4);
				transforms_header->clip_range_data_offset = align_to(transforms_header->constant_track_data_offset + constant_data_size, 4);

				// Copy our segment start indices, they do not change
				if (input_transforms_header.has_multiple_segments())
					std::memcpy(transforms_header->get_segment_start_indices(), input_transforms_header.get_segment_start_indices(), segment_start_indices_size);

				// Setup our database header
				tracks_database_header* tracks_db_header = transforms_header->get_database_header();
				tracks_db_header->clip_header_offset = clip_header_offset;

				// Update our clip header offset
				clip_header_offset += sizeof(database_runtime_clip_header);
				clip_header_offset += sizeof(database_runtime_segment_header) * input_transforms_header.num_segments;

				// Write our new segment headers
				const uint32_t segment_data_base_offset = transforms_header->clip_range_data_offset + clip_range_data_size;
				rewrite_segment_headers(tier_mapping, list_index, input_transforms_header, input_segment_headers, segment_data_base_offset, transforms_header->get_stripped_segment_headers());

				// Copy our sub-track types, they do not change
				std::memcpy(transforms_header->get_sub_track_types(), input_transforms_header.get_sub_track_types(), packed_sub_track_buffer_size);

				// Copy our constant track data, it does not change
				std::memcpy(transforms_header->get_constant_track_data(), input_transforms_header.get_constant_track_data(), constant_data_size);

				// Copy our clip range data, it does not change
				std::memcpy(transforms_header->get_clip_range_data(), input_transforms_header.get_clip_range_data(), clip_range_data_size);

				// Write our new segment data
				rewrite_segment_data(tier_mapping, list_index, input_transforms_header, input_segment_headers, *transforms_header, transforms_header->get_stripped_segment_headers());

				if (metadata_size != 0)
				{
					optional_metadata_header* metadata_header = bit_cast<optional_metadata_header*>(buffer_start + buffer_size - sizeof(optional_metadata_header));
					uint32_t metadata_offset = metadata_start_offset;	// Relative to the start of our compressed_tracks

					// Setup our metadata offsets
					if (metadata_track_list_name_size != 0)
					{
						metadata_header->track_list_name = metadata_offset;
						metadata_offset += metadata_track_list_name_size;
					}
					else
						metadata_header->track_list_name = invalid_ptr_offset();

					if (metadata_track_names_size != 0)
					{
						metadata_header->track_name_offsets = metadata_offset;
						metadata_offset += metadata_track_names_size;
					}
					else
						metadata_header->track_name_offsets = invalid_ptr_offset();

					if (metadata_parent_track_indices_size != 0)
					{
						metadata_header->parent_track_indices = metadata_offset;
						metadata_offset += metadata_parent_track_indices_size;
					}
					else
						metadata_header->parent_track_indices = invalid_ptr_offset();

					if (metadata_track_descriptions_size != 0)
					{
						metadata_header->track_descriptions = metadata_offset;
						metadata_offset += metadata_track_descriptions_size;
					}
					else
						metadata_header->track_descriptions = invalid_ptr_offset();

					// Strip the contributing error data, no longer needed
					metadata_header->contributing_error = invalid_ptr_offset();

					ACL_ASSERT((metadata_offset - metadata_start_offset) == metadata_size, "Unexpected metadata size");

					// Copy our metadata, it does not change
					std::memcpy(metadata_header->get_track_list_name(*out_compressed_tracks[list_index]), input_metadata_header.get_track_list_name(*input_tracks), metadata_track_list_name_size);
					std::memcpy(metadata_header->get_track_name_offsets(*out_compressed_tracks[list_index]), input_metadata_header.get_track_name_offsets(*input_tracks), metadata_track_names_size);
					std::memcpy(metadata_header->get_parent_track_indices(*out_compressed_tracks[list_index]), input_metadata_header.get_parent_track_indices(*input_tracks), metadata_parent_track_indices_size);
					std::memcpy(metadata_header->get_track_descriptions(*out_compressed_tracks[list_index]), input_metadata_header.get_track_descriptions(*input_tracks), metadata_track_descriptions_size);
				}

				// Finish the compressed tracks raw buffer header
				buffer_header->size = buffer_size;
				buffer_header->hash = hash32(safe_ptr_cast<const uint8_t>(header), buffer_size - sizeof(raw_buffer_header));	// Hash everything but the raw buffer header

				ACL_ASSERT(out_compressed_tracks[list_index]->is_valid(true).empty(), "Failed to build compressed tracks");
			}
		}

		// Returns the number of clips written
		inline uint32_t write_database_clip_metadata(const compressed_tracks* const* db_compressed_tracks_list, uint32_t num_tracks, database_clip_metadata* clip_metadatas)
		{
			if (clip_metadatas == nullptr)
				return num_tracks;	// Nothing to write

			uint32_t clip_header_offset = 0;

			for (uint32_t tracks_index = 0; tracks_index < num_tracks; ++tracks_index)
			{
				const compressed_tracks* tracks = db_compressed_tracks_list[tracks_index];

				database_clip_metadata& clip_metadata = clip_metadatas[tracks_index];
				clip_metadata.clip_hash = tracks->get_hash();
				clip_metadata.clip_header_offset = clip_header_offset;

				const transform_tracks_header& transforms_header = get_transform_tracks_header(*tracks);
				clip_header_offset += sizeof(database_runtime_clip_header) + sizeof(database_runtime_segment_header) * transforms_header.num_segments;
			}

			return num_tracks;
		}

		// Returns a pointer to the first frame of the given segment and the number of frames contained
		inline const frame_tier_mapping* find_segment_frames(const database_tier_mapping& tier_mapping, uint32_t tracks_index, uint32_t segment_index, uint32_t& out_num_frames)
		{
			for (uint32_t frame_index = 0; frame_index < tier_mapping.num_frames; ++frame_index)
			{
				const frame_tier_mapping& frame = tier_mapping.frames[frame_index];
				if (frame.tracks_index != tracks_index)
					continue;	// This is not the tracks instance we care about

				if (frame.segment_index != segment_index)
					continue;	// This is not the segment we care about

				// Found our first frame, count how many we have
				uint32_t num_segment_frames = 0;
				for (uint32_t frame_index2 = frame_index; frame_index2 < tier_mapping.num_frames; ++frame_index2)
				{
					const frame_tier_mapping& frame2 = tier_mapping.frames[frame_index2];
					if (frame2.tracks_index != tracks_index)
						break;	// This is not the tracks instance we care about

					if (frame2.segment_index != segment_index)
						break;	// This is not the segment we care about

					num_segment_frames++;
				}

				// Done
				out_num_frames = num_segment_frames;
				return &frame;
			}

			// This segment doesn't contain any frames
			out_num_frames = 0;
			return nullptr;
		}

		// Returns the number of bits contained in the segment animated data
		inline uint32_t calculate_segment_animated_data_bit_size(const frame_tier_mapping* frames, uint32_t num_frames)
		{
			if (num_frames == 0)
				return 0;	// Empty segment

			// Every frame in a segment has the same size
			return frames->frame_bit_size * num_frames;
		}

		// Returns a bitset with the which frames are present in this segment
		inline uint32_t build_sample_indices(const frame_tier_mapping* frames, uint32_t num_frames)
		{
			const bitset_description desc = bitset_description::make_from_num_bits<32>();

			uint32_t sample_indices = 0;
			for (uint32_t frame_index = 0; frame_index < num_frames; ++frame_index)
			{
				const frame_tier_mapping& frame = frames[frame_index];

				bitset_set(&sample_indices, desc, frame.segment_frame_index, true);
			}

			return sample_indices;
		}

		// Returns the number of bytes written
		inline uint32_t write_tier_segment_data(const frame_tier_mapping* frames, uint32_t num_frames, uint8_t* out_segment_data)
		{
			uint64_t num_bits_written = 0;

			for (uint32_t frame_index = 0; frame_index < num_frames; ++frame_index)
			{
				const frame_tier_mapping& frame = frames[frame_index];

				const uint32_t frame_bit_offset = frame.segment_frame_index * frame.frame_bit_size;
				memcpy_bits(out_segment_data, num_bits_written, frame.animated_data, frame_bit_offset, frame.frame_bit_size);
				num_bits_written += frame.frame_bit_size;
			}

			return uint32_t(num_bits_written + 7) / 8;
		}

		// Returns the number of chunks written
		inline uint32_t write_database_chunk_descriptions(const frame_assignment_context& context, const compression_database_settings& settings, quality_tier tier, database_chunk_description* chunk_descriptions)
		{
			ACL_ASSERT(tier != quality_tier::highest_importance, "No chunks for the high importance tier");
			const database_tier_mapping& tier_mapping = context.get_tier_mapping(tier);
			if (tier_mapping.is_empty())
				return 0;	// No data

			const uint32_t max_chunk_size = settings.max_chunk_size;
			const uint32_t simd_padding = 15;

			uint32_t bulk_data_offset = 0;
			uint32_t chunk_size = sizeof(database_chunk_header);
			uint32_t num_chunks = 0;

			for (uint32_t tracks_index = 0; tracks_index < context.num_compressed_tracks; ++tracks_index)
			{
				const compressed_tracks* tracks = context.compressed_tracks_list[tracks_index];
				const transform_tracks_header& transforms_header = get_transform_tracks_header(*tracks);

				for (uint32_t segment_index = 0; segment_index < transforms_header.num_segments; ++segment_index)
				{
					uint32_t num_segment_frames;
					const frame_tier_mapping* segment_frames = find_segment_frames(tier_mapping, tracks_index, segment_index, num_segment_frames);

					const uint32_t segment_data_bit_size = calculate_segment_animated_data_bit_size(segment_frames, num_segment_frames);
					const uint32_t segment_data_size = (segment_data_bit_size + 7) / 8;
					ACL_ASSERT(segment_data_size + simd_padding + uint32_t(sizeof(database_chunk_segment_header)) <= max_chunk_size, "Segment is larger than our max chunk size");

					const uint32_t new_chunk_size = chunk_size + segment_data_size + simd_padding + sizeof(database_chunk_segment_header);
					if (new_chunk_size >= max_chunk_size)
					{
						// Chunk is full, write it out and start a new one
						if (chunk_descriptions != nullptr)
						{
							chunk_descriptions[num_chunks].size = max_chunk_size;
							chunk_descriptions[num_chunks].offset = bulk_data_offset;
						}

						bulk_data_offset += max_chunk_size;
						chunk_size = sizeof(database_chunk_header);
						num_chunks++;
					}

					chunk_size += segment_data_size + sizeof(database_chunk_segment_header);

					ACL_ASSERT(chunk_size <= max_chunk_size, "Expected a valid chunk size, segment is larger than max chunk size?");
				}
			}

			// If we have leftover data, finalize our last chunk
			// Because of this, it might end up being larger than the max chunk size which is fine since we can't split a segment over two chunks
			if (chunk_size != sizeof(database_chunk_header))
			{
				if (chunk_descriptions != nullptr)
				{
					chunk_descriptions[num_chunks].size = chunk_size + simd_padding;	// Last chunk needs padding
					chunk_descriptions[num_chunks].offset = bulk_data_offset;
				}

				num_chunks++;
			}

			return num_chunks;
		}

		// Returns the size of the bulk data
		inline uint32_t write_database_bulk_data(const frame_assignment_context& context, const compression_database_settings& settings, quality_tier tier, const compressed_tracks* const* db_compressed_tracks_list, uint8_t* bulk_data)
		{
			ACL_ASSERT(tier != quality_tier::highest_importance, "No bulk data for the high importance tier");
			const database_tier_mapping& tier_mapping = context.get_tier_mapping(tier);
			if (tier_mapping.is_empty())
				return 0;	// No data

			const uint32_t max_chunk_size = settings.max_chunk_size;
			const uint32_t simd_padding = 15;

			database_chunk_header* chunk_header = nullptr;
			database_chunk_segment_header* segment_chunk_headers = nullptr;

			uint32_t bulk_data_offset = 0;
			uint32_t chunk_sample_data_offset = 0;
			uint32_t chunk_size = sizeof(database_chunk_header);
			uint32_t chunk_index = 0;

			uint32_t clip_header_offset = 0;

			if (bulk_data != nullptr)
			{
				// Setup our chunk headers
				chunk_header = safe_ptr_cast<database_chunk_header>(bulk_data);
				chunk_header->index = chunk_index;
				chunk_header->size = 0;
				chunk_header->num_segments = 0;

				segment_chunk_headers = chunk_header->get_segment_headers();
			}

			// We first iterate to find our chunk delimitations and write our headers
			for (uint32_t tracks_index = 0; tracks_index < context.num_compressed_tracks; ++tracks_index)
			{
				const compressed_tracks* tracks = db_compressed_tracks_list[tracks_index];
				const transform_tracks_header& transforms_header = get_transform_tracks_header(*tracks);

				uint32_t segment_header_offset = clip_header_offset + sizeof(database_runtime_clip_header);

				for (uint32_t segment_index = 0; segment_index < transforms_header.num_segments; ++segment_index)
				{
					uint32_t num_segment_frames;
					const frame_tier_mapping* segment_frames = find_segment_frames(tier_mapping, tracks_index, segment_index, num_segment_frames);

					const uint32_t segment_data_bit_size = calculate_segment_animated_data_bit_size(segment_frames, num_segment_frames);
					const uint32_t segment_data_size = (segment_data_bit_size + 7) / 8;
					ACL_ASSERT(segment_data_size + simd_padding + uint32_t(sizeof(database_chunk_segment_header)) <= max_chunk_size, "Segment is larger than our max chunk size");

					const uint32_t new_chunk_size = chunk_size + segment_data_size + simd_padding + sizeof(database_chunk_segment_header);
					if (new_chunk_size >= max_chunk_size)
					{
						// Finalize our chunk header
						if (bulk_data != nullptr)
							chunk_header->size = max_chunk_size;

						// Chunk is full, start a new one
						bulk_data_offset += max_chunk_size;
						chunk_sample_data_offset = 0;
						chunk_size = sizeof(database_chunk_header);
						chunk_index++;

						// Setup our chunk headers
						if (bulk_data != nullptr)
						{
							chunk_header = safe_ptr_cast<database_chunk_header>(bulk_data + bulk_data_offset);
							chunk_header->index = chunk_index;
							chunk_header->size = 0;
							chunk_header->num_segments = 0;

							segment_chunk_headers = chunk_header->get_segment_headers();
						}
					}

					if (bulk_data != nullptr)
					{
						// TODO: Should we skip segments with no data?

						// Update our chunk headers
						database_chunk_segment_header& segment_chunk_header = segment_chunk_headers[chunk_header->num_segments];
						segment_chunk_header.clip_hash = tracks->get_hash();
						segment_chunk_header.sample_indices = build_sample_indices(segment_frames, num_segment_frames);
						segment_chunk_header.samples_offset = chunk_sample_data_offset;	// Relative to start of sample data for now
						segment_chunk_header.clip_header_offset = clip_header_offset;
						segment_chunk_header.segment_header_offset = segment_header_offset;

						chunk_header->num_segments++;
					}

					chunk_size += segment_data_size + sizeof(database_chunk_segment_header);
					chunk_sample_data_offset += segment_data_size;
					segment_header_offset += sizeof(database_runtime_segment_header);

					ACL_ASSERT(chunk_size <= max_chunk_size, "Expected a valid chunk size, segment is larger than max chunk size?");
				}

				clip_header_offset = segment_header_offset;
			}

			// If we have leftover data, finalize our last chunk
			// Because of this, it might end up being larger than the max chunk size which is fine since we can't split a segment over two chunks
			if (chunk_size != sizeof(database_chunk_header))
			{
				chunk_size += simd_padding;	// Last chunk needs padding

				if (bulk_data != nullptr)
				{
					// Finalize our chunk header
					chunk_header->size = chunk_size;
				}

				bulk_data_offset += chunk_size;
			}

			// Now that our chunk headers are written, write our sample data
			if (bulk_data != nullptr)
			{
				// Reset our header pointers
				chunk_header = safe_ptr_cast<database_chunk_header>(bulk_data);
				segment_chunk_headers = chunk_header->get_segment_headers();

				uint32_t chunk_segment_index = 0;
				for (uint32_t tracks_index = 0; tracks_index < context.num_compressed_tracks; ++tracks_index)
				{
					const compressed_tracks* tracks = db_compressed_tracks_list[tracks_index];
					const transform_tracks_header& transforms_header = get_transform_tracks_header(*tracks);

					for (uint32_t segment_index = 0; segment_index < transforms_header.num_segments; ++segment_index)
					{
						uint32_t num_segment_frames;
						const frame_tier_mapping* segment_frames = find_segment_frames(tier_mapping, tracks_index, segment_index, num_segment_frames);

						const uint32_t segment_data_bit_size = calculate_segment_animated_data_bit_size(segment_frames, num_segment_frames);
						const uint32_t segment_data_size = (segment_data_bit_size + 7) / 8;

						if (chunk_segment_index >= chunk_header->num_segments)
						{
							// We hit the next chunk, update our pointers
							chunk_header = add_offset_to_ptr<database_chunk_header>(chunk_header, chunk_header->size);
							segment_chunk_headers = chunk_header->get_segment_headers();
							chunk_segment_index = 0;
						}

						// Calculate the finale offset for our chunk's data relative to the bulk data start and the final header size
						const uint32_t chunk_data_offset = static_cast<uint32_t>(bit_cast<uint8_t*>(chunk_header) - bulk_data);
						const uint32_t chunk_header_size = sizeof(database_chunk_header) + chunk_header->num_segments * sizeof(database_chunk_segment_header);

						// Update the sample offset from being relative to the start of the sample data to the start of the bulk data
						database_chunk_segment_header& segment_chunk_header = segment_chunk_headers[chunk_segment_index];
						segment_chunk_header.samples_offset = chunk_data_offset + chunk_header_size + segment_chunk_header.samples_offset;

						uint8_t* animated_data = segment_chunk_header.samples_offset.add_to(bulk_data);
						const uint32_t size = write_tier_segment_data(segment_frames, num_segment_frames, animated_data);
						ACL_ASSERT(size == segment_data_size, "Unexpected segment data size"); (void)size; (void)segment_data_size;

						chunk_segment_index++;
					}
				}
			}

			return bulk_data_offset;
		}

		inline compressed_database* build_compressed_database(const frame_assignment_context& context, const compression_database_settings& settings, const compressed_tracks* const* db_compressed_tracks_list)
		{
			// Find our chunk limits and calculate our database size
			const uint32_t num_tracks = write_database_clip_metadata(db_compressed_tracks_list, context.num_compressed_tracks, nullptr);
			const uint32_t num_segments = calculate_num_segments(db_compressed_tracks_list, context.num_compressed_tracks);
			const uint32_t num_medium_chunks = write_database_chunk_descriptions(context, settings, quality_tier::medium_importance, nullptr);
			const uint32_t num_low_chunks = write_database_chunk_descriptions(context, settings, quality_tier::lowest_importance, nullptr);

			// Pad medium tier bulk data to ensure alignment since the lowest tier follows
			const uint32_t bulk_data_medium_size = write_database_bulk_data(context, settings, quality_tier::medium_importance, db_compressed_tracks_list, nullptr);
			const uint32_t aligned_bulk_data_medium_size = align_to(bulk_data_medium_size, k_database_bulk_data_alignment);

			// No need to pad lowest tier since it is last
			const uint32_t bulk_data_low_size = write_database_bulk_data(context, settings, quality_tier::lowest_importance, db_compressed_tracks_list, nullptr);

			uint32_t database_buffer_size = 0;
			database_buffer_size += sizeof(raw_buffer_header);										// Header
			database_buffer_size += sizeof(database_header);										// Header

			database_buffer_size = align_to(database_buffer_size, 4);								// Align chunk descriptions
			database_buffer_size += num_medium_chunks * sizeof(database_chunk_description);			// Chunk descriptions

			database_buffer_size = align_to(database_buffer_size, 4);								// Align chunk descriptions
			database_buffer_size += num_low_chunks * sizeof(database_chunk_description);			// Chunk descriptions

			database_buffer_size = align_to(database_buffer_size, 4);								// Align clip hashes
			database_buffer_size += num_tracks * sizeof(database_clip_metadata);					// Clip metadata

			database_buffer_size = align_to(database_buffer_size, k_database_bulk_data_alignment);	// Align bulk data
			database_buffer_size += aligned_bulk_data_medium_size;									// Bulk data
			database_buffer_size += bulk_data_low_size;												// Bulk data

			uint8_t* database_buffer = allocate_type_array_aligned<uint8_t>(context.allocator, database_buffer_size, alignof(compressed_database));
			std::memset(database_buffer, 0, database_buffer_size);

			compressed_database* database = bit_cast<compressed_database*>(database_buffer);

			const uint8_t* database_buffer_start = database_buffer;

			raw_buffer_header* database_buffer_header = safe_ptr_cast<raw_buffer_header>(database_buffer);
			database_buffer += sizeof(raw_buffer_header);

			const uint8_t* db_header_start = database_buffer;
			database_header* db_header = safe_ptr_cast<database_header>(database_buffer);
			database_buffer += sizeof(database_header);

			// Write our header
			db_header->tag = static_cast<uint32_t>(buffer_tag32::compressed_database);
			db_header->version = compressed_tracks_version16::latest;
			db_header->num_chunks[0] = num_medium_chunks;
			db_header->num_chunks[1] = num_low_chunks;
			db_header->max_chunk_size = settings.max_chunk_size;
			db_header->num_clips = num_tracks;
			db_header->num_segments = num_segments;
			db_header->bulk_data_size[0] = aligned_bulk_data_medium_size;
			db_header->bulk_data_size[1] = bulk_data_low_size;
			db_header->set_is_bulk_data_inline(true);	// Data is always inline when compressing

			database_buffer = align_to(database_buffer, 4);										// Align chunk descriptions
			database_buffer += num_medium_chunks * sizeof(database_chunk_description);			// Chunk descriptions

			database_buffer = align_to(database_buffer, 4);										// Align chunk descriptions
			database_buffer += num_low_chunks * sizeof(database_chunk_description);				// Chunk descriptions

			database_buffer = align_to(database_buffer, 4);										// Align clip hashes
			db_header->clip_metadata_offset = uint32_t(database_buffer - db_header_start);		// Clip metadata
			database_buffer += num_tracks * sizeof(database_clip_metadata);						// Clip metadata

			database_buffer = align_to(database_buffer, k_database_bulk_data_alignment);		// Align bulk data
			if (aligned_bulk_data_medium_size != 0)
				db_header->bulk_data_offset[0] = uint32_t(database_buffer - db_header_start);	// Bulk data
			else
				db_header->bulk_data_offset[0] = invalid_ptr_offset();
			database_buffer += aligned_bulk_data_medium_size;									// Bulk data

			if (bulk_data_low_size != 0)
				db_header->bulk_data_offset[1] = uint32_t(database_buffer - db_header_start);	// Bulk data
			else
				db_header->bulk_data_offset[1] = invalid_ptr_offset();
			database_buffer += bulk_data_low_size;												// Bulk data

			// Write our chunk descriptions
			const uint32_t num_written_medium_chunks = write_database_chunk_descriptions(context, settings, quality_tier::medium_importance, db_header->get_chunk_descriptions_medium());
			ACL_ASSERT(num_written_medium_chunks == num_medium_chunks, "Unexpected amount of data written"); (void)num_written_medium_chunks;

			const uint32_t num_written_low_chunks = write_database_chunk_descriptions(context, settings, quality_tier::lowest_importance, db_header->get_chunk_descriptions_low());
			ACL_ASSERT(num_written_low_chunks == num_low_chunks, "Unexpected amount of data written"); (void)num_written_low_chunks;

			// Write our clip metadata
			const uint32_t num_written_tracks = write_database_clip_metadata(db_compressed_tracks_list, context.num_compressed_tracks, db_header->get_clip_metadatas());
			ACL_ASSERT(num_written_tracks == num_tracks, "Unexpected amount of data written"); (void)num_written_tracks;

			// Write our bulk data
			const uint32_t written_bulk_data_medium_size = write_database_bulk_data(context, settings, quality_tier::medium_importance, db_compressed_tracks_list, db_header->get_bulk_data_medium());
			ACL_ASSERT(written_bulk_data_medium_size == bulk_data_medium_size, "Unexpected amount of data written"); (void)written_bulk_data_medium_size;
			db_header->bulk_data_hash[0] = hash32(db_header->get_bulk_data_medium(), aligned_bulk_data_medium_size);

			const uint32_t written_bulk_data_low_size = write_database_bulk_data(context, settings, quality_tier::lowest_importance, db_compressed_tracks_list, db_header->get_bulk_data_low());
			ACL_ASSERT(written_bulk_data_low_size == bulk_data_low_size, "Unexpected amount of data written"); (void)written_bulk_data_low_size;
			db_header->bulk_data_hash[1] = hash32(db_header->get_bulk_data_low(), bulk_data_low_size);

			ACL_ASSERT(uint32_t(database_buffer - database_buffer_start) == database_buffer_size, "Unexpected amount of data written"); (void)database_buffer_start;

#if defined(ACL_HAS_ASSERT_CHECKS)
			// Make sure nobody overwrote our padding (contained in last chunk if we have data)
			if (bulk_data_low_size != 0)
			{
				for (const uint8_t* padding = database_buffer - 15; padding < database_buffer; ++padding)
					ACL_ASSERT(*padding == 0, "Padding was overwritten");
			}
#endif

			// Finish the raw buffer header
			database_buffer_header->size = database_buffer_size;
			database_buffer_header->hash = hash32(safe_ptr_cast<const uint8_t>(db_header), database_buffer_size - sizeof(raw_buffer_header));	// Hash everything but the raw buffer header

			ACL_ASSERT(database->is_valid(true).empty(), "Failed to build compressed database");

			return database;
		}
	}

	inline error_result build_database(iallocator& allocator, const compression_database_settings& settings,
		const compressed_tracks* const* compressed_tracks_list, uint32_t num_compressed_tracks,
		compressed_tracks** out_compressed_tracks, compressed_database*& out_database)
	{
		using namespace acl_impl;

		// Reset everything just to be safe
		for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
			out_compressed_tracks[list_index] = nullptr;
		out_database = nullptr;

		// Validate everything and early out if something isn't right
		const error_result settings_result = settings.is_valid();
		if (settings_result.any())
			return error_result("Compression database settings are invalid");

		if (compressed_tracks_list == nullptr || num_compressed_tracks == 0)
			return error_result("No compressed track list provided");

		for (uint32_t list_index = 0; list_index < num_compressed_tracks; ++list_index)
		{
			const compressed_tracks* tracks = compressed_tracks_list[list_index];
			if (tracks == nullptr)
				return error_result("Compressed track list contains a null entry");

			const error_result tracks_result = tracks->is_valid(false);
			if (tracks_result.any())
				return error_result("Compressed track instance is invalid");

			if (tracks->has_database())
				return error_result("Compressed track instance is already bound to a database");

			if (tracks->has_stripped_keyframes())
				return error_result("Compressed track instance has keyframes stripped");

			const tracks_header& header = get_tracks_header(*tracks);
			if (!header.get_has_metadata())
				return error_result("Compressed track instance does not contain any metadata");

			const optional_metadata_header& metadata_header = get_optional_metadata_header(*tracks);
			if (!metadata_header.contributing_error.is_valid())
				return error_result("Compressed track instance does not contain contributing error metadata");
		}

		// Calculate how many frames are movable to the database
		// A frame is movable if it isn't the first or last frame of a segment
		const uint32_t num_frames = calculate_num_frames(compressed_tracks_list, num_compressed_tracks);
		if (num_frames == 0)
			return error_result("All compressed track lists are empty");

		const uint32_t num_movable_frames = calculate_num_movable_frames(compressed_tracks_list, num_compressed_tracks);
		ACL_ASSERT(num_movable_frames < num_frames, "Cannot move out more frames than we have");

		// Calculate how many frames we'll move to every tier
		const uint32_t num_low_importance_frames = std::min<uint32_t>(num_movable_frames, uint32_t(settings.low_importance_tier_proportion * float(num_frames)));
		const uint32_t num_medium_importance_frames = std::min<uint32_t>(num_movable_frames - num_low_importance_frames, uint32_t(settings.medium_importance_tier_proportion * float(num_frames)));
		ACL_ASSERT(num_low_importance_frames + num_medium_importance_frames <= num_movable_frames, "Cannot move out more frames than we have");

		// Non-movable frames end up being high importance and remain in the compressed clip
		const uint32_t num_high_importance_frames = num_frames - num_medium_importance_frames - num_low_importance_frames;

		frame_assignment_context context(allocator, compressed_tracks_list, num_compressed_tracks, num_movable_frames);
		context.set_tier_num_frames(quality_tier::highest_importance, num_high_importance_frames);
		context.set_tier_num_frames(quality_tier::medium_importance, num_medium_importance_frames);
		context.set_tier_num_frames(quality_tier::lowest_importance, num_low_importance_frames);

		// Assign every frame to its tier
		assign_frames_to_tiers(context);

		// Build our new compressed track instances with the high importance tier data
		build_compressed_tracks(context, out_compressed_tracks);

		// Build our database with the lower tier data
		out_database = build_compressed_database(context, settings, out_compressed_tracks);

		return error_result();
	}

	inline error_result split_database_bulk_data(iallocator& allocator, const compressed_database& database, compressed_database*& out_split_database, uint8_t*& out_bulk_data_medium, uint8_t*& out_bulk_data_low)
	{
		using namespace acl_impl;

		const error_result result = database.is_valid(true);
		if (result.any())
			return result;

		if (!database.is_bulk_data_inline())
			return error_result("Bulk data is not inline in source database");

		const uint32_t total_size = database.get_total_size();
		const uint32_t bulk_data_medium_size = database.get_bulk_data_size(quality_tier::medium_importance);
		const uint32_t bulk_data_low_size = database.get_bulk_data_size(quality_tier::lowest_importance);
		const uint32_t db_size = total_size - bulk_data_medium_size - bulk_data_low_size;

		// Allocate and setup our new database
		uint8_t* database_buffer = allocate_type_array_aligned<uint8_t>(allocator, db_size, alignof(compressed_database));
		out_split_database = bit_cast<compressed_database*>(database_buffer);

		std::memcpy(database_buffer, &database, db_size);

		raw_buffer_header* database_buffer_header = safe_ptr_cast<raw_buffer_header>(database_buffer);
		database_buffer += sizeof(raw_buffer_header);

		database_header* db_header = safe_ptr_cast<database_header>(database_buffer);
		database_buffer += sizeof(database_header);

		db_header->bulk_data_offset[0] = invalid_ptr_offset();
		db_header->bulk_data_offset[1] = invalid_ptr_offset();
		db_header->set_is_bulk_data_inline(false);

		database_buffer_header->size = db_size;
		database_buffer_header->hash = hash32(safe_ptr_cast<const uint8_t>(db_header), db_size - sizeof(raw_buffer_header));	// Hash everything but the raw buffer header
		ACL_ASSERT(out_split_database->is_valid(true).empty(), "Failed to split database");

		// Allocate and setup our new bulk data
		uint8_t* bulk_data_medium_buffer = bulk_data_medium_size != 0 ? allocate_type_array_aligned<uint8_t>(allocator, bulk_data_medium_size, k_database_bulk_data_alignment) : nullptr;
		out_bulk_data_medium = bulk_data_medium_buffer;

		std::memcpy(bulk_data_medium_buffer, database.get_bulk_data(quality_tier::medium_importance), bulk_data_medium_size);

		uint8_t* bulk_data_low_buffer = bulk_data_low_size != 0 ? allocate_type_array_aligned<uint8_t>(allocator, bulk_data_low_size, k_database_bulk_data_alignment) : nullptr;
		out_bulk_data_low = bulk_data_low_buffer;

		std::memcpy(bulk_data_low_buffer, database.get_bulk_data(quality_tier::lowest_importance), bulk_data_low_size);

#if defined(ACL_HAS_ASSERT_CHECKS)
		const uint32_t bulk_data_medium_hash = hash32(bulk_data_medium_buffer, bulk_data_medium_size);
		ACL_ASSERT(bulk_data_medium_hash == database.get_bulk_data_hash(quality_tier::medium_importance), "Bulk data hash mismatch");

		const uint32_t bulk_data_low_hash = hash32(bulk_data_low_buffer, bulk_data_low_size);
		ACL_ASSERT(bulk_data_low_hash == database.get_bulk_data_hash(quality_tier::lowest_importance), "Bulk data hash mismatch");
#endif

		return error_result();
	}

	inline error_result strip_database_quality_tier(iallocator& allocator, const compressed_database& database, quality_tier tier, compressed_database*& out_stripped_database)
	{
		using namespace acl_impl;

		const error_result result = database.is_valid(true);
		if (result.any())
			return result;

		if (tier == quality_tier::highest_importance)
			return error_result("The database does not contain data for the high importance tier, it lives inside compressed_tracks");

		if (!database.has_bulk_data(tier))
			return error_result("Cannot strip an empty quality tier");

		const uint32_t tier_index = uint32_t(tier) - 1;
		const bool is_bulk_data_inline = database.is_bulk_data_inline();
		const database_header& ref_header = get_database_header(database);
		const uint32_t num_medium_chunks = tier != quality_tier::medium_importance ? database.get_num_chunks(quality_tier::medium_importance) : 0;
		const uint32_t num_low_chunks = tier != quality_tier::lowest_importance ? database.get_num_chunks(quality_tier::lowest_importance) : 0;
		const uint32_t num_tracks = ref_header.num_clips;

		// Bulk data sizes are already padded for alignment
		const uint32_t bulk_data_medium_size = tier != quality_tier::medium_importance ? database.get_bulk_data_size(quality_tier::medium_importance) : 0;
		const uint32_t bulk_data_low_size = tier != quality_tier::lowest_importance ? database.get_bulk_data_size(quality_tier::lowest_importance) : 0;

		uint32_t database_buffer_size = 0;
		database_buffer_size += sizeof(raw_buffer_header);										// Header
		database_buffer_size += sizeof(database_header);										// Header

		database_buffer_size = align_to(database_buffer_size, 4);								// Align chunk descriptions
		database_buffer_size += num_medium_chunks * sizeof(database_chunk_description);			// Chunk descriptions

		database_buffer_size = align_to(database_buffer_size, 4);								// Align chunk descriptions
		database_buffer_size += num_low_chunks * sizeof(database_chunk_description);			// Chunk descriptions

		database_buffer_size = align_to(database_buffer_size, 4);								// Align clip hashes
		database_buffer_size += num_tracks * sizeof(database_clip_metadata);					// Clip metadata

		database_buffer_size = align_to(database_buffer_size, k_database_bulk_data_alignment);	// Align bulk data
		database_buffer_size += bulk_data_medium_size;											// Bulk data
		database_buffer_size += bulk_data_low_size;												// Bulk data

		// Allocate and setup our new database
		uint8_t* database_buffer = allocate_type_array_aligned<uint8_t>(allocator, database_buffer_size, alignof(compressed_database));
		std::memset(database_buffer, 0, database_buffer_size);
		out_stripped_database = bit_cast<compressed_database*>(database_buffer);

		raw_buffer_header* database_buffer_header = safe_ptr_cast<raw_buffer_header>(database_buffer);
		database_buffer += sizeof(raw_buffer_header);

		const uint8_t* db_header_start = database_buffer;
		database_header* db_header = safe_ptr_cast<database_header>(database_buffer);
		database_buffer += sizeof(database_header);

		// Copy our header
		std::memcpy(db_header, &get_database_header(database), sizeof(database_header));

		database_buffer = align_to(database_buffer, 4);										// Align chunk descriptions
		database_buffer += num_medium_chunks * sizeof(database_chunk_description);			// Chunk descriptions

		database_buffer = align_to(database_buffer, 4);										// Align chunk descriptions
		database_buffer += num_low_chunks * sizeof(database_chunk_description);				// Chunk descriptions

		database_buffer = align_to(database_buffer, 4);										// Align clip hashes
		db_header->clip_metadata_offset = uint32_t(database_buffer - db_header_start);		// Clip metadata
		database_buffer += num_tracks * sizeof(database_clip_metadata);						// Clip metadata

		database_buffer = align_to(database_buffer, k_database_bulk_data_alignment);		// Align bulk data
		if (bulk_data_medium_size != 0)
			db_header->bulk_data_offset[0] = uint32_t(database_buffer - db_header_start);	// Bulk data
		else
			db_header->bulk_data_offset[0] = invalid_ptr_offset();
		database_buffer += bulk_data_medium_size;											// Bulk data

		if (bulk_data_low_size != 0)
			db_header->bulk_data_offset[1] = uint32_t(database_buffer - db_header_start);	// Bulk data
		else
			db_header->bulk_data_offset[1] = invalid_ptr_offset();
		database_buffer += bulk_data_low_size;												// Bulk data

		// Zero out our stripped tier
		db_header->num_chunks[tier_index] = 0;
		db_header->bulk_data_size[tier_index] = 0;
		db_header->bulk_data_offset[tier_index] = invalid_ptr_offset();
		db_header->bulk_data_hash[tier_index] = hash32(nullptr, 0);

		// Copy our chunk descriptions
		if (tier != quality_tier::medium_importance)
			std::memcpy(db_header->get_chunk_descriptions_medium(), ref_header.get_chunk_descriptions_medium(), num_medium_chunks * sizeof(database_chunk_description));
		else if (tier != quality_tier::lowest_importance)
			std::memcpy(db_header->get_chunk_descriptions_low(), ref_header.get_chunk_descriptions_low(), num_low_chunks * sizeof(database_chunk_description));

		// Copy our clip metadata
		std::memcpy(db_header->get_clip_metadatas(), ref_header.get_clip_metadatas(), num_tracks * sizeof(database_clip_metadata));

		// Copy our bulk data
		if (is_bulk_data_inline)
		{
			// Copy the remaining bulk data
			if (tier != quality_tier::medium_importance)
			{
				const uint8_t* src_bulk_data = database.get_bulk_data(quality_tier::medium_importance);
				uint8_t* dst_bulk_data = db_header->get_bulk_data_medium();
				std::memcpy(dst_bulk_data, src_bulk_data, bulk_data_medium_size);
			}
			else if (tier != quality_tier::lowest_importance)
			{
				const uint8_t* src_bulk_data = database.get_bulk_data(quality_tier::lowest_importance);
				uint8_t* dst_bulk_data = db_header->get_bulk_data_low();
				std::memcpy(dst_bulk_data, src_bulk_data, bulk_data_low_size);
			}
		}

		database_buffer_header->size = database_buffer_size;
		database_buffer_header->hash = hash32(safe_ptr_cast<const uint8_t>(db_header), database_buffer_size - sizeof(raw_buffer_header));	// Hash everything but the raw buffer header
		ACL_ASSERT(out_stripped_database->is_valid(true).empty(), "Failed to strip database");

#if defined(ACL_HAS_ASSERT_CHECKS)
		if (is_bulk_data_inline)
		{
			if (tier != quality_tier::medium_importance)
			{
				const uint32_t bulk_data_size = out_stripped_database->get_bulk_data_size(quality_tier::medium_importance);
				const uint8_t* bulk_data = out_stripped_database->get_bulk_data(quality_tier::medium_importance);
				const uint32_t bulk_data_hash = hash32(bulk_data, bulk_data_size);
				ACL_ASSERT(bulk_data_hash == database.get_bulk_data_hash(quality_tier::medium_importance), "Bulk data hash mismatch");
			}
			else if (tier != quality_tier::lowest_importance)
			{
				const uint32_t bulk_data_size = out_stripped_database->get_bulk_data_size(quality_tier::lowest_importance);
				const uint8_t* bulk_data = out_stripped_database->get_bulk_data(quality_tier::lowest_importance);
				const uint32_t bulk_data_hash = hash32(bulk_data, bulk_data_size);
				ACL_ASSERT(bulk_data_hash == database.get_bulk_data_hash(quality_tier::lowest_importance), "Bulk data hash mismatch");
			}
		}
#endif

		return error_result();
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
