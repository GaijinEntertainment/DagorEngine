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
#include "acl/core/track_types.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/math/quat_packing.h"
#include "acl/math/vector4_packing.h"

#include <rtm/quatf.h>
#include <rtm/qvvf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		class track_stream
		{
		public:
			uint8_t* get_raw_sample_ptr(uint32_t sample_index)
			{
				ACL_ASSERT(sample_index < m_num_samples, "Invalid sample index. %u >= %u", sample_index, m_num_samples);
				uint32_t offset = sample_index * m_sample_size;
				return m_samples + offset;
			}

			const uint8_t* get_raw_sample_ptr(uint32_t sample_index) const
			{
				ACL_ASSERT(sample_index < m_num_samples, "Invalid sample index. %u >= %u", sample_index, m_num_samples);
				uint32_t offset = sample_index * m_sample_size;
				return m_samples + offset;
			}

			template<typename sample_type>
			sample_type RTM_SIMD_CALL get_raw_sample(uint32_t sample_index) const
			{
				const uint8_t* ptr = get_raw_sample_ptr(sample_index);
				return *safe_ptr_cast<const sample_type>(ptr);
			}

#if defined(RTM_NO_INTRINSICS)
			template<typename sample_type>
			void RTM_SIMD_CALL set_raw_sample(uint32_t sample_index, const sample_type& sample)
#else
			template<typename sample_type>
			void RTM_SIMD_CALL set_raw_sample(uint32_t sample_index, sample_type sample)
#endif
			{
				ACL_ASSERT(m_sample_size == sizeof(sample_type), "Unexpected sample size. %u != %zu", m_sample_size, sizeof(sample_type));
				uint8_t* ptr = get_raw_sample_ptr(sample_index);
				*safe_ptr_cast<sample_type>(ptr) = sample;
			}

			uint32_t get_num_samples_allocated() const { return m_num_samples_allocated; }
			uint32_t get_num_samples() const { return m_num_samples; }
			uint32_t get_sample_size() const { return m_sample_size; }
			float get_sample_rate() const { return m_sample_rate; }
			animation_track_type8 get_track_type() const { return m_type; }
			uint8_t get_bit_rate() const { return m_bit_rate; }
			bool is_bit_rate_variable() const { return m_bit_rate != k_invalid_bit_rate; }
			float get_finite_duration() const { return calculate_finite_duration(m_num_samples, m_sample_rate); }

			uint32_t get_packed_sample_size() const
			{
				if (m_type == animation_track_type8::rotation)
					return get_packed_rotation_size(m_format.rotation);
				else
					return get_packed_vector_size(m_format.vector);
			}

			// Used during loop optimization, the last sample is removed since the first one repeats
			void strip_last_sample()
			{
				ACL_ASSERT(m_num_samples_allocated > 1, "Too few samples to strip");
				if (m_num_samples_allocated <= 1)
					return;

				m_num_samples = m_num_samples_allocated - 1;
			}

		protected:
			track_stream(animation_track_type8 type, track_format8 format) noexcept
				: m_allocator(nullptr)
				, m_samples(nullptr)
				, m_num_samples_allocated(0)
				, m_num_samples(0)
				, m_sample_size(0)
				, m_sample_rate(0.0F)
				, m_type(type)
				, m_format(format)
				, m_bit_rate(0)
			{}

			track_stream(iallocator& allocator, uint32_t num_samples, uint32_t sample_size, float sample_rate, animation_track_type8 type, track_format8 format, uint8_t bit_rate)
				: m_allocator(&allocator)
				, m_samples(bit_cast<uint8_t*>(allocator.allocate(sample_size * size_t(num_samples) + k_padding, 16)))
				, m_num_samples_allocated(num_samples)
				, m_num_samples(num_samples)
				, m_sample_size(sample_size)
				, m_sample_rate(sample_rate)
				, m_type(type)
				, m_format(format)
				, m_bit_rate(bit_rate)
			{}

			track_stream(const track_stream&) = delete;
			track_stream(track_stream&& other) noexcept
				: m_allocator(other.m_allocator)
				, m_samples(other.m_samples)
				, m_num_samples_allocated(other.m_num_samples_allocated)
				, m_num_samples(other.m_num_samples)
				, m_sample_size(other.m_sample_size)
				, m_sample_rate(other.m_sample_rate)
				, m_type(other.m_type)
				, m_format(other.m_format)
				, m_bit_rate(other.m_bit_rate)
			{
				new(&other) track_stream(other.m_type, other.m_format);
			}

			~track_stream()
			{
				if (m_allocator != nullptr)
					m_allocator->deallocate(m_samples, m_sample_size * size_t(m_num_samples_allocated) + k_padding);
			}

			track_stream& operator=(const track_stream&) = delete;
			track_stream& operator=(track_stream&& rhs) noexcept
			{
				std::swap(m_allocator, rhs.m_allocator);
				std::swap(m_samples, rhs.m_samples);
				std::swap(m_num_samples_allocated, rhs.m_num_samples_allocated);
				std::swap(m_num_samples, rhs.m_num_samples);
				std::swap(m_sample_size, rhs.m_sample_size);
				std::swap(m_sample_rate, rhs.m_sample_rate);
				std::swap(m_type, rhs.m_type);
				std::swap(m_format, rhs.m_format);
				std::swap(m_bit_rate, rhs.m_bit_rate);
				return *this;
			}

			void duplicate_impl(track_stream& copy) const
			{
				ACL_ASSERT(copy.m_type == m_type, "Attempting to duplicate streams with incompatible types!");
				if (m_allocator != nullptr)
				{
					copy.m_allocator = m_allocator;
					copy.m_samples = bit_cast<uint8_t*>(m_allocator->allocate(m_sample_size * size_t(m_num_samples) + k_padding, 16));
					copy.m_num_samples_allocated = m_num_samples;
					copy.m_num_samples = m_num_samples;
					copy.m_sample_size = m_sample_size;
					copy.m_sample_rate = m_sample_rate;
					copy.m_format = m_format;
					copy.m_bit_rate = m_bit_rate;

					std::memcpy(copy.m_samples, m_samples, (size_t)m_sample_size * m_num_samples);
				}
			}

			// In order to guarantee the safety of unaligned SIMD loads of every byte, we add some padding
			static constexpr uint32_t k_padding = 15;

			iallocator*				m_allocator;
			uint8_t*				m_samples;
			uint32_t				m_num_samples_allocated;
			uint32_t				m_num_samples;
			uint32_t				m_sample_size;
			float					m_sample_rate;

			animation_track_type8	m_type;
			track_format8			m_format;
			uint8_t					m_bit_rate;
		};

		class rotation_track_stream final : public track_stream
		{
		public:
			rotation_track_stream() noexcept : track_stream(animation_track_type8::rotation, track_format8(rotation_format8::quatf_full)) {}
			rotation_track_stream(iallocator& allocator, uint32_t num_samples, uint32_t sample_size, float sample_rate, rotation_format8 format, uint8_t bit_rate = k_invalid_bit_rate)
				: track_stream(allocator, num_samples, sample_size, sample_rate, animation_track_type8::rotation, track_format8(format), bit_rate)
			{}
			rotation_track_stream(const rotation_track_stream&) = delete;
			rotation_track_stream(rotation_track_stream&& other) noexcept
				: track_stream(static_cast<track_stream&&>(other))
			{}
			~rotation_track_stream() = default;

			rotation_track_stream& operator=(const rotation_track_stream&) = delete;
			rotation_track_stream& operator=(rotation_track_stream&& rhs) noexcept
			{
				track_stream::operator=(static_cast<track_stream&&>(rhs));
				return *this;
			}

			rotation_track_stream duplicate() const
			{
				rotation_track_stream copy;
				duplicate_impl(copy);
				return copy;
			}

			rotation_format8 get_rotation_format() const { return m_format.rotation; }

			rtm::quatf RTM_SIMD_CALL get_sample(uint32_t sample_index) const
			{
				const rtm::vector4f rotation = get_raw_sample<rtm::vector4f>(sample_index);

				switch (m_format.rotation)
				{
				case rotation_format8::quatf_full:
					return rtm::vector_to_quat(rotation);
				case rotation_format8::quatf_drop_w_full:
				case rotation_format8::quatf_drop_w_variable:
					// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
					// isn't very accurate on small inputs, we need to normalize
					return rtm::quat_normalize(rtm::quat_from_positive_w(rotation));
				default:
					ACL_ASSERT(false, "Invalid or unsupported rotation format: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, get_rotation_format_name(m_format.rotation));
					return rtm::vector_to_quat(rotation);
				}
			};

			rtm::quatf RTM_SIMD_CALL get_sample_clamped(uint32_t sample_index) const
			{
				return get_sample((std::min)(sample_index, m_num_samples - 1));
			}
		};

		class translation_track_stream final : public track_stream
		{
		public:
			translation_track_stream() noexcept : track_stream(animation_track_type8::translation, track_format8(vector_format8::vector3f_full)) {}
			translation_track_stream(iallocator& allocator, uint32_t num_samples, uint32_t sample_size, float sample_rate, vector_format8 format, uint8_t bit_rate = k_invalid_bit_rate)
				: track_stream(allocator, num_samples, sample_size, sample_rate, animation_track_type8::translation, track_format8(format), bit_rate)
			{}
			translation_track_stream(const translation_track_stream&) = delete;
			translation_track_stream(translation_track_stream&& other) noexcept
				: track_stream(static_cast<track_stream&&>(other))
			{}
			~translation_track_stream() = default;

			translation_track_stream& operator=(const translation_track_stream&) = delete;
			translation_track_stream& operator=(translation_track_stream&& rhs) noexcept
			{
				track_stream::operator=(static_cast<track_stream&&>(rhs));
				return *this;
			}

			translation_track_stream duplicate() const
			{
				translation_track_stream copy;
				duplicate_impl(copy);
				return copy;
			}

			vector_format8 get_vector_format() const { return m_format.vector; }

			rtm::vector4f RTM_SIMD_CALL get_sample(uint32_t sample_index) const
			{
				return get_raw_sample<rtm::vector4f>(sample_index);
			}

			rtm::vector4f RTM_SIMD_CALL get_sample_clamped(uint32_t sample_index) const
			{
				return get_sample((std::min)(sample_index, m_num_samples - 1));
			}
		};

		class scale_track_stream final : public track_stream
		{
		public:
			scale_track_stream() noexcept : track_stream(animation_track_type8::scale, track_format8(vector_format8::vector3f_full)) {}
			scale_track_stream(iallocator& allocator, uint32_t num_samples, uint32_t sample_size, float sample_rate, vector_format8 format, uint8_t bit_rate = k_invalid_bit_rate)
				: track_stream(allocator, num_samples, sample_size, sample_rate, animation_track_type8::scale, track_format8(format), bit_rate)
			{}
			scale_track_stream(const scale_track_stream&) = delete;
			scale_track_stream(scale_track_stream&& other) noexcept
				: track_stream(static_cast<track_stream&&>(other))
			{}
			~scale_track_stream() = default;

			scale_track_stream& operator=(const scale_track_stream&) = delete;
			scale_track_stream& operator=(scale_track_stream&& rhs) noexcept
			{
				track_stream::operator=(static_cast<track_stream&&>(rhs));
				return *this;
			}

			scale_track_stream duplicate() const
			{
				scale_track_stream copy;
				duplicate_impl(copy);
				return copy;
			}

			vector_format8 get_vector_format() const { return m_format.vector; }

			rtm::vector4f RTM_SIMD_CALL get_sample(uint32_t sample_index) const
			{
				return get_raw_sample<rtm::vector4f>(sample_index);
			}

			rtm::vector4f RTM_SIMD_CALL get_sample_clamped(uint32_t sample_index) const
			{
				return get_sample((std::min)(sample_index, m_num_samples - 1));
			}
		};

		// For a rotation track, the extent only tells us if the track is constant or not
		// since the min/max we maintain aren't valid rotations.
		// Similarly, the center isn't a valid rotation and is meaningless.
		class track_stream_range
		{
		public:
			static track_stream_range RTM_SIMD_CALL from_min_max(rtm::vector4f_arg0 min, rtm::vector4f_arg1 max)
			{
				return track_stream_range(min, max, rtm::vector_sub(max, min));
			}

			static track_stream_range RTM_SIMD_CALL from_min_extent(rtm::vector4f_arg0 min, rtm::vector4f_arg1 extent)
			{
				return track_stream_range(min, rtm::vector_add(min, extent), extent);
			}

			track_stream_range()
				: m_min(rtm::vector_zero())
				, m_max(rtm::vector_zero())
				, m_extent(rtm::vector_zero())
			{}

			rtm::vector4f RTM_SIMD_CALL get_min() const { return m_min; }
			rtm::vector4f RTM_SIMD_CALL get_max() const { return m_max; }

			rtm::vector4f RTM_SIMD_CALL get_center() const { return rtm::vector_mul_add(m_extent, 0.5F, m_min); }
			rtm::vector4f RTM_SIMD_CALL get_extent() const { return m_extent; }

			bool is_constant(float threshold) const
			{
				// If our error threshold is zero we want to test if we are binary exact
				// This is used by raw clips, we must preserve the original values
				if (threshold == 0.0F)
					return rtm::vector_all_equal(m_min, m_max);
				else
					return rtm::vector_all_less_equal(m_extent, rtm::vector_set(threshold));
			}

		private:
			track_stream_range(rtm::vector4f_arg0 min, rtm::vector4f_arg1 max, rtm::vector4f_arg2 extent)
				: m_min(min)
				, m_max(max)
				, m_extent(extent)
			{
				ACL_ASSERT(rtm::vector_all_greater_equal(max, min), "Max must be greater or equal to min");
				ACL_ASSERT(rtm::vector_all_greater_equal(extent, rtm::vector_zero()) && rtm::vector_is_finite(extent), "Extent must be positive and finite");
			}

			rtm::vector4f	m_min;
			rtm::vector4f	m_max;
			rtm::vector4f	m_extent;
		};

		struct transform_range
		{
			track_stream_range rotation;
			track_stream_range translation;
			track_stream_range scale;
		};

		struct segment_context;

		struct transform_streams
		{
			transform_streams() = default;

			// Can't copy
			transform_streams(const transform_streams&) = delete;
			transform_streams& operator=(const transform_streams&) = delete;

			// Can move
			transform_streams(transform_streams&&) = default;
			transform_streams& operator=(transform_streams&&) = default;

			rtm::qvvf default_value					= rtm::qvv_identity();

			// Sample 0 before we normalize over the segment.
			// This is used when a sub-track is constant over the segment.
			// These values are normalized over the clip (but not over the segment).
			rtm::vector4f constant_rotation			= rtm::vector_zero();
			rtm::vector4f constant_translation		= rtm::vector_zero();
			rtm::vector4f constant_scale			= rtm::vector_zero();

			segment_context* segment				= nullptr;
			uint32_t bone_index						= k_invalid_track_index;
			uint32_t parent_bone_index				= k_invalid_track_index;
			uint32_t output_index					= k_invalid_track_index;

			rotation_track_stream rotations;
			translation_track_stream translations;
			scale_track_stream scales;

			bool is_rotation_constant				= false;
			bool is_rotation_default				= false;
			bool is_translation_constant			= false;
			bool is_translation_default				= false;
			bool is_scale_constant					= false;
			bool is_scale_default					= false;

			uint8_t padding[2]						= {};

			bool is_stripped_from_output() const { return output_index == k_invalid_track_index; }

			transform_streams duplicate() const
			{
				transform_streams copy;
				copy.default_value = default_value;
				copy.constant_rotation = constant_rotation;
				copy.constant_translation = constant_translation;
				copy.constant_scale = constant_scale;
				copy.segment = segment;
				copy.bone_index = bone_index;
				copy.parent_bone_index = parent_bone_index;
				copy.output_index = output_index;
				copy.rotations = rotations.duplicate();
				copy.translations = translations.duplicate();
				copy.scales = scales.duplicate();
				copy.is_rotation_constant = is_rotation_constant;
				copy.is_rotation_default = is_rotation_default;
				copy.is_translation_constant = is_translation_constant;
				copy.is_translation_default = is_translation_default;
				copy.is_scale_constant = is_scale_constant;
				copy.is_scale_default = is_scale_default;
				return copy;
			}
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
