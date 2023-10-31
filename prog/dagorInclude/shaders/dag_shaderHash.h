//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_strUtil.h>
#include <hash/sha1.h>

struct ShaderHashValue
{
  typedef uint8_t ValueType[20];
  ValueType value;

  friend bool operator==(const ShaderHashValue &l, const ShaderHashValue &r)
  {
    G_STATIC_ASSERT(sizeof(ShaderHashValue) == sizeof(ValueType));
    G_STATIC_ASSERT(20 == sizeof(ValueType));
    return memcmp(l.value, r.value, sizeof(ValueType)) == 0;
  }

  friend bool operator!=(const ShaderHashValue &l, const ShaderHashValue &r) { return !(l == r); }

  template <typename T>
  static ShaderHashValue calculate(dag::ConstSpan<T> data)
  {
    ShaderHashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data.data()), static_cast<int>(data_size(data)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  template <typename T>
  static ShaderHashValue calculate(const T *data, size_t count)
  {
    ShaderHashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data), static_cast<int>(count * sizeof(T)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  static ShaderHashValue fromString(const char *str, int len = -1)
  {
    ShaderHashValue value;
    str_hex_to_data_buf(value.value, sizeof(value.value), str, nullptr, len);
    return value;
  }

  void convertToString(char *buffer, size_t size) const
  {
    G_ASSERT(size > (sizeof(ShaderHashValue) * 2));
    data_to_str_hex_buf(buffer, size, value, sizeof(value));
  }
};
