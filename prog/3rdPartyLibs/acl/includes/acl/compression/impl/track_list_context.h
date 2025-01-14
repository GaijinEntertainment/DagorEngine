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
#include "acl/core/iallocator.h"
#include "acl/core/bitset.h"
#include "acl/core/track_desc.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/track_range.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct scalar_bit_rate
		{
			uint8_t value;
		};

		struct qvv_bit_rate
		{
			uint8_t rotation;
			uint8_t translation;
			uint8_t scale;
		};

		union track_bit_rate
		{
			scalar_bit_rate		scalar;
			qvv_bit_rate		qvv;

			track_bit_rate() : qvv{k_invalid_bit_rate, k_invalid_bit_rate, k_invalid_bit_rate} {}
		};

		struct track_list_context
		{
			iallocator* allocator = nullptr;
			const track_array* reference_list = nullptr;

			track_array track_list;

			track_range* range_list = nullptr;
			uint32_t* constant_tracks_bitset = nullptr;
			track_bit_rate* bit_rate_list = nullptr;
			uint32_t* track_output_indices = nullptr;

			uint32_t num_tracks = 0;
			uint32_t num_output_tracks = 0;
			uint32_t num_samples = 0;
			float sample_rate = 0.0F;
			float duration = 0.0F;

			sample_looping_policy looping_policy = sample_looping_policy::non_looping;

			track_list_context() = default;

			~track_list_context()
			{
				if (allocator != nullptr)
				{
					deallocate_type_array(*allocator, range_list, num_tracks);

					const bitset_description bitset_desc = bitset_description::make_from_num_bits(num_tracks);
					deallocate_type_array(*allocator, constant_tracks_bitset, bitset_desc.get_size());

					deallocate_type_array(*allocator, bit_rate_list, num_tracks);

					deallocate_type_array(*allocator, track_output_indices, num_output_tracks);
				}
			}

			bool is_valid() const { return allocator != nullptr; }
			bool is_constant(uint32_t track_index) const { return bitset_test(constant_tracks_bitset, bitset_description::make_from_num_bits(num_tracks), track_index); }

			track_list_context(const track_list_context&) = delete;
			track_list_context(track_list_context&&) = delete;
			track_list_context& operator=(const track_list_context&) = delete;
			track_list_context& operator=(track_list_context&&) = delete;
		};

		// Promote scalar tracks to vector tracks for SIMD alignment and padding
		inline track_array copy_and_promote_track_list(iallocator& allocator, const track_array& ref_track_list, bool& out_are_samples_valid)
		{
			using namespace rtm;

			const uint32_t num_tracks = ref_track_list.get_num_tracks();
			const uint32_t num_samples = ref_track_list.get_num_samples_per_track();
			const float sample_rate = ref_track_list.get_sample_rate();
			bool are_samples_valid = true;

			track_array out_track_list(allocator, num_tracks);

			for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
			{
				const track& ref_track = ref_track_list[track_index];
				track& out_track = out_track_list[track_index];

				switch (ref_track.get_type())
				{
				case track_type8::float1f:
				{
					const track_float1f& typed_ref_track = track_cast<const track_float1f>(ref_track);
					track_vector4f track = track_vector4f::make_reserve(ref_track.get_description<track_desc_scalarf>(), allocator, num_samples, sample_rate);
					for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					{
						const vector4f sample = vector_load1(&typed_ref_track[sample_index]);
						are_samples_valid &= scalar_is_finite(vector_get_x_as_scalar(sample));
						track[sample_index] = sample;
					}
					out_track = std::move(track);
					break;
				}
				case track_type8::float2f:
				{
					const track_float2f& typed_ref_track = track_cast<const track_float2f>(ref_track);
					track_vector4f track = track_vector4f::make_reserve(ref_track.get_description<track_desc_scalarf>(), allocator, num_samples, sample_rate);
					for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					{
						const vector4f sample = vector_load2(&typed_ref_track[sample_index]);
						are_samples_valid &= vector_is_finite2(sample);
						track[sample_index] = sample;
					}
					out_track = std::move(track);
					break;
				}
				case track_type8::float3f:
				{
					const track_float3f& typed_ref_track = track_cast<const track_float3f>(ref_track);
					track_vector4f track = track_vector4f::make_reserve(ref_track.get_description<track_desc_scalarf>(), allocator, num_samples, sample_rate);
					for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					{
						const vector4f sample = vector_load3(&typed_ref_track[sample_index]);
						are_samples_valid &= vector_is_finite3(sample);
						track[sample_index] = sample;
					}
					out_track = std::move(track);
					break;
				}
				case track_type8::float4f:
				{
					const track_float4f& typed_ref_track = track_cast<const track_float4f>(ref_track);
					track_vector4f track = track_vector4f::make_reserve(ref_track.get_description<track_desc_scalarf>(), allocator, num_samples, sample_rate);
					for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					{
						const vector4f sample = vector_load(&typed_ref_track[sample_index]);
						are_samples_valid &= vector_is_finite(sample);
						track[sample_index] = sample;
					}
					out_track = std::move(track);
					break;
				}
				case track_type8::vector4f:
				{
					const track_vector4f& typed_ref_track = track_cast<const track_vector4f>(ref_track);
					track_vector4f track = track_vector4f::make_reserve(ref_track.get_description<track_desc_scalarf>(), allocator, num_samples, sample_rate);
					for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
					{
						const vector4f sample = typed_ref_track[sample_index];
						are_samples_valid &= vector_is_finite(sample);
						track[sample_index] = sample;
					}
					out_track = std::move(track);
					break;
				}
				case track_type8::qvvf:
				default:
					ACL_ASSERT(false, "Unexpected track type");
					are_samples_valid = false;
					break;
				}
			}

			out_are_samples_valid = are_samples_valid;
			return out_track_list;
		}

		inline uint32_t* create_output_track_mapping(iallocator& allocator, const track_array& track_list, uint32_t& out_num_output_tracks)
		{
			const uint32_t num_tracks = track_list.get_num_tracks();
			uint32_t num_output_tracks = num_tracks;
			for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
			{
				const uint32_t output_index = track_list[track_index].get_output_index();
				if (output_index == k_invalid_track_index)
					num_output_tracks--;	// Stripped from the output
			}

			uint32_t* output_indices = allocate_type_array<uint32_t>(allocator, num_output_tracks);
			for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
			{
				const uint32_t output_index = track_list[track_index].get_output_index();
				if (output_index != k_invalid_track_index)
					output_indices[output_index] = track_index;
			}

			out_num_output_tracks = num_output_tracks;
			return output_indices;
		}

		inline bool initialize_context(iallocator& allocator, const track_array& track_list, track_list_context& context)
		{
			ACL_ASSERT(track_list.is_valid().empty(), "Invalid track list");
			ACL_ASSERT(!context.is_valid(), "Context already initialized");

			bool are_samples_valid = true;

			context.allocator = &allocator;
			context.reference_list = &track_list;
			context.track_list = copy_and_promote_track_list(allocator, track_list, are_samples_valid);
			context.range_list = nullptr;
			context.constant_tracks_bitset = nullptr;
			context.track_output_indices = nullptr;
			context.num_tracks = track_list.get_num_tracks();
			context.num_output_tracks = 0;
			context.num_samples = track_list.get_num_samples_per_track();
			context.sample_rate = track_list.get_sample_rate();
			context.duration = track_list.get_finite_duration();
			context.looping_policy = track_list.get_looping_policy();

			context.track_output_indices = create_output_track_mapping(allocator, track_list, context.num_output_tracks);

			return are_samples_valid;
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
