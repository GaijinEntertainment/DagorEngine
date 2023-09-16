//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/*
 * Dagor Engine 4 - Game Libraries
 * Copyright (C) 2003-2023  Gaijin Entertainment.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef _DAGOR_GAMELIB_LANDMESH_LMESHHOLES_H_
#define _DAGOR_GAMELIB_LANDMESH_LMESHHOLES_H_

#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <math/dag_Point2.h>
#include <EASTL/vector.h>

class HeightmapHandler;

class LandMeshHolesManager
{
  struct LandMeshHolesCell
  {
    struct HoleData
    {
      bool shapeIntersection;
      bool round;
      union
      {
        Matrix3 inverseProjTm;
        TMatrix inverseTm;
      };
      HoleData(const TMatrix &tm, bool is_round, bool shape_intersection);
    };
    eastl::vector<HoleData> holes;
    bool check(const Point2 &p, const HeightmapHandler *hmapHandler) const;
  };

public:
  struct HoleArgs
  {
    bool shapeIntersection;
    bool round;
    TMatrix tm;

    HoleArgs(const TMatrix &tm, bool is_round, bool shape_intersection);
    BBox2 getBBox2() const;
  };

  LandMeshHolesManager(const HeightmapHandler *hmap_handler, int cells_count = 16);
  void clearAndAddHoles(const eastl::vector<HoleArgs> &holes);
  void clearHoles();
  bool check(const Point2 &p) const;

private:
  int holeCellsCount;
  BBox2 holesRegion;
  Point2 cellSize;
  Point2 cellSizeInv;
  eastl::vector<LandMeshHolesCell> cells;
  const HeightmapHandler *hmapHandler;
};

#endif
