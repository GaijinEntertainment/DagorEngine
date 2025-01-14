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

#include "acl/version.h"
#include "acl/core/error.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/segment_context.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// We use template arguments to support lambdas and avoid using std::function since they might allocate memory
		// without using our allocator.
		// group_filter_action_type = std::function<bool(animation_track_type8 group_type, uint32_t bone_index)>
		template<typename group_filter_action_type>
		inline void get_num_sub_tracks(const segment_context& segment,
			group_filter_action_type& group_filter_action,
			uint32_t& out_num_rotation_sub_tracks, uint32_t& out_num_translation_sub_tracks, uint32_t& out_num_scale_sub_tracks)
		{
			uint32_t num_rotation_sub_tracks = 0;
			uint32_t num_translation_sub_tracks = 0;
			uint32_t num_scale_sub_tracks = 0;
			for (uint32_t bone_index = 0; bone_index < segment.num_bones; ++bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (bone_stream.output_index == k_invalid_track_index)
					continue;	// Stripped

				if (group_filter_action(animation_track_type8::rotation, bone_index))
					num_rotation_sub_tracks++;

				if (group_filter_action(animation_track_type8::translation, bone_index))
					num_translation_sub_tracks++;

				if (group_filter_action(animation_track_type8::scale, bone_index))
					num_scale_sub_tracks++;
			}

			out_num_rotation_sub_tracks = num_rotation_sub_tracks;
			out_num_translation_sub_tracks = num_translation_sub_tracks;
			out_num_scale_sub_tracks = num_scale_sub_tracks;
		}

		inline void get_num_animated_sub_tracks(const segment_context& segment,
			uint32_t& out_num_animated_rotation_sub_tracks, uint32_t& out_num_animated_translation_sub_tracks, uint32_t& out_num_animated_scale_sub_tracks)
		{
			const auto animated_group_filter_action = [&segment](animation_track_type8 group_type, uint32_t bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (group_type == animation_track_type8::rotation)
					return !bone_stream.is_rotation_constant;
				else if (group_type == animation_track_type8::translation)
					return !bone_stream.is_translation_constant;
				else
					return !bone_stream.is_scale_constant;
			};

			get_num_sub_tracks(segment, animated_group_filter_action, out_num_animated_rotation_sub_tracks, out_num_animated_translation_sub_tracks, out_num_animated_scale_sub_tracks);
		}

		// We use template arguments to support lambdas and avoid using std::function since they might allocate memory
		// without using our allocator.
		// group_filter_action_type = std::function<bool(animation_track_type8 group_type, uint32_t bone_index)>
		// group_entry_action_type = std::function<void(animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)>
		// group_flush_action_type = std::function<void(animation_track_type8 group_type, uint32_t group_size)>
		template<typename group_filter_action_type, typename group_entry_action_type, typename group_flush_action_type>
		inline void group_writer(const segment_context& segment, const uint32_t* output_bone_mapping, uint32_t num_output_bones,
			group_filter_action_type& group_filter_action, group_entry_action_type& group_entry_action, group_flush_action_type& group_flush_action)
		{
			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Groups are sorted per sub-track type. All rotation groups come first followed by translations then scales.
			// The last group of each sub-track may or may not have padding. The last group might be less than 4 sub-tracks.

			const auto group_writer_impl = [output_bone_mapping, num_output_bones, &group_filter_action, &group_entry_action, &group_flush_action](animation_track_type8 group_type)
			{
				uint32_t group_size = 0;

				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					const uint32_t bone_index = output_bone_mapping[output_index];

					if (group_filter_action(group_type, bone_index))
					{
						group_entry_action(group_type, group_size, bone_index);
						group_size++;
					}

					if (group_size == 4)
					{
						// Group full, write it out and move onto to the next group
						group_flush_action(group_type, group_size);
						group_size = 0;
					}
				}

				// If group has leftover tracks, write it out
				if (group_size != 0)
					group_flush_action(group_type, group_size);
			};

			// Output sub-tracks in natural order: rot, trans, scale
			group_writer_impl(animation_track_type8::rotation);
			group_writer_impl(animation_track_type8::translation);

			if (segment.clip->has_scale)
				group_writer_impl(animation_track_type8::scale);
		}

		// We use template arguments to support lambdas and avoid using std::function since they might allocate memory
		// without using our allocator.
		// group_filter_action_type = std::function<bool(animation_track_type8 group_type, uint32_t bone_index)>
		// group_entry_action_type = std::function<void(animation_track_type8 group_type, uint32_t group_size, uint32_t bone_index)>
		// group_flush_action_type = std::function<void(animation_track_type8 group_type, uint32_t group_size)>
		template<typename group_filter_action_type, typename group_entry_action_type, typename group_flush_action_type>
		inline void animated_group_writer(const segment_context& segment, const uint32_t* output_bone_mapping, uint32_t num_output_bones,
			group_filter_action_type& group_filter_action, group_entry_action_type& group_entry_action, group_flush_action_type& group_flush_action)
		{
			const auto animated_group_filter_action = [&segment, &group_filter_action](animation_track_type8 group_type, uint32_t bone_index)
			{
				const transform_streams& bone_stream = segment.bone_streams[bone_index];
				if (group_type == animation_track_type8::rotation)
					return !bone_stream.is_rotation_constant && group_filter_action(group_type, bone_index);
				else if (group_type == animation_track_type8::translation)
					return !bone_stream.is_translation_constant && group_filter_action(group_type, bone_index);
				else
					return !bone_stream.is_scale_constant && group_filter_action(group_type, bone_index);
			};

			group_writer(segment, output_bone_mapping, num_output_bones, animated_group_filter_action, group_entry_action, group_flush_action);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
