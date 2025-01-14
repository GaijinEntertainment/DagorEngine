#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2022 Nicholas Frechette & Animation Compression Library contributors
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

	//////////////////////////////////////////////////////////////////////////
	// This enum dictates how interpolation samples are calculated based on the sample time.
	// DO NOT CHANGE THESE VALUES AS THEY ARE USED TO INDEX INTO FIXED SIZED BUFFERS THAT MAY NOT
	// ACCOMODATE ALL VALUES.
	enum class sample_rounding_policy
	{
		//////////////////////////////////////////////////////////////////////////
		// If the sample time lies between two samples, both sample indices
		// are returned and the interpolation alpha lies in between.
		none = 0,

		//////////////////////////////////////////////////////////////////////////
		// If the sample time lies between two samples, both sample indices
		// are returned and the interpolation will be 0.0.
		// 
		// Note that when this is used to sample a clip with a partially streamed database
		// the sample returned is the same that would have been had the database been
		// fully streamed in. Meaning that if we have 3 samples A, B, C and you sample
		// between B and C, normally B would be returned. If B has been moved into the
		// database and is missing, B will be reconstructed through interpolation.
		floor = 1,

		//////////////////////////////////////////////////////////////////////////
		// If the sample time lies between two samples, both sample indices
		// are returned and the interpolation will be 1.0.
		// 
		// Note that when this is used to sample a clip with a partially streamed database
		// the sample returned is the same that would have been had the database been
		// fully streamed in. Meaning that if we have 3 samples A, B, C and you sample
		// between A and B, normally B would be returned. If B has been moved into the
		// database and is missing, B will be reconstructed through interpolation.
		ceil = 2,

		//////////////////////////////////////////////////////////////////////////
		// If the sample time lies between two samples, both sample indices
		// are returned and the interpolation will be 0.0 or 1.0 depending
		// on which sample is nearest.
		// 
		// Note that this behaves similarly to floor and ceil above when used with a partially
		// streamed database.
		nearest = 3,

		//////////////////////////////////////////////////////////////////////////
		// Specifies that the rounding policy must be queried for every track independently.
		// Note that this feature can be enabled/disabled in the 'decompression_settings' struct
		// provided during decompression. Since this is a niche feature, the default settings
		// disable it.
		// 
		// WARNING: Using per track rounding may not behave as intended if a partially streamed
		// database is used. For performance reasons, unlike the modes above, only a single value
		// will be interpolated: the one at the specified sample time. This means that if we have
		// 3 samples A, B, C and you sample between B and C with 'floor', if B has been moved to
		// a database and is missing, B (interpolated) is not returned. Normally, A and C would be
		// used to interpolate at the sample time specified as such we do not calculate where B lies.
		// As a result of this, A would be returned unlike the behavior of 'floor' when used to sample
		// all tracks. This is the behavior chosen by design. During decompression, samples are
		// unpacked and interpolated before we know which track they belong to. As such, when the
		// per track mode is used, we output 4 samples (instead of just the one we need), one for
		// each possible mode above. One we know which sample we need (among the 4), we can simply
		// index with the rounding mode to grab it. This is very fast and the cost is largely hidden.
		// Supporting the same behavior as the rounding modes above when a partially streamed in
		// database is used would require us to interpolate 3 samples instead of 1 which would be
		// a lot more expensive for rotation sub-tracks. It would also add a lot of code complexity.
		// For those reasons, the behavior differs. A future task will allow tracks to be split
		// into different layers where database settings can be independent. This will allow us
		// to place special tracks into their own layer where key frames are not removed. A separate
		// task may allow us to specify per track whether the values can be interpolated or not.
		// This would allow us to detect boundary key frames (where the value changes) and retain
		// those we need to ensure that the end result is identical.
		per_track = 4,
	};

	// The number of sample rounding policies
	constexpr size_t k_num_sample_rounding_policies = 5;

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
