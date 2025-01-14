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

// Included only once from compress.h

#include "acl/version.h"
#include "acl/core/buffer_tag.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error.h"
#include "acl/core/error_result.h"
#include "acl/core/floating_point_exceptions.h"
#include "acl/core/iallocator.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/output_stats.h"
#include "acl/compression/track_array.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline error_result compress_track_list(iallocator& allocator, const track_array& track_list, const compression_settings& settings, compressed_tracks*& out_compressed_tracks, output_stats& out_stats)
	{
		using namespace acl_impl;

		error_result result = track_list.is_valid();
		if (result.any())
			return result;

		// Disable floating point exceptions during compression because we leverage all SIMD lanes
		// and we might intentionally divide by zero, etc.
		scope_disable_fp_exceptions fp_off;

		if (track_list.get_track_category() == track_category8::transformf)
			result = compress_transform_track_list(allocator, track_array_cast<track_array_qvvf>(track_list), settings, nullptr, additive_clip_format8::none, out_compressed_tracks, out_stats);
		else
			result = compress_scalar_track_list(allocator, track_list, settings, out_compressed_tracks, out_stats);

		return result;
	}

	inline error_result compress_track_list(iallocator& allocator, const track_array_qvvf& track_list, const compression_settings& settings, const track_array_qvvf& additive_base_track_list, additive_clip_format8 additive_format, compressed_tracks*& out_compressed_tracks, output_stats& out_stats)
	{
		using namespace acl_impl;

		error_result result = track_list.is_valid();
		if (result.any())
			return result;

		if (additive_format != additive_clip_format8::none)
		{
			result = additive_base_track_list.is_valid();
			if (result.any())
				return result;
		}

		// Disable floating point exceptions during compression because we leverage all SIMD lanes
		// and we might intentionally divide by zero, etc.
		scope_disable_fp_exceptions fp_off;

		return compress_transform_track_list(allocator, track_list, settings, &additive_base_track_list, additive_format, out_compressed_tracks, out_stats);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
