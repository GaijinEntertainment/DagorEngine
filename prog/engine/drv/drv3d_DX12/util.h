#pragma once

#include <EASTL/bitset.h>
#include <EASTL/span.h>

/*!
 * @file
 */

/**
 * @brief Performs a bitwise OR operation on a specific bit of the given bitset.
 *
 * @tparam  N   The size of the bitset.
 * 
 * @param [in] s    The bitset on which the operation is performed.
 * @param [in] i    The index of the bit to perform the operation on.
 * @param [in] b    The boolean value to OR with the bit at index 'i'. Default is 'true'.
 * @return          A reference to the modified bitset after performing the operation.
 * 
 * @note The function also performs a bounds check and throws std::out_of_range if \b i does not correspond to a valid position in the bitset.
 * 
 * This function modifies the bitset by performing a bitwise OR operation on the bit at index \b i.
 * The result is stored back in the <b>i</b>-th bit of the original bitset.
 */
template <size_t N>
inline eastl::bitset<N> &or_bit(eastl::bitset<N> &s, size_t i, bool b = true)
{
  return s.set(i, s.test(i) || b);
}

/**
 * @brief Appends a string literal to a character buffer.
 * 
 * @tparam  N   The length of the literal.
 * 
 * @param [in] at   Pointer to position in the character buffer to append to.
 * @param [in] ed   Pointer to the end of the character buffer.
 * @param [in] lit  The string literal to append.
 * @return          A pointer to the next available position in the buffer after appending the literal.
 * 
 * @note In case there is not enough space in the buffer to append the entire literal, it will be cropped to fit.
 */
template <size_t N>
inline char *append_literal(char *at, char *ed, const char (&lit)[N])
{
  auto d = static_cast<size_t>(ed - at);
  auto c = min(d, N - 1);
  memcpy(at, lit, c);
  return at + c;
}

/**
 * @brief Appends a name to a string mask.
 *
 * @tparam  N       The length of the name literal.
 * 
 * @param [in] beg      Pointer to the beginning of a character buffer containing the string mask.
 * @param [in] at       Pointer to the in the character buffer to append to.
 * @param [in] ed       Pointer to the end of the character buffer.
 * @param [in] name     The name to append. It must be a string literal.
 * @return              A pointer to the next available position in the buffer after appending the name.
 * 
 * This function appends \b name to the character buffer. 
 * In case the buffer is already not empty, <c>" | "</c> is inserted to separate its initial content from the appended literal.
 */
template <size_t N>
inline char *append_or_mask_value_name(char *beg, char *at, char *ed, const char (&name)[N])
{
  if (beg != at)
  {
    at = append_literal(at, ed, " | ");
  }
  return append_literal(at, ed, name);
}

/**
 * @brief Aligns value to a specified alignment.
 * 
 * @tparam  T           The \b value and \b alignment type.
 * 
 * @param [in] value        The value to align.
 * @param [in] alignment    The alignment value to align to.
 * @return                  The aligned value, i.e. minimal multiple of the \b alignment greater or equal than \b value.
*/
template <typename T>
inline T align_value(T value, T alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Constructs span instance from a string literal.
 *
 * @tparam  N   The length of the literal.
 * 
 * @param [in] sl   A character buffer storing the string literal.
 * @return          The constructed span instance.
 */
template <size_t N>
inline eastl::span<const char> string_literal_span(const char (&sl)[N])
{
  return {sl, N - 1};
}

/**
 * @brief Constructs span instance from a string literal.
 *
 * @tparam  N   The length of the literal.
 * 
 * @param [in] sl   A wide character buffer storing the string literal.
 * @return          The constructed span instance.
 */
template <size_t N>
inline eastl::span<const wchar_t> string_literal_span(const wchar_t (&sl)[N])
{
  return {sl, N - 1};
}

/**
 * @brief Applies the given function object to the index of each set bit in a bit mask.
 *
 * @tparam  F   The type of the function object. The type must provide a <c>operator()(uint32_t)</c> overload.
 * 
 * @param [in] m   The bitmask to scan for set bits.
 * @param [in] f   The function object to be applied to the index of each set bit.
 */
template <typename F>
inline void for_each_set_bit(uint32_t m, F f)
{
  while (0 != m)
  {
    uint32_t i = __bsf_unsafe(m);
    m ^= 1u << i;
    f(i);
  }
}
