#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/error_result.h"
#include "acl/core/memory_utils.h"
#include "acl/core/impl/compiler_utils.h"

#include <rtm/scalarf.h>

#include <cstdint>
#include <cstring>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// We only support up to 4294967295 tracks. We reserve 4294967295 for the invalid index
	constexpr uint32_t k_invalid_track_index = 0xFFFFFFFFU;

	//////////////////////////////////////////////////////////////////////////
	// The various supported track types.
	// Note: be careful when changing values here as they might be serialized.
	enum class track_type8 : uint8_t
	{
		float1f		= 0,
		float2f		= 1,
		float3f		= 2,
		float4f		= 3,
		vector4f	= 4,

		//float1d	= 5,
		//float2d	= 6,
		//float3d	= 7,
		//float4d	= 8,
		//vector4d	= 9,

		//quatf		= 10,
		//quatd		= 11,

		qvvf		= 12,
		//qvvd		= 13,

		//int1i		= 14,
		//int2i		= 15,
		//int3i		= 16,
		//int4i		= 17,
		//vector4i	= 18,

		//int1q		= 19,
		//int2q		= 20,
		//int3q		= 21,
		//int4q		= 22,
		//vector4q	= 23,
	};

	//////////////////////////////////////////////////////////////////////////
	// The categories of track types.
	enum class track_category8 : uint8_t
	{
		scalarf		= 0,
		scalard		= 1,
		//scalari	= 2,
		//scalarq	= 3,
		transformf	= 4,
		transformd	= 5,
	};

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Returns the string representation for the provided track type.
	// TODO: constexpr
	inline const char* get_track_type_name(track_type8 type)
	{
		switch (type)
		{
		case track_type8::float1f:			return "float1f";
		case track_type8::float2f:			return "float2f";
		case track_type8::float3f:			return "float3f";
		case track_type8::float4f:			return "float4f";
		case track_type8::vector4f:			return "vector4f";
		case track_type8::qvvf:				return "qvvf";
		default:							return "<Invalid>";
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the track type from its string representation.
	// Returns true on success, false otherwise.
	inline bool get_track_type(const char* type, track_type8& out_type)
	{
		// Entries in the same order as the enum integral value
		static const char* k_track_type_names[] =
		{
			"float1f",
			"float2f",
			"float3f",
			"float4f",
			"vector4f",

			"float1d",
			"float2d",
			"float3d",
			"float4d",
			"vector4d",

			"quatf",
			"quatd",

			"qvvf",
		};

		static_assert(get_array_size(k_track_type_names) == (size_t)track_type8::qvvf + 1, "Unexpected array size");

		ACL_ASSERT(type != nullptr, "Track type name cannot be null");
		if (type == nullptr)
			return false;

		for (size_t type_index = 0; type_index < get_array_size(k_track_type_names); ++type_index)
		{
			const char* type_name = k_track_type_names[type_index];
			if (std::strncmp(type, type_name, std::strlen(type_name)) == 0)
			{
				out_type = safe_static_cast<track_type8>(type_index);
				return true;
			}
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the track category for the provided track type.
	inline track_category8 get_track_category(track_type8 type)
	{
		// Entries in the same order as the enum integral value
		static constexpr track_category8 k_track_type_to_category[]
		{
			track_category8::scalarf,		// float1f
			track_category8::scalarf,		// float2f
			track_category8::scalarf,		// float3f
			track_category8::scalarf,		// float4f
			track_category8::scalarf,		// vector4f

			track_category8::scalard,		// float1d
			track_category8::scalard,		// float2d
			track_category8::scalard,		// float3d
			track_category8::scalard,		// float4d
			track_category8::scalard,		// vector4d

			track_category8::transformf,	// quatf
			track_category8::transformd,	// quatd

			track_category8::transformf,	// qvvf
		};

		static_assert(get_array_size(k_track_type_to_category) == (size_t)track_type8::qvvf + 1, "Unexpected array size");

		ACL_ASSERT(type <= track_type8::qvvf, "Unexpected track type");
		return type <= track_type8::qvvf ? k_track_type_to_category[static_cast<uint32_t>(type)] : track_category8::scalarf;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the num of elements within a sample for the provided track type.
	inline uint32_t get_track_num_sample_elements(track_type8 type)
	{
		// Entries in the same order as the enum integral value
		static constexpr uint32_t k_track_type_to_num_elements[]
		{
			1,	// float1f
			2,	// float2f
			3,	// float3f
			4,	// float4f
			4,	// vector4f

			1,	// float1d
			2,	// float2d
			3,	// float3d
			4,	// float4d
			4,	// vector4d

			4,	// quatf
			4,	// quatd

			12,	// qvvf
		};

		static_assert(get_array_size(k_track_type_to_num_elements) == (size_t)track_type8::qvvf + 1, "Unexpected array size");

		ACL_ASSERT(type <= track_type8::qvvf, "Unexpected track type");
		return type <= track_type8::qvvf ? k_track_type_to_num_elements[static_cast<uint32_t>(type)] : 0;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
