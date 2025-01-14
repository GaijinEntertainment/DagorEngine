#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2024 Nicholas Frechette & Animation Compression Library contributors
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

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// Holds compression timing stats for diagnostics
		struct compression_stats_t
		{
#if defined(ACL_USE_SJSON)
			// Total compression time
			double total_elapsed_seconds = 0.0;

			// High level passes
			double initialization_elapsed_seconds = 0.0;
			double optimize_looping_elapsed_seconds = 0.0;
			double convert_rotations_elapsed_seconds = 0.0;
			double extract_clip_ranges_elapsed_seconds = 0.0;
			double compact_constant_sub_tracks_elapsed_seconds = 0.0;
			double normalize_clip_elapsed_seconds = 0.0;
			double segmenting_elapsed_seconds = 0.0;
			double extract_segment_ranges_elapsed_seconds = 0.0;
			double normalize_segment_elapsed_seconds = 0.0;
			double bit_rate_optimization_elapsed_seconds = 0.0;
			double keyframe_stripping_elapsed_seconds = 0.0;
			double output_packing_elapsed_seconds = 0.0;
#endif
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
