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
#include "acl/core/string.h"
#include "acl/core/track_desc.h"
#include "acl/core/track_traits.h"
#include "acl/core/track_types.h"

#include <rtm/math.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C4582: 'union': constructor is not implicitly called (/Wall)
	// This is fine because a track is empty until it is constructed with a valid description.
	// Afterwards, access is typesafe.
	#pragma warning(disable : 4582)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// An untyped track of data. A track is a time series of values sampled
	// uniformly over time at a specific sample rate. Tracks can either own
	// their memory or reference an external buffer.
	// For convenience, this type can be cast with the `track_cast(..)` family
	// of functions. Each track type has the same size as every track description
	// is contained within a union.
	//////////////////////////////////////////////////////////////////////////
	class track
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// Creates an empty, untyped track.
		track() noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Move constructor for a track.
		track(track&& other) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Destroys the track. If it owns the memory referenced, it will be freed.
		~track();

		//////////////////////////////////////////////////////////////////////////
		// Move assignment for a track.
		track& operator=(track&& other) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to an untyped sample at the specified index.
		void* operator[](uint32_t index);

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to an untyped sample at the specified index.
		const void* operator[](uint32_t index) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the track owns its memory, false otherwise.
		bool is_owner() const { return m_allocator != nullptr; }

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the track owns its memory, false otherwise.
		bool is_ref() const { return m_allocator == nullptr; }

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the track doesn't contain any data, false otherwise.
		bool is_empty() const { return m_num_samples == 0; }

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to the allocator instance or nullptr if there is none present.
		iallocator* get_allocator() const { return m_allocator; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of samples contained within the track.
		// This does not account for the repeating first sample when the wrap
		// looping policy is used.
		// See `sample_looping_policy` for details.
		uint32_t get_num_samples() const { return m_num_samples; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the stride in bytes in between samples as laid out in memory.
		// This is always sizeof(sample_type) unless the memory isn't owned internally.
		uint32_t get_stride() const { return m_stride; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track type.
		track_type8 get_type() const { return m_type; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track category.
		track_category8 get_category() const { return m_category; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the size in bytes of each track sample.
		uint32_t get_sample_size() const { return m_sample_size; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track sample rate.
		// A track has its sampled uniformly distributed in time at a fixed rate (e.g. 30 samples per second).
		float get_sample_rate() const { return m_sample_rate; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track name.
		const string& get_name() const { return m_name; }

		//////////////////////////////////////////////////////////////////////////
		// Sets the track name.
		void set_name(const string& name) { m_name = name.get_copy(); }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track output index.
		// When compressing, it is often desirable to strip or re-order the tracks we output.
		// This can be used to sort by LOD or to strip stale tracks. Tracks with an invalid
		// track index are stripped in the output.
		uint32_t get_output_index() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track description.
		template<typename desc_type>
		desc_type& get_description();

		//////////////////////////////////////////////////////////////////////////
		// Returns the track description.
		template<typename desc_type>
		const desc_type& get_description() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns a copy of the track where the memory will be owned by the copy.
		track get_copy(iallocator& allocator) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns a reference to the track where the memory isn't owned.
		track get_ref() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether a track is valid or not.
		// A track is valid if:
		//    - It is empty
		//    - It has a positive and finite sample rate
		//    - A valid description
		error_result is_valid() const;

	protected:
		//////////////////////////////////////////////////////////////////////////
		// We prohibit copying, use get_copy() and get_ref() instead.
		track(const track&) = delete;
		track& operator=(const track&) = delete;

		//////////////////////////////////////////////////////////////////////////
		// Internal constructor.
		// Creates an empty, untyped track.
		track(track_type8 type, track_category8 category) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Internal constructor.
		track(iallocator* allocator, uint8_t* data, uint32_t num_samples, uint32_t stride, size_t data_size, float sample_rate, track_type8 type, track_category8 category, uint8_t sample_size) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Internal helper.
		void get_copy_impl(iallocator& allocator, track& out_track) const;

		//////////////////////////////////////////////////////////////////////////
		// Internal helper.
		void get_ref_impl(track& out_track) const;

		iallocator*				m_allocator;		// Optional allocator that owns the memory
		uint8_t*				m_data;				// Pointer to the samples

		uint32_t				m_num_samples;		// The number of samples
		uint32_t				m_stride;			// The stride in bytes in between samples as layed out in memory
		size_t					m_data_size;		// The total size of the buffer used by the samples

		float					m_sample_rate;		// The track sample rate

		track_type8				m_type;				// The track type
		track_category8			m_category;			// The track category
		uint16_t				m_sample_size;		// The size in bytes of each sample

		//////////////////////////////////////////////////////////////////////////
		// A union of every track description.
		// This ensures every track has the same size regardless of its type.
		union track_desc_untyped
		{
			track_desc_scalarf		scalar;
			track_desc_transformf	transform;

			track_desc_untyped() noexcept {};
			explicit track_desc_untyped(const track_desc_scalarf& desc) noexcept : scalar(desc) {}
			explicit track_desc_untyped(const track_desc_transformf& desc) noexcept : transform(desc) {}
		};

		track_desc_untyped		m_desc;				// The track description

		string					m_name;				// An optional name
	};

	//////////////////////////////////////////////////////////////////////////
	// A typed track of data. See `track` for details.
	//////////////////////////////////////////////////////////////////////////
	template<track_type8 track_type_>
	class track_typed final : public track
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// The track type.
		static constexpr track_type8 type = track_type_;

		//////////////////////////////////////////////////////////////////////////
		// The track category.
		static constexpr track_category8 category = track_traits<track_type_>::category;

		//////////////////////////////////////////////////////////////////////////
		// The type of each sample in this track.
		using sample_type = typename track_traits<track_type_>::sample_type;

		//////////////////////////////////////////////////////////////////////////
		// The type of the track description.
		using desc_type = typename track_traits<track_type_>::desc_type;

		//////////////////////////////////////////////////////////////////////////
		// Constructs an empty typed track.
		track_typed() noexcept : track(type, category) { static_assert(sizeof(track_typed) == sizeof(track), "You cannot add member variables to this class"); }

		//////////////////////////////////////////////////////////////////////////
		// Destroys the track and potentially frees any memory it might own.
		~track_typed() = default;

		//////////////////////////////////////////////////////////////////////////
		// Move assignment for a track.
		track_typed(track_typed&& other) noexcept : track(static_cast<track&&>(other)) {}

		//////////////////////////////////////////////////////////////////////////
		// Move assignment for a track.
		track_typed& operator=(track_typed&& other) noexcept { return static_cast<track_typed&>(track::operator=(static_cast<track&&>(other))); }

		//////////////////////////////////////////////////////////////////////////
		// Returns the sample at the specified index.
		// If this track does not own the memory, mutable references aren't allowed and an
		// invalid reference will be returned, leading to a crash.
		sample_type& operator[](uint32_t index);

		//////////////////////////////////////////////////////////////////////////
		// Returns the sample at the specified index.
		const sample_type& operator[](uint32_t index) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to the sample data.
		// Note that if the sample stride is not sizeof(sample_type) then samples
		// are not contiguous in memory!
		sample_type* get_data();

		//////////////////////////////////////////////////////////////////////////
		// Note that if the sample stride is not sizeof(sample_type) then samples
		// are not contiguous in memory!
		const sample_type* get_data() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track description.
		desc_type& get_description();

		//////////////////////////////////////////////////////////////////////////
		// Returns the track description.
		const desc_type& get_description() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track type.
		track_type8 get_type() const { return type; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track category.
		track_category8 get_category() const { return category; }

		//////////////////////////////////////////////////////////////////////////
		// Returns a copy of the track where the memory will be owned by the copy.
		track_typed get_copy(iallocator& allocator) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns a reference to the track where the memory isn't owned.
		track_typed get_ref() const;

		//////////////////////////////////////////////////////////////////////////
		// Creates a track that copies the data and owns the memory.
		static track_typed<track_type_> make_copy(const desc_type& desc, iallocator& allocator, const sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride = sizeof(sample_type));

		//////////////////////////////////////////////////////////////////////////
		// Creates a track and preallocates but does not initialize the memory that it owns.
		static track_typed<track_type_> make_reserve(const desc_type& desc, iallocator& allocator, uint32_t num_samples, float sample_rate);

		//////////////////////////////////////////////////////////////////////////
		// Creates a track and takes ownership of the already allocated memory.
		static track_typed<track_type_> make_owner(const desc_type& desc, iallocator& allocator, sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride = sizeof(sample_type));

		//////////////////////////////////////////////////////////////////////////
		// Creates a track that just references the data without owning it.
		static track_typed<track_type_> make_ref(const desc_type& desc, sample_type* data, uint32_t num_samples, float sample_rate, uint32_t stride = sizeof(sample_type));

	private:
		//////////////////////////////////////////////////////////////////////////
		// We prohibit copying, use get_copy() and get_ref() instead.
		track_typed(const track_typed&) = delete;
		track_typed& operator=(const track_typed&) = delete;

		//////////////////////////////////////////////////////////////////////////
		// Internal constructor.
		track_typed(iallocator* allocator, uint8_t* data, uint32_t num_samples, uint32_t stride, size_t data_size, float sample_rate, const desc_type& desc) noexcept;
	};

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track into the desired track type while asserting for safety.
	template<typename track_type>
	track_type& track_cast(track& track_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track into the desired track type while asserting for safety.
	template<typename track_type>
	const track_type& track_cast(const track& track_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track into the desired track type. Returns nullptr if the types
	// are not compatible or if the input is nullptr.
	template<typename track_type>
	track_type* track_cast(track* track_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track into the desired track type. Returns nullptr if the types
	// are not compatible or if the input is nullptr.
	template<typename track_type>
	const track_type* track_cast(const track* track_);

	//////////////////////////////////////////////////////////////////////////
	// Create aliases for the various typed track types.

	using track_float1f			= track_typed<track_type8::float1f>;
	using track_float2f			= track_typed<track_type8::float2f>;
	using track_float3f			= track_typed<track_type8::float3f>;
	using track_float4f			= track_typed<track_type8::float4f>;
	using track_vector4f		= track_typed<track_type8::vector4f>;
	using track_qvvf			= track_typed<track_type8::qvvf>;

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/track.impl.h"

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

ACL_IMPL_FILE_PRAGMA_POP
