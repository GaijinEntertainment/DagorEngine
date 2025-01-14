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
#include "acl/core/compressed_tracks.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/interpolation_utils.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/database/database.h"
#include "acl/decompression/impl/decompression.scalar.h"
#include "acl/decompression/impl/decompression.transform.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C4582: 'union': constructor is not implicitly called (/Wall)
	// This is fine because a context is empty until it is constructed with a valid clip.
	// Afterwards, access is typesafe.
	#pragma warning(disable : 4582)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		union persistent_universal_decompression_context
		{
			persistent_scalar_decompression_context_v0 scalar;
			persistent_transform_decompression_context_v0 transform;

			//////////////////////////////////////////////////////////////////////////

			persistent_universal_decompression_context() noexcept {}
			const compressed_tracks* get_compressed_tracks() const { return scalar.tracks; }
			compressed_tracks_version16 get_version() const { return scalar.tracks->get_version(); }
			sample_looping_policy get_looping_policy() const
			{
				const track_type8 track_type = scalar.tracks->get_track_type();
				switch (track_type)
				{
				case track_type8::float1f:
				case track_type8::float2f:
				case track_type8::float3f:
				case track_type8::float4f:
				case track_type8::vector4f:
					return static_cast<sample_looping_policy>(scalar.looping_policy);
				case track_type8::qvvf:
					return static_cast<sample_looping_policy>(transform.looping_policy);
				default:
					ACL_ASSERT(false, "Invalid track type");
					return sample_looping_policy::non_looping;
				}
			}
			bool is_initialized() const { return scalar.is_initialized(); }
			void reset()
			{
				// Just reset the tracks pointer, this will mark us as no longer initialized indicating everything is stale
				scalar.tracks = nullptr;
			}
		};

		template<class decompression_settings_type, class database_settings_type>
		inline bool initialize_v0(persistent_universal_decompression_context& context, const compressed_tracks& tracks, const database_context<database_settings_type>* database)
		{
			const track_type8 track_type = tracks.get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				return initialize_v0<decompression_settings_type>(context.scalar, tracks, database);
			case track_type8::qvvf:
				return initialize_v0<decompression_settings_type>(context.transform, tracks, database);
			default:
				ACL_ASSERT(false, "Invalid track type");
				return false;
			}
		}

		template<class decompression_settings_type, class database_settings_type>
		inline bool relocated_v0(persistent_universal_decompression_context& context, const compressed_tracks& tracks, const database_context<database_settings_type>* database)
		{
			const track_type8 track_type = tracks.get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				return relocated_v0<decompression_settings_type>(context.scalar, tracks, database);
			case track_type8::qvvf:
				return relocated_v0<decompression_settings_type>(context.transform, tracks, database);
			default:
				ACL_ASSERT(false, "Invalid track type");
				return false;
			}
		}

		inline bool is_bound_to_v0(const persistent_universal_decompression_context& context, const compressed_tracks& tracks)
		{
			if (!context.is_initialized())
				return false;	// Not bound to anything when not initialized

			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				return is_bound_to_v0(context.scalar, tracks);
			case track_type8::qvvf:
				return is_bound_to_v0(context.transform, tracks);
			default:
				ACL_ASSERT(false, "Invalid track type");
				return false;
			}
		}

		inline bool is_bound_to(const persistent_universal_decompression_context& context, const compressed_database& database)
		{
			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				return is_bound_to_v0(context.scalar, database);
			case track_type8::qvvf:
				return is_bound_to_v0(context.transform, database);
			default:
				ACL_ASSERT(false, "Invalid track type");
				return false;
			}
		}

		template<class decompression_settings_type>
		inline void set_looping_policy_v0(const persistent_universal_decompression_context& context, sample_looping_policy policy)
		{
			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				set_looping_policy_v0<decompression_settings_type>(context.scalar, policy);
				break;
			case track_type8::qvvf:
				set_looping_policy_v0<decompression_settings_type>(context.transform, policy);
				break;
			default:
				ACL_ASSERT(false, "Invalid track type");
				break;
			}
		}

		template<class decompression_settings_type>
		inline void seek_v0(persistent_universal_decompression_context& context, float sample_time, sample_rounding_policy rounding_policy)
		{
			ACL_ASSERT(context.is_initialized(), "Context is not initialized");

			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				seek_v0<decompression_settings_type>(context.scalar, sample_time, rounding_policy);
				break;
			case track_type8::qvvf:
				seek_v0<decompression_settings_type>(context.transform, sample_time, rounding_policy);
				break;
			default:
				ACL_ASSERT(false, "Invalid track type");
				break;
			}
		}

		template<class decompression_settings_type, class track_writer_type>
		inline void decompress_tracks_v0(const persistent_universal_decompression_context& context, track_writer_type& writer)
		{
			ACL_ASSERT(context.is_initialized(), "Context is not initialized");

			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				decompress_tracks_v0<decompression_settings_type>(context.scalar, writer);
				break;
			case track_type8::qvvf:
				decompress_tracks_v0<decompression_settings_type>(context.transform, writer);
				break;
			default:
				ACL_ASSERT(false, "Invalid track type");
				break;
			}
		}

		template<class decompression_settings_type, class track_writer_type>
		inline void decompress_track_v0(const persistent_universal_decompression_context& context, uint32_t track_index, track_writer_type& writer)
		{
			ACL_ASSERT(context.is_initialized(), "Context is not initialized");

			const track_type8 track_type = context.scalar.tracks->get_track_type();
			switch (track_type)
			{
			case track_type8::float1f:
			case track_type8::float2f:
			case track_type8::float3f:
			case track_type8::float4f:
			case track_type8::vector4f:
				decompress_track_v0<decompression_settings_type>(context.scalar, track_index, writer);
				break;
			case track_type8::qvvf:
				decompress_track_v0<decompression_settings_type>(context.transform, track_index, writer);
				break;
			default:
				ACL_ASSERT(false, "Invalid track type");
				break;
			}
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

ACL_IMPL_FILE_PRAGMA_POP
