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
#include "acl/core/error.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/database/database_streamer.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// Implements a null streamer where we simply use the provided bulk data buffer and
	// perform no operations on it as everything is already streamed in.
	// This streamer is the default in-memory streaming implementation.
	// It cannot be shared between multiple tiers.
	////////////////////////////////////////////////////////////////////////////////
	class null_database_streamer final : public database_streamer
	{
	public:
		null_database_streamer(const uint8_t* bulk_data, uint32_t bulk_data_size)
			: database_streamer(&m_request, 1)
			, m_bulk_data(bulk_data)
			, m_bulk_data_size(bulk_data_size)
		{
			ACL_ASSERT(bulk_data != nullptr, "Bulk data buffer cannot be null");
		}

		virtual bool is_initialized() const override { return m_bulk_data_size == 0 || m_bulk_data != nullptr; }

		virtual const uint8_t* get_bulk_data(quality_tier tier) const override
		{
			ACL_ASSERT(tier != quality_tier::highest_importance, "Cannot stream the highest importance tier");
			(void)tier;
			return m_bulk_data;
		}

		virtual void stream_in(uint32_t offset, uint32_t size, bool can_allocate_bulk_data, quality_tier tier, streaming_request_id request_id) override
		{
			ACL_ASSERT(offset < m_bulk_data_size, "Steam offset is outside of the bulk data range");
			ACL_ASSERT(size <= m_bulk_data_size, "Stream size is larger than the bulk data size");
			ACL_ASSERT(uint64_t(offset) + uint64_t(size) <= uint64_t(m_bulk_data_size), "Streaming request is outside of the bulk data range");
			(void)offset;
			(void)size;
			(void)can_allocate_bulk_data;
			(void)tier;

			complete(request_id);
		}

		virtual void stream_out(uint32_t offset, uint32_t size, bool can_deallocate_bulk_data, quality_tier tier, streaming_request_id request_id) override
		{
			ACL_ASSERT(offset < m_bulk_data_size, "Steam offset is outside of the bulk data range");
			ACL_ASSERT(size <= m_bulk_data_size, "Stream size is larger than the bulk data size");
			ACL_ASSERT(uint64_t(offset) + uint64_t(size) <= uint64_t(m_bulk_data_size), "Streaming request is outside of the bulk data range");
			(void)offset;
			(void)size;
			(void)can_deallocate_bulk_data;
			(void)tier;

			complete(request_id);
		}

	private:
		null_database_streamer(const null_database_streamer&) = delete;
		null_database_streamer& operator=(const null_database_streamer&) = delete;

		const uint8_t* m_bulk_data;
		uint32_t m_bulk_data_size;

		streaming_request m_request;	// Everything is synchronous, we only need one request
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
