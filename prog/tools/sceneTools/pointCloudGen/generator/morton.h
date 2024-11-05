// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/algorithm.h>
#include <math/dag_Point3.h>

inline uint64_t expandBits(uint32_t v)
{
  uint64_t x = v & 0x1fffff; // 21 bits from each value. Total 21 * 3 = 63 fits into 64 bit
  x = (x | x << 32) & 0x1f00000000ffff;
  x = (x | x << 16) & 0x1f0000ff0000ff;
  x = (x | x << 8) & 0x100f00f00f00f00f;
  x = (x | x << 4) & 0x10c30c30c30c30c3;
  x = (x | x << 2) & 0x1249249249249249;
  return x;
}

// 63 bit morton code
inline uint64_t getMortonCode(Point3 pos, Point3 min, Point3 max)
{
  uint64_t code = 0;
  const auto x = static_cast<uint32_t>(eastl::min(eastl::max(1048576 * (pos.x - min.x) / (max.x - min.x), 0), 1048575));
  const auto y = static_cast<uint32_t>(eastl::min(eastl::max(1048576 * (pos.y - min.y) / (max.y - min.y), 0), 1048575));
  const auto z = static_cast<uint32_t>(eastl::min(eastl::max(1048576 * (pos.z - min.z) / (max.z - min.z), 0), 1048575));

  code |= expandBits(x) | expandBits(y) << 1 | expandBits(z) << 2;
  return code;
}
