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
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/track_types.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/compression/impl/track_list_context.h"

#include <rtm/mask4i.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct quantization_scales
		{
			rtm::vector4f max_value;
			rtm::vector4f inv_max_value;

			explicit quantization_scales(uint32_t num_bits)
			{
				ACL_ASSERT(num_bits > 0, "Cannot decay with 0 bits");
				ACL_ASSERT(num_bits < 31, "Attempting to decay on too many bits");

				const float max_value_ = rtm::scalar_safe_to_float((1 << num_bits) - 1);
				max_value = rtm::vector_set(max_value_);
				inv_max_value = rtm::vector_set(1.0F / max_value_);
			}
		};

		// Decays the input value through quantization by packing and unpacking a normalized input value
		inline rtm::vector4f RTM_SIMD_CALL decay_vector4_uXX(rtm::vector4f_arg0 value, const quantization_scales& scales)
		{
			using namespace rtm;

			ACL_ASSERT(vector_all_greater_equal(value, vector_zero()) && vector_all_less_equal(value, rtm::vector_set(1.0F)), "Expected normalized unsigned input value: %f, %f, %f, %f", (float)vector_get_x(value), (float)vector_get_y(value), (float)vector_get_z(value), (float)vector_get_w(value));

			const vector4f packed_value = vector_round_symmetric(vector_mul(value, scales.max_value));
			const vector4f decayed_value = vector_mul(packed_value, scales.inv_max_value);
			return decayed_value;
		}

		// Packs a normalized input value through quantization
		inline rtm::vector4f RTM_SIMD_CALL pack_vector4_uXX(rtm::vector4f_arg0 value, const quantization_scales& scales)
		{
			using namespace rtm;

			ACL_ASSERT(vector_all_greater_equal(value, vector_zero()) && vector_all_less_equal(value, rtm::vector_set(1.0F)), "Expected normalized unsigned input value: %f, %f, %f, %f", (float)vector_get_x(value), (float)vector_get_y(value), (float)vector_get_z(value), (float)vector_get_w(value));

			return vector_round_symmetric(vector_mul(value, scales.max_value));
		}

		inline void quantize_scalarf_track(track_list_context& context, uint32_t track_index)
		{
			using namespace rtm;

			const track& ref_track = (*context.reference_list)[track_index];
			track_vector4f& mut_track = track_cast<track_vector4f>(context.track_list[track_index]);

			const vector4f precision = vector_load1(&mut_track.get_description().precision);
			const uint32_t ref_element_size = ref_track.get_sample_size();
			const uint32_t num_samples = mut_track.get_num_samples();

			const scalarf_range& range = context.range_list[track_index].range.scalarf;
			const vector4f range_min = range.get_min();
			const vector4f range_extent = range.get_extent();

			const vector4f zero = vector_zero();
			const mask4f all_true_mask = mask_set(true, true, true, true);
			mask4f sample_mask = mask_set(false, false, false, false);
			std::memcpy(&sample_mask, &all_true_mask, ref_element_size);

			vector4f raw_sample = zero;
			uint8_t best_bit_rate = k_highest_bit_rate;	// Default to raw if we fail to find something better

			// First we look for the best bit rate possible that keeps us within our precision target
			for (uint8_t bit_rate = k_highest_bit_rate - 1; bit_rate != 0; --bit_rate)	// Skip the raw bit rate and the constant bit rate
			{
				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
				const quantization_scales scales(num_bits_at_bit_rate);

				bool is_error_to_high = false;
				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
				{
					std::memcpy(&raw_sample, ref_track[sample_index], ref_element_size);

					const vector4f normalized_sample = mut_track[sample_index];

					// Decay our value through quantization
					const vector4f decayed_normalized_sample = decay_vector4_uXX(normalized_sample, scales);

					// Undo normalization
					const vector4f decayed_sample = vector_mul_add(decayed_normalized_sample, range_extent, range_min);

					const vector4f delta = vector_abs(vector_sub(raw_sample, decayed_sample));
					const vector4f masked_delta = vector_select(sample_mask, delta, zero);
					if (!vector_all_less_equal(masked_delta, precision))
					{
						is_error_to_high = true;
						break;
					}
				}

				if (is_error_to_high)
					break;	// Our error is too high, use the previous bit rate

				// We were accurate enough, this is the best bit rate so far
				best_bit_rate = bit_rate;
			}

			context.bit_rate_list[track_index].scalar.value = best_bit_rate;

			// Done, update our track with the final result
			if (best_bit_rate == k_highest_bit_rate)
			{
				// We can't quantize this track, keep it raw
				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					std::memcpy(&mut_track[sample_index], ref_track[sample_index], ref_element_size);
			}
			else
			{
				// Use the selected bit rate to quantize our track
				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(best_bit_rate);
				const quantization_scales scales(num_bits_at_bit_rate);

				for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					mut_track[sample_index] = pack_vector4_uXX(mut_track[sample_index], scales);
			}
		}

		inline void quantize_tracks(track_list_context& context)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			context.bit_rate_list = allocate_type_array<track_bit_rate>(*context.allocator, context.num_tracks);

			for (uint32_t track_index = 0; track_index < context.num_tracks; ++track_index)
			{
				const bool is_track_constant = context.is_constant(track_index);
				if (is_track_constant)
					continue;	// Constant tracks don't need to be modified

				const track_range& range = context.range_list[track_index];

				switch (range.category)
				{
				case track_category8::scalarf:
					quantize_scalarf_track(context, track_index);
					break;
				case track_category8::scalard:
				case track_category8::transformf:
				case track_category8::transformd:
				default:
					ACL_ASSERT(false, "Invalid track category");
					break;
				}
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
