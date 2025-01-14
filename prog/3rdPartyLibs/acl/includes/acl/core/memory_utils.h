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
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/error.h"

#include <rtm/math.h>

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
#include <memory>
#include <algorithm>

// For byte swapping intrinsics
#if defined(RTM_COMPILER_MSVC)
	#include <cstdlib>
#elif defined(__APPLE__)
	#include <libkern/OSByteOrder.h>
#endif

// For __prefetch
#if defined(RTM_NEON64_INTRINSICS) && defined(RTM_COMPILER_MSVC)
	#include <intrin.h>
#endif

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Allows static branching without any warnings

	template<bool expression_result>
	struct static_condition { static constexpr bool test() { return true; } };

	template<>
	struct static_condition<false> { static constexpr bool test() { return false; } };

	//////////////////////////////////////////////////////////////////////////
	// Various miscellaneous utilities related to alignment

	RTM_FORCE_INLINE constexpr bool is_power_of_two(size_t input)
	{
		return input != 0 && (input & (input - 1)) == 0;
	}

	template<typename Type>
	RTM_FORCE_INLINE constexpr bool is_alignment_valid(size_t alignment)
	{
		return is_power_of_two(alignment) && alignment >= alignof(Type);
	}

	template<typename PtrType>
	RTM_FORCE_INLINE bool is_aligned_to(PtrType* value, size_t alignment)
	{
		ACL_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
		return (acl_impl::bit_cast<intptr_t>(value) & (alignment - 1)) == 0;
	}

	template<typename IntegralType>
	RTM_FORCE_INLINE bool is_aligned_to(IntegralType value, size_t alignment)
	{
		ACL_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
		return (static_cast<size_t>(value) & (alignment - 1)) == 0;
	}

	template<typename PtrType>
	RTM_FORCE_INLINE constexpr bool is_aligned(PtrType* value)
	{
		return is_aligned_to(value, alignof(PtrType));
	}

	template<typename PtrType>
	RTM_FORCE_INLINE PtrType* align_to(PtrType* value, size_t alignment)
	{
		ACL_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
		return acl_impl::bit_cast<PtrType*>((acl_impl::bit_cast<intptr_t>(value) + (alignment - 1)) & ~(alignment - 1));
	}

	template<typename IntegralType>
	RTM_FORCE_INLINE IntegralType align_to(IntegralType value, size_t alignment)
	{
		ACL_ASSERT(is_power_of_two(alignment), "Alignment value must be a power of two");
		return static_cast<IntegralType>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
	}

	template<typename PreviousMemberType, typename NextMemberType>
	RTM_FORCE_INLINE constexpr size_t get_required_padding()
	{
		// align_to(sizeof(PreviousMemberType), alignof(NextMemberType)) - sizeof(PreviousMemberType)
		return ((sizeof(PreviousMemberType) + (alignof(NextMemberType) - 1)) & ~(alignof(NextMemberType)- 1)) - sizeof(PreviousMemberType);
	}

	template<typename ElementType, size_t num_elements>
	RTM_FORCE_INLINE constexpr size_t get_array_size(ElementType const (&)[num_elements]) { return num_elements; }

	//////////////////////////////////////////////////////////////////////////
	// Type safe casting

	namespace memory_impl
	{
		template<typename DestPtrType, typename SrcType>
		struct safe_ptr_to_ptr_cast_impl
		{
			RTM_FORCE_INLINE static DestPtrType* cast(SrcType* input)
			{
				ACL_ASSERT(is_aligned_to(input, alignof(DestPtrType)), "bit_cast would result in an unaligned pointer");
				return acl_impl::bit_cast<DestPtrType*>(input);
			}
		};

		template<typename SrcType>
		struct safe_ptr_to_ptr_cast_impl<void, SrcType>
		{
			RTM_FORCE_INLINE static constexpr void* cast(SrcType* input) { return input; }
		};

		template<typename DestPtrType, typename SrcType>
		struct safe_int_to_ptr_cast_impl
		{
			RTM_FORCE_INLINE static DestPtrType* cast(SrcType input)
			{
				ACL_ASSERT(is_aligned_to(input, alignof(DestPtrType)), "bit_cast would result in an unaligned pointer");
				return acl_impl::bit_cast<DestPtrType*>(static_cast<uintptr_t>(input));
			}
		};

		template<typename SrcType>
		struct safe_int_to_ptr_cast_impl<void, SrcType>
		{
			RTM_FORCE_INLINE static constexpr void* cast(SrcType input) { return acl_impl::bit_cast<void*>(static_cast<uintptr_t>(input)); }
		};
	}

	// Casts a pointer type into a pointer of a different type with an alignment runtime check
	template<typename dest_ptr_t, typename src_ptr_t>
	RTM_FORCE_INLINE dest_ptr_t* safe_ptr_cast(src_ptr_t* input)
	{
		return memory_impl::safe_ptr_to_ptr_cast_impl<dest_ptr_t, src_ptr_t>::cast(input);
	}

	// Casts an integral type into a pointer type with an alignment runtime check
	template<typename dest_ptr_t, typename src_int_t>
	RTM_FORCE_INLINE dest_ptr_t* safe_ptr_cast(src_int_t input)
	{
		return memory_impl::safe_int_to_ptr_cast_impl<dest_ptr_t, src_int_t>::cast(input);
	}

#if defined(RTM_COMPILER_GCC)
	// GCC sometimes complains about comparisons being always true due to partial template
	// evaluation. Disable that warning since we know it is safe.
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

	namespace memory_impl
	{
		template<typename Type, bool is_enum = true>
		struct safe_underlying_type { using type = typename std::underlying_type<Type>::type; };

		template<typename Type>
		struct safe_underlying_type<Type, false> { using type = Type; };

		template<typename DstType, typename SrcType, bool is_floating_point = false>
		struct is_static_cast_safe_s
		{
			RTM_FORCE_INLINE static bool test(SrcType input)
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
			RTM_FORCE_INLINE static bool test(SrcType input)
			{
				return SrcType(DstType(input)) == input;
			}
		};

		template<typename DstType, typename SrcType>
		RTM_FORCE_INLINE bool is_static_cast_safe(SrcType input)
		{
			// TODO: In C++17 this should be folded to constexpr if
			return is_static_cast_safe_s<DstType, SrcType, static_condition<(std::is_floating_point<SrcType>::value || std::is_floating_point<DstType>::value)>::test()>::test(input);
		}
	}

	template<typename DstType, typename SrcType>
	RTM_FORCE_INLINE DstType safe_static_cast(SrcType input)
	{
#if defined(ACL_HAS_ASSERT_CHECKS)
		const bool is_safe = memory_impl::is_static_cast_safe<DstType, SrcType>(input);
		ACL_ASSERT(is_safe, "Unsafe static cast resulted in data loss");
#endif

		return static_cast<DstType>(input);
	}

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif

	//////////////////////////////////////////////////////////////////////////
	// Endian and raw memory support

	template<typename OutputPtrType, typename InputPtrType, typename offset_type>
	RTM_FORCE_INLINE OutputPtrType* add_offset_to_ptr(InputPtrType* ptr, offset_type offset)
	{
		return safe_ptr_cast<OutputPtrType>(acl_impl::bit_cast<uintptr_t>(ptr) + offset);
	}

	RTM_FORCE_INLINE uint16_t byte_swap(uint16_t value)
	{
#if defined(RTM_COMPILER_MSVC)
		return _byteswap_ushort(value);
#elif defined(__APPLE__)
		return OSSwapInt16(value);
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
		return __builtin_bswap16(value);
#else
		return (value & 0x00FF) << 8 | (value & 0xFF00) >> 8;
#endif
	}

	RTM_FORCE_INLINE uint32_t byte_swap(uint32_t value)
	{
#if defined(RTM_COMPILER_MSVC)
		return _byteswap_ulong(value);
#elif defined(__APPLE__)
		return OSSwapInt32(value);
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
		return __builtin_bswap32(value);
#else
		value = (value & 0x0000FFFF) << 16 | (value & 0xFFFF0000) >> 16;
		value = (value & 0x00FF00FF) << 8 | (value & 0xFF00FF00) >> 8;
		return value;
#endif
	}

	RTM_FORCE_INLINE uint64_t byte_swap(uint64_t value)
	{
#if defined(RTM_COMPILER_MSVC)
		return _byteswap_uint64(value);
#elif defined(__APPLE__)
		return OSSwapInt64(value);
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
		return __builtin_bswap64(value);
#else
		value = (value & 0x00000000FFFFFFFF) << 32 | (value & 0xFFFFFFFF00000000) >> 32;
		value = (value & 0x0000FFFF0000FFFF) << 16 | (value & 0xFFFF0000FFFF0000) >> 16;
		value = (value & 0x00FF00FF00FF00FF) << 8 | (value & 0xFF00FF00FF00FF00) >> 8;
		return value;
#endif
	}

	// We copy bits assuming big-endian ordering for 'dest' and 'src'
	inline void memcpy_bits(void* dest, uint64_t dest_bit_offset, const void* src, uint64_t src_bit_offset, uint64_t num_bits_to_copy)
	{
		if (num_bits_to_copy == 0)
			return;	// Nothing to copy

		while (true)
		{
			uint64_t src_byte_offset = src_bit_offset / 8;
			uint32_t src_byte_bit_offset = safe_static_cast<uint32_t>(src_bit_offset % 8);
			uint64_t dest_byte_offset = dest_bit_offset / 8;
			uint32_t dest_byte_bit_offset = safe_static_cast<uint32_t>(dest_bit_offset % 8);

			const uint8_t* src_bytes = add_offset_to_ptr<const uint8_t>(src, src_byte_offset);
			uint8_t* dest_byte = add_offset_to_ptr<uint8_t>(dest, dest_byte_offset);

			// We'll copy only as many bits as there fits within 'dest' or as there are left
			uint32_t num_bits_dest_remain_in_byte = 8 - dest_byte_bit_offset;
			uint32_t num_bits_src_remain_in_byte = 8 - src_byte_bit_offset;
			uint64_t num_bits_copied = std::min<uint64_t>(std::min<uint32_t>(num_bits_dest_remain_in_byte, num_bits_src_remain_in_byte), num_bits_to_copy);
			uint32_t num_bits_copied_u32 = safe_static_cast<uint32_t>(num_bits_copied);

			// We'll shift and mask to retain the 'dest' bits prior to our offset and whatever remains after the copy
			uint32_t dest_shift_offset = dest_byte_bit_offset;
			uint32_t dest_byte_mask = ~(0xFFU >> dest_shift_offset) | ~(0xFFU << (8 - num_bits_copied_u32 - dest_byte_bit_offset));

			uint32_t src_shift_offset = 8 - src_byte_bit_offset - num_bits_copied_u32;
			uint32_t src_byte_mask = 0xFFU >> (8 - num_bits_copied_u32);
			uint32_t src_insert_shift_offset = 8 - num_bits_copied_u32 - dest_byte_bit_offset;

			uint32_t partial_dest_value = *dest_byte & dest_byte_mask;
			uint32_t partial_src_value = (*src_bytes >> src_shift_offset) & src_byte_mask;
			*dest_byte = safe_static_cast<uint8_t>(partial_dest_value | (partial_src_value << src_insert_shift_offset));

			if (num_bits_to_copy <= num_bits_copied)
				break;	// Done

			num_bits_to_copy -= num_bits_copied;
			dest_bit_offset += num_bits_copied;
			src_bit_offset += num_bits_copied;
		}
	}

	template<typename data_type>
	RTM_FORCE_INLINE data_type unaligned_load(const void* input)
	{
		data_type result;
		std::memcpy(&result, input, sizeof(data_type));
		return result;
	}

	template<typename data_type>
	RTM_FORCE_INLINE data_type aligned_load(const void* input)
	{
		return *safe_ptr_cast<const data_type>(input);
	}

	template<typename data_type>
	RTM_FORCE_INLINE void unaligned_write(data_type input, void* output)
	{
		std::memcpy(output, &input, sizeof(data_type));
	}

	// TODO: Add support for streaming prefetch (ptr, 0, 0) for arm
	RTM_FORCE_INLINE void memory_prefetch(const void* ptr)
	{
#if defined(RTM_SSE2_INTRINSICS)
		_mm_prefetch(acl_impl::bit_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
		__builtin_prefetch(ptr, 0, 3);
#elif defined(RTM_NEON64_INTRINSICS) && defined(RTM_COMPILER_MSVC)
		__prefetch(ptr);
#else
		(void)ptr;
#endif
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
