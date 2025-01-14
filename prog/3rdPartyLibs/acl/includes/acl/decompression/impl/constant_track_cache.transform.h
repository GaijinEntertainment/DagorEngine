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
#include "acl/core/track_formats.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/impl/track_cache.h"
#include "acl/decompression/impl/decompression_context.transform.h"
#include "acl/math/quat_packing.h"
#include "acl/math/quatf.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

#define ACL_IMPL_USE_CONSTANT_PREFETCH

ACL_IMPL_FILE_PRAGMA_PUSH

// We only initialize some variables when we need them which prompts the compiler to complain
// The usage is perfectly safe and because this code is VERY hot and needs to be as fast as possible,
// we disable the warning to avoid zeroing out things we don't need
#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
	// warning C26451: Arithmetic overflow: Using operator '*' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator '*' to avoid overflow (io.2).
	// We can't overflow because compressed clips cannot contain more than 4 GB worth of data
	#pragma warning(disable : 26451)
	// warning C6385: Reading invalid data from '...':  the readable size is '512' bytes, but '528' bytes may be read.
	// We properly handle overflow, this is a false positive
	#pragma warning(disable : 6385)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

#if defined(ACL_IMPL_USE_CONSTANT_PREFETCH)
#define ACL_IMPL_CONSTANT_PREFETCH(ptr) memory_prefetch(ptr)
#else
#define ACL_IMPL_CONSTANT_PREFETCH(ptr) (void)(ptr)
#endif

	namespace acl_impl
	{
		struct constant_track_cache_v0
		{
			// Our constant rotation samples are packed in groups of 4 samples (16 floats, 64 bytes) in AOS form when we have full precision
			// and with SOA form otherwise (12 floats, 48 bytes). To avoid stalling, we unpack 16 samples at a time and cache the result in AOS form.
			// This means we need 3-4 cache lines to unpack. If we cache miss, we'll be able to queue up as many uops as we can before we stall.
			// We can also queue up enough independent uops to avoid stalling on the square-root (6-21 cycles depending on the platform).
			// Constant rotation samples are fairly common, see here: https://nfrechette.github.io/2020/08/09/animation_data_numbers/
			// CMU has 64.41%, Paragon has 47.69%, and Fortnite has 62.84%.
			// Following these numbers, it is common for clips to have at least 10 constant rotation samples to unpack.

			track_cache_quatf_v0<32, 1> rotations;

			// Points to our packed sub-track data
			const uint8_t*	constant_data_rotations;
			const uint8_t*	constant_data_translations;
			const uint8_t*	constant_data_scales;

			template<class decompression_settings_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void initialize(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const transform_tracks_header& transform_header = get_transform_tracks_header(*decomp_context.tracks);

				rotations.num_left_to_unpack = transform_header.num_constant_rotation_samples;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				const rotation_format8 packed_format = is_rotation_format_variable(rotation_format) ? get_highest_variant_precision(get_rotation_variant(rotation_format)) : rotation_format;
				const uint32_t packed_rotation_size = get_packed_rotation_size(packed_format);
				const uint32_t packed_translation_size = get_packed_vector_size(vector_format8::vector3f_full);

				constant_data_rotations = transform_header.get_constant_track_data();
				constant_data_translations = constant_data_rotations + packed_rotation_size * transform_header.num_constant_rotation_samples;
				constant_data_scales = constant_data_translations + packed_translation_size * transform_header.num_constant_translation_samples;
			}

			template<class decompression_settings_type>
			RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void unpack_rotation_group(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const uint32_t num_left_to_unpack = rotations.num_left_to_unpack;
				if (num_left_to_unpack == 0)
					return;	// Nothing left to do, we are done

				// If we have less than half our cache filled with samples, unpack some more
				const uint32_t num_cached = rotations.get_num_cached();
				if (num_cached >= 16)
					return;	// Enough cached, nothing to do

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);

				uint32_t num_to_unpack = std::min<uint32_t>(num_left_to_unpack, 16);
				rotations.num_left_to_unpack = num_left_to_unpack - num_to_unpack;

				// Write index will be either 0 or 16 here since we always unpack 16 at a time
				const uint32_t cache_write_index = rotations.cache_write_index % 32;
				rotations.cache_write_index += num_to_unpack;

				const uint8_t* constant_track_data = constant_data_rotations;
				rtm::quatf* cache_ptr = &rotations.cached_samples[0][cache_write_index];

				if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
				{
					for (uint32_t unpack_index = num_to_unpack; unpack_index != 0; --unpack_index)
					{
						// Unpack
						const rtm::quatf sample = unpack_quat_128(constant_track_data);

						// Cache
						*cache_ptr = sample;

						// Update our pointers
						constant_track_data += sizeof(rtm::float4f);
						cache_ptr++;
					}
				}
				else
				{
					while (num_to_unpack != 0)
					{
						const uint32_t unpack_count = std::min<uint32_t>(num_to_unpack, 4);
						num_to_unpack -= unpack_count;

						// Unpack
						// Always load 4x rotations, we might contain garbage in a few lanes but it's fine
						// The last group contains no padding so we have to make to align our reads properly
						const uint32_t load_size = unpack_count * sizeof(float);

						rtm::vector4f xxxx = rtm::vector_load(bit_cast<const float*>(constant_track_data + load_size * 0));
						rtm::vector4f yyyy = rtm::vector_load(bit_cast<const float*>(constant_track_data + load_size * 1));
						rtm::vector4f zzzz = rtm::vector_load(bit_cast<const float*>(constant_track_data + load_size * 2));

						// Update our read ptr
						constant_track_data += load_size * 3;

						rtm::vector4f wwww = quat_from_positive_w4(xxxx, yyyy, zzzz);

						// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
						// isn't very accurate on small inputs, we need to normalize
						if (decompression_settings_type::get_rotation_normalization_policy() == rotation_normalization_policy_t::always)
							quat_normalize4(xxxx, yyyy, zzzz, wwww);

						rtm::vector4f sample0;
						rtm::vector4f sample1;
						rtm::vector4f sample2;
						rtm::vector4f sample3;
						RTM_MATRIXF_TRANSPOSE_4X4(xxxx, yyyy, zzzz, wwww, sample0, sample1, sample2, sample3);

						// Cache
						cache_ptr[0] = rtm::vector_to_quat(sample0);
						cache_ptr[1] = rtm::vector_to_quat(sample1);
						cache_ptr[2] = rtm::vector_to_quat(sample2);
						cache_ptr[3] = rtm::vector_to_quat(sample3);
						cache_ptr += 4;
					}
				}

#if defined(ACL_HAS_ASSERT_CHECKS)
				num_to_unpack = std::min<uint32_t>(num_left_to_unpack, 16);
				for (uint32_t unpack_index = 0; unpack_index < num_to_unpack; ++unpack_index)
				{
					const rtm::quatf rotation = rotations.cached_samples[0][cache_write_index + unpack_index];

					ACL_ASSERT(rtm::quat_is_finite(rotation), "Rotation is not valid!");
					ACL_ASSERT(rtm::quat_is_normalized(rotation), "Rotation is not normalized!");
				}
#endif

				// Update our pointer
				constant_data_rotations = constant_track_data;
			}

			template<class decompression_settings_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_rotation_groups(const persistent_transform_decompression_context_v0& decomp_context, uint32_t num_groups_to_skip)
			{
				// We only support skipping full groups
				const uint32_t num_left_to_unpack = rotations.num_left_to_unpack;
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				ACL_ASSERT(num_to_skip < num_left_to_unpack, "Cannot skip rotations that aren't present");

				rotations.num_left_to_unpack = num_left_to_unpack - num_to_skip;

				const uint8_t* constant_track_data = constant_data_rotations;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
					constant_track_data += num_to_skip * sizeof(rtm::float4f);
				else
					constant_track_data += num_to_skip * sizeof(rtm::float3f);

				constant_data_rotations = constant_track_data;

				// Prefetch our group
				ACL_IMPL_CONSTANT_PREFETCH(constant_track_data);
			}

			template<class decompression_settings_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::quatf RTM_SIMD_CALL unpack_rotation_within_group(const persistent_transform_decompression_context_v0& decomp_context, uint32_t unpack_index) const
			{
				ACL_ASSERT(unpack_index < rotations.num_left_to_unpack && unpack_index < 4, "Cannot unpack sample that isn't present");

				rtm::quatf sample;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
				{
					const uint8_t* constant_track_data = constant_data_rotations + (unpack_index * sizeof(rtm::float4f));
					sample = unpack_quat_128(constant_track_data);
				}
				else
				{
					// Data is in SOA form
					const uint32_t group_size = std::min<uint32_t>(rotations.num_left_to_unpack, 4);
					const float* constant_track_data = bit_cast<const float*>(constant_data_rotations) + unpack_index;
					const float x = constant_track_data[group_size * 0];
					const float y = constant_track_data[group_size * 1];
					const float z = constant_track_data[group_size * 2];
					const rtm::vector4f sample_v = rtm::vector_set(x, y, z, 0.0F);
					sample = rtm::quat_from_positive_w(sample_v);

					// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
					// isn't very accurate on small inputs, we need to normalize
					if (decompression_settings_type::get_rotation_normalization_policy() == rotation_normalization_policy_t::always)
						sample = rtm::quat_normalize(sample);
				}

				ACL_ASSERT(rtm::quat_is_finite(sample), "Sample is not valid!");
				ACL_ASSERT(rtm::quat_is_normalized(sample), "Sample is not normalized!");
				return sample;
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK const rtm::quatf& RTM_SIMD_CALL consume_rotation()
			{
				ACL_ASSERT(rotations.cache_read_index < rotations.cache_write_index, "Attempting to consume a constant sample that isn't cached");
				const uint32_t cache_read_index = rotations.cache_read_index++;
				return rotations.cached_samples[0][cache_read_index % 32];
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_translation_groups(uint32_t num_groups_to_skip)
			{
				const uint8_t* constant_track_data = constant_data_translations;

				// We only support skipping full groups
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				constant_track_data += num_to_skip * sizeof(rtm::float3f);

				constant_data_translations = constant_track_data;

				// Prefetch our group
				ACL_IMPL_CONSTANT_PREFETCH(constant_track_data);
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_translation_within_group(uint32_t unpack_index) const
			{
				ACL_ASSERT(unpack_index < 4, "Cannot unpack sample that isn't present");

				const uint8_t* constant_track_data = constant_data_translations + (unpack_index * sizeof(rtm::float3f));
				const rtm::vector4f sample = rtm::vector_load(constant_track_data);
				ACL_ASSERT(rtm::vector_is_finite3(sample), "Sample is not valid!");
				return sample;
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK const uint8_t* consume_translation()
			{
				const uint8_t* sample_ptr = constant_data_translations;
				constant_data_translations += sizeof(rtm::float3f);
				return sample_ptr;
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_scale_groups(uint32_t num_groups_to_skip)
			{
				const uint8_t* constant_track_data = constant_data_scales;

				// We only support skipping full groups
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				constant_track_data += num_to_skip * sizeof(rtm::float3f);

				constant_data_scales = constant_track_data;

				// Prefetch our group
				ACL_IMPL_CONSTANT_PREFETCH(constant_track_data);
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_scale_within_group(uint32_t unpack_index) const
			{
				ACL_ASSERT(unpack_index < 4, "Cannot unpack sample that isn't present");

				const uint8_t* constant_track_data = constant_data_scales + (unpack_index * sizeof(rtm::float3f));
				const rtm::vector4f sample = rtm::vector_load(constant_track_data);
				ACL_ASSERT(rtm::vector_is_finite3(sample), "Sample is not valid!");
				return sample;
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK const uint8_t* consume_scale()
			{
				const uint8_t* sample_ptr = constant_data_scales;
				constant_data_scales += sizeof(rtm::float3f);
				return sample_ptr;
			}
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif

ACL_IMPL_FILE_PRAGMA_POP
