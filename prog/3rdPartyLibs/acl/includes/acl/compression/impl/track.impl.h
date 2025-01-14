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

// Included only once from track.h

#include "acl/version.h"
#include "acl/core/iallocator.h"
#include "acl/core/string.h"
#include "acl/core/track_desc.h"
#include "acl/core/track_traits.h"
#include "acl/core/track_types.h"
#include "acl/core/impl/bit_cast.impl.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline track::track() noexcept
		: m_allocator(nullptr)
		, m_data(nullptr)
		, m_num_samples(0)
		, m_stride(0)
		, m_data_size(0)
		, m_sample_rate(0.0F)
		, m_type(track_type8::float1f)
		, m_category(track_category8::scalarf)
		, m_sample_size(0)
	{}

	inline track::track(track&& other) noexcept
		: m_allocator(other.m_allocator)
		, m_data(other.m_data)
		, m_num_samples(other.m_num_samples)
		, m_stride(other.m_stride)
		, m_data_size(other.m_data_size)
		, m_sample_rate(other.m_sample_rate)
		, m_type(other.m_type)
		, m_category(other.m_category)
		, m_sample_size(other.m_sample_size)
		, m_desc(other.m_desc)
		, m_name(std::move(other.m_name))
	{
		other.m_allocator = nullptr;
		other.m_data = nullptr;
	}

	inline track::~track()
	{
		if (is_owner())
		{
			// We own the memory, free it
			m_allocator->deallocate(m_data, m_data_size);
		}
	}

	inline track& track::operator=(track&& other) noexcept
	{
		std::swap(m_allocator, other.m_allocator);
		std::swap(m_data, other.m_data);
		std::swap(m_num_samples, other.m_num_samples);
		std::swap(m_stride, other.m_stride);
		std::swap(m_data_size, other.m_data_size);
		std::swap(m_sample_rate, other.m_sample_rate);
		std::swap(m_type, other.m_type);
		std::swap(m_category, other.m_category);
		std::swap(m_sample_size, other.m_sample_size);
		std::swap(m_desc, other.m_desc);
		std::swap(m_name, other.m_name);
		return *this;
	}

	inline void* track::operator[](uint32_t index)
	{
		ACL_ASSERT(index < m_num_samples, "Invalid sample index. %u >= %u", index, m_num_samples);
		return m_data + (size_t(index) * m_stride);
	}

	inline const void* track::operator[](uint32_t index) const
	{
		ACL_ASSERT(index < m_num_samples, "Invalid sample index. %u >= %u", index, m_num_samples);
		return m_data + (size_t(index) * m_stride);
	}

	inline uint32_t track::get_output_index() const
	{
		switch (m_category)
		{
		case track_category8::scalarf:		return m_desc.scalar.output_index;
		case track_category8::transformf:	return m_desc.transform.output_index;
		case track_category8::scalard:
		case track_category8::transformd:
		default:
			ACL_ASSERT(false, "Invalid category");
			return k_invalid_track_index;
		}
	}

	template<>
	inline track_desc_scalarf& track::get_description()
	{
		ACL_ASSERT(track_desc_scalarf::category == m_category, "Unexpected track category");
		return m_desc.scalar;
	}

	template<>
	inline track_desc_transformf& track::get_description()
	{
		ACL_ASSERT(track_desc_transformf::category == m_category, "Unexpected track category");
		return m_desc.transform;
	}

	template<>
	inline const track_desc_scalarf& track::get_description() const
	{
		ACL_ASSERT(track_desc_scalarf::category == m_category, "Unexpected track category");
		return m_desc.scalar;
	}

	template<>
	inline const track_desc_transformf& track::get_description() const
	{
		ACL_ASSERT(track_desc_transformf::category == m_category, "Unexpected track category");
		return m_desc.transform;
	}

	inline track track::get_copy(iallocator& allocator) const
	{
		track track_;
		get_copy_impl(allocator, track_);
		return track_;
	}

	inline track track::get_ref() const
	{
		track track_;
		get_ref_impl(track_);
		return track_;
	}

	inline error_result track::is_valid() const
	{
		if (m_data == nullptr)
			return error_result();

		if (m_num_samples == 0xFFFFFFFFU)
			return error_result("Too many samples");

		if (m_sample_rate <= 0.0F || !rtm::scalar_is_finite(m_sample_rate))
			return error_result("Invalid sample rate");

		switch (m_category)
		{
		case track_category8::scalarf:		return m_desc.scalar.is_valid();
		case track_category8::transformf:	return m_desc.transform.is_valid();
		case track_category8::scalard:
		case track_category8::transformd:
		default:							return error_result("Invalid category");
		}
	}

	inline track::track(track_type8 type, track_category8 category) noexcept
		: m_allocator(nullptr)
		, m_data(nullptr)
		, m_num_samples(0)
		, m_stride(0)
		, m_data_size(0)
		, m_sample_rate(0.0F)
		, m_type(type)
		, m_category(category)
		, m_sample_size(0)
		, m_desc()
		, m_name()
	{}

	inline track::track(iallocator* allocator, uint8_t* data, uint32_t num_samples, uint32_t stride, size_t data_size, float sample_rate, track_type8 type, track_category8 category, uint8_t sample_size) noexcept
		: m_allocator(allocator)
		, m_data(data)
		, m_num_samples(num_samples)
		, m_stride(stride)
		, m_data_size(data_size)
		, m_sample_rate(sample_rate)
		, m_type(type)
		, m_category(category)
		, m_sample_size(sample_size)
		, m_desc()
		, m_name()
	{}

	inline void track::get_copy_impl(iallocator& allocator, track& out_track) const
	{
		out_track.m_allocator = &allocator;
		out_track.m_data = acl_impl::bit_cast<uint8_t*>(allocator.allocate(m_data_size));
		out_track.m_num_samples = m_num_samples;
		out_track.m_stride = m_stride;
		out_track.m_data_size = m_data_size;
		out_track.m_sample_rate = m_sample_rate;
		out_track.m_type = m_type;
		out_track.m_category = m_category;
		out_track.m_sample_size = m_sample_size;
		out_track.m_desc = m_desc;
		out_track.m_name = m_name.get_copy(allocator);

		std::memcpy(out_track.m_data, m_data, m_data_size);
	}

	inline void track::get_ref_impl(track& out_track) const
	{
		out_track.m_allocator = nullptr;
		out_track.m_data = m_data;
		out_track.m_num_samples = m_num_samples;
		out_track.m_stride = m_stride;
		out_track.m_data_size = m_data_size;
		out_track.m_sample_rate = m_sample_rate;
		out_track.m_type = m_type;
		out_track.m_category = m_category;
		out_track.m_sample_size = m_sample_size;
		out_track.m_desc = m_desc;
		out_track.m_name = m_name.get_copy();
	}

	template<track_type8 track_type_>
	inline typename track_typed<track_type_>::sample_type& track_typed<track_type_>::operator[](uint32_t index)
	{
		ACL_ASSERT(index < m_num_samples, "Invalid sample index. %u >= %u", index, m_num_samples);
		return *acl_impl::bit_cast<sample_type*>(m_data + (size_t(index) * m_stride));
	}

	template<track_type8 track_type_>
	inline const typename track_typed<track_type_>::sample_type& track_typed<track_type_>::operator[](uint32_t index) const
	{
		ACL_ASSERT(index < m_num_samples, "Invalid sample index. %u >= %u", index, m_num_samples);
		return *acl_impl::bit_cast<const sample_type*>(m_data + (size_t(index) * m_stride));
	}

	template<track_type8 track_type_>
	inline typename track_typed<track_type_>::sample_type* track_typed<track_type_>::get_data()
	{
		ACL_ASSERT(m_stride == sizeof(sample_type), "Samples are not contiguous in memory, this function is unsafe");
		return acl_impl::bit_cast<sample_type*>(m_data);
	}

	template<track_type8 track_type_>
	inline const typename track_typed<track_type_>::sample_type* track_typed<track_type_>::get_data() const
	{
		ACL_ASSERT(m_stride == sizeof(sample_type), "Samples are not contiguous in memory, this function is unsafe");
		return acl_impl::bit_cast<const sample_type*>(m_data);
	}

	template<track_type8 track_type_>
	inline typename track_typed<track_type_>::desc_type& track_typed<track_type_>::get_description()
	{
		return track::get_description<desc_type>();
	}

	template<track_type8 track_type_>
	inline const typename track_typed<track_type_>::desc_type& track_typed<track_type_>::get_description() const
	{
		return track::get_description<desc_type>();
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::get_copy(iallocator& allocator) const
	{
		track_typed track_;
		track::get_copy_impl(allocator, track_);
		return track_;
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::get_ref() const
	{
		track_typed track_;
		track::get_ref_impl(track_);
		return track_;
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::make_copy(const typename track_typed<track_type_>::desc_type& desc, iallocator& allocator, const sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride)
	{
		const size_t num_samples_ = num_samples;
		const size_t data_size = num_samples_ * sizeof(sample_type);
		const uint8_t* data_raw = acl_impl::bit_cast<const uint8_t*>(data);

		// Copy the data manually to avoid preserving the stride
		sample_type* data_copy = acl_impl::bit_cast<sample_type*>(allocator.allocate(data_size));
		for (size_t index = 0; index < num_samples_; ++index)
			data_copy[index] = *acl_impl::bit_cast<const sample_type*>(data_raw + (index * stride));

		return track_typed<track_type_>(&allocator, acl_impl::bit_cast<uint8_t*>(data_copy), num_samples, sizeof(sample_type), data_size, sample_rate, desc);
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::make_reserve(const typename track_typed<track_type_>::desc_type& desc, iallocator& allocator, uint32_t num_samples, float sample_rate)
	{
		const size_t data_size = size_t(num_samples) * sizeof(sample_type);
		return track_typed<track_type_>(&allocator, acl_impl::bit_cast<uint8_t*>(allocator.allocate(data_size)), num_samples, sizeof(sample_type), data_size, sample_rate, desc);
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::make_owner(const typename track_typed<track_type_>::desc_type& desc, iallocator& allocator, sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride)
	{
		const size_t data_size = size_t(num_samples) * stride;
		return track_typed<track_type_>(&allocator, acl_impl::bit_cast<uint8_t*>(data), num_samples, stride, data_size, sample_rate, desc);
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_> track_typed<track_type_>::make_ref(const typename track_typed<track_type_>::desc_type& desc, sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride)
	{
		const size_t data_size = size_t(num_samples) * stride;
		return track_typed<track_type_>(nullptr, acl_impl::bit_cast<uint8_t*>(data), num_samples, stride, data_size, sample_rate, desc);
	}

	template<track_type8 track_type_>
	inline track_typed<track_type_>::track_typed(iallocator* allocator, uint8_t* data, uint32_t num_samples, uint32_t stride, size_t data_size, float sample_rate, const typename track_typed<track_type_>::desc_type& desc) noexcept
		: track(allocator, data, num_samples, stride, data_size, sample_rate, type, category, sizeof(sample_type))
	{
		m_desc = track_desc_untyped(desc);
	}

	template<typename track_type>
	inline track_type& track_cast(track& track_)
	{
		ACL_ASSERT(track_type::type == track_.get_type() || track_.is_empty(), "Unexpected track type");
		return static_cast<track_type&>(track_);
	}

	template<typename track_type>
	inline const track_type& track_cast(const track& track_)
	{
		ACL_ASSERT(track_type::type == track_.get_type() || track_.is_empty(), "Unexpected track type");
		return static_cast<const track_type&>(track_);
	}

	template<typename track_type>
	inline track_type* track_cast(track* track_)
	{
		if (track_ == nullptr || (track_type::type != track_->get_type() && !track_->is_empty()))
			return nullptr;

		return static_cast<track_type*>(track_);
	}

	template<typename track_type>
	inline const track_type* track_cast(const track* track_)
	{
		if (track_ == nullptr || (track_type::type != track_->get_type() && !track_->is_empty()))
			return nullptr;

		return static_cast<const track_type*>(track_);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
