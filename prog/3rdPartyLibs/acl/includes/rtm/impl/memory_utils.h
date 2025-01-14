#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
// Copyright (c) 2018 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/version.h"
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/error.h"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
#include <memory>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////
// This file contains various memory related utility functions and other misc helpers.
// Everything is hidden in an impl namespace even though they could be useful and used
// by anyone only to avoid polluting the rtm namespace. If these become useful enough,
// they might be moved into their own external dependency.
//////////////////////////////////////////////////////////////////////////

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// Allows static branching without any warnings
		//////////////////////////////////////////////////////////////////////////
		template<bool expression_result>
		struct static_condition { static constexpr bool test() RTM_NO_EXCEPT { return true; } };

		template<>
		struct static_condition<false> { static constexpr bool test() RTM_NO_EXCEPT { return false; } };

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the input is a power of two, false otherwise.
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool is_power_of_two(size_t input) RTM_NO_EXCEPT
		{
			return input != 0 && (input & (input - 1)) == 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the alignment provided satisfies the requirement for the provided Type, false otherwise.
		//////////////////////////////////////////////////////////////////////////
		template<typename Type>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool is_alignment_valid(size_t alignment) RTM_NO_EXCEPT
		{
			return is_power_of_two(alignment) && alignment >= alignof(Type);
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the pointer provided satisfies the specified alignment, false otherwise.
		//////////////////////////////////////////////////////////////////////////
		template<typename PtrType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool is_aligned_to(PtrType* value, size_t alignment) RTM_NO_EXCEPT
		{
			RTM_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
			return (bit_cast<intptr_t>(value) & (alignment - 1)) == 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the integral value provided satisfies the specified alignment, false otherwise.
		//////////////////////////////////////////////////////////////////////////
		template<typename IntegralType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool is_aligned_to(IntegralType value, size_t alignment) RTM_NO_EXCEPT
		{
			RTM_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
			return (static_cast<size_t>(value) & (alignment - 1)) == 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns true if the provided pointer satisfies the alignment of PtrType, false otherwise.
		//////////////////////////////////////////////////////////////////////////
		template<typename PtrType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool is_aligned(PtrType* value) RTM_NO_EXCEPT
		{
			return is_aligned_to(value, alignof(PtrType));
		}

		//////////////////////////////////////////////////////////////////////////
		// The input pointer is rounded up to the desired alignment.
		//////////////////////////////////////////////////////////////////////////
		template<typename PtrType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE PtrType* align_to(PtrType* value, size_t alignment) RTM_NO_EXCEPT
		{
			RTM_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
			return bit_cast<PtrType*>((bit_cast<intptr_t>(value) + (alignment - 1)) & ~(alignment - 1));
		}

		//////////////////////////////////////////////////////////////////////////
		// The input integral value is rounded up to the desired alignment.
		//////////////////////////////////////////////////////////////////////////
		template<typename IntegralType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE IntegralType align_to(IntegralType value, size_t alignment) RTM_NO_EXCEPT
		{
			RTM_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
			return static_cast<IntegralType>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns the array size for the provided array.
		//////////////////////////////////////////////////////////////////////////
		template<typename ElementType, size_t num_elements>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr size_t get_array_size(ElementType const (&)[num_elements]) RTM_NO_EXCEPT { return num_elements; }

		//////////////////////////////////////////////////////////////////////////
		// Type safe casting
		//////////////////////////////////////////////////////////////////////////
		template<typename DestPtrType, typename SrcType>
		struct safe_ptr_to_ptr_cast_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE static DestPtrType* cast(SrcType* input) RTM_NO_EXCEPT
			{
				RTM_ASSERT(is_aligned_to(input, alignof(DestPtrType)), "Cast would result in an unaligned pointer");
				return bit_cast<DestPtrType*>(input);
			}
		};

		template<typename SrcType>
		struct safe_ptr_to_ptr_cast_impl<void, SrcType>
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK static RTM_FORCE_INLINE constexpr void* cast(SrcType* input) RTM_NO_EXCEPT { return input; }
		};

		template<typename DestPtrType, typename SrcType>
		struct safe_int_to_ptr_cast_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE static DestPtrType* cast(SrcType input) RTM_NO_EXCEPT
			{
				RTM_ASSERT(is_aligned_to(input, alignof(DestPtrType)), "Cast would result in an unaligned pointer");
				return bit_cast<DestPtrType*>(input);
			}
		};

		template<typename SrcType>
		struct safe_int_to_ptr_cast_impl<void, SrcType>
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK static RTM_FORCE_INLINE constexpr void* cast(SrcType input) RTM_NO_EXCEPT { return bit_cast<void*>(input); }
		};

		template<typename DestPtrType, typename SrcType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE DestPtrType* safe_ptr_cast(SrcType* input) RTM_NO_EXCEPT
		{
			return safe_ptr_to_ptr_cast_impl<DestPtrType, SrcType>::cast(input);
		}

		template<typename DestPtrType, typename SrcType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE DestPtrType* safe_ptr_cast(SrcType input) RTM_NO_EXCEPT
		{
			return safe_int_to_ptr_cast_impl<DestPtrType, SrcType>::cast(input);
		}

#if defined(RTM_COMPILER_GCC)
		// GCC sometimes complains about comparisons being always true due to partial template
		// evaluation. Disable that warning since we know it is safe.
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

		template<typename Type, bool is_enum = true>
		struct safe_underlying_type { using type = typename std::underlying_type<Type>::type; };

		template<typename Type>
		struct safe_underlying_type<Type, false> { using type = Type; };

		template<typename DstType, typename SrcType, bool is_floating_point = false>
		struct is_static_cast_safe_s
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK static RTM_FORCE_INLINE bool test(SrcType input) RTM_NO_EXCEPT
			{
				using SrcRealType = typename safe_underlying_type<SrcType, std::is_enum<SrcType>::value>::type;

				if (static_condition<std::is_signed<DstType>::value == std::is_signed<SrcRealType>::value>::test())
					return SrcType(DstType(input)) == input;
				else if (static_condition<std::is_signed<SrcRealType>::value>::test())
					return int64_t(input) >= 0 && SrcType(DstType(input)) == input;
				else
					return uint64_t(input) <= uint64_t((std::numeric_limits<DstType>::max)());
			};
		};

		template<typename DstType, typename SrcType>
		struct is_static_cast_safe_s<DstType, SrcType, true>
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK static RTM_FORCE_INLINE bool test(SrcType input) RTM_NO_EXCEPT
			{
				return SrcType(DstType(input)) == input;
			}
		};

		template<typename DstType, typename SrcType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool is_static_cast_safe(SrcType input) RTM_NO_EXCEPT
		{
			// TODO: In C++17 this should be folded to constexpr if
			return is_static_cast_safe_s<DstType, SrcType, static_condition<std::is_floating_point<SrcType>::value || std::is_floating_point<DstType>::value>::test()>::test(input);
		}

		template<typename DstType, typename SrcType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE DstType safe_static_cast(SrcType input) RTM_NO_EXCEPT
		{
#if defined(RTM_HAS_ASSERT_CHECKS)
			const bool is_safe = is_static_cast_safe<DstType, SrcType>(input);
			RTM_ASSERT(is_safe, "Unsafe static cast resulted in data loss");
#endif

			return static_cast<DstType>(input);
		}

#if defined(RTM_COMPILER_GCC)
		#pragma GCC diagnostic pop
#endif

		//////////////////////////////////////////////////////////////////////////
		// Reads a DataType from the input pointer regardless of its alignment.
		//////////////////////////////////////////////////////////////////////////
		template<typename DataType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE DataType unaligned_read(const void* input) RTM_NO_EXCEPT
		{
			DataType result;
			std::memcpy(&result, input, sizeof(DataType));
			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		// Reads a DataType from the input pointer with an alignment check.
		//////////////////////////////////////////////////////////////////////////
		template<typename DataType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE DataType aligned_read(const void* input) RTM_NO_EXCEPT
		{
			return *safe_ptr_cast<const DataType, const void*>(input);
		}

		//////////////////////////////////////////////////////////////////////////
		// Writes a DataType into the output pointer regardless of its alignment.
		//////////////////////////////////////////////////////////////////////////
		template<typename DataType>
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void unaligned_write(DataType input, void* output) RTM_NO_EXCEPT
		{
			std::memcpy(output, &input, sizeof(DataType));
		}
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
