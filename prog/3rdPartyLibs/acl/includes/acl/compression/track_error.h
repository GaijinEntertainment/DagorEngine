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

#include "acl/version.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/track_array.h"
#include "acl/compression/transform_error_metrics.h"
#include "acl/decompression/decompress.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// A struct that contains the track index that has the worst error,
	// its error, and the sample time at which it happens.
	//////////////////////////////////////////////////////////////////////////
	struct track_error
	{
		//////////////////////////////////////////////////////////////////////////
		// The track index with the worst error.
		uint32_t index = k_invalid_track_index;

		//////////////////////////////////////////////////////////////////////////
		// The worst error for the track index.
		float error = 0.0F;

		//////////////////////////////////////////////////////////////////////////
		// The sample time that has the worst error.
		float sample_time = 0.0F;
	};

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between a raw track array and its
	// compressed tracks.
	// Supports scalar tracks only.
	//
	// Note: This function uses SFINAE to prevent it from matching when it shouldn't.
	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type> = nullptr>
	track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks, decompression_context_type& context);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between a raw track array and its
	// compressed tracks.
	// Supports scalar and transform tracks.
	//
	// Note: This function uses SFINAE to prevent it from matching when it shouldn't.
	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type> = nullptr>
	track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks, decompression_context_type& context, const itransform_error_metric& error_metric);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between a raw track array and its
	// compressed tracks.
	// Supports transform tracks with an additive base.
	//
	// Note: This function uses SFINAE to prevent it from matching when it shouldn't.
	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type> = nullptr>
	track_error calculate_compression_error(iallocator& allocator, const track_array_qvvf& raw_tracks, decompression_context_type& context, const itransform_error_metric& error_metric, const track_array_qvvf& additive_base_tracks);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between two compressed tracks instances.
	// Supports scalar tracks only.
	//
	// Note: This function uses SFINAE to prevent it from matching when it shouldn't.
	template<class decompression_context_type0, class decompression_context_type1, acl_impl::is_decompression_context<decompression_context_type0> = nullptr, acl_impl::is_decompression_context<decompression_context_type1> = nullptr>
	track_error calculate_compression_error(iallocator& allocator, decompression_context_type0& context0, decompression_context_type1& context1);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between two raw track arrays.
	// Supports scalar tracks only.
	track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks0, const track_array& raw_tracks1);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the worst compression error between two raw track arrays.
	// Supports scalar and transform tracks.
	track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks0, const track_array& raw_tracks1, const itransform_error_metric& error_metric);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/track_error.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
