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
#include "acl/core/iallocator.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/database/database_streamer.h"

#include <cstdint>
#include <cstring>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// Implements a debug streamer where we duplicate the bulk data in memory and use
	// memcpy to stream in the data. Streamed out data is explicitly set to 0xCD with memset.
	// It cannot be shared between tiers.
	////////////////////////////////////////////////////////////////////////////////
	class debug_database_streamer final : public database_streamer
	{
	public:
		debug_database_streamer(iallocator& allocator, const uint8_t* bulk_data, uint32_t bulk_data_size)
			: database_streamer(m_requests, k_max_num_requests)
			, m_allocator(allocator)
			, m_src_bulk_data(bulk_data)
			, m_streamed_bulk_data(nullptr)
			, m_bulk_data_size(bulk_data_size)
		{
		}

		virtual ~debug_database_streamer() override
		{
			deallocate_type_array(m_allocator, m_streamed_bulk_data, m_bulk_data_size);
		}

		virtual bool is_initialized() const override { return m_bulk_data_size == 0 || m_src_bulk_data != nullptr; }

		virtual const uint8_t* get_bulk_data(quality_tier tier) const override
		{
			ACL_ASSERT(tier != quality_tier::highest_importance, "Cannot stream the highest importance tier");
			(void)tier;
			return m_streamed_bulk_data;
		}

		virtual void stream_in(uint32_t offset, uint32_t size, bool can_allocate_bulk_data, quality_tier tier, streaming_request_id request_id) override
		{
			ACL_ASSERT(offset < m_bulk_data_size, "Steam offset is outside of the bulk data range");
			ACL_ASSERT(size <= m_bulk_data_size, "Stream size is larger than the bulk data size");
			ACL_ASSERT(uint64_t(offset) + uint64_t(size) <= uint64_t(m_bulk_data_size), "Streaming request is outside of the bulk data range");
			(void)tier;

			if (can_allocate_bulk_data)
			{
				ACL_ASSERT(m_streamed_bulk_data == nullptr, "Bulk data already allocated");

				m_streamed_bulk_data = allocate_type_array<uint8_t>(m_allocator, m_bulk_data_size);

				std::memset(m_streamed_bulk_data, 0xCD, m_bulk_data_size);
			}

			std::memcpy(m_streamed_bulk_data + offset, m_src_bulk_data + offset, size);
			complete(request_id);
		}

		virtual void stream_out(uint32_t offset, uint32_t size, bool can_deallocate_bulk_data, quality_tier tier, streaming_request_id request_id) override
		{
			ACL_ASSERT(offset < m_bulk_data_size, "Steam offset is outside of the bulk data range");
			ACL_ASSERT(size <= m_bulk_data_size, "Stream size is larger than the bulk data size");
			ACL_ASSERT(uint64_t(offset) + uint64_t(size) <= uint64_t(m_bulk_data_size), "Streaming request is outside of the bulk data range");
			(void)tier;

			std::memset(m_streamed_bulk_data + offset, 0xCD, size);

			if (can_deallocate_bulk_data)
			{
				ACL_ASSERT(m_streamed_bulk_data != nullptr, "Bulk data already deallocated");

				deallocate_type_array(m_allocator, m_streamed_bulk_data, m_bulk_data_size);
				m_streamed_bulk_data = nullptr;
			}

			complete(request_id);
		}

	private:
		debug_database_streamer(const debug_database_streamer&) = delete;
		debug_database_streamer& operator=(const debug_database_streamer&) = delete;

		iallocator& m_allocator;
		const uint8_t* m_src_bulk_data;
		uint8_t* m_streamed_bulk_data;
		uint32_t m_bulk_data_size;

		static constexpr uint32_t k_max_num_requests = k_num_database_tiers;	// One per database tier
		streaming_request m_requests[k_max_num_requests];
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
