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
#include "acl/core/additive_utils.h"
#include "acl/core/bitset.h"
#include "acl/core/iallocator.h"
#include "acl/core/iterator.h"
#include "acl/core/error.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/segment_context.h"
#include "acl/compression/impl/transform_clip_adapters.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		template<class clip_adapter_t>
		inline void sort_transform_indices_parent_first(
			const clip_adapter_t& clip,
			uint32_t* sorted_transforms_parent_first,
			uint32_t num_transforms)
		{
			static_assert(std::is_base_of<transform_clip_adapter_t, clip_adapter_t>::value, "Clip adapter must derive from transform_clip_adapter_t");

			// We sort our transform indices by parent first
			// If two transforms have the same parent index, we sort them by their transform index
			const auto sort_predicate = [&clip](const uint32_t lhs_transform_index, const uint32_t rhs_transform_index)
			{
				const uint32_t lhs_parent_index = clip.get_transform_parent_index(lhs_transform_index);
				const uint32_t rhs_parent_index = clip.get_transform_parent_index(rhs_transform_index);

				// If the transforms don't have the same parent, sort by the parent index
				// We add 1 to parent indices to cause the invalid index to wrap around to 0
				// since parents come first, they'll have the lowest value
				if (lhs_parent_index != rhs_parent_index)
					return (lhs_parent_index + 1) < (rhs_parent_index + 1);

				// Both transforms have the same parent, sort by their index
				return lhs_transform_index < rhs_transform_index;
			};

			std::sort(sorted_transforms_parent_first, sorted_transforms_parent_first + num_transforms, sort_predicate);
		}

		inline bool initialize_clip_context(iallocator& allocator, const track_array_qvvf& track_list, const compression_settings& settings, additive_clip_format8 additive_format, clip_context& out_clip_context)
		{
			const uint32_t num_transforms = track_list.get_num_tracks();
			const uint32_t num_samples = track_list.get_num_samples_per_track();
			const float sample_rate = track_list.get_sample_rate();
			const sample_looping_policy looping_policy = track_list.get_looping_policy();

			// Create a single segment with the whole clip
			out_clip_context.segments = allocate_type_array<segment_context>(allocator, 1);
			out_clip_context.ranges = nullptr;
			out_clip_context.metadata = allocate_type_array<transform_metadata>(allocator, num_transforms);
			out_clip_context.leaf_transform_chains = nullptr;
			out_clip_context.sorted_transforms_parent_first = allocate_type_array<uint32_t>(allocator, num_transforms);
			out_clip_context.clip_shell_metadata = nullptr;
			out_clip_context.contributing_error = nullptr;
			out_clip_context.num_segments = 1;
			out_clip_context.num_bones = num_transforms;
			out_clip_context.num_samples_allocated = num_samples;
			out_clip_context.num_samples = num_samples;
			out_clip_context.sample_rate = sample_rate;
			out_clip_context.duration = track_list.get_finite_duration();
			out_clip_context.looping_policy = looping_policy;
			out_clip_context.additive_format = additive_format;
			out_clip_context.are_rotations_normalized = false;
			out_clip_context.are_translations_normalized = false;
			out_clip_context.are_scales_normalized = false;
			out_clip_context.has_additive_base = additive_format != additive_clip_format8::none;
			out_clip_context.num_leaf_transforms = 0;
			out_clip_context.allocator = &allocator;

			bool are_samples_valid = true;

			segment_context& segment = out_clip_context.segments[0];

			transform_streams* bone_streams = allocate_type_array<transform_streams>(allocator, num_transforms);

			for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
			{
				const track_qvvf& track = track_list[transform_index];
				const track_desc_transformf& desc = track.get_description();

				transform_streams& bone_stream = bone_streams[transform_index];

				bone_stream.segment = &segment;
				bone_stream.bone_index = transform_index;
				bone_stream.parent_bone_index = desc.parent_index;
				bone_stream.output_index = desc.output_index;
				bone_stream.default_value = desc.default_value;

				bone_stream.rotations = rotation_track_stream(allocator, num_samples, sizeof(rtm::quatf), sample_rate, rotation_format8::quatf_full);
				bone_stream.translations = translation_track_stream(allocator, num_samples, sizeof(rtm::vector4f), sample_rate, vector_format8::vector3f_full);
				bone_stream.scales = scale_track_stream(allocator, num_samples, sizeof(rtm::vector4f), sample_rate, vector_format8::vector3f_full);

				// Constant and default detection is handled during sub-track compacting
				bone_stream.is_rotation_constant = false;
				bone_stream.is_rotation_default = false;
				bone_stream.is_translation_constant = false;
				bone_stream.is_translation_default = false;
				bone_stream.is_scale_constant = false;
				bone_stream.is_scale_default = false;

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					const rtm::qvvf& transform = track[sample_index];

					// If we request raw data and we are already normalized, retain the original value
					// otherwise we normalize for safety
					rtm::quatf rotation;
					if (settings.rotation_format != rotation_format8::quatf_full || !rtm::quat_is_normalized(transform.rotation))
						rotation = rtm::quat_normalize(transform.rotation);
					else
						rotation = transform.rotation;

					are_samples_valid &= rtm::quat_is_finite(rotation);
					are_samples_valid &= rtm::vector_is_finite3(transform.translation);
					are_samples_valid &= rtm::vector_is_finite3(transform.scale);

					bone_stream.rotations.set_raw_sample(sample_index, rotation);
					bone_stream.translations.set_raw_sample(sample_index, transform.translation);
					bone_stream.scales.set_raw_sample(sample_index, transform.scale);
				}

				transform_metadata& metadata = out_clip_context.metadata[transform_index];
				metadata.transform_chain = nullptr;
				metadata.parent_index = desc.parent_index;
				metadata.precision = desc.precision;
				metadata.shell_distance = desc.shell_distance;

				out_clip_context.sorted_transforms_parent_first[transform_index] = transform_index;
			}

			out_clip_context.has_scale = true;	// Scale detection is handled during sub-track compacting
			out_clip_context.decomp_touched_bytes = 0;
			out_clip_context.decomp_touched_cache_lines = 0;

			segment.bone_streams = bone_streams;
			segment.clip = &out_clip_context;
			segment.ranges = nullptr;
			segment.contributing_error = nullptr;
			segment.num_samples_allocated = num_samples;
			segment.num_samples = num_samples;
			segment.num_bones = num_transforms;
			segment.clip_sample_offset = 0;
			segment.segment_index = 0;
			segment.are_rotations_normalized = false;
			segment.are_translations_normalized = false;
			segment.are_scales_normalized = false;

			segment.animated_rotation_bit_size = 0;
			segment.animated_translation_bit_size = 0;
			segment.animated_scale_bit_size = 0;
			segment.animated_pose_bit_size = 0;
			segment.animated_data_size = 0;
			segment.range_data_size = 0;
			segment.total_header_size = 0;

			// Initialize our hierarchy information
			if (num_transforms != 0)
			{
				// Calculate which bones are leaf bones that have no children
				bitset_description bone_bitset_desc = bitset_description::make_from_num_bits(num_transforms);
				const size_t bitset_size = bone_bitset_desc.get_size();
				uint32_t* is_leaf_bitset = allocate_type_array<uint32_t>(allocator, bitset_size);
				bitset_reset(is_leaf_bitset, bone_bitset_desc, false);

				// By default everything is marked as a leaf
				// We'll then iterate on every transform and mark their parent as non-leaf
				bitset_set_range(is_leaf_bitset, bone_bitset_desc, 0, num_transforms, true);

#if defined(ACL_HAS_ASSERT_CHECKS)
				uint32_t num_root_bones = 0;
#endif

				// Move and validate the input data
				for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
				{
					const transform_metadata& metadata = out_clip_context.metadata[transform_index];

					const bool is_root = metadata.parent_index == k_invalid_track_index;

					// If we have a parent, mark it as not being a leaf bone (it has at least one child)
					if (!is_root)
						bitset_set(is_leaf_bitset, bone_bitset_desc, metadata.parent_index, false);

#if defined(ACL_HAS_ASSERT_CHECKS)
					if (is_root)
						num_root_bones++;
#endif
				}

				const uint32_t num_leaf_transforms = bitset_count_set_bits(is_leaf_bitset, bone_bitset_desc);
				out_clip_context.num_leaf_transforms = num_leaf_transforms;

				// Build our transform chains
				// Each leaf transform is part of a unique chain
				// When a leaf is found, we assign it a new chain. We then iterate through the parent
				// transforms, adding them to the chain
				// Each non-leaf transform visited is assigned the first chain that contains it
				// This allows easy traversal between one transform and its parents
				uint32_t* leaf_transform_chains = allocate_type_array<uint32_t>(allocator, num_leaf_transforms * bitset_size);
				out_clip_context.leaf_transform_chains = leaf_transform_chains;

				uint32_t leaf_index = 0;
				for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
				{
					if (!bitset_test(is_leaf_bitset, bone_bitset_desc, transform_index))
						continue;	// Skip non-leaf bones

					uint32_t* bone_chain = leaf_transform_chains + (leaf_index * bitset_size);
					bitset_reset(bone_chain, bone_bitset_desc, false);

					uint32_t chain_bone_index = transform_index;
					while (chain_bone_index != k_invalid_track_index)
					{
						bitset_set(bone_chain, bone_bitset_desc, chain_bone_index, true);

						transform_metadata& metadata = out_clip_context.metadata[chain_bone_index];

						// We assign a bone chain the first time we find a bone that isn't part of one already
						if (metadata.transform_chain == nullptr)
							metadata.transform_chain = bone_chain;

						chain_bone_index = metadata.parent_index;
					}

					leaf_index++;
				}

				ACL_ASSERT(num_root_bones > 0, "No root bone found. The root bones must have a parent index = 0xFFFF");
				ACL_ASSERT(leaf_index == num_leaf_transforms, "Invalid number of leaf bone found");
				deallocate_type_array(allocator, is_leaf_bitset, bitset_size);

				// We sort our transform indices by parent first
				// If two transforms have the same parent index, we sort them by their transform index
				sort_transform_indices_parent_first(
					transform_clip_context_adapter_t(out_clip_context),
					out_clip_context.sorted_transforms_parent_first,
					num_transforms);
			}

			return are_samples_valid;
		}

		inline void destroy_clip_context(clip_context& context)
		{
			if (context.allocator == nullptr)
				return;	// Not initialized

			iallocator& allocator = *context.allocator;

			for (segment_context& segment : context.segment_iterator())
				destroy_segment_context(allocator, segment);

			deallocate_type_array(allocator, context.segments, context.num_segments);
			deallocate_type_array(allocator, context.ranges, context.num_bones);
			deallocate_type_array(allocator, context.metadata, context.num_bones);

			bitset_description bone_bitset_desc = bitset_description::make_from_num_bits(context.num_bones);
			deallocate_type_array(allocator, context.leaf_transform_chains, size_t(context.num_leaf_transforms) * bone_bitset_desc.get_size());

			deallocate_type_array(allocator, context.sorted_transforms_parent_first, context.num_bones);
			deallocate_type_array(allocator, context.contributing_error, context.num_samples);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
