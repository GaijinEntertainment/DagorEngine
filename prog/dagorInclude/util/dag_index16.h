//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace dag
{
struct Index16;
}

struct dag::Index16
{
  static constexpr int INVALID = (unsigned short)~(unsigned short)0u;

  Index16() = default;
  explicit Index16(int i) : idx(i >= 0 ? i : INVALID) {}

  bool valid() const { return idx != INVALID; }
  explicit operator bool() const { return valid(); }

  unsigned index() const { return idx; }
  explicit operator unsigned() const { return index(); }
  explicit operator int() const { return idx != INVALID ? int(idx) : -1; }

  bool operator==(const Index16 &ni) const { return idx == ni.idx; }
  bool operator!=(const Index16 &ni) const { return idx != ni.idx; }
  bool operator<(const Index16 &ni) const { return idx < ni.idx; }

  Index16 &operator++()
  {
    ++idx;
    return *this;
  }
  Index16 preceeding() const { return operator bool() ? Index16(idx - 1) : Index16(); }

private:
  unsigned short idx = INVALID;
};
