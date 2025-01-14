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
	// Calculate the number of samples present from a duration and sample rate.
	// Conceptually, a clip with 1 sample at any sample rate has a single static
	// pose and as such no definite duration. A clip with 2 samples at 30 FPS
	// has a sample at time 0.0 and another at time 1/30s for a duration of 1/30s.
	// We consider a 0.0 duration as having no samples, an infinite duration as
	// having 1 sample, and otherwise having at least 1 sample.
	// This does not account for the repeating first sample when the wrap
	// looping policy is used.
	// See `sample_looping_policy` for details.
	//////////////////////////////////////////////////////////////////////////
	uint32_t calculate_num_samples(float duration, float sample_rate);

	//////////////////////////////////////////////////////////////////////////
	// Calculate a clip duration from its number of samples and sample rate.
	// Conceptually, a clip with 1 sample at any sample rate has a single static
	// pose and as such no definite duration. A clip with 2 samples at 30 FPS
	// has a sample at time 0.0 and another at time 1/30s for a duration of 1/30s.
	// We consider a 0.0 duration as having no samples, an infinite duration as
	// having 1 sample, and otherwise having at least 1 sample.
	// This does not account for the repeating first sample when the wrap
	// looping policy is used.
	// See `sample_looping_policy` for details.
	//////////////////////////////////////////////////////////////////////////
	float calculate_duration(uint32_t num_samples, float sample_rate);

	//////////////////////////////////////////////////////////////////////////
	// Calculate a clip duration from its number of samples and sample rate
	// and returns a finite duration suitable for clamping.
	// This does not account for the repeating first sample when the wrap
	// looping policy is used.
	// See `sample_looping_policy` for details.
	//////////////////////////////////////////////////////////////////////////
	float calculate_finite_duration(uint32_t num_samples, float sample_rate);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/core/impl/time_utils.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
