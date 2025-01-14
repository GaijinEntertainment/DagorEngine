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
#include "acl/core/error.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/sample_rounding_policy.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	// If the sample rate is available, prefer using find_linear_interpolation_samples_with_sample_rate
	// instead. It is faster and more accurate.
	// If the wrap looping policy is used:
	//     - the `num_samples` value must be the actual number of samples
	//       stored in memory, it must not include the artificially repeating first sample.
	//     - the duration value must include the repeating first sample
	void find_linear_interpolation_samples_with_duration(
		uint32_t num_samples,
		float duration,
		float sample_time,
		sample_rounding_policy rounding_policy,
		sample_looping_policy looping_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	// If the sample rate is available, prefer using find_linear_interpolation_samples_with_sample_rate
	// instead. It is faster and more accurate.
	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	void find_linear_interpolation_samples_with_duration(
		uint32_t num_samples,
		float duration,
		float sample_time,
		sample_rounding_policy rounding_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	void find_linear_interpolation_samples_with_sample_rate(
		uint32_t num_samples,
		float sample_rate,
		float sample_time,
		sample_rounding_policy rounding_policy,
		sample_looping_policy looping_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	void find_linear_interpolation_samples_with_sample_rate(
		uint32_t num_samples,
		float sample_rate,
		float sample_time,
		sample_rounding_policy rounding_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// This function does not support looping.
	float find_linear_interpolation_alpha(float sample_index, uint32_t sample_index0, uint32_t sample_index1, sample_rounding_policy rounding_policy, sample_looping_policy looping_policy);

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// This function does not support looping.
	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	float find_linear_interpolation_alpha(float sample_index, uint32_t sample_index0, uint32_t sample_index1, sample_rounding_policy rounding_policy);

	//////////////////////////////////////////////////////////////////////////
	// Modify an interpolation alpha value given a sample rounding policy
	float apply_rounding_policy(float interpolation_alpha, sample_rounding_policy policy);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/core/impl/interpolation_utils.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
