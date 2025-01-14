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
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/enum_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// Various constants for range reduction.
	constexpr uint8_t k_segment_range_reduction_num_bits_per_component = 8;
	constexpr uint8_t k_segment_range_reduction_num_bytes_per_component = 1;
	constexpr uint32_t k_clip_range_reduction_vector3_range_size = sizeof(float) * 6;

	////////////////////////////////////////////////////////////////////////////////
	// range_reduction_flags8 represents the types of range reduction we support as a bit field.
	//
	// BE CAREFUL WHEN CHANGING VALUES IN THIS ENUM
	// The range reduction strategy is serialized in the compressed data, if you change a value
	// the compressed clips will be invalid. If you do, bump the appropriate algorithm versions.
	enum class range_reduction_flags8 : uint8_t
	{
		none				= 0x00,

		// Flags to determine which tracks have range reduction applied
		rotations			= 0x01,
		translations		= 0x02,
		scales				= 0x04,
		//Properties		= 0x08,		// TODO: Implement this

		all_tracks			= 0x07,		// rotations | translations | scales
	};

	ACL_IMPL_ENUM_FLAGS_OPERATORS(range_reduction_flags8)

	//////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Returns a string of the algorithm name suitable for display.
	// TODO: constexpr
	inline const char* get_range_reduction_name(range_reduction_flags8 flags)
	{
		// Some compilers have trouble with constexpr operator| with enums in a case switch statement
		if (flags == range_reduction_flags8::none)
			return "range_reduction::none";
		else if (flags == range_reduction_flags8::rotations)
			return "range_reduction::rotations";
		else if (flags == range_reduction_flags8::translations)
			return "range_reduction::translations";
		else if (flags == range_reduction_flags8::scales)
			return "range_reduction::scales";
		else if (flags == (range_reduction_flags8::rotations | range_reduction_flags8::translations))
			return "range_reduction::rotations | range_reduction::translations";
		else if (flags == (range_reduction_flags8::rotations | range_reduction_flags8::scales))
			return "range_reduction::rotations | range_reduction::scales";
		else if (flags == (range_reduction_flags8::translations | range_reduction_flags8::scales))
			return "range_reduction::translations | range_reduction::scales";
		else if (flags == (range_reduction_flags8::rotations | range_reduction_flags8::translations | range_reduction_flags8::scales))
			return "range_reduction::rotations | range_reduction::translations | range_reduction::scales";
		else
			return "<Invalid>";
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
