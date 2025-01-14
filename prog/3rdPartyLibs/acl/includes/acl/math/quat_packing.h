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
#include "acl/core/error.h"
#include "acl/core/memory_utils.h"
#include "acl/core/track_formats.h"
#include "acl/core/track_types.h"
#include "acl/math/scalar_packing.h"
#include "acl/math/vector4_packing.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>
#include <rtm/packing/quatf.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline void RTM_SIMD_CALL pack_quat_128(rtm::quatf_arg0 rotation, uint8_t* out_rotation_data)
	{
		pack_vector4_128(rtm::quat_to_vector(rotation), out_rotation_data);
	}

	inline rtm::quatf RTM_SIMD_CALL unpack_quat_128(const uint8_t* data_ptr)
	{
		return rtm::vector_to_quat(unpack_vector4_128(data_ptr));
	}

	inline void RTM_SIMD_CALL pack_quat_96(rtm::quatf_arg0 rotation, uint8_t* out_rotation_data)
	{
		rtm::vector4f rotation_xyz = rtm::quat_to_vector(rtm::quat_ensure_positive_w(rotation));
		pack_vector3_96(rotation_xyz, out_rotation_data);
	}

	// Assumes the 'data_ptr' is padded in order to load up to 16 bytes from it
	inline rtm::quatf RTM_SIMD_CALL unpack_quat_96_unsafe(const uint8_t* data_ptr)
	{
		rtm::vector4f rotation_xyz = unpack_vector3_96_unsafe(data_ptr);
		return rtm::quat_from_positive_w(rotation_xyz);
	}

	inline void RTM_SIMD_CALL pack_quat_48(rtm::quatf_arg0 rotation, uint8_t* out_rotation_data)
	{
		rtm::vector4f rotation_xyz = rtm::quat_to_vector(rtm::quat_ensure_positive_w(rotation));
		pack_vector3_s48_unsafe(rotation_xyz, out_rotation_data);
	}

	inline rtm::quatf RTM_SIMD_CALL unpack_quat_48(const uint8_t* data_ptr)
	{
		rtm::vector4f rotation_xyz = unpack_vector3_s48_unsafe(data_ptr);
		return rtm::quat_from_positive_w(rotation_xyz);
	}

	inline void RTM_SIMD_CALL pack_quat_32(rtm::quatf_arg0 rotation, uint8_t* out_rotation_data)
	{
		rtm::vector4f rotation_xyz = rtm::quat_to_vector(rtm::quat_ensure_positive_w(rotation));
		pack_vector3_32(rotation_xyz, 11, 11, 10, false, out_rotation_data);
	}

	inline rtm::quatf RTM_SIMD_CALL unpack_quat_32(const uint8_t* data_ptr)
	{
		rtm::vector4f rotation_xyz = unpack_vector3_32(11, 11, 10, false, data_ptr);
		return rtm::quat_from_positive_w(rotation_xyz);
	}

	//////////////////////////////////////////////////////////////////////////

	constexpr uint32_t get_packed_rotation_size(rotation_format8 format)
	{
		return format == rotation_format8::quatf_full ? (sizeof(float) * 4) : (sizeof(float) * 3);
	}

	constexpr uint32_t get_range_reduction_rotation_size(rotation_format8 format)
	{
		return format == rotation_format8::quatf_full ? (sizeof(float) * 8) : (sizeof(float) * 6);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
