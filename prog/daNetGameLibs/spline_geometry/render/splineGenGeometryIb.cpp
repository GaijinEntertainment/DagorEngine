// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometryIb.h"
#include "splineGenGeometryShaderVar.h"
#include <3d/dag_lockSbuffer.h>
#include <util/dag_string.h>

SplineGenGeometryIb::SplineGenGeometryIb(int slices_, int stripes_) : slices(slices_) { resize(stripes_); }

void SplineGenGeometryIb::resize(int stripes)
{
  if (stripes > maxStripes)
  {
    maxStripes = stripes;

    int count = 3 * getMaxTriangleCount();
    int ibSize = sizeof(uint16_t) * count;
    indexBuffer.close();
    String buffName = String(0, "SplineGenGeometryIb_%d", slices);
    indexBuffer = dag::create_ib(ibSize, SBCF_BIND_INDEX, buffName);
    fillIndexBuffer(count);
  }
}

void SplineGenGeometryIb::fillIndexBuffer(uint32_t count_indices)
{
  if (auto ib_data = lock_sbuffer<uint16_t>(indexBuffer.getBuf(), 0, count_indices, VBLOCK_WRITEONLY))
  {
    for (uint32_t i = 0; i < getMaxTriangleCount() / 2; i++)
    {
      IPoint2 coord = getQuadCoord(i);
      uint32_t currId = decodeVertexCoord(coord);
      uint32_t rotId = decodeVertexCoord(coord + IPoint2(1, 0));
      uint32_t stipeId = decodeVertexCoord(coord + IPoint2(0, 1));
      uint32_t diagonalId = decodeVertexCoord(coord + IPoint2(1, 1));
      ib_data[(i * 2 + 0) * 3 + 0] = static_cast<uint16_t>(currId);
      ib_data[(i * 2 + 0) * 3 + 1] = static_cast<uint16_t>(stipeId);
      ib_data[(i * 2 + 0) * 3 + 2] = static_cast<uint16_t>(rotId);
      // when coord.y == stripes - 1 the second triangle will have an area of 0, but it
      // makes the code a lot simpler
      ib_data[(i * 2 + 1) * 3 + 0] = static_cast<uint16_t>(diagonalId);
      ib_data[(i * 2 + 1) * 3 + 1] = static_cast<uint16_t>(rotId);
      ib_data[(i * 2 + 1) * 3 + 2] = static_cast<uint16_t>(stipeId);
    }
  }
}

IPoint2 SplineGenGeometryIb::getQuadCoord(uint32_t quad_id) const
{
  uint32_t strip = quad_id / slices;
  uint32_t rotation = quad_id % slices;
  return IPoint2(rotation, strip);
}

uint32_t SplineGenGeometryIb::decodeVertexCoord(IPoint2 coord) const { return coord.x % (slices + 1) + coord.y * (slices + 1); }

int SplineGenGeometryIb::getMaxTriangleCount() const { return maxStripes * slices * 2; }
