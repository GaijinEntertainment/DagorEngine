#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Animation Compression Library contributors
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

#if defined(ACL_USE_SJSON)

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"
#include "acl/core/scope_profiler.h"
#include "acl/compression/impl/track_list_context.h"
#include "acl/compression/output_stats.h"

#include <sjson/writer.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline void write_compression_stats(const track_list_context& context, const compressed_tracks& tracks, const scope_profiler& compression_time, output_stats& stats)
		{
			ACL_ASSERT(stats.writer != nullptr, "Attempted to log stats without a writer");
			if (stats.writer == nullptr)
				return;

			const uint32_t raw_size = context.reference_list->get_raw_size();
			const uint32_t compressed_size = tracks.get_size();
			const double compression_ratio = double(raw_size) / double(compressed_size);

			sjson::ObjectWriter& writer = *stats.writer;
			writer["algorithm_name"] = get_algorithm_name(algorithm_type8::uniformly_sampled);
			//writer["algorithm_uid"] = settings.get_hash();
			writer["clip_name"] = context.reference_list->get_name().c_str();
			writer["raw_size"] = raw_size;
			writer["compressed_size"] = compressed_size;
			writer["compression_ratio"] = compression_ratio;
			writer["compression_time"] = compression_time.get_elapsed_seconds();
			writer["duration"] = context.duration;
			writer["num_samples"] = context.num_samples;
			writer["num_tracks"] = context.num_tracks;
			writer["looping"] = tracks.get_looping_policy() == sample_looping_policy::wrap;
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP

#endif	// #if defined(ACL_USE_SJSON)
