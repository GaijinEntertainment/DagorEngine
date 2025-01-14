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
#include "acl/core/compressed_database.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/error.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/database/database_settings.h"
#include "acl/decompression/database/database_streamer.h"
#include "acl/decompression/database/impl/database_context.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Encapsulates the possible streaming request results.
	//////////////////////////////////////////////////////////////////////////
	enum class database_stream_request_result
	{
		//////////////////////////////////////////////////////////////////////////
		// Streaming is done for the requested tier
		done,

		//////////////////////////////////////////////////////////////////////////
		// The streaming request has been dispatched
		dispatched,

		//////////////////////////////////////////////////////////////////////////
		// The streaming request has been ignored because streaming is already in progress
		streaming_in_progress,

		//////////////////////////////////////////////////////////////////////////
		// The database context isn't initialized
		context_not_initialized,

		//////////////////////////////////////////////////////////////////////////
		// Invalid database tier specified
		invalid_database_tier,

		//////////////////////////////////////////////////////////////////////////
		// Ran out of streaming requests
		no_free_streaming_requests,
	};

	//////////////////////////////////////////////////////////////////////////
	// Database decompression context for the uniformly sampled algorithm. The context
	// allows various streaming actions to be performed on the database.
	//
	// Both the constructor and destructor are public because it is safe to place
	// instances of this context on the stack or as member variables.
	//
	// Note that the context only manages the streaming bookkeeping, the actual IO is
	// handled by the database_streamer implementation provided.
	//////////////////////////////////////////////////////////////////////////
	template<class database_settings_type>
	class database_context
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// An alias to the database settings type.
		using settings_type = database_settings_type;

		//////////////////////////////////////////////////////////////////////////
		// Constructs a context instance.
		database_context();

		//////////////////////////////////////////////////////////////////////////
		// Destructs a context instance.
		~database_context();

		//////////////////////////////////////////////////////////////////////////
		// Returns the compressed database bound to this context instance.
		const compressed_database* get_compressed_database() const;

		//////////////////////////////////////////////////////////////////////////
		// Initializes the context instance to a particular compressed database instance.
		// No streaming can be performed and it is assumed that all the data is present in memory.
		// Returns whether initialization was successful or not.
		bool initialize(iallocator& allocator, const compressed_database& database);

		//////////////////////////////////////////////////////////////////////////
		// Initializes the context instance to a particular compressed database instance.
		// The streamer instances will be used to issue IO stream in/out requests.
		// If a tier is stripped, the null_database_streamer can be used.
		// Returns whether initialization was successful or not.
		bool initialize(iallocator& allocator, const compressed_database& database, database_streamer& medium_tier_streamer, database_streamer& low_tier_streamer);

		//////////////////////////////////////////////////////////////////////////
		// Returns true if this context instance is bound to a compressed database instance, false otherwise.
		bool is_initialized() const;

		//////////////////////////////////////////////////////////////////////////
		// Resets the context instance to its default constructed state. If the context was
		// currently bound to a database instance, it will no longer be bound to it.
		void reset();

		//////////////////////////////////////////////////////////////////////////
		// If the bound compressed database instance has relocated elsewhere in memory, this function
		// rebinds the context to it, avoiding the need to re-initialize it entirely.
		// Returns whether rebinding was successful or not.
		bool relocated(const compressed_database& database);

		//////////////////////////////////////////////////////////////////////////
		// If the bound compressed database instance has relocated elsewhere in memory, this function
		// rebinds the context to it, avoiding the need to re-initialize it entirely.
		// This can also be called if the streamers relocated, it will cause them to be rebound as well.
		// Assumes that the streamers have retained the same bulk data state as well. If it is
		// not the case, reset and re-initialize the context.
		// Returns whether rebinding was successful or not.
		bool relocated(const compressed_database& database, database_streamer& medium_tier_streamer, database_streamer& low_tier_streamer);

		//////////////////////////////////////////////////////////////////////////
		// Returns true if this context instance is bound to the specified database instance, false otherwise.
		bool is_bound_to(const compressed_database& database) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether the database bound contains the provided compressed tracks instance data.
		bool contains(const compressed_tracks& tracks) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not we have streamed in any of the database bulk data for the specified tier (medium or low).
		bool is_streamed_in(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not a streaming request is in flight for the specified tier (medium or low).
		bool is_streaming(quality_tier tier) const;

		//////////////////////////////////////////////////////////////////////////
		// Issues a stream in request and returns the current status for the specified tier (medium or low).
		// By default, every chunk will be streamed in but they can be streamed progressively
		// by providing a number of chunks.
		database_stream_request_result stream_in(quality_tier tier, uint32_t num_chunks_to_stream = ~0U);

		//////////////////////////////////////////////////////////////////////////
		// Issues a stream out request and returns the current status for the specified tier (medium or low).
		// By default, every chunk will be streamed out but they can be streamed progressively
		// by providing a number of chunks.
		database_stream_request_result stream_out(quality_tier tier, uint32_t num_chunks_to_stream = ~0U);

	private:
		database_context(const database_context& other) = delete;
		database_context& operator=(const database_context& other) = delete;

		// Internal context data
		acl_impl::database_context_v0 m_context;

		static_assert(std::is_base_of<database_settings, settings_type>::value, "database_settings_type must derive from database_settings!");

		// TODO: I'd like to assert here but we use a dummy pointer to init the decompression context which triggers this
		//static_assert(settings_type::version_supported() != compressed_tracks_version16::none, "database_settings_type must support at least one version");
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/decompression/database/impl/database.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
