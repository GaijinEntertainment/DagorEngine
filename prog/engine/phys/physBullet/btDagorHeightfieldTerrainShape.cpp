/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btDagorHeightfieldTerrainShape.h"

#include <LinearMath/btTransformUtil.h>

void btDagorHeightfieldTerrainShape::setStep(int in_step)
{
  if (in_step <= 0)
    in_step = 1;

  step = in_step;
  width = originalWidth / in_step;
  length = originalHeight / in_step;

  hmapCellSize = htCell * in_step;
  hmapInvCellSize = 1.f / (htCell * in_step);
}

void btDagorHeightfieldTerrainShape::getAabb(const btTransform &t, btVector3 &aabbMin, btVector3 &aabbMax) const
{
  // optimize: probably, we never transform heightfield?
  // aabbMin = btVector3(worldBox[0].x, worldBox[0].y, worldBox[0].z);
  // aabbMax = btVector3(worldBox[1].x, worldBox[1].y, worldBox[1].z);
  // debug("%~p3 %~p3", worldBox[0], worldBox[1]);
  // return;
  Point3 half = 0.5f * worldBox.width();
  Point3 wbcenter = worldBox.center();
  btVector3 halfExtents = btVector3(half.x, half.y, half.z);
  halfExtents += btVector3(getMargin(), getMargin(), getMargin());


  btMatrix3x3 abs_b = t.getBasis().absolute();
  btVector3 center = t(btVector3(wbcenter.x, wbcenter.y, wbcenter.z));
  btVector3 extent = btVector3(abs_b[0].dot(halfExtents), abs_b[1].dot(halfExtents), abs_b[2].dot(halfExtents));

  aabbMin = center - extent;
  aabbMax = center + extent;
}


/// this returns the vertex in bullet-local coordinates
/// process all triangles within the provided axis-aligned bounding box
/**
  basic algorithm:
    - convert input aabb to local coordinates (scale down and shift for local origin)
    - convert input aabb to a range of heightfield grid points (quantize)
    - iterate over all triangles in that subset of the grid
 */
void btDagorHeightfieldTerrainShape::processAllTriangles(btTriangleCallback *callback, const btVector3 &aabbMin,
  const btVector3 &aabbMax) const
{
// quantize the aabbMin and aabbMax, and adjust the start/end ranges
#if BT_USE_DOUBLE_PRECISION
  vec4f bminmax = v_make_vec4f(aabbMin.getX(), aabbMin.getZ(), aabbMax.getX(), aabbMax.getZ());
#else
  vec4f bminmax = v_perm_xyab(v_perm_xzxz(v_ldu((const float *)aabbMin)), v_perm_xzxz(v_ldu((const float *)aabbMax)));
#endif
  vec4i vStartEndXZ = v_cvt_floori(v_mul(v_sub(bminmax, v_perm_xyxy(v_ldu_half(&worldPosOfs.x))), v_splats(hmapInvCellSize)));

  alignas(16) int start[4];
  v_sti(start, vStartEndXZ);
  int startX = start[0], startZ = start[1], endX = start[2], endZ = start[3];
  if (startX >= width - 1 || startZ >= length - 1 || endX < 0 || endZ < 0)
    return;

  startX = max(startX, 0);
  endX = min(endX, width - 2);
  startZ = max(startZ, 0);
  endZ = min(endZ, length - 2);
  float minAAHt = aabbMin.getY();
  uint16_t minAaHtInt = uint16_t(max(minAAHt - hMin, 0.f) / hScaleRaw);
  // todo: optimize, so that
  //  a) we get heights with direct access, so remove multiplication from heightmapData[x + y*hmapWidth.x]
  //  b) we reuse 2 vertices at least from previous horizontal quad
  //  c) we can reuse all heights from previous horizontal line of quads (will require some additional memory)
  float startXfloat0 = startX * hmapCellSize + worldPosOfs.x, startZfloat = startZ * hmapCellSize + worldPosOfs.y;
  for (int j = startZ; j <= endZ; j++, startZfloat += hmapCellSize)
  {
    float startXfloat = startXfloat0;
    for (int x = startX; x <= endX; x++, startXfloat += hmapCellSize)
    {
      uint16_t htInt[4];
      htInt[0] = hData.decodePixelUnsafe((x)*step, (j)*step);
      htInt[1] = hData.decodePixelUnsafe((x + 1) * step, (j)*step);
      htInt[2] = hData.decodePixelUnsafe((x)*step, (j + 1) * step);
      htInt[3] = hData.decodePixelUnsafe((x + 1) * step, (j + 1) * step);
      uint16_t maxHtInt = max(max(htInt[0], htInt[1]), max(htInt[2], htInt[3]));
      if (maxHtInt < minAaHtInt)
        continue;
      float height[4];
      for (int i = 0; i < 4; ++i)
        height[i] = htInt[i] * hScaleRaw + hMin;
      btVector3 vertices[3];
      bool holesCheck[4] = {false, false, false, false};
      if (holes)
      {
        holesCheck[0] = holes->check(Point2(startXfloat, startZfloat));
        holesCheck[1] = holes->check(Point2(startXfloat + hmapCellSize, startZfloat));
        holesCheck[2] = holes->check(Point2(startXfloat, startZfloat + hmapCellSize));
        holesCheck[3] = holes->check(Point2(startXfloat + hmapCellSize, startZfloat + hmapCellSize));
      }
      if (!((j + x) & 1))
      {
        // first triangle
        vertices[0].setValue(startXfloat, height[0], startZfloat);                               // getVertex(x,j,vertices[0]);
        vertices[1].setValue(startXfloat + hmapCellSize, height[1], startZfloat);                // getVertex(x+1,j,vertices[1]);
        vertices[2].setValue(startXfloat + hmapCellSize, height[3], startZfloat + hmapCellSize); // getVertex(x+1,j,vertices[1]);
        // getVertex(x+1,j+1,vertices[2]);
        if (!(holesCheck[0] || holesCheck[1] || holesCheck[3]))
          callback->processTriangle(vertices, x, j);
        // second triangle
        // getVertex(x,j,vertices[0]);
        // getVertex(x+1,j+1,vertices[1]);
        vertices[1] = vertices[2];                                                // getVertex(x+1,j+1,vertices[1]);
        vertices[2].setValue(startXfloat, height[2], startZfloat + hmapCellSize); // getVertex(x,j+1,vertices[2]);
        if (!(holesCheck[0] || holesCheck[2] || holesCheck[3]))
          callback->processTriangle(vertices, x, j);
      }
      else
      {
        // first triangle
        vertices[0].setValue(startXfloat, height[0], startZfloat);                // getVertex(x,j,vertices[0]);
        vertices[1].setValue(startXfloat, height[2], startZfloat + hmapCellSize); // getVertex(x,j+1,vertices[1]);
        vertices[2].setValue(startXfloat + hmapCellSize, height[1], startZfloat); // getVertex(x+1,j,vertices[2]);
        if (!(holesCheck[0] || holesCheck[1] || holesCheck[2]))
          callback->processTriangle(vertices, x, j);
        // second triangle
        vertices[0] = vertices[2]; // getVertex(x+1,j,vertices[0]);
        // getVertex(x,j+1,vertices[1]);
        vertices[2].setValue(startXfloat + hmapCellSize, height[3], startZfloat + hmapCellSize); // getVertex(x+1,j+1,vertices[2]);
        if (!(holesCheck[1] || holesCheck[2] || holesCheck[3]))
          callback->processTriangle(vertices, x, j);
      }
    }
  }
}
