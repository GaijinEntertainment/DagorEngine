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
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"

#include <rtm/qvvf.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	// Note: It is safe for both pose buffers to alias since the data if sorted parent first
	inline void local_to_object_space(const uint32_t* parent_indices, const rtm::qvvf* local_pose, uint32_t num_transforms, rtm::qvvf* out_object_pose)
	{
		if (num_transforms == 0)
			return;	// Nothing to do

		out_object_pose[0] = local_pose[0];

		for (uint32_t bone_index = 1; bone_index < num_transforms; ++bone_index)
		{
			const uint32_t parent_bone_index = parent_indices[bone_index];
			ACL_ASSERT(parent_bone_index < num_transforms, "Invalid parent bone index: %u >= %u", parent_bone_index, num_transforms);

			out_object_pose[bone_index] = rtm::qvv_normalize(rtm::qvv_mul(local_pose[bone_index], out_object_pose[parent_bone_index]));
		}
	}

	// Note: It is safe for both pose buffers to alias since the data if sorted parent first
	inline void object_to_local_space(const uint32_t* parent_indices, const rtm::qvvf* object_pose, uint32_t num_transforms, rtm::qvvf* out_local_pose)
	{
		if (num_transforms == 0)
			return;	// Nothing to do

		out_local_pose[0] = object_pose[0];

		for (uint32_t bone_index = 1; bone_index < num_transforms; ++bone_index)
		{
			const uint32_t parent_bone_index = parent_indices[bone_index];
			ACL_ASSERT(parent_bone_index < num_transforms, "Invalid parent bone index: %u >= %u", parent_bone_index, num_transforms);

			const rtm::qvvf inv_parent_transform = rtm::qvv_inverse(object_pose[parent_bone_index]);
			out_local_pose[bone_index] = rtm::qvv_normalize(rtm::qvv_mul(inv_parent_transform, object_pose[bone_index]));
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
