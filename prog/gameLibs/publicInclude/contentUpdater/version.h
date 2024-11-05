//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <EASTL/array.h>


namespace updater
{

struct Version
{
  static const uint32_t MINOR_VERSION_MASK;
  static const uint32_t MAJOR_VERSION_MASK;

  struct String
  {
    eastl::array<char, 17> buffer; // Max possible string is 255.255.255.255 + zero

    String(uint32_t version);

    operator const char *() const { return buffer.data(); }
  };

  using Array = eastl::array<uint32_t, 4>;

  uint32_t value;

  explicit Version(uint32_t _value = 0u) : value(_value) {}
  explicit Version(const char *str);
  explicit Version(const Array &arr);

  uint32_t getMinor() const { return MINOR_VERSION_MASK & value; }

  uint32_t getMajor() const { return MAJOR_VERSION_MASK & value; }

  explicit operator bool() const { return value != 0u; }

  bool isCompatible(const Version &rhs) const { return getMajor() == rhs.getMajor(); }

  bool operator==(const Version &rhs) const { return value == rhs.value; }

  bool operator!=(const Version &rhs) const { return value != rhs.value; }

  bool operator<(const Version &rhs) const { return value < rhs.value; }

  bool operator>(const Version &rhs) const { return value > rhs.value; }

  bool operator<=(const Version &rhs) const { return value <= rhs.value; }

  bool operator>=(const Version &rhs) const { return value >= rhs.value; }

  String to_string() const { return String{value}; }

  Array to_array() const { return {(value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF}; }
};

Version read_remote_version_file(const char *folder);
} // namespace updater
