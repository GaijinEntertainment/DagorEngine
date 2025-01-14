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
#include "acl/core/scope_profiler.h"
#include "acl/core/track_formats.h"
#include "acl/core/track_types.h"
#include "acl/core/range_reduction_types.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/compression_stats.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline track_stream_range calculate_track_range(const track_stream& stream, bool is_vector4)
		{
			const uint32_t num_samples = stream.get_num_samples();
			if (num_samples == 0)
				return track_stream_range::from_min_max(rtm::vector_zero(), rtm::vector_zero());

			rtm::vector4f min = rtm::vector_set(1e10F);
			rtm::vector4f max = rtm::vector_set(-1e10F);

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const rtm::vector4f sample = stream.get_raw_sample<rtm::vector4f>(sample_index);

				min = rtm::vector_min(min, sample);
				max = rtm::vector_max(max, sample);
			}

			// Set the 4th component to zero if we don't need it
			if (!is_vector4)
			{
				min = rtm::vector_set_w(min, 0.0F);
				max = rtm::vector_set_w(max, 0.0F);
			}

			return track_stream_range::from_min_max(min, max);
		}

		inline void extract_bone_ranges_impl(const segment_context& segment, transform_range* bone_ranges)
		{
			const bool has_scale = segment_context_has_scale(segment);

			for (uint32_t bone_index = 0; bone_index < segment.num_bones; ++bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				transform_range& bone_range = bone_ranges[bone_index];

				bone_range.rotation = calculate_track_range(bone_stream.rotations, true);
				bone_range.translation = calculate_track_range(bone_stream.translations, false);

				if (has_scale)
					bone_range.scale = calculate_track_range(bone_stream.scales, false);
				else
					bone_range.scale = track_stream_range();
			}
		}

		inline void extract_clip_bone_ranges(iallocator& allocator, clip_context& context, compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler extract_clip_ranges_time;
#endif

			context.ranges = allocate_type_array<transform_range>(allocator, context.num_bones);

			ACL_ASSERT(context.num_segments == 1, "context must contain a single segment!");
			const segment_context& segment = context.segments[0];

			acl_impl::extract_bone_ranges_impl(segment, context.ranges);

#if defined(ACL_USE_SJSON)
			compression_stats.extract_clip_ranges_elapsed_seconds = extract_clip_ranges_time.get_elapsed_seconds();
#endif
		}

		inline void extract_segment_bone_ranges(iallocator& allocator, clip_context& context, compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler extract_segment_ranges_time;
#endif

			const rtm::vector4f one = rtm::vector_set(1.0F);
			const rtm::vector4f zero = rtm::vector_zero();
			const float max_range_value_flt = float((1 << k_segment_range_reduction_num_bits_per_component) - 1);
			const rtm::vector4f max_range_value = rtm::vector_set(max_range_value_flt);
			const rtm::vector4f inv_max_range_value = rtm::vector_set(1.0F / max_range_value_flt);

			// Segment ranges are always normalized and live between [0.0 ... 1.0]

			auto fixup_range = [&](const track_stream_range& range)
			{
				// In our compressed format, we store the minimum value of the track range quantized on 8 bits.
				// To get the best accuracy, we pick the value closest to the true minimum that is slightly lower.
				// This is to ensure that we encompass the lowest value even after quantization.
				const rtm::vector4f range_min = range.get_min();
				const rtm::vector4f scaled_min = rtm::vector_mul(range_min, max_range_value);
				const rtm::vector4f quantized_min0 = rtm::vector_clamp(rtm::vector_floor(scaled_min), zero, max_range_value);
				const rtm::vector4f quantized_min1 = rtm::vector_max(rtm::vector_sub(quantized_min0, one), zero);

				const rtm::vector4f padded_range_min0 = rtm::vector_mul(quantized_min0, inv_max_range_value);
				const rtm::vector4f padded_range_min1 = rtm::vector_mul(quantized_min1, inv_max_range_value);

				// Check if min0 is below or equal to our original range minimum value, if it is, it is good
				// enough to use otherwise min1 is guaranteed to be lower.
				const rtm::mask4f is_min0_lower_mask = rtm::vector_less_equal(padded_range_min0, range_min);
				const rtm::vector4f padded_range_min = rtm::vector_select(is_min0_lower_mask, padded_range_min0, padded_range_min1);

				// The story is different for the extent. We do not store the max, instead we use the extent
				// for performance reasons: a single mul/add is required to reconstruct the original value.
				// Now that our minimum value changed, our extent also changed.
				// We want to pick the extent value that brings us closest to our original max value while
				// being slightly larger to encompass it.
				const rtm::vector4f range_max = range.get_max();
				const rtm::vector4f range_extent = rtm::vector_sub(range_max, padded_range_min);
				const rtm::vector4f scaled_extent = rtm::vector_mul(range_extent, max_range_value);
				const rtm::vector4f quantized_extent0 = rtm::vector_clamp(rtm::vector_ceil(scaled_extent), zero, max_range_value);
				const rtm::vector4f quantized_extent1 = rtm::vector_min(rtm::vector_add(quantized_extent0, one), max_range_value);

				const rtm::vector4f padded_range_extent0 = rtm::vector_mul(quantized_extent0, inv_max_range_value);
				const rtm::vector4f padded_range_extent1 = rtm::vector_mul(quantized_extent1, inv_max_range_value);

				// Check if extent0 is above or equal to our original range maximum value, if it is, it is good
				// enough to use otherwise extent1 is guaranteed to be higher.
				const rtm::mask4f is_extent0_higher_mask = rtm::vector_greater_equal(padded_range_extent0, range_max);
				const rtm::vector4f padded_range_extent = rtm::vector_select(is_extent0_higher_mask, padded_range_extent0, padded_range_extent1);

				return track_stream_range::from_min_extent(padded_range_min, padded_range_extent);
			};

			for (segment_context& segment : context.segment_iterator())
			{
				segment.ranges = allocate_type_array<transform_range>(allocator, segment.num_bones);

				acl_impl::extract_bone_ranges_impl(segment, segment.ranges);

				for (uint32_t bone_index = 0; bone_index < segment.num_bones; ++bone_index)
				{
					const transform_streams& bone_stream = segment.bone_streams[bone_index];
					transform_range& bone_range = segment.ranges[bone_index];

					if (!bone_stream.is_rotation_constant && context.are_rotations_normalized)
						bone_range.rotation = fixup_range(bone_range.rotation);

					if (!bone_stream.is_translation_constant && context.are_translations_normalized)
						bone_range.translation = fixup_range(bone_range.translation);

					if (!bone_stream.is_scale_constant && context.are_scales_normalized)
						bone_range.scale = fixup_range(bone_range.scale);
				}
			}

#if defined(ACL_USE_SJSON)
			compression_stats.extract_segment_ranges_elapsed_seconds = extract_segment_ranges_time.get_elapsed_seconds();
#endif
		}

		inline rtm::vector4f RTM_SIMD_CALL normalize_sample(rtm::vector4f_arg0 sample, const track_stream_range& range)
		{
			const rtm::vector4f range_min = range.get_min();
			const rtm::vector4f range_extent = range.get_extent();
			const rtm::mask4f is_range_zero_mask = rtm::vector_less_than(range_extent, rtm::vector_set(0.000000001F));

			rtm::vector4f normalized_sample = rtm::vector_div(rtm::vector_sub(sample, range_min), range_extent);
			// Clamp because the division might be imprecise
			normalized_sample = rtm::vector_min(normalized_sample, rtm::vector_set(1.0F));
			return rtm::vector_select(is_range_zero_mask, rtm::vector_zero(), normalized_sample);
		}

		inline void normalize_rotation_streams(transform_streams* bone_streams, const transform_range* bone_ranges, uint32_t num_bones)
		{
			const rtm::vector4f one = rtm::vector_set(1.0F);
			const rtm::vector4f zero = rtm::vector_zero();

			for (uint32_t bone_index = 0; bone_index < num_bones; ++bone_index)
			{
				transform_streams& bone_stream = bone_streams[bone_index];
				const transform_range& bone_range = bone_ranges[bone_index];

				// We expect all our samples to have the same width of sizeof(rtm::vector4f)
				ACL_ASSERT(bone_stream.rotations.get_sample_size() == sizeof(rtm::vector4f), "Unexpected rotation sample size. %u != %zu", bone_stream.rotations.get_sample_size(), sizeof(rtm::vector4f));

				// Constant or default tracks are not normalized
				if (bone_stream.is_rotation_constant)
					continue;

				const uint32_t num_samples = bone_stream.rotations.get_num_samples();

				const rtm::vector4f range_min = bone_range.rotation.get_min();
				const rtm::vector4f range_extent = bone_range.rotation.get_extent();
				const rtm::mask4f is_range_zero_mask = rtm::vector_less_than(range_extent, rtm::vector_set(0.000000001F));

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					// normalized value is between [0.0 .. 1.0]
					// value = (normalized value * range extent) + range min
					// normalized value = (value - range min) / range extent
					const rtm::vector4f rotation = bone_stream.rotations.get_raw_sample<rtm::vector4f>(sample_index);
					rtm::vector4f normalized_rotation = rtm::vector_div(rtm::vector_sub(rotation, range_min), range_extent);
					// Clamp because the division might be imprecise
					normalized_rotation = rtm::vector_min(normalized_rotation, one);
					normalized_rotation = rtm::vector_select(is_range_zero_mask, zero, normalized_rotation);

#if defined(ACL_HAS_ASSERT_CHECKS)
					switch (bone_stream.rotations.get_rotation_format())
					{
					case rotation_format8::quatf_full:
						ACL_ASSERT(rtm::vector_all_greater_equal(normalized_rotation, zero) && rtm::vector_all_less_equal(normalized_rotation, one), "Invalid normalized rotation. 0.0 <= [%f, %f, %f, %f] <= 1.0", (float)rtm::vector_get_x(normalized_rotation), (float)rtm::vector_get_y(normalized_rotation), (float)rtm::vector_get_z(normalized_rotation), (float)rtm::vector_get_w(normalized_rotation));
						break;
					case rotation_format8::quatf_drop_w_full:
					case rotation_format8::quatf_drop_w_variable:
						ACL_ASSERT(rtm::vector_all_greater_equal3(normalized_rotation, zero) && rtm::vector_all_less_equal3(normalized_rotation, one), "Invalid normalized rotation. 0.0 <= [%f, %f, %f] <= 1.0", (float)rtm::vector_get_x(normalized_rotation), (float)rtm::vector_get_y(normalized_rotation), (float)rtm::vector_get_z(normalized_rotation));
						break;
					}
#endif

					bone_stream.rotations.set_raw_sample(sample_index, normalized_rotation);
				}
			}
		}

		inline void normalize_translation_streams(transform_streams* bone_streams, const transform_range* bone_ranges, uint32_t num_bones)
		{
			const rtm::vector4f one = rtm::vector_set(1.0F);
			const rtm::vector4f zero = rtm::vector_zero();

			for (uint32_t bone_index = 0; bone_index < num_bones; ++bone_index)
			{
				transform_streams& bone_stream = bone_streams[bone_index];
				const transform_range& bone_range = bone_ranges[bone_index];

				// We expect all our samples to have the same width of sizeof(rtm::vector4f)
				ACL_ASSERT(bone_stream.translations.get_sample_size() == sizeof(rtm::vector4f), "Unexpected translation sample size. %u != %zu", bone_stream.translations.get_sample_size(), sizeof(rtm::vector4f));

				// Constant or default tracks are not normalized
				if (bone_stream.is_translation_constant)
					continue;

				const uint32_t num_samples = bone_stream.translations.get_num_samples();

				const rtm::vector4f range_min = bone_range.translation.get_min();
				const rtm::vector4f range_extent = bone_range.translation.get_extent();
				const rtm::mask4f is_range_zero_mask = rtm::vector_less_than(range_extent, rtm::vector_set(0.000000001F));

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					// normalized value is between [0.0 .. 1.0]
					// value = (normalized value * range extent) + range min
					// normalized value = (value - range min) / range extent
					const rtm::vector4f translation = bone_stream.translations.get_raw_sample<rtm::vector4f>(sample_index);
					rtm::vector4f normalized_translation = rtm::vector_div(rtm::vector_sub(translation, range_min), range_extent);
					// Clamp because the division might be imprecise
					normalized_translation = rtm::vector_min(normalized_translation, one);
					normalized_translation = rtm::vector_select(is_range_zero_mask, zero, normalized_translation);

					ACL_ASSERT(rtm::vector_all_greater_equal3(normalized_translation, zero) && rtm::vector_all_less_equal3(normalized_translation, one), "Invalid normalized translation. 0.0 <= [%f, %f, %f] <= 1.0", (float)rtm::vector_get_x(normalized_translation), (float)rtm::vector_get_y(normalized_translation), (float)rtm::vector_get_z(normalized_translation));

					bone_stream.translations.set_raw_sample(sample_index, normalized_translation);
				}
			}
		}

		inline void normalize_scale_streams(transform_streams* bone_streams, const transform_range* bone_ranges, uint32_t num_bones)
		{
			const rtm::vector4f one = rtm::vector_set(1.0F);
			const rtm::vector4f zero = rtm::vector_zero();

			for (uint32_t bone_index = 0; bone_index < num_bones; ++bone_index)
			{
				transform_streams& bone_stream = bone_streams[bone_index];
				const transform_range& bone_range = bone_ranges[bone_index];

				// We expect all our samples to have the same width of sizeof(rtm::vector4f)
				ACL_ASSERT(bone_stream.scales.get_sample_size() == sizeof(rtm::vector4f), "Unexpected scale sample size. %u != %zu", bone_stream.scales.get_sample_size(), sizeof(rtm::vector4f));

				// Constant or default tracks are not normalized
				if (bone_stream.is_scale_constant)
					continue;

				const uint32_t num_samples = bone_stream.scales.get_num_samples();

				const rtm::vector4f range_min = bone_range.scale.get_min();
				const rtm::vector4f range_extent = bone_range.scale.get_extent();
				const rtm::mask4f is_range_zero_mask = rtm::vector_less_than(range_extent, rtm::vector_set(0.000000001F));

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					// normalized value is between [0.0 .. 1.0]
					// value = (normalized value * range extent) + range min
					// normalized value = (value - range min) / range extent
					const rtm::vector4f scale = bone_stream.scales.get_raw_sample<rtm::vector4f>(sample_index);
					rtm::vector4f normalized_scale = rtm::vector_div(rtm::vector_sub(scale, range_min), range_extent);
					// Clamp because the division might be imprecise
					normalized_scale = rtm::vector_min(normalized_scale, one);
					normalized_scale = rtm::vector_select(is_range_zero_mask, zero, normalized_scale);

					ACL_ASSERT(rtm::vector_all_greater_equal3(normalized_scale, zero) && rtm::vector_all_less_equal3(normalized_scale, one), "Invalid normalized scale. 0.0 <= [%f, %f, %f] <= 1.0", (float)rtm::vector_get_x(normalized_scale), (float)rtm::vector_get_y(normalized_scale), (float)rtm::vector_get_z(normalized_scale));

					bone_stream.scales.set_raw_sample(sample_index, normalized_scale);
				}
			}
		}

		inline void normalize_clip_streams(clip_context& context, range_reduction_flags8 range_reduction, compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler normalize_clip_time;
#endif

			ACL_ASSERT(context.num_segments == 1, "context must contain a single segment!");
			segment_context& segment = context.segments[0];

			const bool has_scale = segment_context_has_scale(segment);

			if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations))
			{
				normalize_rotation_streams(segment.bone_streams, context.ranges, segment.num_bones);
				context.are_rotations_normalized = true;
			}

			if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations))
			{
				normalize_translation_streams(segment.bone_streams, context.ranges, segment.num_bones);
				context.are_translations_normalized = true;
			}

			if (has_scale && are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales))
			{
				normalize_scale_streams(segment.bone_streams, context.ranges, segment.num_bones);
				context.are_scales_normalized = true;
			}

#if defined(ACL_USE_SJSON)
			compression_stats.normalize_clip_elapsed_seconds = normalize_clip_time.get_elapsed_seconds();
#endif
		}

		inline void normalize_segment_streams(clip_context& context, range_reduction_flags8 range_reduction, compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler normalize_segments_time;
#endif

			for (segment_context& segment : context.segment_iterator())
			{
				if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations))
				{
					normalize_rotation_streams(segment.bone_streams, segment.ranges, segment.num_bones);
					segment.are_rotations_normalized = true;
				}

				if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations))
				{
					normalize_translation_streams(segment.bone_streams, segment.ranges, segment.num_bones);
					segment.are_translations_normalized = true;
				}

				const bool has_scale = segment_context_has_scale(segment);
				if (has_scale && are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales))
				{
					normalize_scale_streams(segment.bone_streams, segment.ranges, segment.num_bones);
					segment.are_scales_normalized = true;
				}

				uint32_t range_data_size = 0;
				uint32_t range_data_rotation_num = 0;

				for (uint32_t bone_index = 0; bone_index < segment.num_bones; ++bone_index)
				{
					const transform_streams& bone_stream = segment.bone_streams[bone_index];
					if (bone_stream.is_stripped_from_output())
						continue;

					if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::rotations) && !bone_stream.is_rotation_constant)
					{
						ACL_ASSERT(bone_stream.rotations.get_rotation_format() != rotation_format8::quatf_full, "Normalization only supported on drop W variants");
						range_data_size += k_segment_range_reduction_num_bytes_per_component * 6;
						range_data_rotation_num++;
					}

					if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::translations) && !bone_stream.is_translation_constant)
						range_data_size += k_segment_range_reduction_num_bytes_per_component * 6;

					if (are_any_enum_flags_set(range_reduction, range_reduction_flags8::scales) && !bone_stream.is_scale_constant)
						range_data_size += k_segment_range_reduction_num_bytes_per_component * 6;
				}

				// The last partial rotation group is padded to 4 elements to keep decompression fast
				const uint32_t partial_group_size_rotation = range_data_rotation_num % 4;
				if (partial_group_size_rotation != 0)
					range_data_size += (4 - partial_group_size_rotation) * k_segment_range_reduction_num_bytes_per_component * 6;

				segment.range_data_size = range_data_size;
			}

#if defined(ACL_USE_SJSON)
			compression_stats.normalize_segment_elapsed_seconds = normalize_segments_time.get_elapsed_seconds();
#endif
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
