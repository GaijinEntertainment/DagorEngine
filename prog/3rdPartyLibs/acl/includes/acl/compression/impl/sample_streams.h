#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"
#include "acl/core/time_utils.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/math/quat_packing.h"
#include "acl/math/vector4_packing.h"
#include "acl/compression/impl/track_stream.h"
#include "acl/compression/impl/normalize.transform.h"
#include "acl/compression/impl/convert_rotation.transform.h"
#include "acl/compression/impl/transform_clip_adapters.h"

#include <rtm/quatf.h>
#include <rtm/qvvf.h>
#include <rtm/vector4f.h>

#include <cstdint>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline rtm::vector4f RTM_SIMD_CALL load_rotation_sample(const uint8_t* ptr, rotation_format8 format, uint8_t bit_rate)
		{
			switch (format)
			{
			case rotation_format8::quatf_full:
				return unpack_vector4_128(ptr);
			case rotation_format8::quatf_drop_w_full:
				return unpack_vector3_96_unsafe(ptr);
			case rotation_format8::quatf_drop_w_variable:
				ACL_ASSERT(bit_rate != k_invalid_bit_rate, "Invalid bit rate!");
				if (is_constant_bit_rate(bit_rate))
				{
					return unpack_vector3_u48_unsafe(ptr);
				}
				else if (is_raw_bit_rate(bit_rate))
					return unpack_vector3_96_unsafe(ptr);
				else
				{
					const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
					return unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, ptr, 0);
				}
			default:
				ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(format));
				return rtm::vector_zero();
			}
		}

		inline rtm::vector4f RTM_SIMD_CALL load_vector_sample(const uint8_t* ptr, vector_format8 format, uint8_t bit_rate)
		{
			switch (format)
			{
			case vector_format8::vector3f_full:
				return unpack_vector3_96_unsafe(ptr);
			case vector_format8::vector3f_variable:
				ACL_ASSERT(bit_rate != k_invalid_bit_rate, "Invalid bit rate!");
				if (is_constant_bit_rate(bit_rate))
					return unpack_vector3_u48_unsafe(ptr);
				else if (is_raw_bit_rate(bit_rate))
					return unpack_vector3_96_unsafe(ptr);
				else
				{
					const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
					return unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, ptr, 0);
				}
			default:
				ACL_ASSERT(false, "Invalid or unsupported vector format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_vector_format_name(format));
				return rtm::vector_zero();
			}
		}

		inline rtm::quatf RTM_SIMD_CALL rotation_to_quat_32(rtm::vector4f_arg0 rotation, rotation_format8 format)
		{
			switch (format)
			{
			case rotation_format8::quatf_full:
				return rtm::vector_to_quat(rotation);
			case rotation_format8::quatf_drop_w_full:
			case rotation_format8::quatf_drop_w_variable:
				// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
				// isn't very accurate on small inputs, we need to normalize
				return rtm::quat_normalize(rtm::quat_from_positive_w(rotation));
			default:
				ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(format));
				return rtm::quat_identity();
			}
		}

		// Gets a rotation sample from the format/bit rate stored
		inline rtm::quatf RTM_SIMD_CALL get_rotation_sample(const transform_streams& bone_steams, uint32_t sample_index)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;

			const rotation_format8 format = bone_steams.rotations.get_rotation_format();
			const uint8_t bit_rate = bone_steams.rotations.get_bit_rate();

			if (format == rotation_format8::quatf_drop_w_variable && is_constant_bit_rate(bit_rate))
				sample_index = 0;

			const uint8_t* quantized_ptr = bone_steams.rotations.get_raw_sample_ptr(sample_index);

			rtm::vector4f packed_rotation = acl_impl::load_rotation_sample(quantized_ptr, format, bit_rate);

			if (!bone_steams.is_rotation_constant && clip->are_rotations_normalized && !is_raw_bit_rate(bit_rate))
			{
				if (segment->are_rotations_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.rotation.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.rotation.get_extent();

					packed_rotation = rtm::vector_mul_add(packed_rotation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.rotation.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.rotation.get_extent();

				packed_rotation = rtm::vector_mul_add(packed_rotation, clip_range_extent, clip_range_min);
			}

			return acl_impl::rotation_to_quat_32(packed_rotation, format);
		}

		// Gets a rotation sample at the specified bit rate
		inline rtm::quatf RTM_SIMD_CALL get_rotation_sample(const transform_streams& bone_steams, const transform_streams& raw_bone_steams, uint32_t sample_index, uint8_t bit_rate)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			const rotation_format8 format = bone_steams.rotations.get_rotation_format();

			// Pack and unpack at our desired bit rate
			rtm::vector4f packed_rotation;

			if (is_constant_bit_rate(bit_rate))
			{
				packed_rotation = decay_vector3_u48(bone_steams.constant_rotation);
			}
			else if (is_raw_bit_rate(bit_rate))
			{
				const uint8_t* quantized_ptr = raw_bone_steams.rotations.get_raw_sample_ptr(segment->clip_sample_offset + sample_index);
				const rtm::vector4f rotation = acl_impl::load_rotation_sample(quantized_ptr, rotation_format8::quatf_full, k_invalid_bit_rate);
				packed_rotation = convert_rotation(rotation, rotation_format8::quatf_full, format);
			}
			else
			{
				const uint8_t* quantized_ptr = bone_steams.rotations.get_raw_sample_ptr(sample_index);
				const rtm::vector4f rotation = acl_impl::load_rotation_sample(quantized_ptr, format, 0);

				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
				packed_rotation = decay_vector3_uXX(rotation, num_bits_at_bit_rate);
			}

			if (!is_raw_bit_rate(bit_rate))
			{
				if (segment->are_rotations_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.rotation.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.rotation.get_extent();

					packed_rotation = rtm::vector_mul_add(packed_rotation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.rotation.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.rotation.get_extent();

				packed_rotation = rtm::vector_mul_add(packed_rotation, clip_range_extent, clip_range_min);
			}

			return acl_impl::rotation_to_quat_32(packed_rotation, format);
		}

		// Gets a rotation sample with the desired format
		inline rtm::quatf RTM_SIMD_CALL get_rotation_sample(const transform_streams& bone_steams, uint32_t sample_index, rotation_format8 desired_format)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;

			const uint8_t* quantized_ptr = bone_steams.rotations.get_raw_sample_ptr(sample_index);
			const rotation_format8 format = bone_steams.rotations.get_rotation_format();

			const rtm::vector4f rotation = acl_impl::load_rotation_sample(quantized_ptr, format, 0);

			// Pack and unpack in our desired format
			rtm::vector4f packed_rotation;

			switch (desired_format)
			{
			case rotation_format8::quatf_full:
			case rotation_format8::quatf_drop_w_full:
				packed_rotation = rotation;
				break;
			case rotation_format8::quatf_drop_w_variable:
			default:
				ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(desired_format));
				packed_rotation = rtm::vector_zero();
				break;
			}

			const bool are_rotations_normalized = clip->are_rotations_normalized && !bone_steams.is_rotation_constant;
			if (are_rotations_normalized)
			{
				if (segment->are_rotations_normalized)
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.rotation.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.rotation.get_extent();

					packed_rotation = rtm::vector_mul_add(packed_rotation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.rotation.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.rotation.get_extent();

				packed_rotation = rtm::vector_mul_add(packed_rotation, clip_range_extent, clip_range_min);
			}

			return acl_impl::rotation_to_quat_32(packed_rotation, format);
		}

		// Gets a translation sample from the format/bit rate stored
		inline rtm::vector4f RTM_SIMD_CALL get_translation_sample(const transform_streams& bone_steams, uint32_t sample_index)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			const bool are_translations_normalized = clip->are_translations_normalized;

			const vector_format8 format = bone_steams.translations.get_vector_format();
			const uint8_t bit_rate = bone_steams.translations.get_bit_rate();

			if (format == vector_format8::vector3f_variable && is_constant_bit_rate(bit_rate))
				sample_index = 0;

			const uint8_t* quantized_ptr = bone_steams.translations.get_raw_sample_ptr(sample_index);

			rtm::vector4f packed_translation = acl_impl::load_vector_sample(quantized_ptr, format, bit_rate);

			if (!bone_steams.is_translation_constant && are_translations_normalized && !is_raw_bit_rate(bit_rate))
			{
				if (segment->are_translations_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.translation.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.translation.get_extent();

					packed_translation = rtm::vector_mul_add(packed_translation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.translation.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.translation.get_extent();

				packed_translation = rtm::vector_mul_add(packed_translation, clip_range_extent, clip_range_min);
			}

			return packed_translation;
		}

		// Gets a translation sample at the specified bit rate
		inline rtm::vector4f RTM_SIMD_CALL get_translation_sample(const transform_streams& bone_steams, const transform_streams& raw_bone_steams, uint32_t sample_index, uint8_t bit_rate)
		{
			ACL_ASSERT(bone_steams.translations.get_vector_format() == vector_format8::vector3f_full, "Expected floating point vector format");

			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			ACL_ASSERT(clip->are_translations_normalized, "Translations must be normalized to support variable bit rates.");

			// Pack and unpack at our desired bit rate
			rtm::vector4f packed_translation;

			if (is_constant_bit_rate(bit_rate))
			{
				ACL_ASSERT(segment->are_translations_normalized, "Translations must be normalized to support variable bit rates.");
				packed_translation = decay_vector3_u48(bone_steams.constant_translation);
			}
			else if (is_raw_bit_rate(bit_rate))
			{
				packed_translation = raw_bone_steams.translations.get_sample(segment->clip_sample_offset + sample_index);
			}
			else
			{
				const rtm::vector4f translation = bone_steams.translations.get_sample(sample_index);

				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
				packed_translation = decay_vector3_uXX(translation, num_bits_at_bit_rate);
			}

			if (!is_raw_bit_rate(bit_rate))
			{
				if (segment->are_translations_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.translation.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.translation.get_extent();

					packed_translation = rtm::vector_mul_add(packed_translation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.translation.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.translation.get_extent();

				packed_translation = rtm::vector_mul_add(packed_translation, clip_range_extent, clip_range_min);
			}

			return packed_translation;
		}

		// Gets a translation sample with the desired format
		inline rtm::vector4f RTM_SIMD_CALL get_translation_sample(const transform_streams& bone_steams, uint32_t sample_index, vector_format8 desired_format)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			const bool are_translations_normalized = clip->are_translations_normalized && !bone_steams.is_translation_constant;
			const uint8_t* quantized_ptr = bone_steams.translations.get_raw_sample_ptr(sample_index);
			const vector_format8 format = bone_steams.translations.get_vector_format();

			const rtm::vector4f translation = acl_impl::load_vector_sample(quantized_ptr, format, 0);

			// Pack and unpack in our desired format
			rtm::vector4f packed_translation;

			switch (desired_format)
			{
			case vector_format8::vector3f_full:
				packed_translation = translation;
				break;
			case vector_format8::vector3f_variable:
			default:
				ACL_ASSERT(false, "Invalid or unsupported vector format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_vector_format_name(desired_format));
				packed_translation = rtm::vector_zero();
				break;
			}

			if (are_translations_normalized)
			{
				if (segment->are_translations_normalized)
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					rtm::vector4f segment_range_min = segment_bone_range.translation.get_min();
					rtm::vector4f segment_range_extent = segment_bone_range.translation.get_extent();

					packed_translation = rtm::vector_mul_add(packed_translation, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				rtm::vector4f clip_range_min = clip_bone_range.translation.get_min();
				rtm::vector4f clip_range_extent = clip_bone_range.translation.get_extent();

				packed_translation = rtm::vector_mul_add(packed_translation, clip_range_extent, clip_range_min);
			}

			return packed_translation;
		}

		// Gets a scale sample from the format/bit rate stored
		inline rtm::vector4f RTM_SIMD_CALL get_scale_sample(const transform_streams& bone_steams, uint32_t sample_index)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;

			const vector_format8 format = bone_steams.scales.get_vector_format();
			const uint8_t bit_rate = bone_steams.scales.get_bit_rate();

			if (format == vector_format8::vector3f_variable && is_constant_bit_rate(bit_rate))
				sample_index = 0;

			const uint8_t* quantized_ptr = bone_steams.scales.get_raw_sample_ptr(sample_index);

			rtm::vector4f packed_scale = acl_impl::load_vector_sample(quantized_ptr, format, bit_rate);

			if (!bone_steams.is_scale_constant && clip->are_scales_normalized && !is_raw_bit_rate(bit_rate))
			{
				if (segment->are_scales_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.scale.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.scale.get_extent();

					packed_scale = rtm::vector_mul_add(packed_scale, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.scale.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.scale.get_extent();

				packed_scale = rtm::vector_mul_add(packed_scale, clip_range_extent, clip_range_min);
			}

			return packed_scale;
		}

		// Gets a scale sample at the specified bit rate
		inline rtm::vector4f RTM_SIMD_CALL get_scale_sample(const transform_streams& bone_steams, const transform_streams& raw_bone_steams, uint32_t sample_index, uint8_t bit_rate)
		{
			ACL_ASSERT(bone_steams.scales.get_vector_format() == vector_format8::vector3f_full, "Expected floating point vector format");

			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			ACL_ASSERT(clip->are_scales_normalized, "Scales must be normalized to support variable bit rates.");

			// Pack and unpack at our desired bit rate
			rtm::vector4f packed_scale;

			if (is_constant_bit_rate(bit_rate))
			{
				ACL_ASSERT(segment->are_scales_normalized, "Scales must be normalized to support variable bit rates.");
				packed_scale = decay_vector3_u48(bone_steams.constant_scale);
			}
			else if (is_raw_bit_rate(bit_rate))
			{
				packed_scale = raw_bone_steams.scales.get_sample(segment->clip_sample_offset + sample_index);
			}
			else
			{
				const rtm::vector4f scale = bone_steams.scales.get_sample(sample_index);

				const uint32_t num_bits_at_bit_rate = get_num_bits_at_bit_rate(bit_rate);
				packed_scale = decay_vector3_uXX(scale, num_bits_at_bit_rate);
			}

			if (!is_raw_bit_rate(bit_rate))
			{
				if (segment->are_scales_normalized && !is_constant_bit_rate(bit_rate))
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					const rtm::vector4f segment_range_min = segment_bone_range.scale.get_min();
					const rtm::vector4f segment_range_extent = segment_bone_range.scale.get_extent();

					packed_scale = rtm::vector_mul_add(packed_scale, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				const rtm::vector4f clip_range_min = clip_bone_range.scale.get_min();
				const rtm::vector4f clip_range_extent = clip_bone_range.scale.get_extent();

				packed_scale = rtm::vector_mul_add(packed_scale, clip_range_extent, clip_range_min);
			}

			return packed_scale;
		}

		// Gets a scale sample with the desired format
		inline rtm::vector4f RTM_SIMD_CALL get_scale_sample(const transform_streams& bone_steams, uint32_t sample_index, vector_format8 desired_format)
		{
			const segment_context* segment = bone_steams.segment;
			const clip_context* clip = segment->clip;
			const bool are_scales_normalized = clip->are_scales_normalized && !bone_steams.is_scale_constant;
			const uint8_t* quantized_ptr = bone_steams.scales.get_raw_sample_ptr(sample_index);
			const vector_format8 format = bone_steams.scales.get_vector_format();

			const rtm::vector4f scale = acl_impl::load_vector_sample(quantized_ptr, format, 0);

			// Pack and unpack in our desired format
			rtm::vector4f packed_scale;

			switch (desired_format)
			{
			case vector_format8::vector3f_full:
				packed_scale = scale;
				break;
			case vector_format8::vector3f_variable:
			default:
				ACL_ASSERT(false, "Invalid or unsupported vector format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_vector_format_name(desired_format));
				packed_scale = scale;
				break;
			}

			if (are_scales_normalized)
			{
				if (segment->are_scales_normalized)
				{
					const transform_range& segment_bone_range = segment->ranges[bone_steams.bone_index];

					rtm::vector4f segment_range_min = segment_bone_range.scale.get_min();
					rtm::vector4f segment_range_extent = segment_bone_range.scale.get_extent();

					packed_scale = rtm::vector_mul_add(packed_scale, segment_range_extent, segment_range_min);
				}

				const transform_range& clip_bone_range = clip->ranges[bone_steams.bone_index];

				rtm::vector4f clip_range_min = clip_bone_range.scale.get_min();
				rtm::vector4f clip_range_extent = clip_bone_range.scale.get_extent();

				packed_scale = rtm::vector_mul_add(packed_scale, clip_range_extent, clip_range_min);
			}

			return packed_scale;
		}

		struct sample_context
		{
			uint32_t track_index;

			uint32_t sample_key;
			float sample_time;

			transform_bit_rates bit_rates;
		};

		template<class clip_adapter_t, class segment_adapter_t>
		inline uint32_t get_uniform_sample_key(const clip_adapter_t& clip, const segment_adapter_t& segment, float sample_time)
		{
			static_assert(std::is_base_of<transform_clip_adapter_t, clip_adapter_t>::value, "Clip adapter must derive from transform_clip_adapter_t");
			static_assert(std::is_base_of<transform_segment_adapter_t, segment_adapter_t>::value, "Segment adapter must derive from transform_segment_adapter_t");

			uint32_t key0 = 0;
			uint32_t key1 = 0;
			float interpolation_alpha = 0.0F;

			// Our samples are uniform, grab the nearest samples
			const sample_rounding_policy rounding_policy = sample_rounding_policy::nearest;

			// Never consider our clip as looping when compressing
			const sample_looping_policy looping_policy = sample_looping_policy::non_looping;

			const uint32_t clip_num_samples = clip.get_num_samples();
			const float sample_rate = clip.get_sample_rate();

			find_linear_interpolation_samples_with_sample_rate(clip_num_samples, sample_rate, sample_time, rounding_policy, looping_policy, key0, key1, interpolation_alpha);

			if (segment.is_valid())
			{
				const uint32_t segment_start_offser = segment.get_start_offset();
				const uint32_t segment_num_samples = segment.get_num_samples();

				// Offset for the current segment and clamp
				key0 = key0 - segment_start_offser;
				if (key0 >= segment_num_samples)
				{
					key0 = 0;
					interpolation_alpha = 1.0F;
				}

				key1 = key1 - segment_start_offser;
				if (key1 >= segment_num_samples)
				{
					key1 = segment_num_samples - 1;
					interpolation_alpha = 0.0F;
				}
			}

			// When we sample uniformly, we always round to the nearest sample.
			// As such, we don't need to interpolate.
			return interpolation_alpha == 0.0F ? key0 : key1;
		}

		inline uint32_t get_uniform_sample_key(const segment_context& segment, float sample_time)
		{
			return get_uniform_sample_key(
				transform_clip_context_adapter_t(*segment.clip),
				transform_segment_context_adapter_t(segment),
				sample_time);
		}

		RTM_FORCE_INLINE rtm::quatf RTM_SIMD_CALL sample_rotation(const sample_context& context, const transform_streams& bone_stream)
		{
			rtm::quatf rotation;
			if (bone_stream.is_rotation_default)
				rotation = bone_stream.default_value.rotation;
			else if (bone_stream.is_rotation_constant)
				rotation = rtm::quat_normalize(get_rotation_sample(bone_stream, 0));
			else
			{
				rotation = get_rotation_sample(bone_stream, context.sample_key);
				rotation = rtm::quat_normalize(rotation);
			}

			return rotation;
		}

		RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL sample_translation(const sample_context& context, const transform_streams& bone_stream)
		{
			if (bone_stream.is_translation_default)
				return bone_stream.default_value.translation;
			else if (bone_stream.is_translation_constant)
				return get_translation_sample(bone_stream, 0);
			else
				return get_translation_sample(bone_stream, context.sample_key);
		}

		RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL sample_scale(const sample_context& context, const transform_streams& bone_stream)
		{
			if (bone_stream.is_scale_default)
				return bone_stream.default_value.scale;
			else if (bone_stream.is_scale_constant)
				return get_scale_sample(bone_stream, 0);
			else
				return get_scale_sample(bone_stream, context.sample_key);
		}

		// Samples all transforms at a point in time
		// Transforms can be raw or quantized
		inline void sample_streams(const transform_streams* bone_streams, uint32_t num_bones, float sample_time, rtm::qvvf* out_local_pose)
		{
			const segment_context* segment_context = bone_streams->segment;
			const bool has_scale = segment_context->clip->has_scale;

			// With uniform sample distributions, we do not interpolate.
			const uint32_t sample_key = get_uniform_sample_key(*segment_context, sample_time);

			acl_impl::sample_context context;
			context.sample_key = sample_key;
			context.sample_time = sample_time;

			for (uint32_t bone_index = 0; bone_index < num_bones; ++bone_index)
			{
				context.track_index = bone_index;

				const transform_streams& bone_stream = bone_streams[bone_index];

				const rtm::quatf rotation = acl_impl::sample_rotation(context, bone_stream);
				const rtm::vector4f translation = acl_impl::sample_translation(context, bone_stream);
				const rtm::vector4f scale = has_scale ? acl_impl::sample_scale(context, bone_stream) : bone_stream.default_value.scale;

				out_local_pose[bone_index] = rtm::qvv_set(rotation, translation, scale);
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
