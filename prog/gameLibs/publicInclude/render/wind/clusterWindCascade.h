//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_tab.h>

#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>

class ClusterWindCascade
{
public:
  union GridClusterIds
  {
    unsigned int allIds;
    unsigned char id[4];
  };

  enum ClusterWindUtils
  {
    SHIFT_SIZE = 0xFF,
    SHIFT_AMOUNT = 8,

    MAX_SIMPLIFIED_BLAST_NUM = 254,
    NO_INDEX = SHIFT_SIZE,
    NO_INDEX_ALL = 0xFFFFFFFF
  };

  enum ClusterWindGridUpdateResult
  {
    X_POSITIV,
    X_NEGATIV,
    Z_POSITIV,
    Z_NEGATIV,
    NO_RESULT
  };

  struct Desc
  {
    Point3 pos;
    int boxNum;
    int maxClusterPerBox;
    float size;
  };

  float size;
  float boxSize;
  int boxWidthNum; // to calculate numBox per axe
  int maxClusterPerBox;
  Tab<GridClusterIds> gridBoxes;
  ToroidalHelper torHelper;
  Point3 position;

  IPoint2 offset;

  BBox2 gridBoundary;

  void clear();
  void initGrid(const ClusterWindCascade::Desc &desc);
  const Point3 getActualPosition() const;
  Point3 getBoxPosition(int boxId);
  bool isClusterOnCascade(const Point3 &pos, float r) const;
  int getAllClusterId(int boxId) const;
  int getClusterId(int boxId, int at);
  int getClosestStaticBoxId(const Point3 &pos) const;
  IPoint2 getStaticBoxXY(int boxId);
  IPoint2 getBoxXY(int boxId);
  void setClusterIndexToBox(int boxId, int clusterId, int at);
  ToroidalQuadRegion updateGridPosition(const Point3 &pos);
  void resetId(int boxId);
  bool isSphereOnBox(const BBox2 &box, const Point2 &pos, float r) const;
  void updateOffset(const IPoint2 &offsetToAdd);
  void setOffset(const IPoint2 &newOffset);
  void removeClusterFromGrids(int clusterId);
  bool isClusterOnBox(int boxId, Point2 pos, float r);

private:
};
