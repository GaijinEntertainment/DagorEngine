#pragma once

#include <array>
#include <cstddef>

#include <debug/dag_assert.h>

class DataAccumulator
{
public:
  template <typename T>
  void appendValue(const T &data_to_copy)
  {
    appendData((void *)(&data_to_copy), sizeof(T));
  }

  template <typename T>
  void appendContainer(const T &container)
  {
    appendData((void *)(container.data()), data_size(container));
  }

  void appendStr(const char *str)
  {
    strcpy((char *)ptr, str);
    ptr += strlen(str);
  }

  void appendStr(const char *str, size_t str_size)
  {
    strcpy((char *)ptr, str);
    ptr += str_size;
  }

  void appendData(void *data_to_copy, uint32_t data_size)
  {
    memcpy(ptr, data_to_copy, data_size);
    ptr += data_size;
    G_ASSERT(size() < MaxSize);
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