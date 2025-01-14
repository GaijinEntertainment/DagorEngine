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

// Included only once from database_streamer.h

#include "acl/version.h"
#include "acl/core/bitset.h"
#include "acl/core/impl/atomic.impl.h"
#include "acl/core/impl/compressed_headers.h"
#include "acl/decompression/database/impl/database_context.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline streaming_request_id make_request_id(uint32_t request_index, uint32_t generation_id)
		{
			return streaming_request_id{ uint64_t(generation_id) << 32 | request_index };
		}

		inline uint32_t get_request_index(streaming_request_id request_id)
		{
			return uint32_t(request_id.value);
		}

		inline uint32_t get_generation_id(streaming_request_id request_id)
		{
			return uint32_t(request_id.value >> 32);
		}

		inline void execute_request(bool success, database_context_v0& context, const streaming_request& request)
		{
			const quality_tier tier = request.tier;
			const uint32_t first_chunk_index = request.first_chunk_index;
			const uint32_t num_streaming_chunks = request.num_streaming_chunks;

			const uint32_t num_chunks_ = context.db->get_num_chunks(tier);
			const bitset_description desc_ = bitset_description::make_from_num_bits(num_chunks_);
			const uint32_t tier_index_ = uint32_t(tier) - 1;

			if (request.action == streaming_action::stream_in)
			{
				// Streaming in
				if (success)
				{
					const uint8_t*& bulk_data_ref = context.bulk_data[tier_index_];
					const uint8_t* bulk_data_ = bulk_data_ref;
					if (bulk_data_ == nullptr)
					{
						// This is the first stream in request, our bulk data should be allocated now, query and cache it
						const database_streamer* streamer_ = context.streamers[tier_index_];
						bulk_data_ = streamer_->get_bulk_data(tier);
						ACL_ASSERT(bulk_data_ != nullptr, "Bulk data should be allocated when we stream in");

						bulk_data_ref = bulk_data_;
					}

					// Register our new chunks
					const database_header& header_ = get_database_header(*context.db);
					const database_chunk_description* chunk_descriptions_ = tier == quality_tier::medium_importance ? header_.get_chunk_descriptions_medium() : header_.get_chunk_descriptions_low();
					const uint32_t end_chunk_index = first_chunk_index + num_streaming_chunks;
					for (uint32_t chunk_index = first_chunk_index; chunk_index < end_chunk_index; ++chunk_index)
					{
						const database_chunk_description& chunk_description = chunk_descriptions_[chunk_index];
						const database_chunk_header* chunk_header = chunk_description.get_chunk_header(bulk_data_);
						ACL_ASSERT(chunk_header->index == chunk_index, "Unexpected chunk index");

						const database_chunk_segment_header* chunk_segment_headers = chunk_header->get_segment_headers();
						const uint32_t num_segments = chunk_header->num_segments;
						for (uint32_t segment_index = 0; segment_index < num_segments; ++segment_index)
						{
							const database_chunk_segment_header& chunk_segment_header = chunk_segment_headers[segment_index];

#if defined(ACL_HAS_ASSERT_CHECKS)
							const database_runtime_clip_header* clip_header = chunk_segment_header.get_clip_header(context.clip_segment_headers);
							ACL_ASSERT(clip_header->clip_hash == chunk_segment_header.clip_hash, "Unexpected clip hash");
#endif

							database_runtime_segment_header* segment_header = chunk_segment_header.get_segment_header(context.clip_segment_headers);
							ACL_ASSERT(segment_header->tier_metadata[tier_index_].load(k_memory_order_relaxed) == 0, "Tier metadata should not be initialized");
							segment_header->tier_metadata[tier_index_].store((uint64_t(chunk_segment_header.samples_offset) << 32) | chunk_segment_header.sample_indices, k_memory_order_relaxed);
						}
					}

					// Mark chunks as done streaming
					uint32_t* loaded_chunks_ = context.loaded_chunks[tier_index_];
					bitset_set_range(loaded_chunks_, desc_, first_chunk_index, num_streaming_chunks, true);
				}

				// Mark chunks as no longer streaming
				uint32_t* streaming_chunks_ = context.streaming_chunks[tier_index_];
				bitset_set_range(streaming_chunks_, desc_, first_chunk_index, num_streaming_chunks, false);
			}
			else
			{
				// Streaming out
				ACL_ASSERT(success, "Cannot cancel/abort stream out request");

				// Mark chunks as done streaming out
				uint32_t* loaded_chunks_ = context.loaded_chunks[tier_index_];
				bitset_set_range(loaded_chunks_, desc_, first_chunk_index, num_streaming_chunks, false);

				// Mark chunks as no longer streaming
				uint32_t* streaming_chunks_ = context.streaming_chunks[tier_index_];
				bitset_set_range(streaming_chunks_, desc_, first_chunk_index, num_streaming_chunks, false);
			}
		}
	}

	inline database_streamer::database_streamer(streaming_request* requests, uint32_t num_requests)
		: m_context(nullptr)
		, m_requests(requests)
		, m_generation_id(0)
		, m_next_request_index(0)
		, m_num_requests(num_requests)
	{
		ACL_ASSERT(requests != nullptr, "Requests must be valid");
		ACL_ASSERT(num_requests != 0, "Must have at least one request");
	}

	inline void database_streamer::complete(streaming_request_id request_id)
	{
		const uint32_t request_index = acl_impl::get_request_index(request_id);
		ACL_ASSERT(request_index < m_num_requests, "Invalid request index");
		if (request_index >= m_num_requests)
			return;

		const uint32_t generation_id = acl_impl::get_generation_id(request_id);

		streaming_request& request = m_requests[request_index];
		ACL_ASSERT(request.is_valid(), "Request is invalid");
		if (!request.is_valid())
			return;

		ACL_ASSERT(request.generation_id == generation_id, "Unexpected request generation id");
		if (request.generation_id != generation_id)
			return;

		acl_impl::execute_request(true, *m_context, request);

		// Mark our request as invalid
		request.reset();
	}

	inline void database_streamer::cancel(streaming_request_id request_id)
	{
		const uint32_t request_index = acl_impl::get_request_index(request_id);
		ACL_ASSERT(request_index < m_num_requests, "Invalid request index");
		if (request_index >= m_num_requests)
			return;

		const uint32_t generation_id = acl_impl::get_generation_id(request_id);

		streaming_request& request = m_requests[request_index];
		ACL_ASSERT(request.is_valid(), "Request is invalid");
		if (!request.is_valid())
			return;

		ACL_ASSERT(request.generation_id == generation_id, "Unexpected request generation id");
		if (request.generation_id != generation_id)
			return;

		acl_impl::execute_request(false, *m_context, request);

		// Mark our request as invalid
		request.reset();
	}

	inline void database_streamer::bind(acl_impl::database_context_v0& context)
	{
		ACL_ASSERT(m_context == nullptr || m_context == &context, "Streamer cannot be bound to two different database contexts");
		m_context = &context;
	}

	inline streaming_request_id database_streamer::build_request(streaming_action action, quality_tier tier, uint32_t first_chunk_index, uint32_t num_streaming_chunks)
	{
		const uint32_t request_index = m_next_request_index;
		streaming_request& request = m_requests[request_index];
		ACL_ASSERT(request.tier == quality_tier::highest_importance, "Ran out of free requests");
		if (request.tier != quality_tier::highest_importance)
			return k_invalid_streamer_request_id;

		// This request entry is valid, we'll use it
		m_next_request_index = (m_next_request_index + 1) % m_num_requests;

		// Get our generation id
		const uint32_t generation_id = m_generation_id++;

		request.action = action;
		request.tier = tier;
		request.first_chunk_index = first_chunk_index;
		request.num_streaming_chunks = num_streaming_chunks;
		request.generation_id = generation_id;

		return acl_impl::make_request_id(request_index, generation_id);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
