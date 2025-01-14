#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/track_traits.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/impl/pre_process.common.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		template<track_type8 k_track_type>
		inline void pre_process_optimize_looping_scalar(track_array& track_list)
		{
			using track_trait_t = track_traits<k_track_type>;
			using track_array_type_t = track_array_typed<k_track_type>;
			using track_type_t = track_typed<k_track_type>;

			track_array_type_t& track_list_ = track_array_cast<track_array_type_t>(track_list);
			const uint32_t last_sample_index = track_list_.get_num_samples_per_track() - 1;

			bool is_looping = true;
			for (track_type_t& track_ : track_list_)
			{
				const track_desc_scalarf& desc = track_.get_description();

				const rtm::vector4f first_sample = track_trait_t::load_as_vector(&track_[0]);
				const rtm::vector4f last_sample = track_trait_t::load_as_vector(&track_[last_sample_index]);
				if (!rtm::vector_all_near_equal(first_sample, last_sample, desc.precision))
				{
					is_looping = false;
					break;
				}
			}

			if (is_looping)
			{
				for (track_type_t& track_ : track_list_)
					track_[last_sample_index] = track_[0];
			}
		}

		template<track_type8 k_track_type>
		inline void pre_process_sanitize_constant_tracks_scalar(track& track_)
		{
			using track_trait_t = track_traits<k_track_type>;
			using sample_type_t = typename track_trait_t::sample_type;
			using track_type_t = track_typed<k_track_type>;

			const uint32_t num_samples_per_track = track_.get_num_samples();
			track_type_t& track__ = track_cast<track_type_t>(track_);

			const track_desc_scalarf& desc = track_.get_description<track_desc_scalarf>();
			const float precision = desc.precision;

			rtm::vector4f min = track_trait_t::load_as_vector(&track__[0]);
			rtm::vector4f max = track_trait_t::load_as_vector(&track__[num_samples_per_track - 1]);

			for (uint32_t sample_index = 1; sample_index < num_samples_per_track; ++sample_index)
			{
				const rtm::vector4f sample = track_trait_t::load_as_vector(&track__[sample_index]);

				min = rtm::vector_min(min, sample);
				max = rtm::vector_max(max, sample);
			}

			const rtm::vector4f extent = rtm::vector_sub(max, min);
			if (rtm::vector_all_less_equal(extent, rtm::vector_set(precision)))
			{
				const sample_type_t constant_sample = track__[0];

				for (uint32_t sample_index = 1; sample_index < num_samples_per_track; ++sample_index)
					track__[sample_index] = constant_sample;
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
