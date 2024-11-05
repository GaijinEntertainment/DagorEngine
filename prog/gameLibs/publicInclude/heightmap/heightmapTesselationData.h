//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/bitvector.h>


class HeightmapHandler;

class HMapTesselationData
{
public:
  HMapTesselationData(HeightmapHandler &in_handler);
  void init(int buffer_size, float tess_cell_size);

  void addTessSphere(const Point2 &pos, float radius);
  void addTessRect(const TMatrix &transform, bool project_volume);
  void tesselatePatch(const IPoint2 &cell, bool enable);
  bool testRegionTesselated(const IBBox2 &region) const;
  bool testRegionTesselated(const BBox2 &region) const;
  bool hasTesselation() const;
  float getTessCellSize() const;

private:
  void addTessRectImpl(const Matrix3 &transform33);
  IPoint2 getPatchXYFromCoord(const IPoint2 &coord) const;
  Point2 getCoordFromPatchXY(const Point2 &patch_xy) const;
  int getPatchNoFromXY(const IPoint2 &patch_xy) const;
  Point2 getCoordFromPosF(const Point2 &pos) const;
  IPoint2 getCoordFromPos(const Point2 &pos) const;
  Point2 getPosFromCoord(const Point2 &coord) const;

  HeightmapHandler &handler;
  int bufferSize = 0;
  eastl::bitvector<> patches;
  bool patchHasTess = false;
  IPoint2 patchNum = IPoint2::ZERO;
  Point2 pivot = Point2::ZERO;
  Point2 worldSize = Point2::ZERO;
  float tessCellSize;
};
