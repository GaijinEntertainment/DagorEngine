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
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/clip_context.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// Interface for transform clip adapters
		// A transform clip adapter offers an API to query transform clip data
		// Derived types should implement all relevant APIs to be compatible
		struct transform_clip_adapter_t
		{
			bool is_valid() const { return false; }

			// Clip wide metadata
			uint32_t get_num_transforms() const { return 0; }
			uint32_t get_num_samples() const { return 0; }
			float get_sample_rate() const { return 0.0F; }
			float get_duration() const { return 0.0F; }
			bool has_scale() const { return false; }
			bool has_additive_base() const { return false; }
			additive_clip_format8 get_additive_format() const { return additive_clip_format8::none; }
			const uint32_t* get_sorted_transforms_parent_first() const { return nullptr; }

			// Per transform metadata
			float get_transform_shell_distance(uint32_t transform_index) const { (void)transform_index; return 0.0F; }
			float get_transform_precision(uint32_t transform_index) const { (void)transform_index; return 0.0F; }
			uint32_t get_transform_parent_index(uint32_t transform_index) const { (void)transform_index; return k_invalid_track_index; }

			// Per transform samples
			rtm::quatf RTM_SIMD_CALL get_transform_rotation(uint32_t transform_index, uint32_t sample_index) const { (void)transform_index; (void)sample_index; return rtm::quat_identity(); }
			rtm::vector4f RTM_SIMD_CALL get_transform_translation(uint32_t transform_index, uint32_t sample_index) const { (void)transform_index; (void)sample_index; return rtm::vector_zero(); }
			rtm::vector4f RTM_SIMD_CALL get_transform_scale(uint32_t transform_index, uint32_t sample_index) const { (void)transform_index; (void)sample_index; return rtm::vector_zero(); }
		};

		// Interface for transform clip segment adapters
		// A transform segment adapter offers an API to query transform clip segment data
		// Derived types should implement all relevant APIs to be compatible
		struct transform_segment_adapter_t
		{
			bool is_valid() const { return false; }

			// Segment wide metadata
			uint32_t get_num_transforms() const { return 0; }
			uint32_t get_num_samples() const { return 0; }
			float get_sample_rate() const { return 0.0F; }
			float get_duration() const { return 0.0F; }
			uint32_t get_start_offset() const { return 0; }
		};

		struct transform_clip_context_adapter_t final : transform_clip_adapter_t
		{
			const clip_context& context;

			explicit transform_clip_context_adapter_t(const clip_context& context_)
				: context(context_)
			{
			}

			// Can't copy or move
			transform_clip_context_adapter_t(const transform_clip_context_adapter_t&) = delete;
			transform_clip_context_adapter_t& operator=(const transform_clip_context_adapter_t&) = delete;

			bool is_valid() const { return true; }

			// Clip wide metadata
			uint32_t get_num_transforms() const { return context.num_bones; }
			uint32_t get_num_samples() const { return context.num_samples; }
			float get_sample_rate() const { return context.sample_rate; }
			float get_duration() const { return context.duration; }
			bool has_scale() const { return context.has_scale; }
			bool has_additive_base() const { return context.has_additive_base; }
			additive_clip_format8 get_additive_format() const { return context.additive_format; }
			const uint32_t* get_sorted_transforms_parent_first() const { return context.sorted_transforms_parent_first; }
			const rigid_shell_metadata_t* get_rigid_shell_metadata() const { return context.clip_shell_metadata; }

			// Per transform metadata
			float get_transform_shell_distance(uint32_t transform_index) const
			{
				return context.metadata[transform_index].shell_distance;
			}

			float get_transform_precision(uint32_t transform_index) const
			{
				return context.metadata[transform_index].precision;
			}

			uint32_t get_transform_parent_index(uint32_t transform_index) const
			{
				return context.metadata[transform_index].parent_index;
			}

			// Per transform samples
			rtm::quatf RTM_SIMD_CALL get_transform_rotation(uint32_t transform_index, uint32_t sample_index) const
			{
				const segment_context& segment = context.segments[0];
				const transform_streams& bone_stream = segment.bone_streams[transform_index];
				return bone_stream.rotations.get_sample_clamped(sample_index);
			}

			rtm::vector4f RTM_SIMD_CALL get_transform_translation(uint32_t transform_index, uint32_t sample_index) const
			{
				const segment_context& segment = context.segments[0];
				const transform_streams& bone_stream = segment.bone_streams[transform_index];
				return bone_stream.translations.get_sample_clamped(sample_index);
			}

			rtm::vector4f RTM_SIMD_CALL get_transform_scale(uint32_t transform_index, uint32_t sample_index) const
			{
				const segment_context& segment = context.segments[0];
				const transform_streams& bone_stream = segment.bone_streams[transform_index];
				return bone_stream.scales.get_sample_clamped(sample_index);
			}
		};

		struct transform_segment_context_adapter_t final : transform_segment_adapter_t
		{
			const segment_context* segment = nullptr;

			transform_segment_context_adapter_t() = default;
			explicit transform_segment_context_adapter_t(const segment_context& segment_)
				: segment(&segment_)
			{
			}

			bool is_valid() const { return segment != nullptr; }

			// Segment wide metadata
			uint32_t get_num_transforms() const { return segment->num_bones; }
			uint32_t get_num_samples() const { return segment->num_samples; }
			uint32_t get_start_offset() const { return segment->clip_sample_offset; }
		};

		struct transform_track_array_adapter_t final : transform_clip_adapter_t
		{
			// Set in constructor
			const track_array_qvvf* track_list = nullptr;
			additive_clip_format8 additive_format = additive_clip_format8::none;

			// Set manually when needed
			const uint32_t* sorted_transforms_parent_first = nullptr;
			const rigid_shell_metadata_t* rigid_shell_metadata = nullptr;

			explicit transform_track_array_adapter_t(const track_array_qvvf& track_list_)
				: track_list(&track_list_)
			{
			}

			transform_track_array_adapter_t(const track_array_qvvf* track_list_, additive_clip_format8 additive_format_)
				: track_list(track_list_)
				, additive_format(additive_format_)
			{
			}

			bool is_valid() const { return track_list != nullptr; }

			// Clip wide metadata
			uint32_t get_num_transforms() const { return track_list->get_num_tracks(); }
			uint32_t get_num_samples() const { return track_list->get_num_samples_per_track(); }
			float get_sample_rate() const { return track_list->get_sample_rate(); }
			float get_duration() const { return track_list->get_finite_duration(); }
			bool has_scale() const { return true; }
			bool has_additive_base() const { return additive_format != additive_clip_format8::none; }
			additive_clip_format8 get_additive_format() const { return additive_format; }
			const uint32_t* get_sorted_transforms_parent_first() const { return sorted_transforms_parent_first; }
			const rigid_shell_metadata_t* get_rigid_shell_metadata() const { return rigid_shell_metadata; }

			// Per transform metadata
			float get_transform_shell_distance(uint32_t transform_index) const
			{
				const track_desc_transformf& desc = (*track_list)[transform_index].get_description();
				return desc.shell_distance;
			}

			float get_transform_precision(uint32_t transform_index) const
			{
				const track_desc_transformf& desc = (*track_list)[transform_index].get_description();
				return desc.precision;
			}

			uint32_t get_transform_parent_index(uint32_t transform_index) const
			{
				const track_desc_transformf& desc = (*track_list)[transform_index].get_description();
				return desc.parent_index;
			}

			// Per transform samples
			rtm::quatf RTM_SIMD_CALL get_transform_rotation(uint32_t transform_index, uint32_t sample_index) const
			{
				sample_index = std::min<uint32_t>(sample_index, track_list->get_num_samples_per_track() - 1);
				return (*track_list)[transform_index][sample_index].rotation;
			}

			rtm::vector4f RTM_SIMD_CALL get_transform_translation(uint32_t transform_index, uint32_t sample_index) const
			{
				sample_index = std::min<uint32_t>(sample_index, track_list->get_num_samples_per_track() - 1);
				return (*track_list)[transform_index][sample_index].translation;
			}

			rtm::vector4f RTM_SIMD_CALL get_transform_scale(uint32_t transform_index, uint32_t sample_index) const
			{
				sample_index = std::min<uint32_t>(sample_index, track_list->get_num_samples_per_track() - 1);
				return (*track_list)[transform_index][sample_index].scale;
			}
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
