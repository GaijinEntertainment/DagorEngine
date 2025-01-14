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
#include "acl/core/memory_utils.h"
#include "acl/core/track_desc.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/clip_context.h"

#include <cstdint>
#include <memory>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t write_track_list_name(const track_array& tracks, char* out_track_list_name)
		{
			ACL_ASSERT(out_track_list_name == nullptr || out_track_list_name[0] == 0, "Buffer overrun detected");

			const uint8_t* output_buffer = bit_cast<uint8_t*>(out_track_list_name);
			const uint8_t* output_buffer_start = output_buffer;

			const string& name = tracks.get_name();
			const uint32_t name_size = uint32_t(name.size() + 1);	// Include null terminator too

			if (out_track_list_name != nullptr)
				std::memcpy(out_track_list_name, name.c_str(), name_size);

			output_buffer += name_size;

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_track_names(const track_array& tracks, const uint32_t* track_output_indices, uint32_t num_output_tracks, uint32_t* out_track_names)
		{
			ACL_ASSERT(out_track_names == nullptr || out_track_names[0] == 0 || num_output_tracks == 0, "Buffer overrun detected");

			uint8_t* output_buffer = bit_cast<uint8_t*>(out_track_names);
			const uint8_t* output_buffer_start = output_buffer;

			// Write offsets first
			uint32_t* track_name_offsets = out_track_names;
			uint32_t offset = sizeof(uint32_t) * num_output_tracks;
			for (uint32_t output_index = 0; output_index < num_output_tracks; ++output_index)
			{
				const uint32_t track_index = track_output_indices[output_index];
				const string& name = tracks[track_index].get_name();
				const uint32_t name_size = uint32_t(name.size() + 1);	// Include null terminator too

				if (out_track_names != nullptr)
					*track_name_offsets = offset;

				track_name_offsets++;
				output_buffer += sizeof(uint32_t);
				offset += name_size;
			}

			// Next write our track names
			char* track_names = safe_ptr_cast<char>(output_buffer);
			for (uint32_t output_index = 0; output_index < num_output_tracks; ++output_index)
			{
				const uint32_t track_index = track_output_indices[output_index];
				const string& name = tracks[track_index].get_name();
				const uint32_t name_size = uint32_t(name.size() + 1);	// Include null terminator too

				if (out_track_names != nullptr)
					std::memcpy(track_names, name.c_str(), name_size);

				track_names += name_size;
				output_buffer += name_size;
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_parent_track_indices(const track_array_qvvf& tracks, const uint32_t* track_output_indices, uint32_t num_output_tracks, uint32_t* out_parent_track_indices)
		{
			ACL_ASSERT(out_parent_track_indices == nullptr || out_parent_track_indices[0] == 0 || num_output_tracks == 0, "Buffer overrun detected");

			auto find_output_index = [track_output_indices, num_output_tracks](uint32_t track_index)
			{
				if (track_index == k_invalid_track_index)
					return k_invalid_track_index;

				for (uint32_t output_index = 0; output_index < num_output_tracks; ++output_index)
				{
					if (track_output_indices[output_index] == track_index)
						return output_index;
				}

				return k_invalid_track_index;
			};

			const uint8_t* output_buffer = bit_cast<uint8_t*>(out_parent_track_indices);
			const uint8_t* output_buffer_start = output_buffer;

			for (uint32_t output_index = 0; output_index < num_output_tracks; ++output_index)
			{
				const uint32_t track_index = track_output_indices[output_index];
				const track_qvvf& track = tracks[track_index];
				const track_desc_transformf& desc = track.get_description();
				const uint32_t parent_track_index = desc.parent_index;

				const uint32_t parent_output_index = find_output_index(parent_track_index);
				if (out_parent_track_indices != nullptr)
					out_parent_track_indices[output_index] = parent_output_index;

				output_buffer += sizeof(uint32_t);
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_track_descriptions(const track_array& tracks, const uint32_t* track_output_indices, uint32_t num_output_tracks, uint8_t* out_track_descriptions)
		{
			ACL_ASSERT(out_track_descriptions == nullptr || out_track_descriptions[0] == 0 || num_output_tracks == 0, "Buffer overrun detected");

			uint8_t* output_buffer = out_track_descriptions;
			const uint8_t* output_buffer_start = output_buffer;

			const bool is_scalar = tracks.get_track_type() != track_type8::qvvf;

			for (uint32_t output_index = 0; output_index < num_output_tracks; ++output_index)
			{
				const uint32_t track_index = track_output_indices[output_index];

				if (is_scalar)
				{
					const track_desc_scalarf& desc = tracks[track_index].get_description<track_desc_scalarf>();

					if (out_track_descriptions != nullptr)
					{
						// We don't write out the output index since the track has already been properly sorted or stripped

						float* data = bit_cast<float*>(output_buffer);
						data[0] = desc.precision;
					}

					output_buffer += sizeof(float) * 1;
				}
				else
				{
					const track_desc_transformf& desc = tracks[track_index].get_description<track_desc_transformf>();

					if (out_track_descriptions != nullptr)
					{
						// We don't write out the output index since the track has already been properly sorted or stripped
						// We don't write out the parent index since it has already been included separately

						float* data = bit_cast<float*>(output_buffer);
						data[0] = desc.precision;
						data[1] = desc.shell_distance;

						rtm::quat_store(desc.default_value.rotation, data + 2);
						rtm::vector_store3(desc.default_value.translation, data + 6);
						rtm::vector_store3(desc.default_value.scale, data + 9);
					}

					output_buffer += sizeof(float) * 12;
				}
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_contributing_error(const clip_context& clip, uint8_t* out_contributing_error)
		{
			ACL_ASSERT(out_contributing_error == nullptr || clip.num_samples == 0 || out_contributing_error[0] == 0, "Buffer overrun detected");

			const uint8_t* output_buffer = out_contributing_error;
			const uint8_t* output_buffer_start = output_buffer;
			keyframe_stripping_metadata_t* contributing_error = bit_cast<keyframe_stripping_metadata_t*>(out_contributing_error);

			for (uint32_t frame_index = 0; frame_index < clip.num_samples; ++frame_index)
			{
				if (out_contributing_error != nullptr)
					*contributing_error = clip.contributing_error[frame_index];

				contributing_error++;
				output_buffer += sizeof(keyframe_stripping_metadata_t);
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
