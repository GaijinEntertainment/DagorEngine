#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/iallocator.h"
#include "acl/core/track_types.h"

#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		class scalarf_range
		{
		public:
			static constexpr track_category8 type = track_category8::scalarf;

			scalarf_range() : m_min(), m_extent() {}
			scalarf_range(rtm::vector4f_arg0 min, rtm::vector4f_arg1 extent) : m_min(min), m_extent(extent) {}

			static scalarf_range RTM_SIMD_CALL from_min_max(rtm::vector4f_arg0 min, rtm::vector4f_arg1 max) { return scalarf_range(min, rtm::vector_sub(max, min)); }
			static scalarf_range RTM_SIMD_CALL from_min_extent(rtm::vector4f_arg0 min, rtm::vector4f_arg1 extent) { return scalarf_range(min, extent); }

			rtm::vector4f RTM_SIMD_CALL get_min() const { return m_min; }
			rtm::vector4f RTM_SIMD_CALL get_max() const { return rtm::vector_add(m_min, m_extent); }

			rtm::vector4f RTM_SIMD_CALL get_center() const { return rtm::vector_add(m_min, rtm::vector_mul(m_extent, 0.5F)); }
			rtm::vector4f RTM_SIMD_CALL get_extent() const { return m_extent; }

			bool is_constant(float threshold) const { return rtm::vector_all_less_than(rtm::vector_abs(m_extent), rtm::vector_set(threshold)); }

		private:
			rtm::vector4f m_min;
			rtm::vector4f m_extent;
		};

		struct track_range
		{
			track_range() : range(), category(track_category8::scalarf) {}
			explicit track_range(const scalarf_range& range_) : range(range_), category(track_category8::scalarf) {}

			bool is_constant(float threshold) const
			{
				switch (category)
				{
				case track_category8::scalarf:	return range.scalarf.is_constant(threshold);
				case track_category8::scalard:
				case track_category8::transformf:
				case track_category8::transformd:
				default:
					ACL_ASSERT(false, "Invalid track category");
					return false;
				}
			}

			union range_union
			{
				scalarf_range scalarf;
				// TODO: Add qvv range and scalard/i/q ranges

				range_union() : scalarf(scalarf_range::from_min_extent(rtm::vector_zero(), rtm::vector_zero())) {}
				explicit range_union(const scalarf_range& range) : scalarf(range) {}
			};

			range_union			range;
			track_category8		category;

			uint8_t				padding[15];
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

ACL_IMPL_FILE_PRAGMA_POP
