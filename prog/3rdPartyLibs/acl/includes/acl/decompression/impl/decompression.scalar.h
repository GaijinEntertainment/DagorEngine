#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/interpolation_utils.h"
#include "acl/core/track_writer.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/decompression/database/database.h"
#include "acl/math/scalar_packing.h"
#include "acl/math/vector4_packing.h"

#include <rtm/scalarf.h>
#include <rtm/vector4f.h>

#include <cstdint>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C26451: Arithmetic overflow: Using operator '*' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator '*' to avoid overflow (io.2).
	// We can't overflow because compressed clips cannot contain more than 4 GB worth of data
	#pragma warning(disable : 26451)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct persistent_scalar_decompression_context_v0
		{
			// Clip related data											//   offsets
			// Only member used to detect if we are initialized, must be first
			const compressed_tracks* tracks = nullptr;						//   0 |   0

			// Cached hash of the bound compressed track instance
			uint32_t tracks_hash = 0;										//   4 |   8

			// Only used when the wrap loop policy isn't supported
			float duration = 0.0F;											//   8 |  12

			// Seeking related data
			float interpolation_alpha = 0.0F;								//  12 |  16
			float sample_time = 0.0F;										//  16 |  20

			uint32_t key_frame_bit_offsets[2] = { 0 };						//  20 |  24	// Variable quantization

			uint8_t looping_policy = 0;										//  28 |  32
			uint8_t rounding_policy = 0;									//  29 |  33

			uint8_t padding_tail[sizeof(void*) == 4 ? 34 : 30] = { 0 };		//  30 |  34

			//////////////////////////////////////////////////////////////////////////

			const compressed_tracks* get_compressed_tracks() const { return tracks; }
			compressed_tracks_version16 get_version() const { return tracks->get_version(); }
			sample_looping_policy get_looping_policy() const { return static_cast<sample_looping_policy>(looping_policy); }
			sample_rounding_policy get_rounding_policy() const { return static_cast<sample_rounding_policy>(rounding_policy); }
			bool is_initialized() const { return tracks != nullptr; }
			void reset()
			{
				// Just reset the tracks pointer, this will mark us as no longer initialized indicating everything is stale
				tracks = nullptr;
			}
		};

		static_assert(sizeof(persistent_scalar_decompression_context_v0) == 64, "Unexpected size");
		static_assert(offsetof(persistent_scalar_decompression_context_v0, tracks) == 0, "tracks pointer needs to be the first member");

		template<class decompression_settings_type, class database_settings_type>
		inline bool initialize_v0(persistent_scalar_decompression_context_v0& context, const compressed_tracks& tracks, const database_context<database_settings_type>* database)
		{
			ACL_ASSERT(tracks.get_algorithm_type() == algorithm_type8::uniformly_sampled, "Invalid algorithm type [" ACL_ASSERT_STRING_FORMAT_SPECIFIER "], expected [" ACL_ASSERT_STRING_FORMAT_SPECIFIER "]", get_algorithm_name(tracks.get_algorithm_type()), get_algorithm_name(algorithm_type8::uniformly_sampled));

			if (database != nullptr)
				return false;	// Database decompression is not supported for scalar tracks

			context.tracks = &tracks;
			context.tracks_hash = tracks.get_hash();
			context.sample_time = -1.0F;

			if (decompression_settings_type::is_wrapping_supported())
			{
				context.duration = tracks.get_finite_duration();
				context.looping_policy = static_cast<uint8_t>(tracks.get_looping_policy());
			}
			else
			{
				context.duration = tracks.get_finite_duration(sample_looping_policy::clamp);
				context.looping_policy = static_cast<uint8_t>(sample_looping_policy::clamp);
			}

			return true;
		}

		template<class decompression_settings_type, class database_settings_type>
		inline bool relocated_v0(persistent_scalar_decompression_context_v0& context, const compressed_tracks& tracks, const database_context<database_settings_type>* database)
		{
			if (context.tracks_hash != tracks.get_hash())
				return false;	// Hash is different, this instance did not relocate, it is different

			if (database != nullptr)
				return false;	// Database decompression is not supported for scalar tracks

			// The instances are identical and might have relocated, update our metadata
			context.tracks = &tracks;

			// Reset the sample time to force seek() to be called again.
			context.sample_time = -1.0F;

			return true;
		}

		inline bool is_bound_to_v0(const persistent_scalar_decompression_context_v0& context, const compressed_tracks& tracks)
		{
			if (context.tracks != &tracks)
				return false;	// Different pointer, no guarantees

			if (context.tracks_hash != tracks.get_hash())
				return false;	// Different hash

			// Must be bound to it!
			return true;
		}

		inline bool is_bound_to_v0(const persistent_scalar_decompression_context_v0& context, const compressed_database& database)
		{
			// Database decompression is not supported for scalar tracks
			(void)context;
			(void)database;
			return false;
		}

		template<class decompression_settings_type>
		inline void set_looping_policy_v0(persistent_scalar_decompression_context_v0& context, sample_looping_policy policy)
		{
			if (!decompression_settings_type::is_wrapping_supported())
				return;	// Only clamping is supported

			if (policy == sample_looping_policy::as_compressed)
				policy = context.tracks->get_looping_policy();

			const sample_looping_policy current_policy = static_cast<sample_looping_policy>(context.looping_policy);
			if (current_policy != policy)
			{
				// Policy changed
				context.duration = context.tracks->get_finite_duration(policy);
				context.looping_policy = static_cast<uint8_t>(policy);
			}
		}

		template<class decompression_settings_type>
		inline void seek_v0(persistent_scalar_decompression_context_v0& context, float sample_time, sample_rounding_policy rounding_policy)
		{
			const acl_impl::tracks_header& header = acl_impl::get_tracks_header(*context.tracks);
			if (header.num_samples == 0)
				return;	// Empty track list

			// Clamp for safety, the caller should normally handle this but in practice, it often isn't the case
			if (decompression_settings_type::clamp_sample_time())
				sample_time = rtm::scalar_clamp(sample_time, 0.0F, context.duration);

			if (context.sample_time == sample_time && context.get_rounding_policy() == rounding_policy)
				return;

			context.sample_time = sample_time;

			// If the wrap looping policy isn't supported, use our statically known value
			const sample_looping_policy looping_policy_ = decompression_settings_type::is_wrapping_supported() ? static_cast<sample_looping_policy>(context.looping_policy) : sample_looping_policy::clamp;

			uint32_t key_frame0;
			uint32_t key_frame1;
			find_linear_interpolation_samples_with_sample_rate(header.num_samples, header.sample_rate, sample_time, rounding_policy, looping_policy_, key_frame0, key_frame1, context.interpolation_alpha);

			context.rounding_policy = static_cast<uint8_t>(rounding_policy);

			const acl_impl::scalar_tracks_header& scalars_header = acl_impl::get_scalar_tracks_header(*context.tracks);

			context.key_frame_bit_offsets[0] = key_frame0 * scalars_header.num_bits_per_frame;
			context.key_frame_bit_offsets[1] = key_frame1 * scalars_header.num_bits_per_frame;
		}

		template<class decompression_settings_type, class track_writer_type>
		inline void decompress_tracks_v0(const persistent_scalar_decompression_context_v0& context, track_writer_type& writer)
		{
			const acl_impl::tracks_header& header = acl_impl::get_tracks_header(*context.tracks);
			const uint32_t num_tracks = header.num_tracks;
			if (num_tracks == 0)
				return;	// Empty track list

			ACL_ASSERT(context.sample_time >= 0.0f, "Context not set to a valid sample time");
			if (context.sample_time < 0.0F)
				return;	// Invalid sample time, we didn't seek yet

			// Due to the SIMD operations, we sometimes overflow in the SIMD lanes not used.
			// Disable floating point exceptions to avoid issues.
			fp_environment fp_env;
			if (decompression_settings_type::disable_fp_exeptions())
				disable_fp_exceptions(fp_env);

			const acl_impl::scalar_tracks_header& scalars_header = acl_impl::get_scalar_tracks_header(*context.tracks);
			const rtm::scalarf interpolation_alpha = rtm::scalar_set(context.interpolation_alpha);

			const sample_rounding_policy rounding_policy = static_cast<sample_rounding_policy>(context.rounding_policy);

			float interpolation_alpha_per_policy[k_num_sample_rounding_policies] = {};
			if (decompression_settings_type::is_per_track_rounding_supported())
			{
				const float alpha = context.interpolation_alpha;
				const float no_rounding_alpha = apply_rounding_policy(alpha, sample_rounding_policy::none);

				interpolation_alpha_per_policy[static_cast<int>(sample_rounding_policy::none)] = no_rounding_alpha;
				interpolation_alpha_per_policy[static_cast<int>(sample_rounding_policy::floor)] = apply_rounding_policy(alpha, sample_rounding_policy::floor);
				interpolation_alpha_per_policy[static_cast<int>(sample_rounding_policy::ceil)] = apply_rounding_policy(alpha, sample_rounding_policy::ceil);
				interpolation_alpha_per_policy[static_cast<int>(sample_rounding_policy::nearest)] = apply_rounding_policy(alpha, sample_rounding_policy::nearest);
				// We'll assert if we attempt to use this, but in case they are skipped/disabled, we interpolate
				interpolation_alpha_per_policy[static_cast<int>(sample_rounding_policy::per_track)] = no_rounding_alpha;
			}

			const acl_impl::track_metadata* per_track_metadata = scalars_header.get_track_metadata();
			const float* constant_values = scalars_header.get_track_constant_values();
			const float* range_values = scalars_header.get_track_range_values();
			const uint8_t* animated_values = scalars_header.get_track_animated_values();

			uint32_t track_bit_offset0 = context.key_frame_bit_offsets[0];
			uint32_t track_bit_offset1 = context.key_frame_bit_offsets[1];

			const track_type8 track_type = header.track_type;

			const compressed_tracks_version16 version = context.get_version();
			const uint8_t* num_bits_at_bit_rate = version == compressed_tracks_version16::v02_00_00 ? k_bit_rate_num_bits_v0 : k_bit_rate_num_bits;

#if defined(ACL_HAS_ASSERT_CHECKS)
			const uint32_t max_bit_rate = version == compressed_tracks_version16::v02_00_00 ? sizeof(k_bit_rate_num_bits_v0) : sizeof(k_bit_rate_num_bits);
#endif

			for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
			{
				const acl_impl::track_metadata& metadata = per_track_metadata[track_index];
				const uint32_t bit_rate = metadata.bit_rate;
				ACL_ASSERT(bit_rate < max_bit_rate, "Invalid bit rate: %u", bit_rate);
				const uint32_t num_bits_per_component = num_bits_at_bit_rate[bit_rate];

				rtm::scalarf alpha = interpolation_alpha;
				if (decompression_settings_type::is_per_track_rounding_supported())
				{
					const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
					ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");

					alpha = rtm::scalar_set(interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)]);
				}

				if (track_type == track_type8::float1f && decompression_settings_type::is_track_type_supported(track_type8::float1f))
				{
					rtm::scalarf value;
					if (num_bits_per_component == 0)	// Constant bit rate
					{
						value = rtm::scalar_load_as_scalar(constant_values);
						constant_values += 1;
					}
					else
					{
						rtm::scalarf value0;
						rtm::scalarf value1;
						if (num_bits_per_component == 32)	// Raw bit rate
						{
							value0 = unpack_scalarf_32_unsafe(animated_values, track_bit_offset0);
							value1 = unpack_scalarf_32_unsafe(animated_values, track_bit_offset1);
						}
						else
						{
							value0 = unpack_scalarf_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset0);
							value1 = unpack_scalarf_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset1);

							const rtm::scalarf range_min = rtm::scalar_load_as_scalar(range_values);
							const rtm::scalarf range_extent = rtm::scalar_load_as_scalar(range_values + 1);
							value0 = rtm::scalar_mul_add(value0, range_extent, range_min);
							value1 = rtm::scalar_mul_add(value1, range_extent, range_min);
							range_values += 2;
						}

						value = rtm::scalar_lerp(value0, value1, alpha);

						const uint32_t num_sample_bits = num_bits_per_component;
						track_bit_offset0 += num_sample_bits;
						track_bit_offset1 += num_sample_bits;
					}

					if (!writer.skip_track_float1(track_index))
						writer.write_float1(track_index, value);
				}
				else if (track_type == track_type8::float2f && decompression_settings_type::is_track_type_supported(track_type8::float2f))
				{
					rtm::vector4f value;
					if (num_bits_per_component == 0)	// Constant bit rate
					{
						value = rtm::vector_load(constant_values);
						constant_values += 2;
					}
					else
					{
						rtm::vector4f value0;
						rtm::vector4f value1;
						if (num_bits_per_component == 32)	// Raw bit rate
						{
							value0 = unpack_vector2_64_unsafe(animated_values, track_bit_offset0);
							value1 = unpack_vector2_64_unsafe(animated_values, track_bit_offset1);
						}
						else
						{
							value0 = unpack_vector2_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset0);
							value1 = unpack_vector2_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset1);

							const rtm::vector4f range_min = rtm::vector_load(range_values);
							const rtm::vector4f range_extent = rtm::vector_load(range_values + 2);
							value0 = rtm::vector_mul_add(value0, range_extent, range_min);
							value1 = rtm::vector_mul_add(value1, range_extent, range_min);
							range_values += 4;
						}

						value = rtm::vector_lerp(value0, value1, alpha);

						const uint32_t num_sample_bits = num_bits_per_component * 2;
						track_bit_offset0 += num_sample_bits;
						track_bit_offset1 += num_sample_bits;
					}

					if (!writer.skip_track_float2(track_index))
						writer.write_float2(track_index, value);
				}
				else if (track_type == track_type8::float3f && decompression_settings_type::is_track_type_supported(track_type8::float3f))
				{
					rtm::vector4f value;
					if (num_bits_per_component == 0)	// Constant bit rate
					{
						value = rtm::vector_load(constant_values);
						constant_values += 3;
					}
					else
					{
						rtm::vector4f value0;
						rtm::vector4f value1;
						if (num_bits_per_component == 32)	// Raw bit rate
						{
							value0 = unpack_vector3_96_unsafe(animated_values, track_bit_offset0);
							value1 = unpack_vector3_96_unsafe(animated_values, track_bit_offset1);
						}
						else
						{
							value0 = unpack_vector3_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset0);
							value1 = unpack_vector3_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset1);

							const rtm::vector4f range_min = rtm::vector_load(range_values);
							const rtm::vector4f range_extent = rtm::vector_load(range_values + 3);
							value0 = rtm::vector_mul_add(value0, range_extent, range_min);
							value1 = rtm::vector_mul_add(value1, range_extent, range_min);
							range_values += 6;
						}

						value = rtm::vector_lerp(value0, value1, alpha);

						const uint32_t num_sample_bits = num_bits_per_component * 3;
						track_bit_offset0 += num_sample_bits;
						track_bit_offset1 += num_sample_bits;
					}

					if (!writer.skip_track_float3(track_index))
						writer.write_float3(track_index, value);
				}
				else if (track_type == track_type8::float4f && decompression_settings_type::is_track_type_supported(track_type8::float4f))
				{
					rtm::vector4f value;
					if (num_bits_per_component == 0)	// Constant bit rate
					{
						value = rtm::vector_load(constant_values);
						constant_values += 4;
					}
					else
					{
						rtm::vector4f value0;
						rtm::vector4f value1;
						if (num_bits_per_component == 32)	// Raw bit rate
						{
							value0 = unpack_vector4_128_unsafe(animated_values, track_bit_offset0);
							value1 = unpack_vector4_128_unsafe(animated_values, track_bit_offset1);
						}
						else
						{
							value0 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset0);
							value1 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset1);

							const rtm::vector4f range_min = rtm::vector_load(range_values);
							const rtm::vector4f range_extent = rtm::vector_load(range_values + 4);
							value0 = rtm::vector_mul_add(value0, range_extent, range_min);
							value1 = rtm::vector_mul_add(value1, range_extent, range_min);
							range_values += 8;
						}

						value = rtm::vector_lerp(value0, value1, alpha);

						const uint32_t num_sample_bits = num_bits_per_component * 4;
						track_bit_offset0 += num_sample_bits;
						track_bit_offset1 += num_sample_bits;
					}

					if (!writer.skip_track_float4(track_index))
						writer.write_float4(track_index, value);
				}
				else if (track_type == track_type8::vector4f && decompression_settings_type::is_track_type_supported(track_type8::vector4f))
				{
					rtm::vector4f value;
					if (num_bits_per_component == 0)	// Constant bit rate
					{
						value = rtm::vector_load(constant_values);
						constant_values += 4;
					}
					else
					{
						rtm::vector4f value0;
						rtm::vector4f value1;
						if (num_bits_per_component == 32)	// Raw bit rate
						{
							value0 = unpack_vector4_128_unsafe(animated_values, track_bit_offset0);
							value1 = unpack_vector4_128_unsafe(animated_values, track_bit_offset1);
						}
						else
						{
							value0 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset0);
							value1 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, track_bit_offset1);

							const rtm::vector4f range_min = rtm::vector_load(range_values);
							const rtm::vector4f range_extent = rtm::vector_load(range_values + 4);
							value0 = rtm::vector_mul_add(value0, range_extent, range_min);
							value1 = rtm::vector_mul_add(value1, range_extent, range_min);
							range_values += 8;
						}

						value = rtm::vector_lerp(value0, value1, alpha);

						const uint32_t num_sample_bits = num_bits_per_component * 4;
						track_bit_offset0 += num_sample_bits;
						track_bit_offset1 += num_sample_bits;
					}

					if (!writer.skip_track_vector4(track_index))
						writer.write_vector4(track_index, value);
				}
			}

			if (decompression_settings_type::disable_fp_exeptions())
				restore_fp_exceptions(fp_env);
		}

		template<class decompression_settings_type, class track_writer_type>
		inline void decompress_track_v0(const persistent_scalar_decompression_context_v0& context, uint32_t track_index, track_writer_type& writer)
		{
			const tracks_header& header = get_tracks_header(*context.tracks);
			if (header.num_tracks == 0)
				return;	// Empty track list

			ACL_ASSERT(context.sample_time >= 0.0f, "Context not set to a valid sample time");
			if (context.sample_time < 0.0F)
				return;	// Invalid sample time, we didn't seek yet

			ACL_ASSERT(track_index < header.num_tracks, "Invalid track index");
			if (track_index >= header.num_tracks)
				return;	// Invalid track index

			// Due to the SIMD operations, we sometimes overflow in the SIMD lanes not used.
			// Disable floating point exceptions to avoid issues.
			fp_environment fp_env;
			if (decompression_settings_type::disable_fp_exeptions())
				disable_fp_exceptions(fp_env);

			const scalar_tracks_header& scalars_header = get_scalar_tracks_header(*context.tracks);

			rtm::scalarf interpolation_alpha = rtm::scalar_set(context.interpolation_alpha);
			if (decompression_settings_type::is_per_track_rounding_supported())
			{
				const sample_rounding_policy rounding_policy = static_cast<sample_rounding_policy>(context.rounding_policy);
				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");

				interpolation_alpha = rtm::scalar_set(apply_rounding_policy(context.interpolation_alpha, rounding_policy_));
			}

			const compressed_tracks_version16 version = context.get_version();
			const uint8_t* num_bits_at_bit_rate = version == compressed_tracks_version16::v02_00_00 ? k_bit_rate_num_bits_v0 : k_bit_rate_num_bits;

#if defined(ACL_HAS_ASSERT_CHECKS)
			const uint32_t max_bit_rate = version == compressed_tracks_version16::v02_00_00 ? sizeof(k_bit_rate_num_bits_v0) : sizeof(k_bit_rate_num_bits);
#endif

			const float* constant_values = scalars_header.get_track_constant_values();
			const float* range_values = scalars_header.get_track_range_values();

			const track_type8 track_type = header.track_type;
			const uint32_t num_element_components = get_track_num_sample_elements(track_type);
			uint32_t track_bit_offset = 0;

			const acl_impl::track_metadata* per_track_metadata = scalars_header.get_track_metadata();
			for (uint32_t scan_track_index = 0; scan_track_index < track_index; ++scan_track_index)
			{
				const acl_impl::track_metadata& metadata = per_track_metadata[scan_track_index];
				const uint32_t bit_rate = metadata.bit_rate;
				ACL_ASSERT(bit_rate < max_bit_rate, "Invalid bit rate: %u", bit_rate);
				const uint32_t num_bits_per_component = num_bits_at_bit_rate[bit_rate];
				track_bit_offset += num_bits_per_component * num_element_components;

				if (num_bits_per_component == 0)	// Constant bit rate
					constant_values += num_element_components;
				else if (num_bits_per_component < 32)	// Not raw bit rate
					range_values += num_element_components * 2;
			}

			const acl_impl::track_metadata& metadata = per_track_metadata[track_index];
			const uint32_t bit_rate = metadata.bit_rate;
			ACL_ASSERT(bit_rate < max_bit_rate, "Invalid bit rate: %u", bit_rate);
			const uint32_t num_bits_per_component = num_bits_at_bit_rate[bit_rate];

			const uint8_t* animated_values = scalars_header.get_track_animated_values();

			if (track_type == track_type8::float1f && decompression_settings_type::is_track_type_supported(track_type8::float1f))
			{
				rtm::scalarf value;
				if (num_bits_per_component == 0)	// Constant bit rate
					value = rtm::scalar_load_as_scalar(constant_values);
				else
				{
					rtm::scalarf value0;
					rtm::scalarf value1;
					if (num_bits_per_component == 32)	// Raw bit rate
					{
						value0 = unpack_scalarf_32_unsafe(animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_scalarf_32_unsafe(animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);
					}
					else
					{
						value0 = unpack_scalarf_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_scalarf_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);

						const rtm::scalarf range_min = rtm::scalar_load_as_scalar(range_values);
						const rtm::scalarf range_extent = rtm::scalar_load_as_scalar(range_values + num_element_components);
						value0 = rtm::scalar_mul_add(value0, range_extent, range_min);
						value1 = rtm::scalar_mul_add(value1, range_extent, range_min);
					}

					value = rtm::scalar_lerp(value0, value1, interpolation_alpha);
				}

				writer.write_float1(track_index, value);
			}
			else if (track_type == track_type8::float2f && decompression_settings_type::is_track_type_supported(track_type8::float2f))
			{
				rtm::vector4f value;
				if (num_bits_per_component == 0)	// Constant bit rate
					value = rtm::vector_load(constant_values);
				else
				{
					rtm::vector4f value0;
					rtm::vector4f value1;
					if (num_bits_per_component == 32)	// Raw bit rate
					{
						value0 = unpack_vector2_64_unsafe(animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector2_64_unsafe(animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);
					}
					else
					{
						value0 = unpack_vector2_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector2_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);

						const rtm::vector4f range_min = rtm::vector_load(range_values);
						const rtm::vector4f range_extent = rtm::vector_load(range_values + num_element_components);
						value0 = rtm::vector_mul_add(value0, range_extent, range_min);
						value1 = rtm::vector_mul_add(value1, range_extent, range_min);
					}

					value = rtm::vector_lerp(value0, value1, interpolation_alpha);
				}

				writer.write_float2(track_index, value);
			}
			else if (track_type == track_type8::float3f && decompression_settings_type::is_track_type_supported(track_type8::float3f))
			{
				rtm::vector4f value;
				if (num_bits_per_component == 0)	// Constant bit rate
					value = rtm::vector_load(constant_values);
				else
				{
					rtm::vector4f value0;
					rtm::vector4f value1;
					if (num_bits_per_component == 32)	// Raw bit rate
					{
						value0 = unpack_vector3_96_unsafe(animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector3_96_unsafe(animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);
					}
					else
					{
						value0 = unpack_vector3_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector3_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);

						const rtm::vector4f range_min = rtm::vector_load(range_values);
						const rtm::vector4f range_extent = rtm::vector_load(range_values + num_element_components);
						value0 = rtm::vector_mul_add(value0, range_extent, range_min);
						value1 = rtm::vector_mul_add(value1, range_extent, range_min);
					}

					value = rtm::vector_lerp(value0, value1, interpolation_alpha);
				}

				writer.write_float3(track_index, value);
			}
			else if (track_type == track_type8::float4f && decompression_settings_type::is_track_type_supported(track_type8::float4f))
			{
				rtm::vector4f value;
				if (num_bits_per_component == 0)	// Constant bit rate
					value = rtm::vector_load(constant_values);
				else
				{
					rtm::vector4f value0;
					rtm::vector4f value1;
					if (num_bits_per_component == 32)	// Raw bit rate
					{
						value0 = unpack_vector4_128_unsafe(animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector4_128_unsafe(animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);
					}
					else
					{
						value0 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);

						const rtm::vector4f range_min = rtm::vector_load(range_values);
						const rtm::vector4f range_extent = rtm::vector_load(range_values + num_element_components);
						value0 = rtm::vector_mul_add(value0, range_extent, range_min);
						value1 = rtm::vector_mul_add(value1, range_extent, range_min);
					}

					value = rtm::vector_lerp(value0, value1, interpolation_alpha);
				}

				writer.write_float4(track_index, value);
			}
			else if (track_type == track_type8::vector4f && decompression_settings_type::is_track_type_supported(track_type8::vector4f))
			{
				rtm::vector4f value;
				if (num_bits_per_component == 0)	// Constant bit rate
					value = rtm::vector_load(constant_values);
				else
				{
					rtm::vector4f value0;
					rtm::vector4f value1;
					if (num_bits_per_component == 32)	// Raw bit rate
					{
						value0 = unpack_vector4_128_unsafe(animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector4_128_unsafe(animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);
					}
					else
					{
						value0 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[0] + track_bit_offset);
						value1 = unpack_vector4_uXX_unsafe(num_bits_per_component, animated_values, context.key_frame_bit_offsets[1] + track_bit_offset);

						const rtm::vector4f range_min = rtm::vector_load(range_values);
						const rtm::vector4f range_extent = rtm::vector_load(range_values + num_element_components);
						value0 = rtm::vector_mul_add(value0, range_extent, range_min);
						value1 = rtm::vector_mul_add(value1, range_extent, range_min);
					}

					value = rtm::vector_lerp(value0, value1, interpolation_alpha);
				}

				writer.write_vector4(track_index, value);
			}

			if (decompression_settings_type::disable_fp_exeptions())
				restore_fp_exceptions(fp_env);
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

ACL_IMPL_FILE_PRAGMA_POP
