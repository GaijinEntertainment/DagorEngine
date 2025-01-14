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
#include "acl/core/additive_utils.h"
#include "acl/core/bitset.h"
#include "acl/core/iallocator.h"
#include "acl/core/iterator.h"
#include "acl/core/error.h"
#include "acl/core/sample_looping_policy.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/track_array.h"
#include "acl/compression/impl/segment_context.h"

#include <rtm/quatf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// Simple iterator utility class to allow easy looping
		class bone_chain_iterator
		{
		public:
			bone_chain_iterator(const uint32_t* chain, bitset_description chain_desc, uint32_t bone_index, uint32_t offset)
				: m_bone_chain(chain)
				, m_bone_chain_desc(chain_desc)
				, m_bone_index(bone_index)
				, m_offset(offset)
			{}

			bone_chain_iterator& operator++()
			{
				ACL_ASSERT(m_offset <= m_bone_index, "Cannot increment the iterator, it is no longer valid");

				// Skip the current bone
				m_offset++;

				// Iterate until we find the next bone part of the chain or until we reach the end of the chain
				// TODO: Use clz or similar to find the next set bit starting at the current index
				while (m_offset < m_bone_index && !bitset_test(m_bone_chain, m_bone_chain_desc, m_offset))
					m_offset++;

				return *this;
			}

			uint32_t operator*() const
			{
				ACL_ASSERT(m_offset <= m_bone_index, "Returned bone index doesn't belong to the bone chain");
				ACL_ASSERT(bitset_test(m_bone_chain, m_bone_chain_desc, m_offset), "Returned bone index doesn't belong to the bone chain");
				return m_offset;
			}

			// We only compare the offset in the bone chain. Two iterators on the same bone index
			// from two different or equal chains will be equal.
			bool operator==(const bone_chain_iterator& other) const { return m_offset == other.m_offset; }
			bool operator!=(const bone_chain_iterator& other) const { return m_offset != other.m_offset; }

		private:
			const uint32_t*		m_bone_chain;
			bitset_description	m_bone_chain_desc;
			uint32_t			m_bone_index;
			uint32_t			m_offset;
		};

		//////////////////////////////////////////////////////////////////////////
		// Simple bone chain container to allow easy looping
		//
		// A bone chain allows looping over all bones up to a specific bone starting
		// at the root bone.
		//////////////////////////////////////////////////////////////////////////
		struct bone_chain
		{
			bone_chain(const uint32_t* chain, bitset_description chain_desc, uint32_t bone_index)
				: m_bone_chain(chain)
				, m_bone_chain_desc(chain_desc)
				, m_bone_index(bone_index)
			{
				// We don't know where this bone chain starts, find the root bone
				// TODO: Use clz or similar to find the next set bit starting at the current index
				uint32_t root_index = 0;
				while (!bitset_test(chain, chain_desc, root_index))
					root_index++;

				m_root_index = root_index;
			}

			acl_impl::bone_chain_iterator begin() const { return acl_impl::bone_chain_iterator(m_bone_chain, m_bone_chain_desc, m_bone_index, m_root_index); }
			acl_impl::bone_chain_iterator end() const { return acl_impl::bone_chain_iterator(m_bone_chain, m_bone_chain_desc, m_bone_index, m_bone_index + 1); }

			const uint32_t*		m_bone_chain;
			bitset_description	m_bone_chain_desc;
			uint32_t			m_root_index;
			uint32_t			m_bone_index;
		};

		// Metadata per transform
		struct transform_metadata
		{
			// The transform chain this transform belongs to (points into leaf_transform_chains in owner context)
			const uint32_t* transform_chain				= nullptr;

			// Parent transform index of this transform, invalid if at the root
			uint32_t parent_index						= k_invalid_track_index;

			// The precision value from the track description for this transform
			float precision								= 0.0F;

			// The local space shell distance from the track description for this transform
			float shell_distance						= 0.0F;
		};

		// Rigid shell information per transform
		struct rigid_shell_metadata_t
		{
			// Dominant local space shell distance (from transform tip)
			float local_shell_distance;

			// Parent space shell distance (from transform root)
			float parent_shell_distance;

			// Precision required on the surface of the rigid shell
			float precision;
		};

		// Represents the working space for a clip (raw or lossy)
		struct clip_context
		{
			// List of segments contained (num_segments present)
			// Raw contexts only have a single segment
			segment_context* segments					= nullptr;

			// List of clip wide range information for each transform (num_bones present)
			transform_range* ranges						= nullptr;

			// List of metadata for each transform (num_bones present)
			// TODO: Same for raw/lossy/additive clip context, can we share?
			transform_metadata* metadata				= nullptr;

			// List of bit sets for each leaf transform to track transform chains (num_leaf_transforms present)
			// TODO: Same for raw/lossy/additive clip context, can we share?
			uint32_t* leaf_transform_chains				= nullptr;

			// List of transform indices sorted by parent first then sibling transforms are sorted by their transform index (num_bones present)
			// TODO: Same for raw/lossy/additive clip context, can we share?
			uint32_t* sorted_transforms_parent_first	= nullptr;

			// List of shell metadata for each transform (num_bones present)
			// Data is aggregate of whole clip
			// Shared between all clip contexts, not owned
			const rigid_shell_metadata_t* clip_shell_metadata	= nullptr;

			// Optional if we request it in the compression settings
			// Sorted by stripping order within this clip
			keyframe_stripping_metadata_t* contributing_error = nullptr;

			uint32_t num_segments						= 0;
			uint32_t num_bones							= 0;	// TODO: Rename num_transforms
			uint32_t num_samples_allocated				= 0;
			uint32_t num_samples						= 0;
			float sample_rate							= 0.0F;

			float duration								= 0.0F;

			sample_looping_policy looping_policy		= sample_looping_policy::non_looping;
			additive_clip_format8 additive_format		= additive_clip_format8::none;

			bool are_rotations_normalized				= false;
			bool are_translations_normalized			= false;
			bool are_scales_normalized					= false;
			bool has_scale								= false;
			bool has_additive_base						= false;
			bool has_stripped_keyframes					= false;

			uint32_t num_leaf_transforms				= 0;

			iallocator* allocator						= nullptr;	// Never null if the context is initialized

			// Stat tracking
			uint32_t decomp_touched_bytes				= 0;
			uint32_t decomp_touched_cache_lines			= 0;

			//////////////////////////////////////////////////////////////////////////

			bool is_initialized() const { return allocator != nullptr; }
			iterator<segment_context> segment_iterator() { return iterator<segment_context>(segments, num_segments); }
			const_iterator<segment_context> segment_iterator() const { return const_iterator<segment_context>(segments, num_segments); }

			bone_chain get_bone_chain(uint32_t bone_index) const
			{
				ACL_ASSERT(bone_index < num_bones, "Invalid bone index: %u >= %u", bone_index, num_bones);
				const transform_metadata& meta = metadata[bone_index];
				return bone_chain(meta.transform_chain, bitset_description::make_from_num_bits(num_bones), bone_index);
			}
		};

		template<class clip_adapter_t>
		void sort_transform_indices_parent_first(
			const clip_adapter_t& clip,
			uint32_t* sorted_transforms_parent_first,
			uint32_t num_transforms);

		bool initialize_clip_context(
			iallocator& allocator,
			const track_array_qvvf& track_list,
			const compression_settings& settings,
			additive_clip_format8 additive_format,
			clip_context& out_clip_context);
		void destroy_clip_context(clip_context& context);

		constexpr bool segment_context_has_scale(const segment_context& segment) { return segment.clip->has_scale; }
		constexpr bool bone_streams_has_scale(const transform_streams& bone_streams) { return segment_context_has_scale(*bone_streams.segment); }
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/clip_context.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
