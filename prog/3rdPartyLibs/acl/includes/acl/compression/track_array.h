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
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/sample_rounding_policy.h"
#include "acl/core/time_utils.h"
#include "acl/core/track_types.h"
#include "acl/core/track_writer.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/track.h"

#include <cstdint>
#include <limits>
#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// An array of tracks.
	// Although each track contained within is untyped, each track must have
	// the same type. They must all have the same sample rate and the same
	// number of samples.
	//////////////////////////////////////////////////////////////////////////
	class track_array
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// Constructs an empty track array.
		track_array() noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Constructs an array with the specified number of tracks.
		// Tracks will be empty and untyped by default.
		track_array(iallocator& allocator, uint32_t num_tracks);

		//////////////////////////////////////////////////////////////////////////
		// Move constructor for a track array.
		track_array(track_array&& other) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Destroys a track array.
		~track_array();

		//////////////////////////////////////////////////////////////////////////
		// Move assignment for a track array.
		track_array& operator=(track_array&& other) noexcept;

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to the allocator instance or nullptr if there is none present.
		iallocator* get_allocator() const { return m_allocator; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of tracks contained in this array.
		uint32_t get_num_tracks() const { return m_num_tracks; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of samples per track in this array.
		// This does not account for the repeating first sample when the wrap
		// looping policy is used.
		// See `sample_looping_policy` for details.
		uint32_t get_num_samples_per_track() const { return m_allocator != nullptr && m_num_tracks != 0 ? m_tracks->get_num_samples() : 0; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track type for tracks in this array.
		track_type8 get_track_type() const { return m_allocator != nullptr && m_num_tracks != 0 ? m_tracks->get_type() : track_type8::float1f; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track category for tracks in this array.
		track_category8 get_track_category() const { return m_allocator != nullptr && m_num_tracks != 0 ? m_tracks->get_category() : track_category8::scalarf; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the sample rate for tracks in this array.
		float get_sample_rate() const { return m_allocator != nullptr && m_num_tracks != 0 ? m_tracks->get_sample_rate() : 0.0F; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the duration for tracks in this array.
		// Note that the duration depends on the looping policy.
		// See `sample_looping_policy` for details.
		float get_duration() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the finite duration for tracks in this array.
		// Note that the duration depends on the looping policy.
		// See `sample_looping_policy` for details.
		float get_finite_duration() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track name.
		const string& get_name() const { return m_name; }

		//////////////////////////////////////////////////////////////////////////
		// Sets the track name.
		void set_name(const string& name) { m_name = name.get_copy(); }

		//////////////////////////////////////////////////////////////////////////
		// Sets the looping policy.
		// Note that when wrap policy is used, an extra repeating
		// first sample is artificially inserted at the end of the clip.
		// This artificial sample maps to the first sample, it only lives once in memory.
		// This allows us to interpolate from the last sample back to the first
		// sample when looping and wrapping during playback.
		// See `sample_looping_policy` for details.
		// The `sample_looping_policy::as_compressed` value isn't allowed here.
		void set_looping_policy(sample_looping_policy policy);

		//////////////////////////////////////////////////////////////////////////
		// Returns the looping policy.
		sample_looping_policy get_looping_policy() const { return m_looping_policy; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track at the specified index.
		track& operator[](uint32_t index);

		//////////////////////////////////////////////////////////////////////////
		// Returns the track at the specified index.
		const track& operator[](uint32_t index) const;

		//////////////////////////////////////////////////////////////////////////
		// Iterator begin() and end() implementations.
		track* begin() { return m_tracks; }
		const track* begin() const { return m_tracks; }
		track* end() { return m_tracks + m_num_tracks; }
		const track* end() const { return m_tracks + m_num_tracks; }

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the track array doesn't contain any data, false otherwise.
		bool is_empty() const { return m_num_tracks == 0; }

		//////////////////////////////////////////////////////////////////////////
		// Returns whether a track array is valid or not.
		// An array is valid if:
		//    - It is empty
		//    - All tracks have the same type
		//    - All tracks have the same number of samples
		//    - All tracks have the same sample rate
		//    - All tracks are valid
		error_result is_valid() const;

		//////////////////////////////////////////////////////////////////////////
		// Sample all tracks within this array at the specified sample time and
		// desired rounding policy. Track samples are written out using the `track_writer` provided.
		// The sample_time value must be within [0, clip duration] inclusive otherwise it will be clamped.
		template<class track_writer_type>
		void sample_tracks(float sample_time, sample_rounding_policy rounding_policy, track_writer_type& writer) const;

		//////////////////////////////////////////////////////////////////////////
		// Sample a single track within this array at the specified sample time and
		// desired rounding policy. The track sample is written out using the `track_writer` provided.
		// The sample_time value must be within [0, clip duration] inclusive otherwise it will be clamped.
		template<class track_writer_type>
		void sample_track(uint32_t track_index, float sample_time, sample_rounding_policy rounding_policy, track_writer_type& writer) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the raw size for this track array. Note that this differs from the actual
		// memory used by an instance of this class. It is meant for comparison against
		// the compressed size.
		uint32_t get_raw_size() const;

	protected:
		//////////////////////////////////////////////////////////////////////////
		// We prohibit copying
		track_array(const track_array&) = delete;
		track_array& operator=(const track_array&) = delete;

		iallocator*				m_allocator;		// The allocator used to allocate our tracks
		track*					m_tracks;			// The track list
		uint32_t				m_num_tracks;		// The number of tracks
		sample_looping_policy	m_looping_policy;	// The looping policy

		string					m_name;				// An optional name
	};

	//////////////////////////////////////////////////////////////////////////
	// A typed track array. See `track_array` for details.
	//////////////////////////////////////////////////////////////////////////
	template<track_type8 track_type_>
	class track_array_typed final : public track_array
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// The track type.
		static constexpr track_type8 type = track_type_;

		//////////////////////////////////////////////////////////////////////////
		// The track category.
		static constexpr track_category8 category = track_traits<track_type_>::category;

		//////////////////////////////////////////////////////////////////////////
		// The track member type.
		using track_member_type = track_typed<track_type_>;

		//////////////////////////////////////////////////////////////////////////
		// Constructs an empty track array.
		track_array_typed() noexcept : track_array() { static_assert(sizeof(track_array_typed) == sizeof(track_array), "You cannot add member variables to this class"); }

		//////////////////////////////////////////////////////////////////////////
		// Constructs an array with the specified number of tracks.
		// Tracks will be empty and untyped by default.
		track_array_typed(iallocator& allocator, uint32_t num_tracks) : track_array(allocator, num_tracks) {}

		//////////////////////////////////////////////////////////////////////////
		// Cannot copy
		track_array_typed(const track_array_typed&) = delete;

		//////////////////////////////////////////////////////////////////////////
		// Move constructor for a track array.
		track_array_typed(track_array_typed&& other) noexcept : track_array(static_cast<track_array&&>(other)) {}

		//////////////////////////////////////////////////////////////////////////
		// Destroys a track array.
		~track_array_typed() = default;

		//////////////////////////////////////////////////////////////////////////
		// Cannot copy
		track_array_typed& operator=(const track_array_typed&) = delete;

		//////////////////////////////////////////////////////////////////////////
		// Move assignment for a track array.
		track_array_typed& operator=(track_array_typed&& other) noexcept { return static_cast<track_array_typed&>(track_array::operator=(static_cast<track_array&&>(other))); }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track type for tracks in this array.
		track_type8 get_track_type() const { return type; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track category for tracks in this array.
		track_category8 get_track_category() const { return category; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the track at the specified index.
		track_member_type& operator[](uint32_t index);

		//////////////////////////////////////////////////////////////////////////
		// Returns the track at the specified index.
		const track_member_type& operator[](uint32_t index) const;

		//////////////////////////////////////////////////////////////////////////
		// Iterator begin() and end() implementations.
		track_member_type* begin() { return track_cast<track_member_type>(m_tracks); }
		const track_member_type* begin() const { return track_cast<track_member_type>(m_tracks); }
		track_member_type* end() { return track_cast<track_member_type>(m_tracks) + m_num_tracks; }
		const track_member_type* end() const { return track_cast<track_member_type>(m_tracks) + m_num_tracks; }
	};

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track array into the desired track array type while asserting for safety.
	template<typename track_array_type>
	track_array_type& track_array_cast(track_array& track_array_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track array into the desired track array type while asserting for safety.
	template<typename track_array_type>
	const track_array_type& track_array_cast(const track_array& track_array_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track array into the desired track array type. Returns nullptr if the types
	// are not compatible or if the input is nullptr.
	template<typename track_array_type>
	track_array_type* track_array_cast(track_array* track_array_);

	//////////////////////////////////////////////////////////////////////////
	// Casts an untyped track array into the desired track array type. Returns nullptr if the types
	// are not compatible or if the input is nullptr.
	template<typename track_array_type>
	const track_array_type* track_array_cast(const track_array* track_array_);

	//////////////////////////////////////////////////////////////////////////
	// Create aliases for the various typed track array types.

	using track_array_float1f	= track_array_typed<track_type8::float1f>;
	using track_array_float2f	= track_array_typed<track_type8::float2f>;
	using track_array_float3f	= track_array_typed<track_type8::float3f>;
	using track_array_float4f	= track_array_typed<track_type8::float4f>;
	using track_array_vector4f	= track_array_typed<track_type8::vector4f>;
	using track_array_qvvf		= track_array_typed<track_type8::qvvf>;

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/track_array.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
