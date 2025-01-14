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
#include "acl/core/buffer_tag.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error_result.h"
#include "acl/core/hash.h"
#include "acl/core/quality_tiers.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/impl/compressed_headers.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// An instance of a compressed database.
	// The compressed data immediately follows this instance in memory.
	// The total size of the buffer can be queried with `get_size()`.
	// A compressed database can either contain all the data inline within its buffer
	// in one blob or it can be split into smaller chunks that can be streamed in and out.
	////////////////////////////////////////////////////////////////////////////////
	class alignas(16) compressed_database final
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Returns the size in bytes of the compressed database.
		// Includes the 'compressed_database' instance size and the size of the inline bulk data (if present).
		uint32_t get_size() const { return m_buffer_header.size; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the total size in bytes of the compressed database.
		// Includes non-inline bulk data.
		uint32_t get_total_size() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the size in bytes of the bulk data for the specified tier (medium or low).
		uint32_t get_bulk_data_size(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not the specified quality tier contains bulk data.
		bool has_bulk_data(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the hash for the compressed database.
		// This is only used for sanity checking in case of memory corruption.
		uint32_t get_hash() const { return m_buffer_header.hash; }

		//////////////////////////////////////////////////////////////////////////
		// Returns the hash of the bulk data for the specified tier (medium or low).
		// This is only used for sanity checking in case of memory corruption.
		uint32_t get_bulk_data_hash(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the binary tag for the compressed database.
		// This uniquely identifies the buffer as a proper 'compressed_database' object.
		buffer_tag32 get_tag() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the binary format version.
		compressed_tracks_version16 get_version() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of chunks contained in this database for the specified tier (medium or low).
		uint32_t get_num_chunks(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the maximum size in bytes of each chunk.
		uint32_t get_max_chunk_size() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of clips contained in this database.
		uint32_t get_num_clips() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns the number of segments contained in this database (for all clips).
		uint32_t get_num_segments() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not the bulk data is stored inline in this compressed database.
		bool is_bulk_data_inline() const;

		//////////////////////////////////////////////////////////////////////////
		// Returns a pointer to the bulk data for the specified tier (medium or low) when it is inline, nullptr otherwise.
		const uint8_t* get_bulk_data(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the compressed database contains the provided compressed tracks instance.
		bool contains(const compressed_tracks& tracks) const;

		////////////////////////////////////////////////////////////////////////////////
		// Returns true if the compressed database is valid and usable.
		// This mainly validates some invariants as well as ensuring that the
		// memory has not been corrupted.
		//
		// check_hash: If true, the compressed tracks hash will also be compared.
		error_result is_valid(bool check_hash) const;

	private:
		////////////////////////////////////////////////////////////////////////////////
		// Hide everything
		compressed_database() = delete;
		compressed_database(const compressed_database&) = delete;
		compressed_database* operator=(const compressed_database&) = delete;

		// Do not use 'delete' or an equivalent allocator function as it won't know
		// the correct allocated size. Deallocating an instance must match how it has
		// been allocated.
		// See iallocator::deallocate
		// e.g: allocator->deallocate(database, database->get_size());
		~compressed_database() = delete;

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

		// Here we define some unspecified padding but the 'database_header' starts here.
		// This is done to ensure that this class is 16 byte aligned without requiring further padding
		// if the 'database_header' ends up causing us to be unaligned.
		uint32_t m_padding[2];
	};

	//////////////////////////////////////////////////////////////////////////
	// Create a compressed_database instance in place from a raw memory buffer.
	// If the buffer does not contain a valid compressed_database instance, nullptr is returned
	// along with an optional error result.
	//////////////////////////////////////////////////////////////////////////
	const compressed_database* make_compressed_database(const void* buffer, error_result* out_error_result = nullptr);
	compressed_database* make_compressed_database(void* buffer, error_result* out_error_result = nullptr);

	constexpr uint32_t k_database_bulk_data_alignment = alignof(acl_impl::database_chunk_header);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/core/impl/compressed_database.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
