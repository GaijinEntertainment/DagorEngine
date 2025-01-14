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
#include "acl/core/bit_manip_utils.h"

#include <cstdint>
#include <limits>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// A bit set description holds the required information to ensure type and memory safety
	// with the various bit set functions.
	////////////////////////////////////////////////////////////////////////////////
	class bitset_description
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Creates an invalid bit set description.
		constexpr bitset_description() : m_size(0) {}

		////////////////////////////////////////////////////////////////////////////////
		// Creates a bit set description from a compile time known number of bits.
		template<uint64_t num_bits>
		static constexpr bitset_description make_from_num_bits()
		{
			static_assert(num_bits <= (std::numeric_limits<uint32_t>::max)() - 31, "Number of bits exceeds the maximum number allowed");
			return bitset_description((uint32_t(num_bits) + 32 - 1) / 32);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Creates a bit set description from a runtime known number of bits.
		inline static bitset_description make_from_num_bits(uint32_t num_bits)
		{
			ACL_ASSERT(num_bits <= (std::numeric_limits<uint32_t>::max)() - 31, "Number of bits exceeds the maximum number allowed");
			return bitset_description((num_bits + 32 - 1) / 32);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the number of 32 bit words used to represent the bitset.
		// 1 == 32 bits, 2 == 64 bits, etc.
		constexpr uint32_t get_size() const { return m_size; }

		////////////////////////////////////////////////////////////////////////////////
		// Returns the number of bits contained within the bit set.
		constexpr uint32_t get_num_bits() const { return m_size * 32; }

		////////////////////////////////////////////////////////////////////////////////
		// Returns the number of bytes used by the bit set.
		constexpr uint32_t get_num_bytes() const { return m_size * sizeof(uint32_t); }

		////////////////////////////////////////////////////////////////////////////////
		// Returns true if the index is valid within the bit set.
		constexpr bool is_bit_index_valid(uint32_t index) const { return index < get_num_bits(); }

	private:
		////////////////////////////////////////////////////////////////////////////////
		// Creates a bit set description from a specified size.
		explicit constexpr bitset_description(uint32_t size) : m_size(size) {}

		// Number of words required to hold the bit set
		// 1 == 32 bits, 2 == 64 bits, etc.
		uint32_t		m_size;
	};

	//////////////////////////////////////////////////////////////////////////
	// A bit set index reference is created from a bit set description and a bit index.
	// It holds the bit set word offset as well as the bit mask required.
	// This is useful if you sample multiple bit sets at the same index.
	//////////////////////////////////////////////////////////////////////////
	struct bitset_index_ref
	{
		bitset_index_ref()
			: desc()
			, offset(0)
			, mask(0)
		{}

		bitset_index_ref(bitset_description desc_, uint32_t bit_index)
			: desc(desc_)
			, offset(bit_index / 32)
			, mask(1U << (31 - (bit_index % 32)))
		{
			ACL_ASSERT(desc_.is_bit_index_valid(bit_index), "Invalid bit index: %d", bit_index);
		}

		bitset_description desc;
		uint32_t offset;
		uint32_t mask;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Resets the entire bit set to the provided value.
	inline void bitset_reset(uint32_t* bitset, bitset_description desc, bool value)
	{
		const uint32_t mask = value ? 0xFFFFFFFF : 0x00000000;
		const uint32_t size = desc.get_size();

		for (uint32_t offset = 0; offset < size; ++offset)
			bitset[offset] = mask;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Sets a specific bit to its desired value.
	inline void bitset_set(uint32_t* bitset, bitset_description desc, uint32_t bit_index, bool value)
	{
		ACL_ASSERT(desc.is_bit_index_valid(bit_index), "Invalid bit index: %d", bit_index);
		(void)desc;

		const uint32_t offset = bit_index / 32;
		const uint32_t mask = 1U << (31 - (bit_index % 32));

		if (value)
			bitset[offset] |= mask;
		else
			bitset[offset] &= ~mask;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Sets a specific bit to its desired value.
	inline void bitset_set(uint32_t* bitset, const bitset_index_ref& ref, bool value)
	{
		if (value)
			bitset[ref.offset] |= ref.mask;
		else
			bitset[ref.offset] &= ~ref.mask;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Sets a specified range of bits to a specified value.
	inline void bitset_set_range(uint32_t* bitset, bitset_description desc, uint32_t start_bit_index, uint32_t num_bits, bool value)
	{
		ACL_ASSERT(desc.is_bit_index_valid(start_bit_index), "Invalid start bit index: %d", start_bit_index);
		ACL_ASSERT(start_bit_index + num_bits <= desc.get_num_bits(), "Invalid num bits: %d > %d", start_bit_index + num_bits, desc.get_num_bits());

		const uint32_t end_bit_offset = start_bit_index + num_bits;
		for (uint32_t offset = start_bit_index; offset < end_bit_offset; ++offset)
			bitset_set(bitset, desc, offset, value);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns the bit value as a specific index.
	inline bool bitset_test(const uint32_t* bitset, bitset_description desc, uint32_t bit_index)
	{
		ACL_ASSERT(desc.is_bit_index_valid(bit_index), "Invalid bit index: %d", bit_index);
		(void)desc;

		const uint32_t offset = bit_index / 32;
		const uint32_t mask = 1U << (31 - (bit_index % 32));

		return (bitset[offset] & mask) != 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Returns the bit value as a specific index.
	inline bool bitset_test(const uint32_t* bitset, const bitset_index_ref& ref)
	{
		return (bitset[ref.offset] & ref.mask) != 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Counts the total number of set (true) bits within the bit set.
	inline uint32_t bitset_count_set_bits(const uint32_t* bitset, bitset_description desc)
	{
		const uint32_t size = desc.get_size();

		// TODO: Optimize for NEON by using the intrinsic directly and unrolling the loop to
		// reduce the number of pairwise add instructions.
		uint32_t num_set_bits = 0;
		for (uint32_t offset = 0; offset < size; ++offset)
			num_set_bits += count_set_bits(bitset[offset]);

		return num_set_bits;
	}

	//////////////////////////////////////////////////////////////////////////
	// Performs the operation: result = ~not_value & and_value
	// Bit sets must have the same description
	// Bit sets can alias
	inline void bitset_and_not(uint32_t* bitset_result, const uint32_t* bitset_not_value, const uint32_t* bitset_and_value, bitset_description desc)
	{
		const uint32_t size = desc.get_size();

		for (uint32_t offset = 0; offset < size; ++offset)
			bitset_result[offset] = and_not(bitset_not_value[offset], bitset_and_value[offset]);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
