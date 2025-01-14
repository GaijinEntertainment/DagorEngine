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
#include "acl/core/algorithm_types.h"
#include "acl/core/buffer_tag.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/error_result.h"
#include "acl/core/hash.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/time_utils.h"
#include "acl/core/track_desc.h"
#include "acl/core/track_types.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// An instance of a compressed tracks.
	// The compressed data immediately follows this instance in memory.
	// The total size of the buffer can be queried with `get_size()`.
	////////////////////////////////////////////////////////////////////////////////
	class alignas(16) compressed_tracks final
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Returns the algorithm type used to compress the tracks.
		algorithm_type8 get_algorithm_type() const;

		////////////////////////////////////////////////////////////////////////////////
		// Returns the size in bytes of the compressed tracks.
		// Includes the 'compressed_tracks' instance size.
		uint32_t get_size() const { return m_buffer_header.size; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the hash for the compressed tracks.
		// This is only used for sanity checking in case of memory corruption.
		uint32_t get_hash() const { return m_buffer_header.hash; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the binary tag for the compressed tracks.
		// This uniquely identifies the buffer as a proper 'compressed_tracks' object.
		buffer_tag32 get_tag() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the binary format version.
		compressed_tracks_version16 get_version() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of tracks contained.
		uint32_t get_num_tracks() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of samples each track contains.
		// This does not account for the repeating first sample when the wrap
		// looping policy is used.
		// See `sample_looping_policy` for details.
		uint32_t get_num_samples_per_track() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the type of the compressed tracks.
		track_type8 get_track_type() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the duration of each track.
		// Note that when wrap policy is used, an extra repeating
		// first sample is artificially inserted at the end of the clip.
		// This artificial sample maps to the first sample, it only lives once in memory.
		// This allows us to interpolate from the last sample back to the first
		// sample when looping and wrapping during playback.
		// See `sample_looping_policy` for details.
		float get_duration(sample_looping_policy looping_policy = sample_looping_policy::as_compressed) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the finite duration of each track.
		// Note that when wrap policy is used, an extra repeating
		// first sample is artificially inserted at the end of the clip.
		// This artificial sample maps to the first sample, it only lives once in memory.
		// This allows us to interpolate from the last sample back to the first
		// sample when looping and wrapping during playback.
		// See `sample_looping_policy` for details.
		float get_finite_duration(sample_looping_policy looping_policy = sample_looping_policy::as_compressed) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the sample rate used by each track.
		float get_sample_rate() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not this clip is split into a compressed database instance.
		// Only supported with qvv transform tracks.
		bool has_database() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not every sub-track's default value is trivial (aka the identity).
		// Non-trivial default values indicate that extra data beyond the clip will be needed at decompression (e.g. bind pose)
		// Only supported with qvv transform tracks.
		bool has_trivial_default_values() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not this clip has had keyframes stripped.
		// Only supported with qvv transform tracks.
		bool has_stripped_keyframes() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the default scale value used during compression.
		// Depending on the additive type, this value will either be 0 or 1.
		// Only supported with qvv transform tracks.
		int32_t get_default_scale() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the looping policy used during compression.
		sample_looping_policy get_looping_policy() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track list name if metadata is present, nullptr otherwise.
		const char* get_name() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track name for the specified track index if metadata is present, k_invalid_track_index otherwise.
		const char* get_track_name(uint32_t track_index) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the parent track index for the specified track index if metadata is present, nullptr otherwise.
		// Only supported with qvv transform tracks.
		uint32_t get_parent_track_index(uint32_t track_index) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the track description for the specified track index if metadata is present.
		// Returns true on success, false otherwise.
		bool get_track_description(uint32_t track_index, track_desc_scalarf& out_description) const;
		bool get_track_description(uint32_t track_index, track_desc_transformf& out_description) const;

		bool is_animated(uint32_t track_index) const;

		////////////////////////////////////////////////////////////////////////////////
		// Returns true if the compressed tracks are valid and usable.
		// This mainly validates some invariants as well as ensuring that the
		// memory has not been corrupted.
		//
		// check_hash: If true, the compressed tracks hash will also be compared.
		error_result is_valid(bool check_hash) const;

	private:
		////////////////////////////////////////////////////////////////////////////////
		// Hide everything
		compressed_tracks() = delete;
		compressed_tracks(const compressed_tracks&) = delete;
		compressed_tracks* operator=(const compressed_tracks&) = delete;

		// Do not use 'delete' or an equivalent allocator function as it won't know
		// the correct allocated size. Deallocating an instance must match how it has
		// been allocated.
		// See iallocator::deallocate
		// e.g: allocator->deallocate(tracks, tracks->get_size());
		~compressed_tracks() = delete;

		////////////////////////////////////////////////////////////////////////////////
		// Raw buffer header that isn't included in the hash.
		////////////////////////////////////////////////////////////////////////////////

		acl_impl::raw_buffer_header		m_buffer_header;

		////////////////////////////////////////////////////////////////////////////////
		// Everything starting here is included in the hash.
		////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Compressed data follows here in memory.
		//////////////////////////////////////////////////////////////////////////

		// Here we define some unspecified padding but the 'tracks_header' starts here.
		// This is done to ensure that this class is 16 byte aligned without requiring further padding
		// if the 'tracks_header' ends up causing us to be unaligned.
		uint32_t m_padding[2];
	};

	//////////////////////////////////////////////////////////////////////////
	// Create a compressed_tracks instance in place from a raw memory buffer.
	// If the buffer does not contain a valid compressed_tracks instance, nullptr is returned
	// along with an optional error result.
	//////////////////////////////////////////////////////////////////////////
	const compressed_tracks* make_compressed_tracks(const void* buffer, error_result* out_error_result = nullptr);

	//////////////////////////////////////////////////////////////////////////
	// Create a compressed_tracks instance in place from a raw memory buffer.
	// If the buffer does not contain a valid compressed_tracks instance, nullptr is returned
	// along with an optional error result.
	//////////////////////////////////////////////////////////////////////////
	compressed_tracks* make_compressed_tracks(void* buffer, error_result* out_error_result = nullptr);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/core/impl/compressed_tracks.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
