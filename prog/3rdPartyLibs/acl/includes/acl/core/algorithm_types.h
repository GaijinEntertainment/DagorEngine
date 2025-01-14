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
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>
#include <cstring>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// algorithm_type8 is an enum that represents every supported algorithm.
	//
	// BE CAREFUL WHEN CHANGING VALUES IN THIS ENUM
	// The algorithm type is serialized in the compressed data, if you change a value
	// the compressed clips will be invalid. If you do, bump the appropriate algorithm versions.
	enum class algorithm_type8 : uint8_t
	{
		uniformly_sampled				= 0,
		//LinearKeyReduction			= 1,
		//SplineKeyReduction			= 2,
	};

	//////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Returns true if the algorithm type is a valid value. Used to validate if
	// memory has been corrupted.
	// TODO: constexpr
	inline bool is_valid_algorithm_type(algorithm_type8 type)
	{
		switch (type)
		{
			case algorithm_type8::uniformly_sampled:
			//case algorithm_type8::LinearKeyReduction:
			//case algorithm_type8::SplineKeyReduction:
				return true;
			default:
				return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns a string of the algorithm name suitable for display.
	// TODO: constexpr
	inline const char* get_algorithm_name(algorithm_type8 type)
	{
		switch (type)
		{
			case algorithm_type8::uniformly_sampled:	return "uniformly_sampled";
			//case algorithm_type8::LinearKeyReduction:	return "LinearKeyReduction";
			//case algorithm_type8::SplineKeyReduction:	return "SplineKeyReduction";
			default:									return "<Invalid>";
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns true if the algorithm type was properly parsed from an input string.
	//
	// type: A string representing the algorithm name to parse. It must match the get_algorithm_name(..) output.
	// out_type: On success, it will contain the the parsed algorithm type otherwise it is left untouched.
	inline bool get_algorithm_type(const char* type, algorithm_type8& out_type)
	{
		ACL_ASSERT(type != nullptr, "Algorithm type name cannot be null");
		if (type == nullptr)
			return false;

		const char* uniformly_sampled_name = "UniformlySampled";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* uniformly_sampled_name_new = "uniformly_sampled";
		if (std::strncmp(type, uniformly_sampled_name, std::strlen(uniformly_sampled_name)) == 0
			|| std::strncmp(type, uniformly_sampled_name_new, std::strlen(uniformly_sampled_name_new)) == 0)
		{
			out_type = algorithm_type8::uniformly_sampled;
			return true;
		}

		return false;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
