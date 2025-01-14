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

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/impl/clip_context.h"
#include "acl/compression/impl/rigid_shell_utils.h"
#include "acl/compression/impl/transform_clip_adapters.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct pre_process_context_t
		{
			iallocator& allocator;

			const pre_process_settings_t& settings;
			track_array& track_list;

		private:
			rigid_shell_metadata_t* shell_metadata = nullptr;
			uint32_t* sorted_transforms_parent_first = nullptr;

			//////////////////////////////////////////////////////////////////////////

		public:
			explicit pre_process_context_t(
				iallocator& allocator_,
				const pre_process_settings_t& settings_,
				track_array& track_list_)
				: allocator(allocator_)
				, settings(settings_)
				, track_list(track_list_)
			{
			}

			// Cannot copy or move
			pre_process_context_t(const pre_process_context_t&) = delete;
			pre_process_context_t& operator=(const pre_process_context_t&) = delete;

			~pre_process_context_t()
			{
				deallocate_type_array(allocator, shell_metadata, track_list.get_num_tracks());
				deallocate_type_array(allocator, sorted_transforms_parent_first, track_list.get_num_tracks());
			}

			const rigid_shell_metadata_t* get_shell_metadata()
			{
				if (shell_metadata == nullptr)
				{
					transform_track_array_adapter_t clip_adapter(
						&track_array_cast<track_array_qvvf>(track_list),
						settings.additive_format);
					clip_adapter.sorted_transforms_parent_first = get_sorted_transforms_parent_first();

					shell_metadata = compute_clip_shell_distances(
						allocator,
						clip_adapter,
						transform_track_array_adapter_t(settings.additive_base, additive_clip_format8::none));
				}

				return shell_metadata;
			}

			const uint32_t* get_sorted_transforms_parent_first()
			{
				if (sorted_transforms_parent_first == nullptr)
				{
					const uint32_t num_transforms = track_list.get_num_tracks();

					sorted_transforms_parent_first = allocate_type_array<uint32_t>(allocator, num_transforms);

					for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
						sorted_transforms_parent_first[transform_index] = transform_index;

					sort_transform_indices_parent_first(
						transform_track_array_adapter_t(track_array_cast<track_array_qvvf>(track_list)),
						sorted_transforms_parent_first,
						num_transforms);
				}

				return sorted_transforms_parent_first;
			}
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
