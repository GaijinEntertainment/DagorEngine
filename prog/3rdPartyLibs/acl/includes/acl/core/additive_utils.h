#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2018 Nicholas Frechette & Animation Compression Library contributors
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

#include <rtm/qvvf.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Describes the format used by the additive clip.
	enum class additive_clip_format8 : uint8_t
	{
		//////////////////////////////////////////////////////////////////////////
		// Clip is not additive
		// Default scale value is 1.0
		none				= 0,

		//////////////////////////////////////////////////////////////////////////
		// Clip is in relative space, transform_mul or equivalent is used to combine them.
		// Default scale value is 1.0
		// transform = transform_mul(additive_transform, base_transform)
		relative			= 1,

		//////////////////////////////////////////////////////////////////////////
		// Clip is in additive space where scale is combined with: base_scale * additive_scale
		// Default scale value is 1.0
		// transform = transform_add0(additive_transform, base_transform)
		additive0			= 2,

		//////////////////////////////////////////////////////////////////////////
		// Clip is in additive space where scale is combined with: base_scale * (1.0 + additive_scale)
		// Default scale value is 0.0
		// transform = transform_add1(additive_transform, base_transform)
		additive1			= 3,
	};

	//////////////////////////////////////////////////////////////////////////

	// TODO: constexpr
	inline const char* get_additive_clip_format_name(additive_clip_format8 format)
	{
		switch (format)
		{
		case additive_clip_format8::none:			return "none";
		case additive_clip_format8::relative:		return "relative";
		case additive_clip_format8::additive0:		return "additive0";
		case additive_clip_format8::additive1:		return "additive1";
		default:									return "<Invalid>";
		}
	}

	inline bool get_additive_clip_format(const char* format, additive_clip_format8& out_format)
	{
		ACL_ASSERT(format != nullptr, "Format name cannot be null");
		if (format == nullptr)
			return false;

		const char* none_format = "None";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* none_format_new = "none";
		if (std::strncmp(format, none_format, std::strlen(none_format)) == 0
			|| std::strncmp(format, none_format_new, std::strlen(none_format_new)) == 0)
		{
			out_format = additive_clip_format8::none;
			return true;
		}

		const char* relative_format = "Relative";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* relative_format_new = "relative";
		if (std::strncmp(format, relative_format, std::strlen(relative_format)) == 0
			|| std::strncmp(format, relative_format_new, std::strlen(relative_format_new)) == 0)
		{
			out_format = additive_clip_format8::relative;
			return true;
		}

		const char* additive0_format = "Additive0";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* additive0_format_new = "additive0";
		if (std::strncmp(format, additive0_format, std::strlen(additive0_format)) == 0
			|| std::strncmp(format, additive0_format_new, std::strlen(additive0_format_new)) == 0)
		{
			out_format = additive_clip_format8::additive0;
			return true;
		}

		const char* additive1_format = "Additive1";	// ACL_DEPRECATED Legacy name, keep for backwards compatibility, remove in 3.0
		const char* additive1_format_new = "additive1";
		if (std::strncmp(format, additive1_format, std::strlen(additive1_format)) == 0
			|| std::strncmp(format, additive1_format_new, std::strlen(additive1_format_new)) == 0)
		{
			out_format = additive_clip_format8::additive1;
			return true;
		}

		return false;
	}

	inline rtm::qvvf RTM_SIMD_CALL transform_add0(rtm::qvvf_arg0 base, rtm::qvvf_arg1 additive)
	{
		const rtm::quatf rotation = rtm::quat_mul(additive.rotation, base.rotation);
		const rtm::vector4f translation = rtm::vector_add(additive.translation, base.translation);
		const rtm::vector4f scale = rtm::vector_mul(additive.scale, base.scale);
		return rtm::qvv_set(rotation, translation, scale);
	}

	inline rtm::qvvf RTM_SIMD_CALL transform_add1(rtm::qvvf_arg0 base, rtm::qvvf_arg1 additive)
	{
		const rtm::quatf rotation = rtm::quat_mul(additive.rotation, base.rotation);
		const rtm::vector4f translation = rtm::vector_add(additive.translation, base.translation);
		const rtm::vector4f scale = rtm::vector_mul(rtm::vector_add(rtm::vector_set(1.0F), additive.scale), base.scale);
		return rtm::qvv_set(rotation, translation, scale);
	}

	inline rtm::qvvf RTM_SIMD_CALL transform_add_no_scale(rtm::qvvf_arg0 base, rtm::qvvf_arg1 additive)
	{
		const rtm::quatf rotation = rtm::quat_mul(additive.rotation, base.rotation);
		const rtm::vector4f translation = rtm::vector_add(additive.translation, base.translation);
		return rtm::qvv_set(rotation, translation, rtm::vector_set(1.0F));
	}

	inline rtm::qvvf RTM_SIMD_CALL apply_additive_to_base(additive_clip_format8 additive_format, rtm::qvvf_arg0 base, rtm::qvvf_arg1 additive)
	{
		switch (additive_format)
		{
		default:
		case additive_clip_format8::none:			return additive;
		case additive_clip_format8::relative:		return rtm::qvv_mul(additive, base);
		case additive_clip_format8::additive0:		return transform_add0(base, additive);
		case additive_clip_format8::additive1:		return transform_add1(base, additive);
		}
	}

	inline rtm::qvvf RTM_SIMD_CALL apply_additive_to_base_no_scale(additive_clip_format8 additive_format, rtm::qvvf_arg0 base, rtm::qvvf_arg1 additive)
	{
		switch (additive_format)
		{
		default:
		case additive_clip_format8::none:			return additive;
		case additive_clip_format8::relative:		return rtm::qvv_mul_no_scale(additive, base);
		case additive_clip_format8::additive0:		return transform_add_no_scale(base, additive);
		case additive_clip_format8::additive1:		return transform_add_no_scale(base, additive);
		}
	}

	inline rtm::qvvf RTM_SIMD_CALL convert_to_relative(rtm::qvvf_arg0 base, rtm::qvvf_arg1 transform)
	{
		return rtm::qvv_mul(transform, rtm::qvv_inverse(base));
	}

	inline rtm::qvvf RTM_SIMD_CALL convert_to_additive0(rtm::qvvf_arg0 base, rtm::qvvf_arg1 transform)
	{
		const rtm::quatf rotation = rtm::quat_mul(transform.rotation, rtm::quat_conjugate(base.rotation));
		const rtm::vector4f translation = rtm::vector_sub(transform.translation, base.translation);
		const rtm::vector4f scale = rtm::vector_div(transform.scale, base.scale);
		return rtm::qvv_set(rotation, translation, scale);
	}

	inline rtm::qvvf RTM_SIMD_CALL convert_to_additive1(rtm::qvvf_arg0 base, rtm::qvvf_arg1 transform)
	{
		const rtm::quatf rotation = rtm::quat_mul(transform.rotation, rtm::quat_conjugate(base.rotation));
		const rtm::vector4f translation = rtm::vector_sub(transform.translation, base.translation);
		const rtm::vector4f scale = rtm::vector_sub(rtm::vector_mul(transform.scale, rtm::vector_reciprocal(base.scale)), rtm::vector_set(1.0F));
		return rtm::qvv_set(rotation, translation, scale);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
