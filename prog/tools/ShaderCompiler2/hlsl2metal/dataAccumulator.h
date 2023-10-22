#pragma once

#include <array>
#include <cstddef>

#include <debug/dag_assert.h>

class DataAccumulator
{
public:
  template <typename T>
  DataAccumulator &append(const T &data_to_copy)
  {
    return appendData((void *)(&data_to_copy), sizeof(T));
  }

  DataAccumulator &appendData(const void *data_to_copy, uint32_t data_size)
  {
    G_ASSERT(size() + data_size <= MaxSize);
    memcpy(ptr, data_to_copy, data_size);
    ptr += data_size;

    return *this;
  }

  std::ptrdiff_t size() const { return ptr - header.data(); }

  const uint8_t *data() const { return header.data(); }

private:
  template <typename V, typename T = typename V::value_type>
  uint32_t data_size(const V &v)
  {
    return v.size() * (uint32_t)sizeof(T);
  }

  static constexpr size_t MaxSize = 1024;
  std::array<uint8_t, MaxSize> header{};
  uint8_t *ptr = header.data();
};