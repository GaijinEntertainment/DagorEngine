#pragma once
#ifndef __GAIJIN_PROTOIO_STRINGOPS__
#define __GAIJIN_PROTOIO_STRINGOPS__

#include <cstddef>

namespace proto
{
  template <typename T>
  inline const char * str_cstr(const T & value)
  {
    return value.c_str();
  }

  template <typename T>
  inline size_t str_size(const T & value)
  {
    return value.size();
  }

  template <typename T>
  inline void str_assign(T & value, const char * src, size_t size)
  {
    value.assign(src, size);
  }

  template <typename T>
  inline T get_enum_from_str(const char * value, T defaultValue)
  {
    T result(defaultValue);
    if (!str_to_enum(value, result))
    {
      result = defaultValue;
    }

    return result;
  }
}

#endif // __GAIJIN_PROTOIO_STRINGOPS__
