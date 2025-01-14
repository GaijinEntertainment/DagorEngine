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
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/compression/impl/track_list_context.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline uint32_t write_track_metadata(const track_list_context& context, track_metadata* per_track_metadata)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			const uint8_t* output_buffer = bit_cast<uint8_t*>(per_track_metadata);
			const uint8_t* output_buffer_start = output_buffer;

			for (uint32_t output_index = 0; output_index < context.num_output_tracks; ++output_index)
			{
				const uint32_t track_index = context.track_output_indices[output_index];

				if (per_track_metadata != nullptr)
					per_track_metadata[output_index].bit_rate = context.is_constant(track_index) ? static_cast<uint8_t>(0) : context.bit_rate_list[track_index].scalar.value;

				output_buffer += sizeof(track_metadata);
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_track_constant_values(const track_list_context& context, float* constant_values)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			uint8_t* output_buffer = bit_cast<uint8_t*>(constant_values);
			const uint8_t* output_buffer_start = output_buffer;

			for (uint32_t output_index = 0; output_index < context.num_output_tracks; ++output_index)
			{
				const uint32_t track_index = context.track_output_indices[output_index];

				if (!context.is_constant(track_index))
					continue;

				const track& ref_track = (*context.reference_list)[track_index];
				const uint32_t element_size = ref_track.get_sample_size();

				if (constant_values != nullptr)
					std::memcpy(output_buffer, ref_track[0], element_size);

				output_buffer += element_size;
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_track_range_values(const track_list_context& context, float* range_values)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			uint8_t* output_buffer = bit_cast<uint8_t*>(range_values);
			const uint8_t* output_buffer_start = output_buffer;

			for (uint32_t output_index = 0; output_index < context.num_output_tracks; ++output_index)
			{
				const uint32_t track_index = context.track_output_indices[output_index];

				if (context.is_constant(track_index))
					continue;

				const uint8_t bit_rate = context.bit_rate_list[track_index].scalar.value;
				if (is_raw_bit_rate(bit_rate))
					continue;

				const track& ref_track = (*context.reference_list)[track_index];
				const track_range& range = context.range_list[track_index];

				const uint32_t element_size = ref_track.get_sample_size();

				if (range_values != nullptr)
				{
					// Only support scalarf for now
					ACL_ASSERT(range.category == track_category8::scalarf, "Unsupported category");
					const rtm::vector4f range_min = range.range.scalarf.get_min();
					const rtm::vector4f range_extent = range.range.scalarf.get_extent();
					std::memcpy(output_buffer, &range_min, element_size);
					std::memcpy(output_buffer + element_size, &range_extent, element_size);
				}

				output_buffer += element_size;	// Min
				output_buffer += element_size;	// Extent
			}

			return safe_static_cast<uint32_t>(output_buffer - output_buffer_start);
		}

		inline uint32_t write_track_animated_values(const track_list_context& context, uint8_t* animated_values)
		{
			ACL_ASSERT(context.is_valid(), "Invalid context");

			uint8_t* output_buffer = animated_values;
			uint64_t output_bit_offset = 0;

			const uint32_t num_components = get_track_num_sample_elements(context.reference_list->get_track_type());
			ACL_ASSERT(num_components <= 4, "Unexpected number of elements");

			for (uint32_t sample_index = 0; sample_index < context.num_samples; ++sample_index)
			{
				for (uint32_t output_index = 0; output_index < context.num_output_tracks; ++output_index)
				{
					const uint32_t track_index = context.track_output_indices[output_index];

					if (context.is_constant(track_index))
						continue;

					const track& ref_track = (*context.reference_list)[track_index];
					const track& mut_track = context.track_list[track_index];

					// Only support scalarf for now
					ACL_ASSERT(ref_track.get_category() == track_category8::scalarf, "Unsupported category");

					const scalar_bit_rate bit_rate = context.bit_rate_list[track_index].scalar;
					const uint64_t num_bits_per_component = get_num_bits_at_bit_rate(bit_rate.value);

					const track& src_track = is_raw_bit_rate(bit_rate.value) ? ref_track : mut_track;

					const uint32_t* sample_u32 = safe_ptr_cast<const uint32_t>(src_track[sample_index]);
					const float* sample_f32 = safe_ptr_cast<const float>(src_track[sample_index]);
					for (uint32_t component_index = 0; component_index < num_components; ++component_index)
					{
						if (animated_values != nullptr)
						{
							uint32_t value;
							if (is_raw_bit_rate(bit_rate.value))
								value = byte_swap(sample_u32[component_index]);
							else
							{
								// TODO: Hacked, our values are still as floats, cast to int, shift, and byte swap
								// Ideally should be done in the cache/mutable track with SIMD
								value = safe_static_cast<uint32_t>(sample_f32[component_index]);
								value = value << (32 - num_bits_per_component);
								value = byte_swap(value);
							}

							memcpy_bits(output_buffer, output_bit_offset, &value, 0, num_bits_per_component);
						}

						output_bit_offset += num_bits_per_component;
					}
				}
			}

			return safe_static_cast<uint32_t>(output_bit_offset);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
