#pragma once

#include <util/dag_inttypes.h>


inline uint32_t size_to_unit_table(uint64_t sz)
{
  uint32_t unitIndex = 0;
  unitIndex += sz >= (1024 * 1024 * 1024);
  unitIndex += sz >= (1024 * 1024);
  unitIndex += sz >= (1024);
  return unitIndex;
}

inline const char *get_unit_name(uint32_t index)
{
  static const char *unitTable[] = {"Bytes", "KiBytes", "MiBytes", "GiBytes"};
  return unitTable[index];
}

inline float compute_unit_type_size(uint64_t sz, uint32_t unit_index) { return static_cast<float>(sz) / (powf(1024, unit_index)); }

class ByteUnits
{
  uint64_t size = 0;

public:
  ByteUnits() = default;

  ByteUnits(const ByteUnits &) = default;
  ByteUnits &operator=(const ByteUnits &) = default;


  ByteUnits(uint64_t v) : size{v} {}
  ByteUnits &operator=(uint64_t v)
  {
    size = v;
    return *this;
  }

  ByteUnits &operator+=(ByteUnits o)
  {
    size += o.size;
    return *this;
  }
  ByteUnits &operator-=(ByteUnits o)
  {
    size -= o.size;
    return *this;
  }

  friend ByteUnits operator+(ByteUnits l, ByteUnits r) { return {l.size + r.size}; }
  friend ByteUnits operator-(ByteUnits l, ByteUnits r) { return {l.size - r.size}; }

  uint64_t value() const { return size; }
  float units() const { return compute_unit_type_size(size, size_to_unit_table(size)); }
  const char *name() const { return get_unit_name(size_to_unit_table(size)); }
};
