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
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	class database_streamer;

	namespace acl_impl
	{
		// TODO: If we need to make the context smaller, we can use offsets for the bitsets instead of pointers
		// from the clip_segment_headers base pointer. The bitsets also follow linearly in memory, we could store only
		// one offset for the base, and index with the tier * desc.size
		struct database_context_v0
		{
			//																	//   offsets
			// Only member used to detect if we are initialized, must be first
			const compressed_database* db = nullptr;							//   0 |   0

			// We use arrays so we can index with (tier - 1) as our index
			// Index 0 = medium importance tier, index 1 = low importance

			// Runtime related data, commonly accessed
			uint8_t* clip_segment_headers = nullptr;							//   4 |   8

			const uint8_t* bulk_data[k_num_database_tiers] = { nullptr };		//   8 |  16

			// Streaming related data not commonly accessed
			database_streamer* streamers[k_num_database_tiers] = { nullptr };	//  16 |  32

			uint32_t* loaded_chunks[k_num_database_tiers] = { nullptr };		//  24 |  48
			uint32_t* streaming_chunks[k_num_database_tiers] = { nullptr };		//  32 |  64

			iallocator* allocator = nullptr;									//  40 |  72

			// Cached hash of the bound database instance
			uint32_t db_hash = 0;												//  44 |  88

			uint8_t padding1[sizeof(void*) == 4 ? 16 : 36] = { 0 };				//  48 |  92

			//														Total size:	    64 | 128

			//////////////////////////////////////////////////////////////////////////

			const compressed_database* get_compressed_database() const { return db; }
			compressed_tracks_version16 get_version() const { return db->get_version(); }
			bool is_initialized() const { return db != nullptr; }
			void reset() { db = nullptr; }
		};

		static_assert((sizeof(database_context_v0) % 64) == 0, "Unexpected size");
		static_assert(offsetof(database_context_v0, db) == 0, "db pointer needs to be the first member, see initialize_v0");
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
