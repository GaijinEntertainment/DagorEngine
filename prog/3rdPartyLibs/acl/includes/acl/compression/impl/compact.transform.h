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
#include "acl/core/track_formats.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"
#include "acl/core/scope_profiler.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/compression_stats.h"
#include "acl/compression/impl/rigid_shell_utils.h"
#include "acl/compression/transform_error_metrics.h"

#include <rtm/qvvf.h>

#include <cstdint>

//////////////////////////////////////////////////////////////////////////
// Apply error correction after constant and default tracks are processed.
// Notes:
//     - original code was adapted and cleaned up a bit, but largely as contributed
//     - zero scale isn't properly handled and needs to be guarded against
//     - regression testing over large data sets shows that it is sometimes a win, sometimes not
//     - overall, it seems to be a net loss over the memory footprint and quality does not
//       measurably improve to justify the loss
//     - I tried various tweaks and failed to make the code a consistent win, see https://github.com/nfrechette/acl/issues/353

//#define ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION
#include "acl/compression/impl/normalize.transform.h"
#endif

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// To detect if a sub-track is constant, we grab the first sample as our reference.
		// We then measure the object space error using the qvv error metric and our
		// dominant shell distance. If the error remains within our dominant precision
		// then the sub-track is constant. We perform the same test using the default
		// sub-track value to determine if it is a default sub-track.

		inline bool RTM_SIMD_CALL are_samples_constant(const compression_settings& settings,
			const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context,
			rtm::vector4f_arg0 reference, uint32_t transform_index, animation_track_type8 sub_track_type)
		{
			const segment_context& lossy_segment = lossy_clip_context.segments[0];
			const transform_streams& raw_transform_stream = lossy_segment.bone_streams[transform_index];

			const rigid_shell_metadata_t& shell = lossy_clip_context.clip_shell_metadata[transform_index];

			const itransform_error_metric& error_metric = *settings.error_metric;

			const bool has_additive_base = lossy_clip_context.has_additive_base;
			const bool needs_conversion = error_metric.needs_conversion(lossy_clip_context.has_scale);

			const uint32_t dirty_transform_indices[2] = { 0, 1 };
			rtm::qvvf local_transforms[2];
			rtm::qvvf base_transforms[2];
			alignas(16) uint8_t local_transforms_converted[1024];	// Big enough for 2 transforms for sure
			alignas(16) uint8_t base_transforms_converted[1024];	// Big enough for 2 transforms for sure

			const size_t metric_transform_size = error_metric.get_transform_size(lossy_clip_context.has_scale);
			ACL_ASSERT(metric_transform_size * 2 <= sizeof(local_transforms_converted), "Transform size is too large");

			itransform_error_metric::convert_transforms_args convert_transforms_args_local;
			convert_transforms_args_local.dirty_transform_indices = &dirty_transform_indices[0];
			convert_transforms_args_local.num_dirty_transforms = 1;
			convert_transforms_args_local.num_transforms = 1;
			convert_transforms_args_local.is_additive_base = false;

			itransform_error_metric::convert_transforms_args convert_transforms_args_base;
			convert_transforms_args_base.dirty_transform_indices = &dirty_transform_indices[0];
			convert_transforms_args_base.num_dirty_transforms = 2;
			convert_transforms_args_base.num_transforms = 2;
			convert_transforms_args_base.transforms = &base_transforms[0];
			convert_transforms_args_base.is_additive_base = true;
			convert_transforms_args_base.is_lossy = false;

			itransform_error_metric::apply_additive_to_base_args apply_additive_to_base_args;
			apply_additive_to_base_args.dirty_transform_indices = &dirty_transform_indices[0];
			apply_additive_to_base_args.num_dirty_transforms = 2;
			apply_additive_to_base_args.base_transforms = needs_conversion ? (const void*)&base_transforms_converted[0] : (const void*)&base_transforms[0];
			apply_additive_to_base_args.local_transforms = needs_conversion ? (const void*)&local_transforms_converted[0] : (const void*)&local_transforms[0];
			apply_additive_to_base_args.num_transforms = 2;

			qvvf_transform_error_metric::calculate_error_args calculate_error_args;
			calculate_error_args.construct_sphere_shell(shell.local_shell_distance);
			calculate_error_args.transform0 = &local_transforms_converted[metric_transform_size * 0];
			calculate_error_args.transform1 = &local_transforms_converted[metric_transform_size * 1];

			const rtm::scalarf precision = rtm::scalar_set(shell.precision);

			const uint32_t num_samples = lossy_clip_context.num_samples;
			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const rtm::quatf raw_rotation = raw_transform_stream.rotations.get_sample_clamped(sample_index);
				const rtm::vector4f raw_translation = raw_transform_stream.translations.get_sample_clamped(sample_index);
				const rtm::vector4f raw_scale = raw_transform_stream.scales.get_sample_clamped(sample_index);

				rtm::qvvf raw_transform = rtm::qvv_set(raw_rotation, raw_translation, raw_scale);
				rtm::qvvf lossy_transform = raw_transform;	// Copy the raw transform

				// Fix up our lossy transform with the reference value
				switch (sub_track_type)
				{
				case animation_track_type8::rotation:
				default:
					lossy_transform.rotation = rtm::vector_to_quat(reference);
					break;
				case animation_track_type8::translation:
					lossy_transform.translation = reference;
					break;
				case animation_track_type8::scale:
					lossy_transform.scale = reference;
					break;
				}

				local_transforms[0] = raw_transform;
				local_transforms[1] = lossy_transform;

				if (needs_conversion)
				{
					convert_transforms_args_local.sample_index = sample_index;

					convert_transforms_args_local.transforms = &local_transforms[0];
					convert_transforms_args_local.is_lossy = false;

					error_metric.convert_transforms(convert_transforms_args_local, &local_transforms_converted[metric_transform_size * 0]);

					convert_transforms_args_local.transforms = &local_transforms[1];
					convert_transforms_args_local.is_lossy = true;

					error_metric.convert_transforms(convert_transforms_args_local, &local_transforms_converted[metric_transform_size * 1]);
				}
				else
					std::memcpy(&local_transforms_converted[0], &local_transforms[0], metric_transform_size * 2);

				if (has_additive_base)
				{
					const segment_context& additive_base_segment = additive_base_clip_context.segments[0];
					const transform_streams& additive_base_bone_stream = additive_base_segment.bone_streams[transform_index];

					// The sample time is calculated from the full clip duration to be consistent with decompression
					const float sample_time = rtm::scalar_min(float(sample_index) / lossy_clip_context.sample_rate, lossy_clip_context.duration);

					const float normalized_sample_time = additive_base_segment.num_samples > 1 ? (sample_time / lossy_clip_context.duration) : 0.0F;
					const float additive_sample_time = additive_base_segment.num_samples > 1 ? (normalized_sample_time * additive_base_clip_context.duration) : 0.0F;

					// With uniform sample distributions, we do not interpolate.
					const uint32_t base_sample_index = get_uniform_sample_key(additive_base_segment, additive_sample_time);

					const rtm::quatf base_rotation = additive_base_bone_stream.rotations.get_sample_clamped(base_sample_index);
					const rtm::vector4f base_translation = additive_base_bone_stream.translations.get_sample_clamped(base_sample_index);
					const rtm::vector4f base_scale = additive_base_bone_stream.scales.get_sample_clamped(base_sample_index);

					const rtm::qvvf base_transform = rtm::qvv_set(base_rotation, base_translation, base_scale);

					base_transforms[0] = base_transform;
					base_transforms[1] = base_transform;

					if (needs_conversion)
					{
						convert_transforms_args_base.sample_index = base_sample_index;
						error_metric.convert_transforms(convert_transforms_args_base, &base_transforms_converted[0]);
					}
					else
						std::memcpy(&base_transforms_converted[0], &base_transforms[0], metric_transform_size * 2);

					error_metric.apply_additive_to_base(apply_additive_to_base_args, &local_transforms_converted[0]);
				}

				const rtm::scalarf vtx_error = error_metric.calculate_error(calculate_error_args);

				// If our error exceeds the desired precision, we are not constant
				if (rtm::scalar_greater_than(vtx_error, precision))
					return false;
			}

			// All samples were tested against the reference value and the error remained within tolerance
			return true;
		}

		inline bool are_rotations_constant(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are constant if we have no samples

			// When we are using full precision, we are only constant if range.min == range.max, meaning
			// we have a single unique and repeating sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.rotation_format == rotation_format8::quatf_full)
				return lossy_clip_context.ranges[transform_index].rotation.is_constant(0.0F);

			const segment_context& segment = lossy_clip_context.segments[0];
			const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, rtm::quat_to_vector(raw_transform_stream.rotations.get_sample(0)), transform_index, animation_track_type8::rotation);
		}

		inline bool are_rotations_default(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, const track_desc_transformf& desc, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are default if we have no samples

			const rtm::vector4f default_bind_rotation = rtm::quat_to_vector(desc.default_value.rotation);

			// When we are using full precision, we are only default if (sample 0 == default value), meaning
			// we have a single unique and repeating default sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.rotation_format == rotation_format8::quatf_full)
			{
				const segment_context& segment = lossy_clip_context.segments[0];
				const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

				const rtm::vector4f rotation = raw_transform_stream.rotations.get_raw_sample<rtm::vector4f>(0);
				return rtm::vector_all_equal(rotation, default_bind_rotation);
			}

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, default_bind_rotation, transform_index, animation_track_type8::rotation);
		}

		inline bool are_translations_constant(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are constant if we have no samples

			// When we are using full precision, we are only constant if range.min == range.max, meaning
			// we have a single unique and repeating sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.translation_format == vector_format8::vector3f_full)
				return lossy_clip_context.ranges[transform_index].translation.is_constant(0.0F);

			const segment_context& segment = lossy_clip_context.segments[0];
			const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, raw_transform_stream.translations.get_sample(0), transform_index, animation_track_type8::translation);
		}

		inline bool are_translations_default(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, const track_desc_transformf& desc, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are default if we have no samples

			const rtm::vector4f default_bind_translation = desc.default_value.translation;

			// When we are using full precision, we are only default if (sample 0 == default value), meaning
			// we have a single unique and repeating default sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.translation_format == vector_format8::vector3f_full)
			{
				const segment_context& segment = lossy_clip_context.segments[0];
				const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

				const rtm::vector4f translation = raw_transform_stream.translations.get_raw_sample<rtm::vector4f>(0);
				return rtm::vector_all_equal(translation, default_bind_translation);
			}

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, default_bind_translation, transform_index, animation_track_type8::translation);
		}

		inline bool are_scales_constant(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are constant if we have no samples

			if (!lossy_clip_context.has_scale)
				return true;	// We are constant if we have no scale

			// When we are using full precision, we are only constant if range.min == range.max, meaning
			// we have a single unique and repeating sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.scale_format == vector_format8::vector3f_full)
				return lossy_clip_context.ranges[transform_index].scale.is_constant(0.0F);

			const segment_context& segment = lossy_clip_context.segments[0];
			const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, raw_transform_stream.scales.get_sample(0), transform_index, animation_track_type8::scale);
		}

		inline bool are_scales_default(const compression_settings& settings, const clip_context& lossy_clip_context, const clip_context& additive_base_clip_context, const track_desc_transformf& desc, uint32_t transform_index)
		{
			if (lossy_clip_context.num_samples == 0)
				return true;	// We are default if we have no samples

			if (!lossy_clip_context.has_scale)
				return true;	// We are default if we have no scale

			const rtm::vector4f default_bind_scale = desc.default_value.scale;

			// When we are using full precision, we are only default if (sample 0 == default value), meaning
			// we have a single unique and repeating default sample
			// We want to test if we are binary exact
			// This is used by raw clips, we must preserve the original values
			if (settings.scale_format == vector_format8::vector3f_full)
			{
				const segment_context& segment = lossy_clip_context.segments[0];
				const transform_streams& raw_transform_stream = segment.bone_streams[transform_index];

				const rtm::vector4f scale = raw_transform_stream.scales.get_raw_sample<rtm::vector4f>(0);
				return rtm::vector_all_equal(scale, default_bind_scale);
			}

			// Otherwise check every sample to make sure we fall within the desired tolerance
			return are_samples_constant(settings, lossy_clip_context, additive_base_clip_context, default_bind_scale, transform_index, animation_track_type8::scale);
		}

		// Compacts constant sub-tracks
		// A sub-track is constant if every sample can be replaced by a single unique sample without exceeding
		// our error threshold.
		// By default, constant sub-tracks will retain the first sample.
		// A constant sub-track is a default sub-track if its unique sample can be replaced by the default value
		// without exceeding our error threshold.
		inline void compact_constant_streams(
			iallocator& allocator,
			clip_context& context,
			const clip_context& additive_base_clip_context,
			const track_array_qvvf& track_list,
			const compression_settings& settings,
			compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler compact_constant_sub_tracks_time;
#endif

			ACL_ASSERT(context.num_segments == 1, "context must contain a single segment!");

			segment_context& segment = context.segments[0];

			const uint32_t num_transforms = context.num_bones;
			const uint32_t num_samples = context.num_samples;

			uint32_t num_default_bone_scales = 0;

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION
			bool has_constant_bone_rotations = false;
			bool has_constant_bone_translations = false;
			bool has_constant_bone_scales = false;
#endif

			// Iterate in any order, doesn't matter
			for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
			{
				const track_desc_transformf& desc = track_list[transform_index].get_description();

				transform_streams& bone_stream = segment.bone_streams[transform_index];

				transform_range& bone_range = context.ranges[transform_index];

				ACL_ASSERT(bone_stream.rotations.get_num_samples() == num_samples, "Rotation sample mismatch!");
				ACL_ASSERT(bone_stream.translations.get_num_samples() == num_samples, "Translation sample mismatch!");
				ACL_ASSERT(bone_stream.scales.get_num_samples() == num_samples, "Scale sample mismatch!");

				// We expect all our samples to have the same width of sizeof(rtm::vector4f)
				ACL_ASSERT(bone_stream.rotations.get_sample_size() == sizeof(rtm::vector4f), "Unexpected rotation sample size. %u != %zu", bone_stream.rotations.get_sample_size(), sizeof(rtm::vector4f));
				ACL_ASSERT(bone_stream.translations.get_sample_size() == sizeof(rtm::vector4f), "Unexpected translation sample size. %u != %zu", bone_stream.translations.get_sample_size(), sizeof(rtm::vector4f));
				ACL_ASSERT(bone_stream.scales.get_sample_size() == sizeof(rtm::vector4f), "Unexpected scale sample size. %u != %zu", bone_stream.scales.get_sample_size(), sizeof(rtm::vector4f));

				if (are_rotations_constant(settings, context, additive_base_clip_context, transform_index))
				{
					rotation_track_stream constant_stream(allocator, 1, bone_stream.rotations.get_sample_size(), bone_stream.rotations.get_sample_rate(), bone_stream.rotations.get_rotation_format());

					const rtm::vector4f default_bind_rotation = rtm::quat_to_vector(desc.default_value.rotation);

					rtm::vector4f rotation = num_samples != 0 ? bone_stream.rotations.get_raw_sample<rtm::vector4f>(0) : default_bind_rotation;

					bone_stream.is_rotation_constant = true;

					if (are_rotations_default(settings, context, additive_base_clip_context, desc, transform_index))
					{
						bone_stream.is_rotation_default = true;
						rotation = default_bind_rotation;
					}

					constant_stream.set_raw_sample(0, rotation);
					bone_stream.rotations = std::move(constant_stream);

					bone_range.rotation = track_stream_range::from_min_extent(rotation, rtm::vector_zero());

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION
					has_constant_bone_rotations = true;
#endif
				}

				if (are_translations_constant(settings, context, additive_base_clip_context, transform_index))
				{
					translation_track_stream constant_stream(allocator, 1, bone_stream.translations.get_sample_size(), bone_stream.translations.get_sample_rate(), bone_stream.translations.get_vector_format());

					const rtm::vector4f default_bind_translation = desc.default_value.translation;

					rtm::vector4f translation = num_samples != 0 ? bone_stream.translations.get_raw_sample<rtm::vector4f>(0) : default_bind_translation;

					bone_stream.is_translation_constant = true;

					if (are_translations_default(settings, context, additive_base_clip_context, desc, transform_index))
					{
						bone_stream.is_translation_default = true;
						translation = default_bind_translation;
					}

					constant_stream.set_raw_sample(0, translation);
					bone_stream.translations = std::move(constant_stream);

					// Zero out W, could be garbage
					bone_range.translation = track_stream_range::from_min_extent(rtm::vector_set_w(translation, 0.0F), rtm::vector_zero());

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION
					has_constant_bone_translations = true;
#endif
				}

				if (are_scales_constant(settings, context, additive_base_clip_context, transform_index))
				{
					scale_track_stream constant_stream(allocator, 1, bone_stream.scales.get_sample_size(), bone_stream.scales.get_sample_rate(), bone_stream.scales.get_vector_format());

					const rtm::vector4f default_bind_scale = desc.default_value.scale;

					rtm::vector4f scale = (context.has_scale && num_samples != 0) ? bone_stream.scales.get_raw_sample<rtm::vector4f>(0) : default_bind_scale;

					bone_stream.is_scale_constant = true;

					if (are_scales_default(settings, context, additive_base_clip_context, desc, transform_index))
					{
						bone_stream.is_scale_default = true;
						scale = default_bind_scale;
					}

					constant_stream.set_raw_sample(0, scale);
					bone_stream.scales = std::move(constant_stream);

					// Zero out W, could be garbage
					bone_range.scale = track_stream_range::from_min_extent(rtm::vector_set_w(scale, 0.0F), rtm::vector_zero());

					num_default_bone_scales += bone_stream.is_scale_default ? 1 : 0;

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION
					has_constant_bone_scales = true;
#endif
				}
			}

			const bool has_scale = num_default_bone_scales != num_transforms;
			context.has_scale = has_scale;

#ifdef ACL_IMPL_ENABLE_CONSTANT_ERROR_CORRECTION

			// Only perform error compensation if our format isn't raw
			const bool is_raw = settings.rotation_format == rotation_format8::quatf_full || settings.translation_format == vector_format8::vector3f_full || settings.scale_format == vector_format8::vector3f_full;

			// Only perform error compensation if we are lossy due to constant sub-tracks
			// In practice, even if we have no constant sub-tracks, we could be lossy if our rotations drop W
			const bool is_lossy = has_constant_bone_rotations || has_constant_bone_translations || (has_scale && has_constant_bone_scales);

			if (!context.has_additive_base && !is_raw && is_lossy)
			{
				// Apply error correction after constant and default tracks are processed.
				// We use object space of the original data as ground truth, and only deviate for 2 reasons, and as briefly as possible.
				//    -Replace an original local value with a new constant value.
				//    -Correct for the manipulation of an original local value by an ancestor ASAP.
				// We aren't modifying raw data here. We're modifying the raw channels generated from the raw data.
				// The raw data is left alone, and is still used at the end of the process to do regression testing.

				struct dirty_state_t
				{
					bool rotation = false;
					bool translation = false;
					bool scale = false;
				};

				dirty_state_t any_constant_changed;
				dirty_state_t* dirty_states = allocate_type_array<dirty_state_t>(allocator, num_transforms);
				rtm::qvvf* original_object_pose = allocate_type_array<rtm::qvvf>(allocator, num_transforms);
				rtm::qvvf* adjusted_object_pose = allocate_type_array<rtm::qvvf>(allocator, num_transforms);

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					// Iterate over parent transforms first
					for (uint32_t bone_index : make_iterator(context.sorted_transforms_parent_first, num_transforms))
					{
						rtm::qvvf& original_object_transform = original_object_pose[bone_index];

						const transform_range& bone_range = context.ranges[bone_index];
						transform_streams& bone_stream = segment.bone_streams[bone_index];
						transform_streams& raw_bone_stream = raw_segment.bone_streams[bone_index];

						const track_desc_transformf& desc = track_list[bone_index].get_description();
						const uint32_t parent_bone_index = desc.parent_index;
						const rtm::qvvf original_local_transform = rtm::qvv_set(
							raw_bone_stream.rotations.get_raw_sample<rtm::quatf>(sample_index),
							raw_bone_stream.translations.get_raw_sample<rtm::vector4f>(sample_index),
							raw_bone_stream.scales.get_raw_sample<rtm::vector4f>(sample_index));

						if (parent_bone_index == k_invalid_track_index)
							original_object_transform = original_local_transform;	// Just copy the root as-is, it has no parent and thus local and object space transforms are equal
						else if (!has_scale)
							original_object_transform = rtm::qvv_normalize(rtm::qvv_mul_no_scale(original_local_transform, original_object_pose[parent_bone_index]));
						else
							original_object_transform = rtm::qvv_normalize(rtm::qvv_mul(original_local_transform, original_object_pose[parent_bone_index]));

						rtm::qvvf adjusted_local_transform = original_local_transform;

						dirty_state_t& constant_changed = dirty_states[bone_index];
						constant_changed.rotation = false;
						constant_changed.translation = false;
						constant_changed.scale = false;

						if (bone_stream.is_rotation_constant)
						{
							const rtm::quatf constant_rotation = rtm::vector_to_quat(bone_range.rotation.get_min());
							if (!rtm::vector_all_near_equal(rtm::quat_to_vector(adjusted_local_transform.rotation), rtm::quat_to_vector(constant_rotation), 0.0F))
							{
								any_constant_changed.rotation = true;
								constant_changed.rotation = true;
								adjusted_local_transform.rotation = constant_rotation;
								raw_bone_stream.rotations.set_raw_sample(sample_index, constant_rotation);
							}
							ACL_ASSERT(bone_stream.rotations.get_num_samples() == 1, "Constant rotation stream mismatch!");
							ACL_ASSERT(rtm::vector_all_near_equal(bone_stream.rotations.get_raw_sample<rtm::vector4f>(0), rtm::quat_to_vector(constant_rotation), 0.0F), "Constant rotation mismatch!");
						}

						if (bone_stream.is_translation_constant)
						{
							const rtm::vector4f constant_translation = bone_range.translation.get_min();
							if (!rtm::vector_all_near_equal3(adjusted_local_transform.translation, constant_translation, 0.0F))
							{
								any_constant_changed.translation = true;
								constant_changed.translation = true;
								adjusted_local_transform.translation = constant_translation;
								raw_bone_stream.translations.set_raw_sample(sample_index, constant_translation);
							}
							ACL_ASSERT(bone_stream.translations.get_num_samples() == 1, "Constant translation stream mismatch!");
							ACL_ASSERT(rtm::vector_all_near_equal3(bone_stream.translations.get_raw_sample<rtm::vector4f>(0), constant_translation, 0.0F), "Constant translation mismatch!");
						}

						if (has_scale && bone_stream.is_scale_constant)
						{
							const rtm::vector4f constant_scale = bone_range.scale.get_min();
							if (!rtm::vector_all_near_equal3(adjusted_local_transform.scale, constant_scale, 0.0F))
							{
								any_constant_changed.scale = true;
								constant_changed.scale = true;
								adjusted_local_transform.scale = constant_scale;
								raw_bone_stream.scales.set_raw_sample(sample_index, constant_scale);
							}
							ACL_ASSERT(bone_stream.scales.get_num_samples() == 1, "Constant scale stream mismatch!");
							ACL_ASSERT(rtm::vector_all_near_equal3(bone_stream.scales.get_raw_sample<rtm::vector4f>(0), constant_scale, 0.0F), "Constant scale mismatch!");
						}

						rtm::qvvf& adjusted_object_transform = adjusted_object_pose[bone_index];
						if (parent_bone_index == k_invalid_track_index)
						{
							adjusted_object_transform = adjusted_local_transform;	// Just copy the root as-is, it has no parent and thus local and object space transforms are equal
						}
						else
						{
							const dirty_state_t& parent_constant_changed = dirty_states[parent_bone_index];
							const rtm::qvvf& parent_adjusted_object_transform = adjusted_object_pose[parent_bone_index];

							if (bone_stream.is_rotation_constant && !constant_changed.rotation)
								constant_changed.rotation = parent_constant_changed.rotation;
							if (bone_stream.is_translation_constant && !constant_changed.translation)
								constant_changed.translation = parent_constant_changed.translation;
							if (has_scale && bone_stream.is_scale_constant && !constant_changed.scale)
								constant_changed.scale = parent_constant_changed.scale;

							// Compensate for the constant changes in your ancestors.
							if (!bone_stream.is_rotation_constant && parent_constant_changed.rotation)
							{
								ACL_ASSERT(any_constant_changed.rotation, "No rotations have changed!");
								adjusted_local_transform.rotation = rtm::quat_normalize(rtm::quat_mul(original_object_transform.rotation, rtm::quat_conjugate(parent_adjusted_object_transform.rotation)));
								raw_bone_stream.rotations.set_raw_sample(sample_index, adjusted_local_transform.rotation);
								bone_stream.rotations.set_raw_sample(sample_index, adjusted_local_transform.rotation);
							}

							if (has_scale)
							{
								if (!bone_stream.is_translation_constant && (parent_constant_changed.rotation || parent_constant_changed.translation || parent_constant_changed.scale))
								{
									ACL_ASSERT(any_constant_changed.rotation || any_constant_changed.translation || any_constant_changed.scale, "No channels have changed!");
									const rtm::quatf inv_rotation = rtm::quat_conjugate(parent_adjusted_object_transform.rotation);
									const rtm::vector4f inv_scale = rtm::vector_reciprocal(parent_adjusted_object_transform.scale);
									adjusted_local_transform.translation = rtm::vector_mul(rtm::quat_mul_vector3(rtm::vector_sub(original_object_transform.translation, parent_adjusted_object_transform.translation), inv_rotation), inv_scale);
									raw_bone_stream.translations.set_raw_sample(sample_index, adjusted_local_transform.translation);
									bone_stream.translations.set_raw_sample(sample_index, adjusted_local_transform.translation);
								}

								if (!bone_stream.is_scale_constant && parent_constant_changed.scale)
								{
									ACL_ASSERT(any_constant_changed.scale, "No scales have changed!");
									adjusted_local_transform.scale = rtm::vector_mul(original_object_transform.scale, rtm::vector_reciprocal(parent_adjusted_object_transform.scale));
									raw_bone_stream.scales.set_raw_sample(sample_index, adjusted_local_transform.scale);
									bone_stream.scales.set_raw_sample(sample_index, adjusted_local_transform.scale);
								}

								adjusted_object_transform = rtm::qvv_normalize(rtm::qvv_mul(adjusted_local_transform, parent_adjusted_object_transform));
							}
							else
							{
								if (!bone_stream.is_translation_constant && (parent_constant_changed.rotation || parent_constant_changed.translation))
								{
									ACL_ASSERT(any_constant_changed.rotation || any_constant_changed.translation, "No channels have changed!");
									const rtm::quatf inv_rotation = rtm::quat_conjugate(parent_adjusted_object_transform.rotation);
									adjusted_local_transform.translation = rtm::quat_mul_vector3(rtm::vector_sub(original_object_transform.translation, parent_adjusted_object_transform.translation), inv_rotation);
									raw_bone_stream.translations.set_raw_sample(sample_index, adjusted_local_transform.translation);
									bone_stream.translations.set_raw_sample(sample_index, adjusted_local_transform.translation);
								}

								adjusted_object_transform = rtm::qvv_normalize(rtm::qvv_mul_no_scale(adjusted_local_transform, parent_adjusted_object_transform));
							}
						}
					}
				}
				deallocate_type_array(allocator, adjusted_object_pose, num_transforms);
				deallocate_type_array(allocator, original_object_pose, num_transforms);
				deallocate_type_array(allocator, dirty_states, num_transforms);

				// We need to do these again, to account for error correction.
				if(any_constant_changed.rotation)
				{
					convert_rotation_streams(allocator, context, settings.rotation_format);
				}

				if (any_constant_changed.rotation || any_constant_changed.translation || any_constant_changed.scale)
				{
					deallocate_type_array(allocator, context.ranges, num_transforms);
					extract_clip_bone_ranges(allocator, context);
				}
			}
#endif

#if defined(ACL_USE_SJSON)
			compression_stats.compact_constant_sub_tracks_elapsed_seconds = compact_constant_sub_tracks_time.get_elapsed_seconds();
#endif
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
