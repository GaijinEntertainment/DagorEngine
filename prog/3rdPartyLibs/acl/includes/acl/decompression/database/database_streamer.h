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
#include "acl/core/quality_tiers.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct database_context_v0;
	}

	//////////////////////////////////////////////////////////////////////////
	// Enum representing a streaming action
	//////////////////////////////////////////////////////////////////////////
	enum class streaming_action
	{
		stream_in,
		stream_out,
	};

	//////////////////////////////////////////////////////////////////////////
	// A streaming request
	//////////////////////////////////////////////////////////////////////////
	struct streaming_request
	{
		streaming_action		action = streaming_action::stream_in;
		quality_tier			tier = quality_tier::highest_importance;	// Marks the request as invalid since we never stream the highest tier
		uint32_t				first_chunk_index = 0;
		uint32_t				num_streaming_chunks = 0;
		uint32_t				generation_id = 0;

		bool is_valid() const { return tier != quality_tier::highest_importance; }
		void reset() { tier = quality_tier::highest_importance; }
	};

	//////////////////////////////////////////////////////////////////////////
	// A streaming request ID
	//
	// The internal value is an implementation detail.
	//////////////////////////////////////////////////////////////////////////
	struct streaming_request_id
	{
		// Opaque value
		uint64_t value;

		bool is_valid() const { return value != ~0ULL; }
	};

	// An invalid streaming request ID
	constexpr streaming_request_id k_invalid_streamer_request_id = { ~0ULL };

	////////////////////////////////////////////////////////////////////////////////
	// The base class for database streamers.
	//
	// Streamer implementations are responsible for allocating/freeing the bulk data as well as
	// streaming the data in/out. Streaming in is safe from any thread but streaming out
	// cannot happen while decompression is in progress otherwise the behavior is undefined.
	// A single streamer instance can be shared between all quality tiers of a database.
	// Streamers cannot be shared by multiple databases.
	// A streamer implementation must also provide a list of streaming requests to use and
	// recycle. A proper implementation should consider how many requests it needs.
	////////////////////////////////////////////////////////////////////////////////
	class database_streamer
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// Streamer destructor
		virtual ~database_streamer() = default;

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the streamer is initialized.
		virtual bool is_initialized() const = 0;

		//////////////////////////////////////////////////////////////////////////
		// Returns a valid pointer to the bulk data used to decompress from for the specified quality tier.
		// Note that the pointer will not be used until after the first successful stream in
		// request is completed. As such, it is safe to allocate the bulk data when the first
		// stream in request happens.
		virtual const uint8_t* get_bulk_data(quality_tier tier) const = 0;

		//////////////////////////////////////////////////////////////////////////
		// Called when we request some data to be streamed in.
		// Only one stream in/out request can be in flight at a time per quality tier.
		// Streaming in animation data can be done while animations are decompressing (async).
		//
		// The offset into the bulk data and the size in bytes to stream in are provided as arguments.
		// On the first stream in request, the bulk data can be allocated but its pointer cannot change with subsequent
		// stream in requests until everything has been streamed out.
		// Once the streaming request has been fulfilled (sync or async), call complete(..) or cancel(..) with the provided
		// request ID.
		virtual void stream_in(uint32_t offset, uint32_t size, bool can_allocate_bulk_data, quality_tier tier, streaming_request_id request_id) = 0;

		//////////////////////////////////////////////////////////////////////////
		// Called when we request some data to be streamed out.
		// Only one stream in/out request can be in flight at a time per quality tier.
		// Streaming out animation data cannot be done while animations are decompressing.
		// Doing so will result in undefined behavior as the data could be in use while we stream it out.
		//
		// The offset into the bulk data and the size in bytes to stream out are provided as arguments.
		// On the last stream out request, the bulk data can be deallocated. It will be allocated again
		// if the data streams back in.
		// Once the streaming request has been fulfilled (sync or async), call complete(..) or cancel(..) with the provided
		// request ID.
		//
		// A stream out request CANNOT be canceled!
		virtual void stream_out(uint32_t offset, uint32_t size, bool can_deallocate_bulk_data, quality_tier tier, streaming_request_id request_id) = 0;

		//////////////////////////////////////////////////////////////////////////
		// Signifies that the streamer has completed this streaming request successfully.
		// The bulk data must be allocated and ready to use.
		void complete(streaming_request_id request_id);

		//////////////////////////////////////////////////////////////////////////
		// Signifies that the streamer will not complete this streaming request.
		void cancel(streaming_request_id request_id);

	protected:
		//////////////////////////////////////////////////////////////////////////
		// Constructs the database streamer.
		// The provided requests will be used and recycled internally when we stream in/out.
		database_streamer(streaming_request* requests, uint32_t num_requests);

	private:
		//////////////////////////////////////////////////////////////////////////
		// Binds this streamer instance to our database context.
		// The same streamer can be bound multiple times to the same database context.
		void bind(acl_impl::database_context_v0& context);

		//////////////////////////////////////////////////////////////////////////
		// Finds and builds a new streaming request if we have one to spare, otherwise
		// the request ID is invalid.
		streaming_request_id build_request(streaming_action action, quality_tier tier, uint32_t first_chunk_index, uint32_t num_streaming_chunks);

		acl_impl::database_context_v0* m_context;
		streaming_request* m_requests;

		uint32_t m_generation_id;
		uint32_t m_next_request_index;
		uint32_t m_num_requests;

		template<class database_settings_type>
		friend class database_context;
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/decompression/database/impl/database_streamer.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
