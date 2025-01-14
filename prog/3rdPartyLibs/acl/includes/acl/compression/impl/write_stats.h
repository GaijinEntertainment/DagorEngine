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

#if defined(ACL_USE_SJSON)

#include "acl/version.h"
#include "acl/core/time_utils.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/transform_error_metrics.h"
#include "acl/compression/track_error.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/compression_stats.h"

#include <sjson/writer.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline void write_summary_segment_stats(const segment_context& segment, rotation_format8 rotation_format, vector_format8 translation_format, vector_format8 scale_format, sjson::ObjectWriter& writer)
		{
			writer["segment_index"] = segment.segment_index;
			writer["num_samples"] = segment.num_samples;

			const uint32_t format_per_track_data_size = get_format_per_track_data_size(*segment.clip, rotation_format, translation_format, scale_format);

			uint32_t segment_size = 0;
			segment_size += format_per_track_data_size;						// Format per track data
			segment_size = align_to(segment_size, 2);						// Align range data
			segment_size += segment.range_data_size;						// Range data
			segment_size = align_to(segment_size, 4);						// Align animated data
			segment_size += segment.animated_data_size;						// Animated track data

			writer["segment_size"] = segment_size;
			writer["animated_frame_size"] = double(segment.animated_pose_bit_size) / 8.0;

			if (segment.clip->has_stripped_keyframes)
			{
				const bitset_description hard_keyframes_desc = bitset_description::make_from_num_bits<32>();
				const uint32_t num_retained_keyframes = bitset_count_set_bits(&segment.hard_keyframes, hard_keyframes_desc);
				const uint32_t num_stripped_keyframes = segment.num_samples - num_retained_keyframes;
				writer["num_stripped_keyframes"] = num_stripped_keyframes;
			}

			if (segment.contributing_error != nullptr)
			{
				writer["num_trivial_keyframes"] = (uint32_t)std::count_if(segment.contributing_error, segment.contributing_error + segment.num_samples,
					[](const keyframe_stripping_metadata_t& keyframe_metadata)
					{
						return keyframe_metadata.is_keyframe_trivial;
					});
			}
		}

		inline void write_detailed_segment_stats(const segment_context& segment, sjson::ObjectWriter& writer)
		{
			uint32_t bit_rate_counts[k_num_bit_rates] = { 0 };

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				const uint8_t rotation_bit_rate = bone_stream.rotations.get_bit_rate();
				if (rotation_bit_rate != k_invalid_bit_rate)
					bit_rate_counts[rotation_bit_rate]++;

				const uint8_t translation_bit_rate = bone_stream.translations.get_bit_rate();
				if (translation_bit_rate != k_invalid_bit_rate)
					bit_rate_counts[translation_bit_rate]++;

				const uint8_t scale_bit_rate = bone_stream.scales.get_bit_rate();
				if (scale_bit_rate != k_invalid_bit_rate)
					bit_rate_counts[scale_bit_rate]++;
			}

			writer["bit_rate_counts"] = [&](sjson::ArrayWriter& bitrate_writer)
			{
				for (uint32_t bit_rate = 0; bit_rate < k_num_bit_rates; ++bit_rate)
					bitrate_writer.push(bit_rate_counts[bit_rate]);
			};

			// We assume that we always interpolate between 2 poses
			const uint32_t animated_pose_byte_size = align_to(segment.animated_pose_bit_size * 2, 8) / 8;
			constexpr uint32_t k_cache_line_byte_size = 64;

			const uint32_t num_segment_header_cache_lines = align_to(segment.total_header_size, k_cache_line_byte_size) / k_cache_line_byte_size;
			const uint32_t num_animated_pose_cache_lines = align_to(animated_pose_byte_size, k_cache_line_byte_size) / k_cache_line_byte_size;
			writer["decomp_touched_bytes"] = segment.clip->decomp_touched_bytes + segment.total_header_size + animated_pose_byte_size;
			writer["decomp_touched_cache_lines"] = segment.clip->decomp_touched_cache_lines + num_segment_header_cache_lines + num_animated_pose_cache_lines;
		}

		inline void write_exhaustive_segment_stats(iallocator& allocator, const segment_context& segment, const clip_context& raw_clip_context, const clip_context& additive_base_clip_context, const compression_settings& settings, const track_array_qvvf& track_list, sjson::ObjectWriter& writer)
		{
			const uint32_t num_bones = raw_clip_context.num_bones;
			const bool has_scale = segment_context_has_scale(segment);

			ACL_ASSERT(!settings.error_metric->needs_conversion(has_scale), "Error metric conversion not supported");

			const auto local_to_object_space_impl = std::mem_fn(has_scale ? &itransform_error_metric::local_to_object_space : &itransform_error_metric::local_to_object_space_no_scale);
			const auto calculate_error_impl = std::mem_fn(has_scale ? &itransform_error_metric::calculate_error : &itransform_error_metric::calculate_error_no_scale);
			const auto apply_additive_to_base_impl = std::mem_fn(has_scale ? &itransform_error_metric::apply_additive_to_base : &itransform_error_metric::apply_additive_to_base_no_scale);

			rtm::qvvf* raw_local_pose = allocate_type_array<rtm::qvvf>(allocator, num_bones);
			rtm::qvvf* base_local_pose = allocate_type_array<rtm::qvvf>(allocator, num_bones);
			rtm::qvvf* lossy_local_pose = allocate_type_array<rtm::qvvf>(allocator, num_bones);

			rtm::qvvf* raw_object_pose = allocate_type_array<rtm::qvvf>(allocator, num_bones);
			rtm::qvvf* lossy_object_pose = allocate_type_array<rtm::qvvf>(allocator, num_bones);

			uint32_t* parent_transform_indices = allocate_type_array<uint32_t>(allocator, num_bones);
			uint32_t* self_transform_indices = allocate_type_array<uint32_t>(allocator, num_bones);

			for (uint32_t transform_index = 0; transform_index < num_bones; ++transform_index)
			{
				const track_qvvf& track = track_list[transform_index];
				const track_desc_transformf& desc = track.get_description();

				parent_transform_indices[transform_index] = desc.parent_index;
				self_transform_indices[transform_index] = transform_index;
			}

			const float sample_rate = raw_clip_context.sample_rate;
			const float ref_duration = calculate_finite_duration(raw_clip_context.num_samples, sample_rate);

			itransform_error_metric::apply_additive_to_base_args apply_additive_to_base_args_raw;
			apply_additive_to_base_args_raw.dirty_transform_indices = self_transform_indices;
			apply_additive_to_base_args_raw.num_dirty_transforms = num_bones;
			apply_additive_to_base_args_raw.local_transforms = raw_local_pose;
			apply_additive_to_base_args_raw.base_transforms = base_local_pose;
			apply_additive_to_base_args_raw.num_transforms = num_bones;

			itransform_error_metric::apply_additive_to_base_args apply_additive_to_base_args_lossy = apply_additive_to_base_args_raw;
			apply_additive_to_base_args_lossy.local_transforms = lossy_local_pose;

			itransform_error_metric::local_to_object_space_args local_to_object_space_args_raw;
			local_to_object_space_args_raw.dirty_transform_indices = self_transform_indices;
			local_to_object_space_args_raw.num_dirty_transforms = num_bones;
			local_to_object_space_args_raw.parent_transform_indices = parent_transform_indices;
			local_to_object_space_args_raw.local_transforms = raw_local_pose;
			local_to_object_space_args_raw.num_transforms = num_bones;

			itransform_error_metric::local_to_object_space_args local_to_object_space_args_lossy = local_to_object_space_args_raw;
			local_to_object_space_args_lossy.local_transforms = lossy_local_pose;

			track_error worst_bone_error;

			writer["error_per_frame_and_bone"] = [&](sjson::ArrayWriter& frames_writer)
			{
				for (uint32_t sample_index = 0; sample_index < segment.num_samples; ++sample_index)
				{
					const float sample_time = rtm::scalar_min(float(segment.clip_sample_offset + sample_index) / sample_rate, ref_duration);

					sample_streams(raw_clip_context.segments[0].bone_streams, num_bones, sample_time, raw_local_pose);
					sample_streams(segment.bone_streams, num_bones, sample_time, lossy_local_pose);

					if (raw_clip_context.has_additive_base)
					{
						const float normalized_sample_time = additive_base_clip_context.num_samples > 1 ? (sample_time / ref_duration) : 0.0F;
						const float additive_sample_time = additive_base_clip_context.num_samples > 1 ? (normalized_sample_time * additive_base_clip_context.duration) : 0.0F;
						sample_streams(additive_base_clip_context.segments[0].bone_streams, num_bones, additive_sample_time, base_local_pose);

						apply_additive_to_base_impl(settings.error_metric, apply_additive_to_base_args_raw, raw_local_pose);
						apply_additive_to_base_impl(settings.error_metric, apply_additive_to_base_args_lossy, lossy_local_pose);
					}

					local_to_object_space_impl(settings.error_metric, local_to_object_space_args_raw, raw_object_pose);
					local_to_object_space_impl(settings.error_metric, local_to_object_space_args_lossy, lossy_object_pose);

					frames_writer.push_newline();
					frames_writer.push([&](sjson::ArrayWriter& frame_writer)
						{
							for (uint32_t bone_index = 0; bone_index < num_bones; ++bone_index)
							{
								const track_qvvf& track = track_list[bone_index];
								const track_desc_transformf& desc = track.get_description();

								itransform_error_metric::calculate_error_args calculate_error_args;
								calculate_error_args.transform0 = raw_object_pose + bone_index;
								calculate_error_args.transform1 = lossy_object_pose + bone_index;
								calculate_error_args.construct_sphere_shell(desc.shell_distance);

								const float error = rtm::scalar_cast(calculate_error_impl(settings.error_metric, calculate_error_args));

								frame_writer.push(error);

								if (error > worst_bone_error.error)
								{
									worst_bone_error.error = error;
									worst_bone_error.index = bone_index;
									worst_bone_error.sample_time = sample_time;
								}
							}
						});
				}
			};

			writer["max_error"] = worst_bone_error.error;
			writer["worst_bone"] = worst_bone_error.index;
			writer["worst_time"] = worst_bone_error.sample_time;

			deallocate_type_array(allocator, raw_local_pose, num_bones);
			deallocate_type_array(allocator, base_local_pose, num_bones);
			deallocate_type_array(allocator, lossy_local_pose, num_bones);

			deallocate_type_array(allocator, raw_object_pose, num_bones);
			deallocate_type_array(allocator, lossy_object_pose, num_bones);

			deallocate_type_array(allocator, parent_transform_indices, num_bones);
			deallocate_type_array(allocator, self_transform_indices, num_bones);
		}

		inline uint32_t calculate_clip_metadata_common_size(const clip_context& clip, const compressed_tracks& compressed_clip)
		{
			const uint32_t segment_header_size = compressed_clip.has_database() || compressed_clip.has_stripped_keyframes() ? sizeof(stripped_segment_header_t) : sizeof(segment_header);

			uint32_t result = 0;

			// Segment start indices and headers
			result += clip.num_segments > 1 ? (uint32_t(sizeof(uint32_t)) * (clip.num_segments + 1)) : 0;
			result += segment_header_size * clip.num_segments;

			// Default/constant track bit sets
			const bitset_description bitset_desc = bitset_description::make_from_num_bits(compressed_clip.get_num_tracks());
			result += bitset_desc.get_num_bytes();
			result += bitset_desc.get_num_bytes();

			return result;
		}

		inline uint32_t calculate_segment_metadata_common_size(const clip_context& clip, const compression_settings& settings)
		{
			const bool is_rotation_variable = is_rotation_format_variable(settings.rotation_format);
			const bool is_translation_variable = is_vector_format_variable(settings.translation_format);
			const bool is_scale_variable = is_vector_format_variable(settings.scale_format);

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t result = 0;

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (bone_stream.is_stripped_from_output())
					continue;

				// Format per track
				if (!bone_stream.is_rotation_constant && is_rotation_variable)
					result++;

				if (!bone_stream.is_translation_constant && is_translation_variable)
					result++;

				if (!bone_stream.is_scale_constant && is_scale_variable)
					result++;
			}

			return result * clip.num_segments;
		}

		inline uint32_t calculate_segment_metadata_rotation_size(const clip_context& clip, range_reduction_flags8 range_reduction)
		{
			if (clip.num_segments == 1)
				return 0;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t result = 0;

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (bone_stream.is_stripped_from_output())
					continue;

				// Range data
				if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations) && !bone_stream.is_rotation_constant)
				{
					const uint32_t num_components = bone_stream.rotations.get_rotation_format() == rotation_format8::quatf_full ? 8U : 6U;
					result += num_components * k_segment_range_reduction_num_bytes_per_component;
				}
			}

			return result * clip.num_segments;
		}

		inline uint32_t calculate_segment_metadata_translation_size(const clip_context& clip, range_reduction_flags8 range_reduction)
		{
			if (clip.num_segments == 1)
				return 0;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t result = 0;

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (bone_stream.is_stripped_from_output())
					continue;

				// Range data
				if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations) && !bone_stream.is_translation_constant)
					result += k_segment_range_reduction_num_bytes_per_component * 6;
			}

			return result * clip.num_segments;
		}

		inline uint32_t calculate_segment_metadata_scale_size(const clip_context& clip, range_reduction_flags8 range_reduction)
		{
			if (clip.num_segments == 1)
				return 0;

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t result = 0;

			for (const transform_streams& bone_stream : segment.const_bone_iterator())
			{
				if (bone_stream.is_stripped_from_output())
					continue;

				// Range data
				if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales) && !bone_stream.is_scale_constant)
					result += k_segment_range_reduction_num_bytes_per_component * 6;
			}

			return result * clip.num_segments;
		}

		inline uint32_t calculate_segment_animated_rotation_size(const clip_context& clip)
		{
			uint32_t result = 0;

			for (const segment_context& segment : clip.segment_iterator())
				result += align_to(segment.animated_rotation_bit_size * segment.num_samples, 8) / 8;	// Convert bits to bytes;

			return result;
		}

		inline uint32_t calculate_segment_animated_translation_size(const clip_context& clip)
		{
			uint32_t result = 0;

			for (const segment_context& segment : clip.segment_iterator())
				result += align_to(segment.animated_translation_bit_size * segment.num_samples, 8) / 8;	// Convert bits to bytes;

			return result;
		}

		inline uint32_t calculate_segment_animated_scale_size(const clip_context& clip)
		{
			uint32_t result = 0;

			for (const segment_context& segment : clip.segment_iterator())
				result += align_to(segment.animated_scale_bit_size * segment.num_samples, 8) / 8;	// Convert bits to bytes;

			return result;
		}

		inline uint32_t calculate_segment_animated_data_size(const clip_context& clip)
		{
			uint32_t result = 0;

			for (const segment_context& segment : clip.segment_iterator())
				result += segment.animated_data_size;

			return result;
		}

		inline void write_stats(iallocator& allocator, const track_array_qvvf& track_list, const clip_context& clip,
			const compressed_tracks& compressed_clip, const compression_settings& settings, const compression_segmenting_settings& segmenting_settings,
			range_reduction_flags8 range_reduction, const clip_context& raw_clip,
			const clip_context& additive_base_clip_context,
			const compression_stats_t& compression_stats,
			output_stats& stats)
		{
			ACL_ASSERT(stats.writer != nullptr, "Attempted to log stats without a writer");
			if (stats.writer == nullptr)
				return;

			const uint32_t raw_size = track_list.get_raw_size();
			const uint32_t compressed_size = compressed_clip.get_size();
			const double compression_ratio = double(raw_size) / double(compressed_size);

			const bitset_description hard_keyframes_desc = bitset_description::make_from_num_bits<32>();
			uint32_t total_num_stripped_keyframes = 0;
			if (clip.has_stripped_keyframes)
			{
				for (const segment_context& segment : clip.segment_iterator())
				{
					const uint32_t num_retained_keyframes = bitset_count_set_bits(&segment.hard_keyframes, hard_keyframes_desc);
					const uint32_t num_stripped_keyframes = segment.num_samples - num_retained_keyframes;
					total_num_stripped_keyframes += num_stripped_keyframes;
				}
			}

			uint32_t num_trivial_keyframes = 0;
			for (const segment_context& segment : clip.segment_iterator())
			{
				if (segment.contributing_error != nullptr)
				{
					num_trivial_keyframes += static_cast<uint32_t>(std::count_if(segment.contributing_error, segment.contributing_error + segment.num_samples,
						[](const keyframe_stripping_metadata_t& keyframe_metadata)
						{
							return keyframe_metadata.is_keyframe_trivial;
						}));
				}
			}

			uint32_t longest_chain_length = 0;
			const bitset_description transform_bitset_desc = bitset_description::make_from_num_bits(clip.num_bones);
			for (uint32_t leaf_index = 0; leaf_index < clip.num_leaf_transforms; ++leaf_index)
			{
				const uint32_t* transform_chain = clip.leaf_transform_chains + (leaf_index * transform_bitset_desc.get_size());
				const uint32_t num_chain_transforms = bitset_count_set_bits(transform_chain, transform_bitset_desc);
				longest_chain_length = std::max<uint32_t>(longest_chain_length, num_chain_transforms);
			}

			sjson::ObjectWriter& writer = *stats.writer;
			writer["algorithm_name"] = get_algorithm_name(algorithm_type8::uniformly_sampled);
			writer["algorithm_uid"] = settings.get_hash();
			writer["clip_name"] = track_list.get_name().c_str();
			writer["raw_size"] = raw_size;
			writer["compressed_size"] = compressed_size;
			writer["compression_ratio"] = compression_ratio;
			writer["compression_time"] = compression_stats.total_elapsed_seconds;
			writer["duration"] = compressed_clip.get_duration();
			writer["num_samples"] = compressed_clip.get_num_samples_per_track();
			writer["num_bones"] = compressed_clip.get_num_tracks();
			writer["rotation_format"] = get_rotation_format_name(settings.rotation_format);
			writer["translation_format"] = get_vector_format_name(settings.translation_format);
			writer["scale_format"] = get_vector_format_name(settings.scale_format);
			writer["has_scale"] = clip.has_scale;
			writer["looping"] = compressed_clip.get_looping_policy() == sample_looping_policy::wrap;
			writer["num_stripped_keyframes"] = total_num_stripped_keyframes;
			writer["num_trivial_keyframes"] = num_trivial_keyframes;
			writer["longest_chain_length"] = longest_chain_length;
			writer["error_metric"] = settings.error_metric->get_name();

			if (are_all_enum_flags_set(stats.logging, stat_logging::detailed) || are_all_enum_flags_set(stats.logging, stat_logging::exhaustive))
			{
				uint32_t num_default_rotation_tracks = 0;
				uint32_t num_default_translation_tracks = 0;
				uint32_t num_default_scale_tracks = 0;
				uint32_t num_constant_rotation_tracks = 0;
				uint32_t num_constant_translation_tracks = 0;
				uint32_t num_constant_scale_tracks = 0;
				uint32_t num_animated_rotation_tracks = 0;
				uint32_t num_animated_translation_tracks = 0;
				uint32_t num_animated_scale_tracks = 0;

				for (const transform_streams& bone_stream : clip.segments[0].bone_iterator())
				{
					if (bone_stream.is_stripped_from_output())
						continue;

					if (bone_stream.is_rotation_default)
						num_default_rotation_tracks++;
					else if (bone_stream.is_rotation_constant)
						num_constant_rotation_tracks++;
					else
						num_animated_rotation_tracks++;

					if (bone_stream.is_translation_default)
						num_default_translation_tracks++;
					else if (bone_stream.is_translation_constant)
						num_constant_translation_tracks++;
					else
						num_animated_translation_tracks++;

					if (bone_stream.is_scale_default)
						num_default_scale_tracks++;
					else if (bone_stream.is_scale_constant)
						num_constant_scale_tracks++;
					else
						num_animated_scale_tracks++;
				}

				const uint32_t num_default_tracks = num_default_rotation_tracks + num_default_translation_tracks + num_default_scale_tracks;
				const uint32_t num_constant_tracks = num_constant_rotation_tracks + num_constant_translation_tracks + num_constant_scale_tracks;
				const uint32_t num_animated_tracks = num_animated_rotation_tracks + num_animated_translation_tracks + num_animated_scale_tracks;

				writer["num_default_rotation_tracks"] = num_default_rotation_tracks;
				writer["num_default_translation_tracks"] = num_default_translation_tracks;
				writer["num_default_scale_tracks"] = num_default_scale_tracks;

				writer["num_constant_rotation_tracks"] = num_constant_rotation_tracks;
				writer["num_constant_translation_tracks"] = num_constant_translation_tracks;
				writer["num_constant_scale_tracks"] = num_constant_scale_tracks;

				writer["num_animated_rotation_tracks"] = num_animated_rotation_tracks;
				writer["num_animated_translation_tracks"] = num_animated_translation_tracks;
				writer["num_animated_scale_tracks"] = num_animated_scale_tracks;

				writer["num_default_tracks"] = num_default_tracks;
				writer["num_constant_tracks"] = num_constant_tracks;
				writer["num_animated_tracks"] = num_animated_tracks;

				const uint32_t clip_header_size = sizeof(raw_buffer_header) + sizeof(tracks_header) + sizeof(transform_tracks_header);
				const uint32_t clip_metadata_common_size = calculate_clip_metadata_common_size(clip, compressed_clip);
				const uint32_t clip_metadata_rotation_constant_size = get_packed_rotation_size(get_highest_variant_precision(get_rotation_variant(settings.rotation_format))) * num_constant_rotation_tracks;
				const uint32_t clip_metadata_translation_constant_size = get_packed_vector_size(vector_format8::vector3f_full) * num_constant_translation_tracks;
				const uint32_t clip_metadata_scale_constant_size = get_packed_vector_size(vector_format8::vector3f_full) * num_constant_scale_tracks;
				writer["clip_header_size"] = clip_header_size;
				writer["clip_metadata_common_size"] = clip_metadata_common_size;
				writer["clip_metadata_rotation_constant_size"] = clip_metadata_rotation_constant_size;
				writer["clip_metadata_translation_constant_size"] = clip_metadata_translation_constant_size;
				writer["clip_metadata_scale_constant_size"] = clip_metadata_scale_constant_size;

				const uint32_t range_rotation_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations) ? get_range_reduction_rotation_size(settings.rotation_format) : 0;
				const uint32_t range_translation_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations) ? k_clip_range_reduction_vector3_range_size : 0;
				const uint32_t range_scale_size = are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales) ? k_clip_range_reduction_vector3_range_size : 0;
				const uint32_t clip_metadata_rotation_animated_size = range_rotation_size * num_animated_rotation_tracks;
				const uint32_t clip_metadata_translation_animated_size = range_translation_size * num_animated_translation_tracks;
				const uint32_t clip_metadata_scale_animated_size = range_scale_size * num_animated_scale_tracks;
				writer["clip_metadata_rotation_animated_size"] = clip_metadata_rotation_animated_size;
				writer["clip_metadata_translation_animated_size"] = clip_metadata_translation_animated_size;
				writer["clip_metadata_scale_animated_size"] = clip_metadata_scale_animated_size;

				const uint32_t segment_metadata_common_size = calculate_segment_metadata_common_size(clip, settings);
				const uint32_t segment_metadata_rotation_size = calculate_segment_metadata_rotation_size(clip, range_reduction);
				const uint32_t segment_metadata_translation_size = calculate_segment_metadata_translation_size(clip, range_reduction);
				const uint32_t segment_metadata_scale_size = calculate_segment_metadata_scale_size(clip, range_reduction);
				const uint32_t segment_animated_rotation_size = calculate_segment_animated_rotation_size(clip);
				const uint32_t segment_animated_translation_size = calculate_segment_animated_translation_size(clip);
				const uint32_t segment_animated_scale_size = calculate_segment_animated_scale_size(clip);
				writer["segment_metadata_common_size"] = segment_metadata_common_size;
				writer["segment_metadata_rotation_size"] = segment_metadata_rotation_size;
				writer["segment_metadata_translation_size"] = segment_metadata_translation_size;
				writer["segment_metadata_scale_size"] = segment_metadata_scale_size;
				writer["segment_animated_rotation_size"] = segment_animated_rotation_size;
				writer["segment_animated_translation_size"] = segment_animated_translation_size;
				writer["segment_animated_scale_size"] = segment_animated_scale_size;

				uint32_t known_data_size = 0;
				known_data_size += clip_header_size;
				known_data_size += clip_metadata_common_size;
				known_data_size += clip_metadata_rotation_constant_size;
				known_data_size += clip_metadata_translation_constant_size;
				known_data_size += clip_metadata_scale_constant_size;
				known_data_size += clip_metadata_rotation_animated_size;
				known_data_size += clip_metadata_translation_animated_size;
				known_data_size += clip_metadata_scale_animated_size;
				known_data_size += segment_metadata_common_size;
				known_data_size += segment_metadata_rotation_size;
				known_data_size += segment_metadata_translation_size;
				known_data_size += segment_metadata_scale_size;
				const uint32_t segment_animated_data_size = calculate_segment_animated_data_size(clip);
				known_data_size += segment_animated_data_size;

				const int32_t unknown_overhead_size = static_cast<int32_t>(compressed_size) - static_cast<int32_t>(known_data_size);
				ACL_ASSERT(unknown_overhead_size >= 0, "Overhead size should be positive");
				writer["unknown_overhead_size"] = unknown_overhead_size;
			}

			writer["segmenting"] = [&](sjson::ObjectWriter& segmenting_writer)
			{
				segmenting_writer["num_segments"] = clip.num_segments;
				segmenting_writer["ideal_num_samples"] = segmenting_settings.ideal_num_samples;
				segmenting_writer["max_num_samples"] = segmenting_settings.max_num_samples;
			};

			writer["segments"] = [&](sjson::ArrayWriter& segments_writer)
			{
				for (const segment_context& segment : clip.segment_iterator())
				{
					segments_writer.push([&](sjson::ObjectWriter& segment_writer)
						{
							write_summary_segment_stats(segment, settings.rotation_format, settings.translation_format, settings.scale_format, segment_writer);

							if (are_all_enum_flags_set(stats.logging, stat_logging::detailed))
								write_detailed_segment_stats(segment, segment_writer);

							if (are_all_enum_flags_set(stats.logging, stat_logging::exhaustive))
								write_exhaustive_segment_stats(allocator, segment, raw_clip, additive_base_clip_context, settings, track_list, segment_writer);
						});
				}
			};

			if (are_all_enum_flags_set(stats.logging, stat_logging::detailed))
			{
				writer["timings"] = [&](sjson::ObjectWriter& timings_writer)
				{
					timings_writer["total"] = compression_stats.total_elapsed_seconds;

					timings_writer["initialization"] = compression_stats.initialization_elapsed_seconds;
					timings_writer["optimize_looping"] = compression_stats.optimize_looping_elapsed_seconds;
					timings_writer["convert_rotations"] = compression_stats.convert_rotations_elapsed_seconds;
					timings_writer["extract_clip_ranges"] = compression_stats.extract_clip_ranges_elapsed_seconds;
					timings_writer["compact_constant_sub_tracks"] = compression_stats.compact_constant_sub_tracks_elapsed_seconds;
					timings_writer["normalize_over_clip"] = compression_stats.normalize_clip_elapsed_seconds;
					timings_writer["segmenting"] = compression_stats.segmenting_elapsed_seconds;
					timings_writer["extract_segment_ranges"] = compression_stats.extract_segment_ranges_elapsed_seconds;
					timings_writer["normalize_over_segments"] = compression_stats.normalize_segment_elapsed_seconds;
					timings_writer["bit_rate_optimization"] = compression_stats.bit_rate_optimization_elapsed_seconds;
					timings_writer["keyframe_stripping"] = compression_stats.keyframe_stripping_elapsed_seconds;
					timings_writer["output_packing"] = compression_stats.output_packing_elapsed_seconds;
				};
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP

#endif	// #if defined(ACL_USE_SJSON)
