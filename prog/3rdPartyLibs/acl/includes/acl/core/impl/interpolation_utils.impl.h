#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2018 Nicholas Frechette & Animation Compression Library contributors
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

// Included only once from interpolation_utils.h

#include "acl/version.h"
#include "acl/core/error.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/sample_rounding_policy.h"
#include "acl/core/time_utils.h"
#include "acl/core/impl/compiler_utils.h"

#include <rtm/scalarf.h>

#include <cstdint>
#include <algorithm>

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
	inline void find_linear_interpolation_samples_with_duration(
		uint32_t num_samples,
		float duration,
		float sample_time,
		sample_rounding_policy rounding_policy,
		sample_looping_policy looping_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha)
	{
		// Samples are evenly spaced, trivially calculate the indices that we need
		ACL_ASSERT(duration >= 0.0F && rtm::scalar_is_finite(duration), "Invalid duration: %f", duration);
		ACL_ASSERT(sample_time >= 0.0F && sample_time <= duration, "Invalid sample time: 0.0 <= %f <= %f", sample_time, duration);
		ACL_ASSERT(num_samples > 0, "Invalid num_samples: %u", num_samples);
		ACL_ASSERT(looping_policy != sample_looping_policy::as_compressed, "As compressed looping policy is not supported");

		const uint32_t last_sample_index = num_samples - 1;

		float sample_rate;
		if (duration == 0.0F)
			sample_rate = 0.0F;
		else if (looping_policy == sample_looping_policy::clamp)
			sample_rate = float(last_sample_index) / duration;
		else
			sample_rate = float(num_samples) / duration;	// Duration includes our repeating first sample
		ACL_ASSERT(sample_rate >= 0.0F && rtm::scalar_is_finite(sample_rate), "Invalid sample_rate: %f", sample_rate);

		float sample_index = sample_time * sample_rate;

		uint32_t sample_index0 = static_cast<uint32_t>(sample_index);
		const uint32_t next_sample_index = sample_index0 + 1;

		uint32_t sample_index1;
		if (looping_policy == sample_looping_policy::clamp)
		{
			sample_index1 = (std::min)(next_sample_index, last_sample_index);
			ACL_ASSERT(sample_index0 <= sample_index1 && sample_index1 < num_samples, "Invalid sample indices: 0 <= %u <= %u < %u", sample_index0, sample_index1, num_samples);
		}
		else
		{
			if (sample_index0 > last_sample_index)
			{
				// We are sampling our repeating first sample with full weight
				ACL_ASSERT(rtm::scalar_near_equal(sample_time, duration, 0.00001F), "Sampling our repeating first sample with full weight");
				sample_index = 0.0F;
				sample_index0 = 0;
				sample_index1 = 0;
			}
			else
			{
				sample_index1 = next_sample_index >= num_samples ? 0 : next_sample_index;
				ACL_ASSERT(sample_index0 < num_samples&& sample_index1 < num_samples, "Invalid sample indices: %u, %u, %u", sample_index0, sample_index1, num_samples);
			}
		}

		const float interpolation_alpha = sample_index - float(sample_index0);
		ACL_ASSERT(interpolation_alpha >= 0.0F && interpolation_alpha <= 1.0F, "Invalid interpolation alpha: 0.0 <= %f <= 1.0", interpolation_alpha);

		out_sample_index0 = sample_index0;
		out_sample_index1 = sample_index1;
		out_interpolation_alpha = apply_rounding_policy(interpolation_alpha, rounding_policy);
	}

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	// If the sample rate is available, prefer using find_linear_interpolation_samples_with_sample_rate
	// instead. It is faster and more accurate.
	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	inline void find_linear_interpolation_samples_with_duration(
		uint32_t num_samples,
		float duration,
		float sample_time,
		sample_rounding_policy rounding_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha)
	{
		find_linear_interpolation_samples_with_duration(num_samples, duration, sample_time, rounding_policy, sample_looping_policy::clamp, out_sample_index0, out_sample_index1, out_interpolation_alpha);
	}

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	inline void find_linear_interpolation_samples_with_sample_rate(
		uint32_t num_samples,
		float sample_rate,
		float sample_time,
		sample_rounding_policy rounding_policy,
		sample_looping_policy looping_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha)
	{
		// Samples are evenly spaced, trivially calculate the indices that we need
		ACL_ASSERT(sample_rate >= 0.0F, "Invalid sample rate: %f", sample_rate);
		ACL_ASSERT(num_samples > 0, "Invalid num_samples: %u", num_samples);
		ACL_ASSERT(looping_policy != sample_looping_policy::as_compressed, "As compressed looping policy is not supported");

		// TODO: Would it be faster to do the index calculation entirely with floating point?
		// SSE4 can floor with a single instruction.
		// We don't need the index1, there are no dependencies there, we can still convert the float index0 into an integer, do the min.
		// This would break the dependency chains, right now the index0 depends on sample_index and interpolation_alpha depends on index0.
		// Generating index0 is slow, and converting it back to float is slow.
		// If we keep index0 as a float and floor it as a float, we can calculate index1 at the same time as the interpolation alpha.

		const uint32_t last_sample_index = num_samples - 1;

		float sample_index = sample_time * sample_rate;

		uint32_t sample_index0 = static_cast<uint32_t>(sample_index);
		const uint32_t next_sample_index = sample_index0 + 1;

		uint32_t sample_index1;
		if (looping_policy == sample_looping_policy::clamp)
		{
			sample_index1 = (std::min)(next_sample_index, last_sample_index);
			ACL_ASSERT(sample_index0 <= sample_index1 && sample_index1 < num_samples, "Invalid sample indices: 0 <= %u <= %u < %u", sample_index0, sample_index1, num_samples);
		}
		else
		{
			if (sample_index0 > last_sample_index)
			{
				// We are sampling our repeating first sample with full weight
				ACL_ASSERT(rtm::scalar_near_equal(sample_time, calculate_finite_duration(num_samples + 1, sample_rate), 0.00001F), "Sampling our repeating first sample with full weight");
				sample_index = 0.0F;
				sample_index0 = 0;
				sample_index1 = 0;
			}
			else
			{
				sample_index1 = next_sample_index >= num_samples ? 0 : next_sample_index;
				ACL_ASSERT(sample_index0 < num_samples&& sample_index1 < num_samples, "Invalid sample indices: %u, %u, %u", sample_index0, sample_index1, num_samples);
			}
		}

		const float interpolation_alpha = sample_index - float(sample_index0);
		ACL_ASSERT(interpolation_alpha >= 0.0F && interpolation_alpha <= 1.0F, "Invalid interpolation alpha: 0.0 <= %f <= 1.0", interpolation_alpha);

		out_sample_index0 = sample_index0;
		out_sample_index1 = sample_index1;
		out_interpolation_alpha = apply_rounding_policy(interpolation_alpha, rounding_policy);
	}

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// The returned sample indices are clamped and do not loop.
	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	inline void find_linear_interpolation_samples_with_sample_rate(
		uint32_t num_samples,
		float sample_rate,
		float sample_time,
		sample_rounding_policy rounding_policy,
		uint32_t& out_sample_index0,
		uint32_t& out_sample_index1,
		float& out_interpolation_alpha)
	{
		find_linear_interpolation_samples_with_sample_rate(num_samples, sample_rate, sample_time, rounding_policy, sample_looping_policy::clamp, out_sample_index0, out_sample_index1, out_interpolation_alpha);
	}

	//////////////////////////////////////////////////////////////////////////
	// Calculates the sample indices and the interpolation required to linearly
	// interpolate when the samples are uniform.
	// This function does not support looping.
	inline float find_linear_interpolation_alpha(float sample_index, uint32_t sample_index0, uint32_t sample_index1, sample_rounding_policy rounding_policy, sample_looping_policy looping_policy)
	{
		ACL_ASSERT(sample_index >= 0.0F, "Invalid sample rate: %f", sample_index);
		ACL_ASSERT(looping_policy != sample_looping_policy::as_compressed, "As compressed looping policy is not supported");
		(void)looping_policy;

		if (rounding_policy == sample_rounding_policy::floor)
			return 0.0F;
		else if (rounding_policy == sample_rounding_policy::ceil)
			return 1.0F;
		else if (sample_index0 == sample_index1)
			return 0.0F;

		ACL_ASSERT(sample_index0 < sample_index1 || (looping_policy == sample_looping_policy::wrap && sample_index1 == 0), "Invalid sample indices: %u >= %u", sample_index0, sample_index1);

		float interpolation_alpha;
		if (sample_index0 < sample_index1)
			interpolation_alpha = (sample_index - float(sample_index0)) / float(sample_index1 - sample_index0);
		else
			interpolation_alpha = (sample_index - float(sample_index0));

		ACL_ASSERT(interpolation_alpha >= 0.0F && interpolation_alpha <= 1.0F, "Invalid interpolation alpha: 0.0 <= %f <= 1.0", interpolation_alpha);

		// If we don't round, we'll interpolate and we need the alpha value unchanged
		// If we require the value per track, we might need the alpha value unchanged as well, rounding is handled later
		if (rounding_policy == sample_rounding_policy::none || rounding_policy == sample_rounding_policy::per_track)
			return interpolation_alpha;
		else // sample_rounding_policy::nearest
			return rtm::scalar_floor(interpolation_alpha + 0.5F);
	}

	ACL_DEPRECATED("Specify explicitly the sample_looping_policy, to be removed in v3.0")
	inline float find_linear_interpolation_alpha(float sample_index, uint32_t sample_index0, uint32_t sample_index1, sample_rounding_policy rounding_policy)
	{
		return find_linear_interpolation_alpha(sample_index, sample_index0, sample_index1, rounding_policy, sample_looping_policy::clamp);
	}

	inline float apply_rounding_policy(float interpolation_alpha, sample_rounding_policy policy)
	{
		switch (policy)
		{
		default:
		case sample_rounding_policy::none:
		case sample_rounding_policy::per_track:
			// If we don't round, we'll interpolate and we need the alpha value unchanged
			// If we require the value per track, we might need the alpha value unchanged as well, rounding is handled later
			return interpolation_alpha;
		case sample_rounding_policy::floor:
			return 0.0F;
		case sample_rounding_policy::ceil:
			return 1.0F;
		case sample_rounding_policy::nearest:
			return rtm::scalar_floor(interpolation_alpha + 0.5F);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
