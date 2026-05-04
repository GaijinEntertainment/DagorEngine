// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <climits>
#include <math/dag_intrin.h>
#include <generic/dag_align.h>
#include <debug/dag_assert.h>
#include <vecmath/dag_vecMath.h>

namespace detail
{

inline constexpr uint32_t WORD_BITS = sizeof(uint32_t) * CHAR_BIT;
inline constexpr uint32_t WORDS_PER_VEC4 = sizeof(vec4f) / sizeof(uint32_t);
template <bool value>
inline constexpr uint32_t WORD_WITHOUT = value ? 0 : ~0u;

template <bool value, bool forward>
uint32_t bit_search_word(uint32_t word)
{
  if constexpr (forward)
    return __ctz_unsafe(value ? word : ~word);
  else
    return WORD_BITS - 1 - __clz_unsafe(value ? word : ~word);
}

template <bool forward>
uint32_t processed_word_bits(uint32_t bit_idx)
{
  if constexpr (forward)
    return bit_idx % WORD_BITS;
  else
    return (WORD_BITS - 1) - (bit_idx % WORD_BITS);
}

template <bool value, bool forward>
uint32_t bit_search_slow(const uint32_t *data, uint32_t size, uint32_t start_bit, uint32_t &current_word)
{
  G_FAST_ASSERT(processed_word_bits<forward>(start_bit) == 0);
  G_FAST_ASSERT(forward || start_bit > 0);

  auto vecContainsValue = [](vec4f v) -> bool {
    if constexpr (value)
      return !v_test_all_bits_zeros(v);
    else
      return !v_test_all_bits_ones(v);
  };

  const auto wordContainsValue = [&](uint32_t w) -> bool { return w != WORD_WITHOUT<value>; };

  const auto foundInWord = [&](uint32_t word, uint32_t word_idx) -> uint32_t {
    current_word = word;
    return word_idx * WORD_BITS + bit_search_word<value, forward>(word);
  };

  const auto notFound = [&]() -> uint32_t {
    current_word = WORD_WITHOUT<value>;
    return size;
  };

  // Words past this boundry must STILL be readable for v_ld to work, up to 4 word boundary.
  const uint32_t wordCount = dag::divide_align_up(size, WORD_BITS);
  uint32_t wordIdx = start_bit / WORD_BITS;

  // Phase 1: linear scan until vec4f-aligned
  while (wordIdx < wordCount && wordIdx % WORDS_PER_VEC4 != (forward ? 0 : WORDS_PER_VEC4 - 1))
  {
    const uint32_t word = data[wordIdx];
    if (wordContainsValue(word))
      return foundInWord(word, wordIdx);
    wordIdx = forward ? wordIdx + 1 : wordIdx - 1;
  }

  if (wordIdx >= wordCount)
    return notFound();

  // Phase 2: SIMD scan
  vec4f vec;
  do
  {
    vec = v_ld(reinterpret_cast<const float *>(data + dag::align_down(wordIdx, WORDS_PER_VEC4)));
    if (vecContainsValue(vec))
      break;
    wordIdx = forward ? wordIdx + WORDS_PER_VEC4 : wordIdx - WORDS_PER_VEC4;
  } while (wordIdx < wordCount);

  if (forward && wordIdx >= wordCount)
    return notFound();

  // Phase 3: linear scan within vec4f
  alignas(16) uint32_t words[WORDS_PER_VEC4];
  v_st(reinterpret_cast<float *>(words), vec);
  while (wordIdx < wordCount)
  {
    const uint32_t word = words[wordIdx % WORDS_PER_VEC4];
    if (wordContainsValue(word))
      return foundInWord(word, wordIdx);
    wordIdx = forward ? wordIdx + 1 : wordIdx - 1;
  }

  return notFound();
}

} // namespace detail


/// @brief Finds the next bit position matching @p value.
///
/// When @p forward is true, returns the lowest position >= @p start_bit.
/// When @p forward is false, returns the highest position <= @p start_bit.
///
/// @pre Storage is vec4f-divisible (capacity always multiple of 4 words).
/// @pre Padding bits (positions >= size) are always zero.
/// @pre Storage is 16-byte aligned.
/// @pre current_word can only be uninitialized on 32-bit aligned boundaries,
///      otherwise it must have been initialized either manually or by a previous call to bit_search.
///
/// @tparam value  Bit polarity to search for (true = set bits, false = clear bits).
/// @tparam forward  Search direction (true = forward, false = backward).
/// @param data  Pointer to the word array.
/// @param size  Logical bit count.
/// @param storage_word_count  Total number of uint32_t words in storage.
/// @param start_bit  Bit position to start searching from.
/// @param[in,out] current_word  Cached word for fast drain in both directions.
///   - Forward uses cache when start_bit % 32 != 0.
///   - Backward uses cache when start_bit % 32 != 31.
///   - When at a word boundary, the slow path loads from memory.
///   - On output, contains all matching bits in the word of the found
///     position. Set to 0 if not found.
/// @return Found bit position, or @p size if not found.
///
template <bool value, bool forward>
uint32_t bit_search(const uint32_t *data, uint32_t size, uint32_t start_bit, uint32_t &current_word)
{
  if (start_bit >= size)
  {
    current_word = detail::WORD_WITHOUT<value>;
    return size;
  }

  // If we haven't processed any bits in the current word -- we definitely need to load it from memory.
  const uint32_t processedWordBits = detail::processed_word_bits<forward>(start_bit);
  if (processedWordBits == 0 || (!forward && start_bit + 1 == size))
    current_word = data[start_bit / detail::WORD_BITS];

  // Fast path: assume that there are many matching bits in the word
  // and try to drain them quickly using the cached current_word.
  uint32_t word = current_word;
  const uint32_t mask = forward ? ~0u << processedWordBits : ~0u >> processedWordBits;
  if constexpr (value)
    word &= mask;
  else
    word |= ~mask;
  if (DAGOR_LIKELY(word != detail::WORD_WITHOUT<value>))
    return dag::align_down(start_bit, detail::WORD_BITS) + detail::bit_search_word<value, forward>(word);

  if (forward)
    start_bit = dag::align_down(start_bit, detail::WORD_BITS) + detail::WORD_BITS;
  else
    start_bit = dag::align_down(start_bit, detail::WORD_BITS) - 1;

  // Slow path: assume an exponential distribution of hole sizes and
  // try to skip over many bits all at once.
  return detail::bit_search_slow<value, forward>(data, size, start_bit, current_word);
}
