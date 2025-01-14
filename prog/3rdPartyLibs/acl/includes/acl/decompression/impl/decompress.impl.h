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

// Included only once from decompress.h

#include "acl/version.h"

#include <type_traits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// SFINAE boilerplate to detect if a template argument derives from acl::decompression_context.
		//////////////////////////////////////////////////////////////////////////
		template<class T>
		using is_decompression_context = typename std::enable_if<std::is_base_of<acl::decompression_context<typename T::settings_type>, T>::value, std::nullptr_t>::type;
	}

	//////////////////////////////////////////////////////////////////////////
	// decompression_context implementation

	template<class decompression_settings_type>
	inline decompression_context<decompression_settings_type>::decompression_context()
	{
		m_context.reset();

		// Deprecation checks
		static_assert(decompression_settings_type::normalize_rotations(), "Override get_rotation_normalization_policy instead; to be removed in v3.0");
	}

	template<class decompression_settings_type>
	inline decompression_context<decompression_settings_type>::~decompression_context()
	{
		reset();
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::initialize(const compressed_tracks& tracks)
	{
		constexpr bool skip_safety_checks = decompression_settings_type::skip_initialize_safety_checks();

		const error_result error = skip_safety_checks ? error_result() : tracks.is_valid(false);
		const bool is_valid = skip_safety_checks || error.empty();
		ACL_ASSERT(is_valid, "Invalid compressed tracks instance: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, error.c_str());
		if (!is_valid)
			return false;	// Invalid compressed tracks instance

		const bool is_version_supported = skip_safety_checks || version_impl_type::is_version_supported(tracks.get_version());
		ACL_ASSERT(is_version_supported, "Unsupported version: %u", tracks.get_version());
		if (!is_version_supported)
			return false;

		const database_context<db_settings_type>* database = nullptr;
		return version_impl_type::template initialize<decompression_settings_type>(m_context, tracks, database);
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::initialize(const compressed_tracks& tracks, const database_context<db_settings_type>& database)
	{
		constexpr bool skip_safety_checks = decompression_settings_type::skip_initialize_safety_checks();

		const error_result error = skip_safety_checks ? error_result() : tracks.is_valid(false);
		const bool is_tracks_valid = skip_safety_checks || error.empty();
		ACL_ASSERT(is_tracks_valid, "Invalid compressed tracks instance: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, error.c_str());
		if (!is_tracks_valid)
			return false;	// Invalid compressed tracks instance

		const bool is_database_valid = skip_safety_checks || database.is_initialized();
		ACL_ASSERT(is_database_valid, "Invalid compressed database instance");
		if (!is_database_valid)
			return false;	// Invalid compressed database instance

		const bool is_version_supported = skip_safety_checks || version_impl_type::is_version_supported(tracks.get_version());
		ACL_ASSERT(is_version_supported, "Unsupported version: %u", tracks.get_version());
		if (!is_version_supported)
			return false;

		const bool is_contained_in_db = skip_safety_checks || database.contains(tracks);
		if (!is_contained_in_db)
			return false;

		return version_impl_type::template initialize<decompression_settings_type>(m_context, tracks, &database);
	}

	template<class decompression_settings_type>
	inline void decompression_context<decompression_settings_type>::reset()
	{
		m_context.reset();
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::relocated(const compressed_tracks& tracks)
	{
		if (!m_context.is_initialized())
			return false;	// Not initialized, cannot be relocated

		constexpr bool skip_safety_checks = decompression_settings_type::skip_initialize_safety_checks();

		const error_result error = skip_safety_checks ? error_result() : tracks.is_valid(false);
		const bool is_valid = skip_safety_checks || error.empty();
		ACL_ASSERT(is_valid, "Invalid compressed tracks instance: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, error.c_str());
		if (!is_valid)
			return false;	// Invalid compressed tracks instance

		const bool is_version_supported = skip_safety_checks || version_impl_type::is_version_supported(tracks.get_version());
		ACL_ASSERT(is_version_supported, "Unsupported version: %u", tracks.get_version());
		if (!is_version_supported)
			return false;

		const database_context<db_settings_type>* database = nullptr;
		return version_impl_type::template relocated<decompression_settings_type>(m_context, tracks, database);
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::relocated(const compressed_tracks& tracks, const database_context<db_settings_type>& database)
	{
		if (!m_context.is_initialized())
			return false;	// Not initialized, cannot be relocated

		constexpr bool skip_safety_checks = decompression_settings_type::skip_initialize_safety_checks();

		const error_result error = skip_safety_checks ? error_result() : tracks.is_valid(false);
		const bool is_tracks_valid = skip_safety_checks || error.empty();
		ACL_ASSERT(is_tracks_valid, "Invalid compressed tracks instance: " ACL_ASSERT_STRING_FORMAT_SPECIFIER, error.c_str());
		if (!is_tracks_valid)
			return false;	// Invalid compressed tracks instance

		const bool is_database_valid = skip_safety_checks || database.is_initialized();
		ACL_ASSERT(is_database_valid, "Invalid compressed database instance");
		if (!is_database_valid)
			return false;	// Invalid compressed database instance

		const bool is_version_supported = skip_safety_checks || version_impl_type::is_version_supported(tracks.get_version());
		ACL_ASSERT(is_version_supported, "Unsupported version: %u", tracks.get_version());
		if (!is_version_supported)
			return false;

		const bool is_contained_in_db = skip_safety_checks || database.contains(tracks);
		if (!is_contained_in_db)
			return false;

		return version_impl_type::template relocated<decompression_settings_type>(m_context, tracks, &database);
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::is_dirty(const compressed_tracks& tracks) const
	{
		// We are dirty if we aren't bound to it
		return !version_impl_type::is_bound_to(m_context, tracks);
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::is_bound_to(const compressed_tracks& tracks) const
	{
		return version_impl_type::is_bound_to(m_context, tracks);
	}

	template<class decompression_settings_type>
	inline bool decompression_context<decompression_settings_type>::is_bound_to(const compressed_database& database) const
	{
		if (!m_context.is_initialized())
			return false;	// Not bound to anything when not initialized

		return version_impl_type::is_bound_to(m_context, database);
	}

	template<class decompression_settings_type>
	inline void decompression_context<decompression_settings_type>::set_looping_policy(sample_looping_policy policy)
	{
		ACL_ASSERT(m_context.is_initialized(), "Context is not initialized");
		version_impl_type::template set_looping_policy<decompression_settings_type>(m_context, policy);
	}

	template<class decompression_settings_type>
	inline sample_looping_policy decompression_context<decompression_settings_type>::get_looping_policy() const
	{
		ACL_ASSERT(m_context.is_initialized(), "Context is not initialized");
		return m_context.get_looping_policy();
	}

	template<class decompression_settings_type>
	inline void decompression_context<decompression_settings_type>::seek(float sample_time, sample_rounding_policy rounding_policy)
	{
		ACL_ASSERT(m_context.is_initialized(), "Context is not initialized");
		ACL_ASSERT(rtm::scalar_is_finite(sample_time), "Invalid sample time");
		ACL_ASSERT(rounding_policy != sample_rounding_policy::per_track || decompression_settings_type::is_per_track_rounding_supported(), "Per track rounding must be enabled");

		if (!m_context.is_initialized())
			return;	// Context is not initialized

		version_impl_type::template seek<decompression_settings_type>(m_context, sample_time, rounding_policy);
	}

	template<class decompression_settings_type>
	template<class track_writer_type>
	inline void decompression_context<decompression_settings_type>::decompress_tracks(track_writer_type& writer)
	{
		static_assert(std::is_base_of<track_writer, track_writer_type>::value, "track_writer_type must derive from track_writer");
		ACL_ASSERT(m_context.is_initialized(), "Context is not initialized");

		if (!m_context.is_initialized())
			return;	// Context is not initialized

		version_impl_type::template decompress_tracks<decompression_settings_type>(m_context, writer);
	}

	template<class decompression_settings_type>
	template<class track_writer_type>
	inline void decompression_context<decompression_settings_type>::decompress_track(uint32_t track_index, track_writer_type& writer)
	{
		static_assert(std::is_base_of<track_writer, track_writer_type>::value, "track_writer_type must derive from track_writer");
		ACL_ASSERT(m_context.is_initialized(), "Context is not initialized");

		if (!m_context.is_initialized())
			return;	// Context is not initialized

		version_impl_type::template decompress_track<decompression_settings_type>(m_context, track_index, writer);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
