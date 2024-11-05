// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/unique_ptr.h>
#include <math/integer/dag_IPoint2.h>

// Responsible for allocating and filling up instance buffers for SplineGenGeometry rendering
// Instance buffers can be shared by SplineGenGeometryManager with the same slices
// The buffer's size is determined by the largest stipes requested
// It's stored in SplineGenGeometryRepository
// Buffers don't shrink to reduce number of allocations
class SplineGenGeometryIb
{
public:
  SplineGenGeometryIb(int slices_, int stripes_);
  void resize(int stripes);

  UniqueBuf indexBuffer;
  const int slices;

private:
  void fillIndexBuffer(uint32_t count_indices);
  IPoint2 getQuadCoord(uint32_t quad_id) const;
  uint32_t decodeVertexCoord(IPoint2 coord) const;
  int getMaxTriangleCount() const;

  int maxStripes = 0;
};
