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
#include "acl/core/scope_profiler.h"
#include "acl/core/track_formats.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/compression_stats.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline rtm::vector4f RTM_SIMD_CALL convert_rotation(rtm::vector4f_arg0 rotation, rotation_format8 from, rotation_format8 to)
		{
			ACL_ASSERT(from == rotation_format8::quatf_full, "Source rotation format must be a full precision quaternion");
			(void)from;

			const rotation_format8 high_precision_format = get_rotation_variant(to) == rotation_variant8::quat ? rotation_format8::quatf_full : rotation_format8::quatf_drop_w_full;
			switch (high_precision_format)
			{
			case rotation_format8::quatf_full:
				// Original format, nothing to do
				return rotation;
			case rotation_format8::quatf_drop_w_full:
				// Drop W, we just ensure it is positive and write it back, the W component can be ignored afterwards
				return rtm::quat_to_vector(rtm::quat_ensure_positive_w(rtm::vector_to_quat(rotation)));
			case rotation_format8::quatf_drop_w_variable:
			default:
				ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(to));
				return rotation;
			}
		}

		inline void convert_rotation_streams(iallocator& allocator, segment_context& segment, rotation_format8 rotation_format)
		{
			const rotation_format8 high_precision_format = get_rotation_variant(rotation_format) == rotation_variant8::quat ? rotation_format8::quatf_full : rotation_format8::quatf_drop_w_full;

			for (transform_streams& bone_stream : segment.bone_iterator())
			{
				// We convert our rotation stream in place. We assume that the original format is quatf_full stored as rtm::quatf
				// For all other formats, we keep the same sample size and either keep Quat_32 or use rtm::vector4f
				ACL_ASSERT(bone_stream.rotations.get_sample_size() == sizeof(rtm::quatf), "Unexpected rotation sample size. %u != %zu", bone_stream.rotations.get_sample_size(), sizeof(rtm::quatf));

				const uint32_t num_samples = bone_stream.rotations.get_num_samples();
				const float sample_rate = bone_stream.rotations.get_sample_rate();
				rotation_track_stream converted_stream(allocator, num_samples, sizeof(rtm::quatf), sample_rate, high_precision_format);

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					rtm::quatf rotation = bone_stream.rotations.get_raw_sample<rtm::quatf>(sample_index);

					switch (high_precision_format)
					{
					case rotation_format8::quatf_full:
						// Original format, nothing to do
						break;
					case rotation_format8::quatf_drop_w_full:
						// Drop W, we just ensure it is positive and write it back, the W component can be ignored afterwards
						rotation = rtm::quat_ensure_positive_w(rotation);
						break;
					case rotation_format8::quatf_drop_w_variable:
					default:
						ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(high_precision_format));
						break;
					}

					converted_stream.set_raw_sample(sample_index, rotation);
				}

				bone_stream.rotations = std::move(converted_stream);
			}
		}

		inline void convert_rotation_streams(
			iallocator& allocator,
			clip_context& context,
			rotation_format8 rotation_format,
			compression_stats_t& compression_stats)
		{
			(void)compression_stats;

#if defined(ACL_USE_SJSON)
			scope_profiler convert_rotations_time;
#endif

			for (segment_context& segment : context.segment_iterator())
				convert_rotation_streams(allocator, segment, rotation_format);

#if defined(ACL_USE_SJSON)
			compression_stats.convert_rotations_elapsed_seconds = convert_rotations_time.get_elapsed_seconds();
#endif
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
