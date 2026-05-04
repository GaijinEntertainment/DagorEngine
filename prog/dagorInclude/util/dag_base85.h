//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stddef.h>
#include <util/dag_stdint.h>

namespace detail
{
/// Encodes a value from 0 - 84 into a printable char, by adding 33, which is !
constexpr char encode_value_to_base_85(uint32_t value) { return 33 + value; }
/// Decodes a character from ! to u into a value from 0 - 84
constexpr uint32_t decode_value_from_base_85(char c) { return uint32_t(c) - 33; }
} // namespace detail

/// The encoder returns up to 5 ASCII characters plus a null terminator for easy string appending.
struct Base85OutputBlock
{
  /// Always null terminated string.
  char data[6];
};

/// Some settings for the base 85 encoder, those control the use of string shortcut optimizations.
struct EncoderSettings
{
  /// Allow to encode 0 into a single 'z'.
  /// Encodes 4 bytes into 1 byte, instead of 5.
  bool allowZCode : 1 = true;
  /// Allow to encode values less than 85 into a single base 85 char terminated with a 'y'.
  /// Encodes 4 bytes into 2 bytes, instead of 5.
  bool allowYCode : 1 = true;
  /// Allow to encode values less than 7225 into two base 85 chars terminated with a 'x'.
  /// Encodes 4 bytes into 3 bytes, instead of 5.
  bool allowXCode : 1 = true;
  /// Allow to encode values less than 614125 into three base 85 chars terminated with a 'w'.
  /// Encodes 4 bytes into 4 bytes, instead of 5.
  bool allowWCode : 1 = true;
};

/// @brief Encodes a single 32 bit word into up to 5 ASCII characters, optimizations may reduce the output down to a single character.
/// @param word The 32 bit word to encode with base 85
/// @param settings Settings to control some optimizations, see type for details. Enables all by default.
/// @return Returns an array of chars, holding a null terminated string with up to 5 ASCII characters.
constexpr Base85OutputBlock encode_32_bit_word_to_base_85_block(uint32_t word, const EncoderSettings &settings = {})
{
  Base85OutputBlock result{};
  uint32_t v0 = word % 85;
  uint32_t v1 = (word / 85) % 85;
  uint32_t v2 = (word / (85 * 85)) % 85;
  uint32_t v3 = (word / (85 * 85 * 85)) % 85;
  uint32_t v4 = (word / (85 * 85 * 85 * 85)) % 85;
  if (!word && settings.allowZCode)
  {
    result.data[0] = 'z';
  }
  // This saves 3 encoding bytes for small sized u32 values
  else if ((0 == (v1 + v2 + v3 + v4)) && settings.allowYCode)
  {
    result.data[0] = detail::encode_value_to_base_85(v0);
    result.data[1] = 'y';
  }
  // This saves 2 encoding bytes for medium sizes u32 values
  else if ((0 == (v2 + v3 + v4)) && settings.allowXCode)
  {
    result.data[0] = detail::encode_value_to_base_85(v0);
    result.data[1] = detail::encode_value_to_base_85(v1);
    result.data[2] = 'x';
  }
  // This results in 1 saved byte for fairly large u32 values
  else if ((0 == (v3 + v4)) && settings.allowWCode)
  {
    result.data[0] = detail::encode_value_to_base_85(v0);
    result.data[1] = detail::encode_value_to_base_85(v1);
    result.data[2] = detail::encode_value_to_base_85(v2);
    result.data[3] = 'w';
  }
  else
  {
    result.data[0] = detail::encode_value_to_base_85(v0);
    result.data[1] = detail::encode_value_to_base_85(v1);
    result.data[2] = detail::encode_value_to_base_85(v2);
    result.data[3] = detail::encode_value_to_base_85(v3);
    result.data[4] = detail::encode_value_to_base_85(v4);
  }
  return result;
}

struct Base85DecodeResult
{
  /// Pointer to the remaining string after base 85 values where decoded. nullptr indicates decoding error.
  const char *nextData = nullptr;
  /// Length of the remaining string after base 85 values where decoded.
  size_t nextLen = 0;
  /// The decoded base 85 value.
  uint32_t decodedValue = 0;
};

/// @brief Decodes a single 32 bit word from the given string and returns the remaining string info.
/// @param data String to decode
/// @param len Length of the string of 'data'
/// @return On error the returns values 'nextData' will be nullptr, cause can only be too short of a string of 'data'. On success it
/// will return the data of the remaining string and the decoded 32 bit word. The decoder can handle all optimizations
/// 'encode_32_bit_word_to_base_85_block' can produce without any extra settings as optimizations are just short representations of
/// repeated encoded zeros.
constexpr Base85DecodeResult dencode_32_bit_word_from_base_85_string(const char *data, size_t len)
{
  if (!data || len < 1)
    return {};

  constexpr uint32_t pow85[] = {1, 85, 7225, 614125, 52200625};
  uint32_t value = 0;

  for (size_t i = 0; i < 4; ++i)
  {
    if (data[i] == static_cast<char>('z' - i))
      return {data + i + 1, len - i - 1, value};
    value += detail::decode_value_from_base_85(data[i]) * pow85[i];
    if (len < i + 2)
      return {};
  }

  value += detail::decode_value_from_base_85(data[4]) * pow85[4];
  return {data + 5, len - 5, value};
}
