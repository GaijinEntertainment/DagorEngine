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

// Included only once from track_formats.h

#include "acl/version.h"
#include "acl/core/error.h"

#include <cstdint>
#include <cstring>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline const char* get_rotation_format_name(rotation_format8 format)
	{
		switch (format)
		{
		case rotation_format8::quatf_full:				return "quatf_full";
		case rotation_format8::quatf_drop_w_full:		return "quatf_drop_w_full";
		case rotation_format8::quatf_drop_w_variable:	return "quatf_drop_w_variable";
		default:										return "<Invalid>";
		}
	}

	inline bool get_rotation_format(const char* format, rotation_format8& out_format)
	{
		ACL_ASSERT(format != nullptr, "Rotation format name cannot be null");
		if (format == nullptr)
			return false;

		const char* quat_128_format = "Quat_128";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* quatf_full_format = "quatf_full";
		if (std::strncmp(format, quat_128_format, std::strlen(quat_128_format)) == 0
			|| std::strncmp(format, quatf_full_format, std::strlen(quatf_full_format)) == 0)
		{
			out_format = rotation_format8::quatf_full;
			return true;
		}

		const char* quatdropw_96_format = "QuatDropW_96";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* quatf_drop_w_full_format = "quatf_drop_w_full";
		if (std::strncmp(format, quatdropw_96_format, std::strlen(quatdropw_96_format)) == 0
			|| std::strncmp(format, quatf_drop_w_full_format, std::strlen(quatf_drop_w_full_format)) == 0)
		{
			out_format = rotation_format8::quatf_drop_w_full;
			return true;
		}

		const char* quatdropw_variable_format = "QuatDropW_Variable";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* quatf_drop_w_variable_format = "quatf_drop_w_variable";
		if (std::strncmp(format, quatdropw_variable_format, std::strlen(quatdropw_variable_format)) == 0
			|| std::strncmp(format, quatf_drop_w_variable_format, std::strlen(quatf_drop_w_variable_format)) == 0)
		{
			out_format = rotation_format8::quatf_drop_w_variable;
			return true;
		}

		return false;
	}

	constexpr const char* get_vector_format_name(vector_format8 format)
	{
		return format == vector_format8::vector3f_full ? "vector3f_full" : "vector3f_variable";
	}

	inline bool get_vector_format(const char* format, vector_format8& out_format)
	{
		ACL_ASSERT(format != nullptr, "Vector format name cannot be null");
		if (format == nullptr)
			return false;

		const char* vector3_96_format = "Vector3_96";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* vector3f_full_format = "vector3f_full";
		if (std::strncmp(format, vector3_96_format, std::strlen(vector3_96_format)) == 0
			|| std::strncmp(format, vector3f_full_format, std::strlen(vector3f_full_format)) == 0)
		{
			out_format = vector_format8::vector3f_full;
			return true;
		}

		const char* vector3_variable_format = "Vector3_Variable";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* vector3f_variable_format = "vector3f_variable";
		if (std::strncmp(format, vector3_variable_format, std::strlen(vector3_variable_format)) == 0
			|| std::strncmp(format, vector3f_variable_format, std::strlen(vector3f_variable_format)) == 0)
		{
			out_format = vector_format8::vector3f_variable;
			return true;
		}

		return false;
	}

	constexpr rotation_variant8 get_rotation_variant(rotation_format8 rotation_format)
	{
		return rotation_format == rotation_format8::quatf_full ? rotation_variant8::quat : rotation_variant8::quat_drop_w;
	}

	constexpr rotation_format8 get_highest_variant_precision(rotation_variant8 variant)
	{
		return variant == rotation_variant8::quat ? rotation_format8::quatf_full : rotation_format8::quatf_drop_w_full;
	}

	constexpr bool is_rotation_format_variable(rotation_format8 format)
	{
		return format == rotation_format8::quatf_drop_w_variable;
	}

	constexpr bool is_rotation_format_full_precision(rotation_format8 format)
	{
		return format == rotation_format8::quatf_full || format == rotation_format8::quatf_drop_w_full;
	}

	constexpr bool is_vector_format_variable(vector_format8 format)
	{
		return format == vector_format8::vector3f_variable;
	}

	constexpr bool is_vector_format_full_precision(vector_format8 format)
	{
		return format == vector_format8::vector3f_full;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
