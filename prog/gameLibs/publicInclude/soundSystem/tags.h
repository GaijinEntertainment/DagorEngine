//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/numeric_limits.h>

namespace sndsys
{
struct Tags
{
  using value_t = int64_t;

  Tags() = default;
  explicit Tags(value_t val) : val(val) {}

  inline Tags operator|(Tags t) const { return Tags(val | t.val); }
  inline Tags operator&(Tags t) const { return Tags(val & t.val); }
  inline Tags &operator|=(Tags t)
  {
    val |= t.val;
    return *this;
  }
  inline Tags &operator&=(Tags t)
  {
    val &= t.val;
    return *this;
  }
  inline Tags operator~() const { return Tags(~val); }

  inline bool operator==(Tags other) const { return val == other.val; }
  inline bool operator!=(Tags other) const { return val != other.val; }
  inline bool operator!() const { return val == empty_val(); }

  inline value_t value() const { return val; }

  static inline constexpr int capacity() { return eastl::numeric_limits<value_t>::digits; }
  static inline constexpr value_t empty_val() { return value_t{}; }
  static inline Tags make_bit(int n) { return Tags(value_t(1) << n); }

private:
  value_t val = {};
};
} // namespace sndsys
