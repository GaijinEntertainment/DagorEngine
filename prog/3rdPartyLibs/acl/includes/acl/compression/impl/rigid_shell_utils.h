#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2022 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/sample_streams.h"
#include "acl/compression/impl/transform_clip_adapters.h"

#include <rtm/qvvf.h>

#include <cstdint>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// We use the raw data to compute the rigid shell
		// For each transform, its rigid shell is formed by the dominant joint (itself or a child)
		// We compute the largest value over the whole clip per transform
		template<class clip_adapter_t>
		inline rigid_shell_metadata_t* compute_clip_shell_distances(
			iallocator& allocator,
			const clip_adapter_t& raw_clip,
			const clip_adapter_t& additive_base_clip)
		{
			static_assert(std::is_base_of<transform_clip_adapter_t, clip_adapter_t>::value, "Clip adapter must derive from transform_clip_adapter_t");

			const uint32_t num_transforms = raw_clip.get_num_transforms();
			if (num_transforms == 0)
				return nullptr;	// No transforms present, no shell distances

			const uint32_t num_samples = raw_clip.get_num_samples();
			if (num_samples == 0)
				return nullptr;	// No samples present, no shell distances

			const bool has_additive_base = raw_clip.has_additive_base();
			const float sample_rate = raw_clip.get_sample_rate();
			const float duration = raw_clip.get_duration();
			const additive_clip_format8 additive_format = raw_clip.get_additive_format();

			rigid_shell_metadata_t* shell_metadata = allocate_type_array<rigid_shell_metadata_t>(allocator, num_transforms);

			// Initialize everything
			for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
			{
				shell_metadata[transform_index].local_shell_distance = raw_clip.get_transform_shell_distance(transform_index);
				shell_metadata[transform_index].precision = raw_clip.get_transform_precision(transform_index);
				shell_metadata[transform_index].parent_shell_distance = 0.0F;
			}

			// Iterate from leaf transforms towards their root, we want to bubble up our shell distance
			for (const uint32_t transform_index : make_reverse_iterator(raw_clip.get_sorted_transforms_parent_first(), num_transforms))
			{
				rigid_shell_metadata_t& shell = shell_metadata[transform_index];

				// Use the accumulated shell distance so far to see how far it deforms with our local transform
				const rtm::vector4f vtx0 = rtm::vector_set(shell.local_shell_distance, 0.0F, 0.0F);
				const rtm::vector4f vtx1 = rtm::vector_set(0.0F, shell.local_shell_distance, 0.0F);
				const rtm::vector4f vtx2 = rtm::vector_set(0.0F, 0.0F, shell.local_shell_distance);

				// Calculate the shell distance in parent space
				rtm::scalarf parent_shell_distance = rtm::scalar_set(0.0F);
				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					const rtm::quatf raw_rotation = raw_clip.get_transform_rotation(transform_index, sample_index);
					const rtm::vector4f raw_translation = raw_clip.get_transform_translation(transform_index, sample_index);
					const rtm::vector4f raw_scale = raw_clip.get_transform_scale(transform_index, sample_index);

					rtm::qvvf raw_transform = rtm::qvv_set(raw_rotation, raw_translation, raw_scale);

					if (has_additive_base)
					{
						// If we are additive, we must apply our local transform on the base to figure out
						// the true shell distance
						const uint32_t base_num_samples = additive_base_clip.get_num_samples();
						const float base_duration = additive_base_clip.get_duration();

						// The sample time is calculated from the full clip duration to be consistent with decompression
						const float sample_time = rtm::scalar_min(float(sample_index) / sample_rate, duration);

						const float normalized_sample_time = base_num_samples > 1 ? (sample_time / duration) : 0.0F;
						const float additive_sample_time = base_num_samples > 1 ? (normalized_sample_time * base_duration) : 0.0F;

						// With uniform sample distributions, we do not interpolate.
						const uint32_t base_sample_index = get_uniform_sample_key(
							additive_base_clip,
							transform_segment_adapter_t(),
							additive_sample_time);

						const rtm::quatf base_rotation = additive_base_clip.get_transform_rotation(transform_index, base_sample_index);
						const rtm::vector4f base_translation = additive_base_clip.get_transform_translation(transform_index, base_sample_index);
						const rtm::vector4f base_scale = additive_base_clip.get_transform_scale(transform_index, base_sample_index);

						const rtm::qvvf base_transform = rtm::qvv_set(base_rotation, base_translation, base_scale);
						raw_transform = apply_additive_to_base(additive_format, base_transform, raw_transform);
					}

					const rtm::vector4f raw_vtx0 = rtm::qvv_mul_point3(vtx0, raw_transform);
					const rtm::vector4f raw_vtx1 = rtm::qvv_mul_point3(vtx1, raw_transform);
					const rtm::vector4f raw_vtx2 = rtm::qvv_mul_point3(vtx2, raw_transform);

					const rtm::scalarf vtx0_distance = rtm::vector_length3_as_scalar(raw_vtx0);
					const rtm::scalarf vtx1_distance = rtm::vector_length3_as_scalar(raw_vtx1);
					const rtm::scalarf vtx2_distance = rtm::vector_length3_as_scalar(raw_vtx2);

					const rtm::scalarf transform_length = rtm::scalar_max(rtm::scalar_max(vtx0_distance, vtx1_distance), vtx2_distance);
					parent_shell_distance = rtm::scalar_max(parent_shell_distance, transform_length);
				}

				shell.parent_shell_distance = rtm::scalar_cast(parent_shell_distance);

				// Add precision since we want to make sure to encompass the maximum amount of error allowed
				// Add it only for non-dominant transforms to account for the error they introduce
				// Dominant transforms will use their own precision
				// If our shell distance has changed, we are non-dominant since a dominant child updated it
				if (shell.local_shell_distance != raw_clip.get_transform_shell_distance(transform_index))
					shell.parent_shell_distance += raw_clip.get_transform_precision(transform_index);

				const uint32_t parent_index = raw_clip.get_transform_parent_index(transform_index);
				if (parent_index != k_invalid_track_index)
				{
					// We have a parent, propagate our shell distance if we are a dominant transform
					// We are a dominant transform if our shell distance in parent space is larger
					// than our parent's shell distance in local space. Otherwise, if we are smaller
					// or equal, it means that the full range of motion of our transform fits within
					// the parent's shell distance.

					rigid_shell_metadata_t& parent_shell = shell_metadata[parent_index];

					if (shell.parent_shell_distance > parent_shell.local_shell_distance)
					{
						// We are the new dominant transform, use our shell distance and precision
						parent_shell.local_shell_distance = shell.parent_shell_distance;
						parent_shell.precision = shell.precision;
					}
				}
			}

			return shell_metadata;
		}

		// We use the raw data to compute the rigid shell
		// For each transform, its rigid shell is formed by the dominant joint (itself or a child)
		// We compute the largest value over the whole segment per transform
		inline void compute_segment_shell_distances(const segment_context& segment, const clip_context& additive_base_clip_context, rigid_shell_metadata_t* out_shell_metadata)
		{
			const uint32_t num_transforms = segment.num_bones;
			if (num_transforms == 0)
				return;	// No transforms present, no shell distances

			const uint32_t num_samples = segment.num_samples;
			if (num_samples == 0)
				return;	// No samples present, no shell distances

			const clip_context& owner_clip_context = *segment.clip;
			const bool has_additive_base = owner_clip_context.has_additive_base;

			// Initialize everything
			for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
			{
				const transform_metadata& metadata = owner_clip_context.metadata[transform_index];

				out_shell_metadata[transform_index].local_shell_distance = metadata.shell_distance;
				out_shell_metadata[transform_index].precision = metadata.precision;
				out_shell_metadata[transform_index].parent_shell_distance = 0.0F;
			}

			// Iterate from leaf transforms towards their root, we want to bubble up our shell distance
			for (const uint32_t transform_index : make_reverse_iterator(owner_clip_context.sorted_transforms_parent_first, num_transforms))
			{
				const transform_streams& segment_bone_stream = segment.bone_streams[transform_index];

				rigid_shell_metadata_t& shell = out_shell_metadata[transform_index];

				// Use the accumulated shell distance so far to see how far it deforms with our local transform
				const rtm::vector4f vtx0 = rtm::vector_set(shell.local_shell_distance, 0.0F, 0.0F);
				const rtm::vector4f vtx1 = rtm::vector_set(0.0F, shell.local_shell_distance, 0.0F);
				const rtm::vector4f vtx2 = rtm::vector_set(0.0F, 0.0F, shell.local_shell_distance);

				// Calculate the shell distance in parent space
				rtm::scalarf parent_shell_distance = rtm::scalar_set(0.0F);
				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					const rtm::quatf raw_rotation = segment_bone_stream.rotations.get_sample_clamped(sample_index);
					const rtm::vector4f raw_translation = segment_bone_stream.translations.get_sample_clamped(sample_index);
					const rtm::vector4f raw_scale = segment_bone_stream.scales.get_sample_clamped(sample_index);

					rtm::qvvf raw_transform = rtm::qvv_set(raw_rotation, raw_translation, raw_scale);

					if (has_additive_base)
					{
						// If we are additive, we must apply our local transform on the base to figure out
						// the true shell distance
						const segment_context& base_segment = additive_base_clip_context.segments[0];
						const transform_streams& base_bone_stream = base_segment.bone_streams[transform_index];

						// The sample time is calculated from the full clip duration to be consistent with decompression
						const float sample_time = rtm::scalar_min(float(sample_index + segment.clip_sample_offset) / owner_clip_context.sample_rate, owner_clip_context.duration);

						const float normalized_sample_time = base_segment.num_samples > 1 ? (sample_time / owner_clip_context.duration) : 0.0F;
						const float additive_sample_time = base_segment.num_samples > 1 ? (normalized_sample_time * additive_base_clip_context.duration) : 0.0F;

						// With uniform sample distributions, we do not interpolate.
						const uint32_t base_sample_index = get_uniform_sample_key(base_segment, additive_sample_time);

						const rtm::quatf base_rotation = base_bone_stream.rotations.get_sample_clamped(base_sample_index);
						const rtm::vector4f base_translation = base_bone_stream.translations.get_sample_clamped(base_sample_index);
						const rtm::vector4f base_scale = base_bone_stream.scales.get_sample_clamped(base_sample_index);

						const rtm::qvvf base_transform = rtm::qvv_set(base_rotation, base_translation, base_scale);
						raw_transform = apply_additive_to_base(owner_clip_context.additive_format, base_transform, raw_transform);
					}

					const rtm::vector4f raw_vtx0 = rtm::qvv_mul_point3(vtx0, raw_transform);
					const rtm::vector4f raw_vtx1 = rtm::qvv_mul_point3(vtx1, raw_transform);
					const rtm::vector4f raw_vtx2 = rtm::qvv_mul_point3(vtx2, raw_transform);

					const rtm::scalarf vtx0_distance = rtm::vector_length3_as_scalar(raw_vtx0);
					const rtm::scalarf vtx1_distance = rtm::vector_length3_as_scalar(raw_vtx1);
					const rtm::scalarf vtx2_distance = rtm::vector_length3_as_scalar(raw_vtx2);

					const rtm::scalarf transform_length = rtm::scalar_max(rtm::scalar_max(vtx0_distance, vtx1_distance), vtx2_distance);
					parent_shell_distance = rtm::scalar_max(parent_shell_distance, transform_length);
				}

				shell.parent_shell_distance = rtm::scalar_cast(parent_shell_distance);

				const transform_metadata& metadata = owner_clip_context.metadata[transform_index];

				// Add precision since we want to make sure to encompass the maximum amount of error allowed
				// Add it only for non-dominant transforms to account for the error they introduce
				// Dominant transforms will use their own precision
				// If our shell distance has changed, we are non-dominant since a dominant child updated it
				if (shell.local_shell_distance != metadata.shell_distance)
					shell.parent_shell_distance += metadata.precision;

				if (metadata.parent_index != k_invalid_track_index)
				{
					// We have a parent, propagate our shell distance if we are a dominant transform
					// We are a dominant transform if our shell distance in parent space is larger
					// than our parent's shell distance in local space. Otherwise, if we are smaller
					// or equal, it means that the full range of motion of our transform fits within
					// the parent's shell distance.

					rigid_shell_metadata_t& parent_shell = out_shell_metadata[metadata.parent_index];

					if (shell.parent_shell_distance > parent_shell.local_shell_distance)
					{
						// We are the new dominant transform, use our shell distance and precision
						parent_shell.local_shell_distance = shell.parent_shell_distance;
						parent_shell.precision = shell.precision;
					}
				}
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
