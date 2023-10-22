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

#ifndef DAGOR_HEIGHTFIELD_TERRAIN_SHAPE_H
#define DAGOR_HEIGHTFIELD_TERRAIN_SHAPE_H

#pragma once

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMath.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <landMesh/lmeshHoles.h>
#include <heightMapLand/dag_compressedHeightmap.h>

#define DEFAULT_STEP 2 // Reduce the number of triangles for optimization.

/// btHeightfieldTerrainShape simulates a 2D heightfield terrain
class btDagorHeightfieldTerrainShape : public btConcaveShape
{
public:
  btDagorHeightfieldTerrainShape(const CompressedHeightmap &data, float htCell, float hmin, float hscale, const Point2 &ofs,
    const BBox3 &box, LandMeshHolesManager *holesManager, int in_step = DEFAULT_STEP) :
    step(in_step),
    hData(data),
    width(data.bw * data.block_width / in_step),
    length(data.bh * data.block_width / in_step),
    originalWidth(data.bw * data.block_width),
    originalHeight(data.bh * data.block_width),
    htCell(htCell),
    hmapCellSize(htCell * in_step),
    hmapInvCellSize(1.f / (htCell * in_step)),
    hMin(hmin),
    hScaleRaw(hscale),
    worldPosOfs(ofs),
    worldBox(box),
    holes(holesManager)
  {
    m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
  }

  void quantizeWithClamp(int *out, const btVector3 &point) const;

  virtual ~btDagorHeightfieldTerrainShape() {}

  virtual void getAabb(const btTransform &t, btVector3 &aabbMin, btVector3 &aabbMax) const;

  virtual void processAllTriangles(btTriangleCallback *callback, const btVector3 &aabbMin, const btVector3 &aabbMax) const;


  virtual void calculateLocalInertia(btScalar /*mass*/, btVector3 &inertia) const
  {
    // moving concave objects not supported
    inertia.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
  }

  void setLocalScaling(const btVector3 & /*scaling*/) {}
  const btVector3 &getLocalScaling() const
  {
    static btVector3 one(1.0, 1.0, 1.0);
    return one;
  }

  // debugging
  virtual const char *getName() const { return "DHEIGHTFIELD"; }

  void setStep(int in_step);
  int getStep() const { return step; }

protected:
  const CompressedHeightmap &hData;
  int width, length;
  float hmapCellSize, hmapInvCellSize, hMin, hScaleRaw, htCell;
  Point2 worldPosOfs;
  BBox3 worldBox;
  int step;
  int originalWidth; // In case of odd width.
  int originalHeight;
  LandMeshHolesManager *holes;
};

#endif // HEIGHTFIELD_TERRAIN_SHAPE_H
