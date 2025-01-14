#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Animation Compression Library contributors
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

// Included only once from compression_level.h

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline const char* get_compression_level_name(compression_level8 level)
	{
		switch (level)
		{
		case compression_level8::lowest:	return "lowest";
		case compression_level8::low:		return "low";
		case compression_level8::medium:	return "medium";
		case compression_level8::high:		return "high";
		case compression_level8::highest:	return "highest";
		case compression_level8::automatic:	return "automatic";
		default:							return "<Invalid>";
		}
	}

	inline bool get_compression_level(const char* level_name, compression_level8& out_level)
	{
		ACL_ASSERT(level_name != nullptr, "Level name cannot be null");
		if (level_name == nullptr)
			return false;

		const char* level_lowest = "Lowest";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* level_lowest_new = "lowest";
		if (std::strncmp(level_name, level_lowest, std::strlen(level_lowest)) == 0
			|| std::strncmp(level_name, level_lowest_new, std::strlen(level_lowest_new)) == 0)
		{
			out_level = compression_level8::lowest;
			return true;
		}

		const char* level_low = "Low";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* level_low_new = "low";
		if (std::strncmp(level_name, level_low, std::strlen(level_low)) == 0
			|| std::strncmp(level_name, level_low_new, std::strlen(level_low_new)) == 0)
		{
			out_level = compression_level8::low;
			return true;
		}

		const char* level_medium = "Medium";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* level_medium_new = "medium";
		if (std::strncmp(level_name, level_medium, std::strlen(level_medium)) == 0
			|| std::strncmp(level_name, level_medium_new, std::strlen(level_medium_new)) == 0)
		{
			out_level = compression_level8::medium;
			return true;
		}

		const char* level_highest = "Highest";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* level_highest_new = "highest";
		if (std::strncmp(level_name, level_highest, std::strlen(level_highest)) == 0
			|| std::strncmp(level_name, level_highest_new, std::strlen(level_highest_new)) == 0)
		{
			out_level = compression_level8::highest;
			return true;
		}

		const char* level_high = "High";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* level_high_new = "high";
		if (std::strncmp(level_name, level_high, std::strlen(level_high)) == 0
			|| std::strncmp(level_name, level_high_new, std::strlen(level_high_new)) == 0)
		{
			out_level = compression_level8::high;
			return true;
		}

		const char* level_automatic = "automatic";
		if (std::strncmp(level_name, level_automatic, std::strlen(level_automatic)) == 0)
		{
			out_level = compression_level8::automatic;
			return true;
		}

		return false;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
