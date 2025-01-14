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

// Included only once from track_array.h

#include "acl/version.h"
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/interpolation_utils.h"
#include "acl/core/track_types.h"
#include "acl/core/track_writer.h"

#include <rtm/quatf.h>
#include <rtm/scalarf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline track_array::track_array() noexcept
		: m_allocator(nullptr)
		, m_tracks(nullptr)
		, m_num_tracks(0)
		, m_looping_policy(sample_looping_policy::non_looping)
	{}

	inline track_array::track_array(iallocator& allocator, uint32_t num_tracks)
		: m_allocator(&allocator)
		, m_tracks(allocate_type_array<track>(allocator, num_tracks))
		, m_num_tracks(num_tracks)
		, m_looping_policy(sample_looping_policy::non_looping)
	{}

	inline track_array::track_array(track_array&& other) noexcept
		: m_allocator(other.m_allocator)
		, m_tracks(other.m_tracks)
		, m_num_tracks(other.m_num_tracks)
		, m_looping_policy(other.m_looping_policy)
		, m_name(std::move(other.m_name))
	{
		other.m_allocator = nullptr;	// Make sure we don't free our data since we no longer own it
	}

	inline track_array::~track_array()
	{
		if (m_allocator != nullptr)
			deallocate_type_array(*m_allocator, m_tracks, m_num_tracks);
	}

	inline track_array& track_array::operator=(track_array&& other) noexcept
	{
		std::swap(m_allocator, other.m_allocator);
		std::swap(m_tracks, other.m_tracks);
		std::swap(m_num_tracks, other.m_num_tracks);
		std::swap(m_looping_policy, other.m_looping_policy);
		std::swap(m_name, other.m_name);
		return *this;
	}

	inline track& track_array::operator[](uint32_t index)
	{
		ACL_ASSERT(index < m_num_tracks, "Invalid track index. %u >= %u", index, m_num_tracks);
		return m_tracks[index];
	}

	inline const track& track_array::operator[](uint32_t index) const
	{
		ACL_ASSERT(index < m_num_tracks, "Invalid track index. %u >= %u", index, m_num_tracks);
		return m_tracks[index];
	}

	inline float track_array::get_duration() const
	{
		if (m_allocator == nullptr || m_num_tracks == 0)
			return 0.0F;

		// When we wrap, we artificially insert a repeating first sample at the end of non-empty clips
		uint32_t num_samples = m_tracks->get_num_samples();
		if (m_looping_policy == sample_looping_policy::wrap && num_samples != 0)
			num_samples++;

		return calculate_duration(num_samples, m_tracks->get_sample_rate());
	}

	inline float track_array::get_finite_duration() const
	{
		if (m_allocator == nullptr || m_num_tracks == 0)
			return 0.0F;

		// When we wrap, we artificially insert a repeating first sample at the end of non-empty clips
		uint32_t num_samples = m_tracks->get_num_samples();
		if (m_looping_policy == sample_looping_policy::wrap && num_samples != 0)
			num_samples++;

		return calculate_finite_duration(num_samples, m_tracks->get_sample_rate());
	}

	inline void track_array::set_looping_policy(sample_looping_policy policy)
	{
		ACL_ASSERT(policy != sample_looping_policy::as_compressed, "As compressed looping policy not supported on raw tracks");
		if (policy == sample_looping_policy::as_compressed)
			return;

		m_looping_policy = policy;
	}

	inline error_result track_array::is_valid() const
	{
		const track_type8 type = get_track_type();
		const uint32_t num_samples = get_num_samples_per_track();
		const float sample_rate = get_sample_rate();

		for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
		{
			const track& track_ = m_tracks[track_index];
			if (track_.get_type() != type)
				return error_result("Tracks must all have the same type within an array");

			if (track_.get_num_samples() != num_samples)
				return error_result("Track array requires the same number of samples in every track");

			if (track_.get_sample_rate() != sample_rate)
				return error_result("Track array requires the same sample rate in every track");

			const error_result result = track_.is_valid();
			if (result.any())
				return result;

			if (track_.get_category() == track_category8::transformf)
			{
				const track_desc_transformf& desc = track_.get_description<track_desc_transformf>();
				if (desc.parent_index != k_invalid_track_index && desc.parent_index >= m_num_tracks)
					return error_result("Invalid parent_index. It must be 'k_invalid_track_index' or a valid track index");
			}
		}

		// Validate output indices
		uint32_t num_outputs = 0;
		for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
		{
			const track& track_ = m_tracks[track_index];
			const uint32_t output_index = track_.get_output_index();
			if (output_index != k_invalid_track_index && output_index >= m_num_tracks)
				return error_result("The output_index must be 'k_invalid_track_index' or less than the number of bones");

			if (output_index != k_invalid_track_index)
			{
				for (uint32_t track_index2 = track_index + 1; track_index2 < m_num_tracks; ++track_index2)
				{
					const track& track2_ = m_tracks[track_index2];
					const uint32_t output_index2 = track2_.get_output_index();
					if (output_index == output_index2)
						return error_result("Duplicate output_index found");
				}

				num_outputs++;
			}
		}

		for (uint32_t output_index = 0; output_index < num_outputs; ++output_index)
		{
			bool found = false;
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track& track_ = m_tracks[track_index];
				const uint32_t output_index_ = track_.get_output_index();
				if (output_index == output_index_)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return error_result("Output indices are not contiguous");
		}

		return error_result();
	}

	template<class track_writer_type>
	inline void track_array::sample_tracks(float sample_time, sample_rounding_policy rounding_policy, track_writer_type& writer) const
	{
		static_assert(std::is_base_of<track_writer, track_writer_type>::value, "track_writer_type must derive from track_writer");
		ACL_ASSERT(is_valid().empty(), "Invalid track array");

		const uint32_t num_samples = get_num_samples_per_track();
		const float sample_rate = get_sample_rate();
		const track_type8 track_type = get_track_type();

		// Clamp for safety, the caller should normally handle this but in practice, it often isn't the case
		const float duration = get_finite_duration();
		sample_time = rtm::scalar_clamp(sample_time, 0.0F, duration);

		uint32_t key_frame0;
		uint32_t key_frame1;
		float interpolation_alpha;

		// Allow per track usage, keeps the code simpler to maintain
		find_linear_interpolation_samples_with_sample_rate(num_samples, sample_rate, sample_time, sample_rounding_policy::per_track, m_looping_policy, key_frame0, key_frame1, interpolation_alpha);

		const float no_rounding_alpha = apply_rounding_policy(interpolation_alpha, sample_rounding_policy::none);

		const float interpolation_alpha_per_policy[k_num_sample_rounding_policies] =
		{
			no_rounding_alpha,	// none
			apply_rounding_policy(interpolation_alpha, sample_rounding_policy::floor),
			apply_rounding_policy(interpolation_alpha, sample_rounding_policy::ceil),
			apply_rounding_policy(interpolation_alpha, sample_rounding_policy::nearest),

			// We'll assert if we attempt to use this, but in case they are skipped/disabled, we interpolate
			no_rounding_alpha,	// per_track
		};

		switch (track_type)
		{
		case track_type8::float1f:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_float1f& track__ = track_cast<track_float1f>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::scalarf value0 = rtm::scalar_load_as_scalar(&track__[key_frame0]);
				const rtm::scalarf value1 = rtm::scalar_load_as_scalar(&track__[key_frame1]);
				const rtm::scalarf value = rtm::scalar_lerp(value0, value1, rtm::scalar_set(alpha));
				writer.write_float1(track_index, value);
			}
			break;
		case track_type8::float2f:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_float2f& track__ = track_cast<track_float2f>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::vector4f value0 = rtm::vector_load2(&track__[key_frame0]);
				const rtm::vector4f value1 = rtm::vector_load2(&track__[key_frame1]);
				const rtm::vector4f value = rtm::vector_lerp(value0, value1, alpha);
				writer.write_float2(track_index, value);
			}
			break;
		case track_type8::float3f:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_float3f& track__ = track_cast<track_float3f>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::vector4f value0 = rtm::vector_load3(&track__[key_frame0]);
				const rtm::vector4f value1 = rtm::vector_load3(&track__[key_frame1]);
				const rtm::vector4f value = rtm::vector_lerp(value0, value1, alpha);
				writer.write_float3(track_index, value);
			}
			break;
		case track_type8::float4f:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_float4f& track__ = track_cast<track_float4f>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::vector4f value0 = rtm::vector_load(&track__[key_frame0]);
				const rtm::vector4f value1 = rtm::vector_load(&track__[key_frame1]);
				const rtm::vector4f value = rtm::vector_lerp(value0, value1, alpha);
				writer.write_float4(track_index, value);
			}
			break;
		case track_type8::vector4f:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_vector4f& track__ = track_cast<track_vector4f>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::vector4f value0 = track__[key_frame0];
				const rtm::vector4f value1 = track__[key_frame1];
				const rtm::vector4f value = rtm::vector_lerp(value0, value1, alpha);
				writer.write_vector4(track_index, value);
			}
			break;
		case track_type8::qvvf:
			for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
			{
				const track_qvvf& track__ = track_cast<track_qvvf>(m_tracks[track_index]);

				const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
				ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");
				const float alpha = interpolation_alpha_per_policy[static_cast<int>(rounding_policy_)];

				const rtm::qvvf& value0 = track__[key_frame0];
				const rtm::qvvf& value1 = track__[key_frame1];
				const rtm::quatf rotation = rtm::quat_lerp(value0.rotation, value1.rotation, alpha);
				const rtm::vector4f translation = rtm::vector_lerp(value0.translation, value1.translation, alpha);
				const rtm::vector4f scale = rtm::vector_lerp(value0.scale, value1.scale, alpha);
				writer.write_rotation(track_index, rotation);
				writer.write_translation(track_index, translation);
				writer.write_scale(track_index, scale);
			}
			break;
		default:
			ACL_ASSERT(false, "Invalid track type");
			break;
		}
	}

	template<class track_writer_type>
	inline void track_array::sample_track(uint32_t track_index, float sample_time, sample_rounding_policy rounding_policy, track_writer_type& writer) const
	{
		static_assert(std::is_base_of<track_writer, track_writer_type>::value, "track_writer_type must derive from track_writer");
		ACL_ASSERT(is_valid().empty(), "Invalid track array");
		ACL_ASSERT(track_index < m_num_tracks, "Invalid track index");

		const track& track_ = m_tracks[track_index];
		const uint32_t num_samples = get_num_samples_per_track();
		const float sample_rate = get_sample_rate();

		// Clamp for safety, the caller should normally handle this but in practice, it often isn't the case
		const float duration = get_finite_duration();
		sample_time = rtm::scalar_clamp(sample_time, 0.0F, duration);

		const sample_rounding_policy rounding_policy_ = writer.get_rounding_policy(rounding_policy, track_index);
		ACL_ASSERT(rounding_policy_ != sample_rounding_policy::per_track, "track_writer::get_rounding_policy() cannot return per_track");

		uint32_t key_frame0;
		uint32_t key_frame1;
		float interpolation_alpha;
		find_linear_interpolation_samples_with_sample_rate(num_samples, sample_rate, sample_time, rounding_policy_, m_looping_policy, key_frame0, key_frame1, interpolation_alpha);

		switch (track_.get_type())
		{
		case track_type8::float1f:
		{
			const track_float1f& track__ = track_cast<track_float1f>(track_);

			const rtm::scalarf value0 = rtm::scalar_load_as_scalar(&track__[key_frame0]);
			const rtm::scalarf value1 = rtm::scalar_load_as_scalar(&track__[key_frame1]);
			const rtm::scalarf value = rtm::scalar_lerp(value0, value1, rtm::scalar_set(interpolation_alpha));
			writer.write_float1(track_index, value);
			break;
		}
		case track_type8::float2f:
		{
			const track_float2f& track__ = track_cast<track_float2f>(track_);

			const rtm::vector4f value0 = rtm::vector_load2(&track__[key_frame0]);
			const rtm::vector4f value1 = rtm::vector_load2(&track__[key_frame1]);
			const rtm::vector4f value = rtm::vector_lerp(value0, value1, interpolation_alpha);
			writer.write_float2(track_index, value);
			break;
		}
		case track_type8::float3f:
		{
			const track_float3f& track__ = track_cast<track_float3f>(track_);

			const rtm::vector4f value0 = rtm::vector_load3(&track__[key_frame0]);
			const rtm::vector4f value1 = rtm::vector_load3(&track__[key_frame1]);
			const rtm::vector4f value = rtm::vector_lerp(value0, value1, interpolation_alpha);
			writer.write_float3(track_index, value);
			break;
		}
		case track_type8::float4f:
		{
			const track_float4f& track__ = track_cast<track_float4f>(track_);

			const rtm::vector4f value0 = rtm::vector_load(&track__[key_frame0]);
			const rtm::vector4f value1 = rtm::vector_load(&track__[key_frame1]);
			const rtm::vector4f value = rtm::vector_lerp(value0, value1, interpolation_alpha);
			writer.write_float4(track_index, value);
			break;
		}
		case track_type8::vector4f:
		{
			const track_vector4f& track__ = track_cast<track_vector4f>(track_);

			const rtm::vector4f value0 = track__[key_frame0];
			const rtm::vector4f value1 = track__[key_frame1];
			const rtm::vector4f value = rtm::vector_lerp(value0, value1, interpolation_alpha);
			writer.write_vector4(track_index, value);
			break;
		}
		case track_type8::qvvf:
		{
			const track_qvvf& track__ = track_cast<track_qvvf>(track_);

			const rtm::qvvf& value0 = track__[key_frame0];
			const rtm::qvvf& value1 = track__[key_frame1];
			const rtm::quatf rotation = rtm::quat_lerp(value0.rotation, value1.rotation, interpolation_alpha);
			const rtm::vector4f translation = rtm::vector_lerp(value0.translation, value1.translation, interpolation_alpha);
			const rtm::vector4f scale = rtm::vector_lerp(value0.scale, value1.scale, interpolation_alpha);
			writer.write_rotation(track_index, rotation);
			writer.write_translation(track_index, translation);
			writer.write_scale(track_index, scale);
			break;
		}
		default:
			ACL_ASSERT(false, "Invalid track type");
			break;
		}
	}

	inline uint32_t track_array::get_raw_size() const
	{
		const uint32_t num_samples = get_num_samples_per_track();
		const track_type8 track_type = get_track_type();

		uint32_t total_size = 0;
		for (uint32_t track_index = 0; track_index < m_num_tracks; ++track_index)
		{
			const track& track_ = m_tracks[track_index];

			if (track_type == track_type8::qvvf)
				total_size += num_samples * 10 * uint32_t(sizeof(float));	// 4 rotation floats, 3 translation floats, 3 scale floats
			else
				total_size += num_samples * track_.get_sample_size();
		}

		return total_size;
	}

	template<track_type8 track_type_>
	inline typename track_array_typed<track_type_>::track_member_type& track_array_typed<track_type_>::operator[](uint32_t index)
	{
		ACL_ASSERT(index < m_num_tracks, "Invalid track index. %u >= %u", index, m_num_tracks);
		return track_cast<track_member_type>(m_tracks[index]);
	}

	template<track_type8 track_type_>
	inline const typename track_array_typed<track_type_>::track_member_type& track_array_typed<track_type_>::operator[](uint32_t index) const
	{
		ACL_ASSERT(index < m_num_tracks, "Invalid track index. %u >= %u", index, m_num_tracks);
		return track_cast<track_member_type>(m_tracks[index]);
	}

	template<typename track_array_type>
	inline track_array_type& track_array_cast(track_array& track_array_)
	{
		ACL_ASSERT(track_array_type::type == track_array_.get_track_type() || track_array_.is_empty(), "Unexpected track type");
		return static_cast<track_array_type&>(track_array_);
	}

	template<typename track_array_type>
	inline const track_array_type& track_array_cast(const track_array& track_array_)
	{
		ACL_ASSERT(track_array_type::type == track_array_.get_track_type() || track_array_.is_empty(), "Unexpected track type");
		return static_cast<const track_array_type&>(track_array_);
	}

	template<typename track_array_type>
	inline track_array_type* track_array_cast(track_array* track_array_)
	{
		if (track_array_ == nullptr || (track_array_type::type != track_array_->get_track_type() && !track_array_->is_empty()))
			return nullptr;

		return static_cast<track_array_type*>(track_array_);
	}

	template<typename track_array_type>
	inline const track_array_type* track_array_cast(const track_array* track_array_)
	{
		if (track_array_ == nullptr || (track_array_type::type != track_array_->get_track_type() && !track_array_->is_empty()))
			return nullptr;

		return static_cast<const track_array_type*>(track_array_);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
