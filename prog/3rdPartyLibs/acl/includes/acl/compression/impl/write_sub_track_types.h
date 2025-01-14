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
#include "acl/core/error.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"
#include "acl/compression/impl/clip_context.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t write_packed_sub_track_types(const clip_context& clip, packed_sub_track_types* out_packed_types, const uint32_t* output_bone_mapping, uint32_t num_output_bones)
		{
			ACL_ASSERT(out_packed_types != nullptr, "'out_packed_types' cannot be null!");

			// Only use the first segment, it contains the necessary information
			const segment_context& segment = clip.segments[0];

			uint32_t packed_entry_index = 0;
			uint32_t packed_entry_size = 0;
			uint32_t packed_entry = 0;

			// Write rotations
			for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
			{
				if (packed_entry_size == 16)
				{
					// We are starting a new entry, write our old one out
					out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
					packed_entry_size = 0;
					packed_entry = 0;
				}

				const uint32_t bone_index = output_bone_mapping[output_index];
				const transform_streams& bone_stream = segment.bone_streams[bone_index];

				uint32_t packed_type;
				if (bone_stream.is_rotation_default)
					packed_type = 0;	// Default
				else if (bone_stream.is_rotation_constant)
					packed_type = 1;	// Constant
				else
					packed_type = 2;	// Animated

				const uint32_t packed_index = output_index % 16;
				packed_entry |= packed_type << ((15 - packed_index) * 2);
				packed_entry_size++;
			}

			if (packed_entry_size != 0)
			{
				// We have a partial entry, write it out
				out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
				packed_entry_size = 0;
				packed_entry = 0;
			}

			// Write translations
			for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
			{
				if (packed_entry_size == 16)
				{
					// We are starting a new entry, write our old one out
					out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
					packed_entry_size = 0;
					packed_entry = 0;
				}

				const uint32_t bone_index = output_bone_mapping[output_index];
				const transform_streams& bone_stream = segment.bone_streams[bone_index];

				uint32_t packed_type;
				if (bone_stream.is_translation_default)
					packed_type = 0;	// Default
				else if (bone_stream.is_translation_constant)
					packed_type = 1;	// Constant
				else
					packed_type = 2;	// Animated

				const uint32_t packed_index = output_index % 16;
				packed_entry |= packed_type << ((15 - packed_index) * 2);
				packed_entry_size++;
			}

			if (packed_entry_size != 0)
			{
				// We have a partial entry, write it out
				out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
				packed_entry_size = 0;
				packed_entry = 0;
			}

			if (clip.has_scale)
			{
				// Write translations
				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					if (packed_entry_size == 16)
					{
						// We are starting a new entry, write our old one out
						out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
						packed_entry_size = 0;
						packed_entry = 0;
					}

					const uint32_t bone_index = output_bone_mapping[output_index];
					const transform_streams& bone_stream = segment.bone_streams[bone_index];

					uint32_t packed_type;
					if (bone_stream.is_scale_default)
						packed_type = 0;	// Default
					else if (bone_stream.is_scale_constant)
						packed_type = 1;	// Constant
					else
						packed_type = 2;	// Animated

					const uint32_t packed_index = output_index % 16;
					packed_entry |= packed_type << ((15 - packed_index) * 2);
					packed_entry_size++;
				}

				if (packed_entry_size != 0)
				{
					// We have a partial entry, write it out
					out_packed_types[packed_entry_index++] = packed_sub_track_types{ packed_entry };
					packed_entry_size = 0;
					packed_entry = 0;
				}
			}

			return packed_entry_index * sizeof(packed_sub_track_types);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
