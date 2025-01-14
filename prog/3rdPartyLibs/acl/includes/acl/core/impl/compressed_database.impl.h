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

// Included only once from compressed_database.h

#include "acl/version.h"
#include "acl/core/impl/bit_cast.impl.h"

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		// Hide these implementations, they shouldn't be needed in user-space
		inline const database_header& get_database_header(const compressed_database& db)
		{
			return *bit_cast<const database_header*>(bit_cast<const uint8_t*>(&db) + sizeof(raw_buffer_header));
		}
	}

	inline uint32_t compressed_database::get_total_size() const
	{
		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		if (header.get_is_bulk_data_inline())
			return m_buffer_header.size;
		else
			return m_buffer_header.size + header.bulk_data_size[0] + header.bulk_data_size[1];
	}

	inline uint32_t compressed_database::get_bulk_data_size(quality_tier tier) const
	{
		ACL_ASSERT(tier != quality_tier::highest_importance, "The database does not contain data for the high importance tier, it lives inside compressed_tracks");
		if (tier == quality_tier::highest_importance)
			return 0;

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		const uint32_t tier_index = uint32_t(tier) - 1;
		return header.bulk_data_size[tier_index];
	}

	inline bool compressed_database::has_bulk_data(quality_tier tier) const
	{
		ACL_ASSERT(tier != quality_tier::highest_importance, "The database does not contain data for the high importance tier, it lives inside compressed_tracks");
		if (tier == quality_tier::highest_importance)
			return false;

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		const uint32_t tier_index = uint32_t(tier) - 1;
		return header.bulk_data_size[tier_index] != 0;
	}

	inline uint32_t compressed_database::get_bulk_data_hash(quality_tier tier) const
	{
		ACL_ASSERT(tier != quality_tier::highest_importance, "The database does not contain data for the high importance tier, it lives inside compressed_tracks");
		if (tier == quality_tier::highest_importance)
			return 0;

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		const uint32_t tier_index = uint32_t(tier) - 1;
		return header.bulk_data_hash[tier_index];
	}

	inline buffer_tag32 compressed_database::get_tag() const { return static_cast<buffer_tag32>(acl_impl::get_database_header(*this).tag); }

	inline compressed_tracks_version16 compressed_database::get_version() const { return acl_impl::get_database_header(*this).version; }

	inline uint32_t compressed_database::get_num_chunks(quality_tier tier) const
	{
		ACL_ASSERT(tier != quality_tier::highest_importance, "The database does not contain data for the high importance tier, it lives inside compressed_tracks");
		if (tier == quality_tier::highest_importance)
			return 0;

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		const uint32_t tier_index = uint32_t(tier) - 1;
		return header.num_chunks[tier_index];
	}

	inline uint32_t compressed_database::get_max_chunk_size() const { return acl_impl::get_database_header(*this).max_chunk_size; }

	inline uint32_t compressed_database::get_num_clips() const { return acl_impl::get_database_header(*this).num_clips; }

	inline uint32_t compressed_database::get_num_segments() const { return acl_impl::get_database_header(*this).num_segments; }

	inline bool compressed_database::is_bulk_data_inline() const { return acl_impl::get_database_header(*this).get_is_bulk_data_inline(); }

	inline const uint8_t* compressed_database::get_bulk_data(quality_tier tier) const
	{
		ACL_ASSERT(tier != quality_tier::highest_importance, "The database does not contain data for the high importance tier, it lives inside compressed_tracks");
		if (tier == quality_tier::highest_importance)
			return nullptr;

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		const uint32_t tier_index = uint32_t(tier) - 1;
		return header.bulk_data_offset[tier_index].safe_add_to(&header);
	}

	inline bool compressed_database::contains(const compressed_tracks& tracks) const
	{
		if (!tracks.has_database())
			return false;	// Clip not bound to anything

		const acl_impl::database_header& db_header = acl_impl::get_database_header(*this);
		const acl_impl::database_clip_metadata* clips = db_header.get_clip_metadatas();
		const uint32_t clip_hash = tracks.get_hash();
		const uint32_t num_clips = db_header.num_clips;
		for (uint32_t clip_index = 0; clip_index < num_clips; ++clip_index)
		{
			if (clips[clip_index].clip_hash == clip_hash)
				return true;	// Contained!
		}

		// We didn't find our clip
		return false;
	}

	inline error_result compressed_database::is_valid(bool check_hash) const
	{
		if (!is_aligned_to(this, alignof(compressed_database)))
			return error_result("Invalid alignment");

		const acl_impl::database_header& header = acl_impl::get_database_header(*this);
		if (header.tag != static_cast<uint32_t>(buffer_tag32::compressed_database))
			return error_result("Invalid tag");

		if (header.version < compressed_tracks_version16::first || header.version > compressed_tracks_version16::latest)
			return error_result("Invalid database version");

		if (check_hash)
		{
			const uint32_t hash = hash32(safe_ptr_cast<const uint8_t>(&m_padding[0]), m_buffer_header.size - sizeof(acl_impl::raw_buffer_header));
			if (hash != m_buffer_header.hash)
				return error_result("Invalid hash");
		}

		return error_result();
	}

	namespace acl_impl
	{
		inline const compressed_database* make_compressed_database_impl(const void* buffer, error_result* out_error_result)
		{
			if (buffer == nullptr)
			{
				if (out_error_result != nullptr)
					*out_error_result = error_result("Buffer is not a valid pointer");

				return nullptr;
			}

			const compressed_database* db = static_cast<const compressed_database*>(buffer);
			if (out_error_result != nullptr)
			{
				const error_result result = db->is_valid(false);
				*out_error_result = result;

				if (result.any())
					return nullptr;
			}

			return db;
		}
	}

	inline const compressed_database* make_compressed_database(const void* buffer, error_result* out_error_result)
	{
		return acl_impl::make_compressed_database_impl(buffer, out_error_result);
	}

	inline compressed_database* make_compressed_database(void* buffer, error_result* out_error_result)
	{
		return const_cast<compressed_database*>(acl_impl::make_compressed_database_impl(buffer, out_error_result));
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
