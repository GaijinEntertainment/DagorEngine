// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"


namespace drv3d_dx12
{

struct ViewportState
{
  int x;
  int y;
  int width;
  int height;
  float minZ;
  float maxZ;

  ViewportState() = default;

  ViewportState(const D3D12_VIEWPORT &vp)
  {
    x = vp.TopLeftX;
    y = vp.TopLeftY;
    width = vp.Width;
    height = vp.Height;
    minZ = vp.MinDepth;
    maxZ = vp.MaxDepth;
  }

  D3D12_RECT asRect() const
  {
    D3D12_RECT result;
    result.left = x;
    result.top = y;
    result.right = x + width;
    result.bottom = y + height;

    return result;
  }

  operator D3D12_VIEWPORT() const
  {
    D3D12_VIEWPORT result;
    result.TopLeftX = x;
    result.TopLeftY = y;
    result.Width = width;
    result.Height = height;
    result.MinDepth = minZ;
    result.MaxDepth = maxZ;
    return result;
  }
};

inline bool operator==(const ViewportState &l, const ViewportState &r)
{
#define CMP_P(n) (l.n == r.n)
  return CMP_P(x) && CMP_P(y) && CMP_P(width) && CMP_P(height) && CMP_P(minZ) && CMP_P(maxZ);
#undef CMP_P
}
inline bool operator!=(const ViewportState &l, const ViewportState &r) { return !(l == r); }
enum class RegionDifference
{
  EQUAL,
  SUBSET,
  SUPERSET
};

inline RegionDifference classify_viewport_diff(const ViewportState &from, const ViewportState &to)
{
  const int32_t dX = to.x - from.x;
  const int32_t dY = to.y - from.y;
  const int32_t dW = (to.width + to.x) - (from.width + from.x);
  const int32_t dH = (to.height + to.y) - (from.height + from.y);

  RegionDifference rectDif = RegionDifference::EQUAL;
  // if all zero, then they are the same
  if (dX | dY | dW | dH)
  {
    // can be either subset or completely different
    if (dX >= 0 && dY >= 0 && dW <= 0 && dH <= 0)
    {
      rectDif = RegionDifference::SUBSET;
    }
    else
    {
      rectDif = RegionDifference::SUPERSET;
    }
  }

  if (RegionDifference::SUPERSET != rectDif)
  {
    // min/max z only affect viewport but not render regions, so it is always a subset if it has
    // changed
    if (to.maxZ != from.maxZ || to.minZ != from.minZ)
    {
      return RegionDifference::SUBSET;
    }
  }
  return rectDif;
}


} // namespace drv3d_dx12
