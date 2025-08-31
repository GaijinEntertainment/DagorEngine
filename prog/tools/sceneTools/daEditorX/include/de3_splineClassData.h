//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_multiPointData.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <math/dag_TMatrix.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <math/dag_e3dColor.h>

class IDagorEdCustomCollider;


namespace splineclass
{
class RoadData;
class SingleGenEntityGroup;
class SingleEntityPool;

class GenEntities;
class LoftGeomGenData;

class AssetData;
struct Attr;
struct SegData;
} // namespace splineclass


struct splineclass::Attr
{
  float scale_h, scale_w;
  float opacity;
  float tc3u, tc3v;
  int followOverride;
  int roadBhvOverride;
};
struct splineclass::SegData
{
  int segN;
  float offset;
  float y;
  splineclass::Attr attr;
};

class splineclass::AssetData
{
public:
  RoadData *road;
  GenEntities *gen;
  LoftGeomGenData *genGeom;
  float sweepWidth, addFuzzySweepHalfWidth;
  float sweep2Width, addFuzzySweep2HalfWidth;
  float lmeshHtConstrSweepWidth;
  float navMeshStripeWidth;
  bool isCustomJumplink;

protected:
  AssetData();
  ~AssetData();
};


class splineclass::RoadData
{
public:
  RoadData()
  {
    borderShape = sideShape = roadShape = sideHammerShape = centerHammerShape = centerBorderShape = wallShape = bridgeShape = NULL;
  }

  ~RoadData()
  {
    del_it(borderShape);
    del_it(sideShape);
    del_it(roadShape);
    del_it(sideHammerShape);
    del_it(centerHammerShape);
    del_it(centerBorderShape);
    del_it(wallShape);
    del_it(bridgeShape);
  }

public:
  static const int MAX_SEP_LINES = 13;
  struct Mats
  {
    SimpleString road;
    SimpleString sides;
    SimpleString vertWalls;
    SimpleString centerBorder;
    SimpleString hammerSides;
    SimpleString hammerCenter;
    SimpleString paintings;
    SimpleString crossRoad;
  } mat;

  struct Lamp
  {
    enum
    {
      PLACE_Center,
      PLACE_Left,
      PLACE_Right,
      PLACE_BothSides
    };

    IObjEntity *entity;
    float roadOffset, eachSize, additionalHeightOffset;
    int placementType;
    bool rotateToRoad;
  };

  // when link is boundingLink, other properties are not used
  unsigned int isBoundingLink : 1;
  unsigned int hasLeftSide : 1, hasRightSide : 1, hasLeftSideBorder : 1, hasRightSideBorder : 1, hasLeftVertWall : 1,
    hasRightVertWall : 1, hasCenterBorder : 1, hasHammerSides : 1, hasHammerCenter : 1, hasPaintings : 1, hasStopLine : 1,
    flipRoadUV : 1, flipCrossRoadUV : 1, isBridge : 1, isCustomBridge : 1, isBridgeSupport : 1;

  unsigned int buildGeom : 1, buildRoadMap : 1;
  unsigned int linesCount : 4;
  float lineWidth;

  float sideSlope, wallSlope;
  int sepLineType[MAX_SEP_LINES];
  SmallTab<Lamp, MidmemAlloc> lamps;
  float curvatureStrength, minStep, maxStep;
  bool useDefaults;
  float leftWallHeight, rightWallHeight;
  float centerBorderHeight, leftBorderHeight, rightBorderHeight;
  float rightHammerYOffs, rightHammerHeight, leftHammerYOffs, leftHammerHeight;
  float centerHammerYOffs, centerHammerHeight;
  float bridgeRoadThickness, bridgeStandHeight;

  float walkablePartVScale, walkablePartUScale;

  float crossTurnRadius;
  float crossRoadUScale, crossRoadVScale;

  //! roadRandId is used to choose parameters for cross-roads
  int roadRankId;

  //! parameters to auto-compute upDir rotation in points using road curvature
  float maxUpdirRot, minUpdirRot, pointRtoUpdirRot, maxPointR;

  struct RoadShapeRec
  {
    RoadShapeRec() : points(midmem) {}

    struct ShapePoint
    {
      Point2 pos;
      bool soft;
    };

    bool isClosed;
    float uScale, vScale;

    Tab<ShapePoint> points;
  };

  RoadShapeRec *borderShape, *sideShape, *roadShape, *sideHammerShape, *centerHammerShape, *centerBorderShape, *wallShape,
    *bridgeShape;
};

class splineclass::SingleGenEntityGroup
{
public:
  SingleGenEntityGroup() : genEntRecs(midmem) {}

public:
  struct GenEntityRec
  {
    short entIdx;
    signed char genTag;

    real weight;
    real width;

    Point2 rotX, rotY, rotZ;
    Point2 xOffset, yOffset, zOffset;
    Point2 scale, yScale;

    struct CablePoints
    {
      Tab<Point3> in;
      Tab<Point3> out;
    };
    CablePoints cablePoints;

    MpPlacementRec mpRec;
  };

  int rseed;   // random seed
  Point2 step; // step on spline (step, dispersion)

  Point2 offset; // x - alogn spline, y - across spline

  Point2 cableRadius;
  Point2 cableSag;
  Point2 cableRaggedDistribution;

  real sumWeight;

  enum
  {
    ORIENT_FENCE,
    ORIENT_WORLD,
    ORIENT_SPLINE,
    ORIENT_NORMAL,
    ORIENT_FENCE_NORM,
    ORIENT_SPLINE_UP,
    ORIENT_NORMAL_XZ,
    ORIENT_WORLD_XZ,
  };

  short orientType;
  unsigned short placeAtStart : 1, placeAtEnd : 1;
  unsigned short placeAtPoint : 1;
  unsigned short placeAtVeryStart : 1, placeAtVeryEnd : 1;
  unsigned short checkSweep1 : 1, checkSweep2 : 1;
  unsigned short tightFenceOrient : 1;
  unsigned short tightFenceIntegral : 1, tightFenceIntegralPerSegment : 1, tightFenceIntegralPerCorner : 1;
  unsigned integralLastEntCount;

  unsigned colorRangeIdx;
  float startPadding, endPadding;

  SmallTab<IDagorEdCustomCollider *, MidmemAlloc> colliders;
  SmallTab<signed char, MidmemAlloc> genTagSeq, genTagFirst, genTagLast;
  SmallTab<float, MidmemAlloc> genTagSumWt;
  unsigned collFilter;
  float aboveHt;
  float minGenHt, maxGenHt;

  bool useLoftSegs;
  bool setSeedToEntities = false;

  Tab<GenEntityRec> genEntRecs;
};


class splineclass::SingleEntityPool
{
public:
  SingleEntityPool() : entPool(midmem), entUsed(0) {}
  ~SingleEntityPool() { clear(); }

  void clear()
  {
    for (int i = 0; i < entPool.size(); i++)
      entPool[i]->destroy();
    clear_and_shrink(entPool);
    entUsed = 0;
  }

  void resetUsedEntities() { entUsed = 0; }
  void deleteUnusedEntities()
  {
    if (entUsed >= entPool.size())
      return;

    for (int i = entPool.size() - 1; i >= entUsed; i--)
      entPool[i]->destroy();
    entPool.resize(entUsed);
  }

public:
  Tab<IObjEntity *> entPool;
  int entUsed;
};


class splineclass::GenEntities
{
public:
  GenEntities() : data(midmem), ent(midmem) {}
  ~GenEntities() { clear(); }

  void clear()
  {
    for (int i = 0; i < ent.size(); i++)
      if (ent[i])
        ent[i]->destroy();
    ent.clear();
    data.clear();
  }

public:
  Tab<SingleGenEntityGroup> data;
  Tab<IObjEntity *> ent;
};


class splineclass::LoftGeomGenData
{
public:
  LoftGeomGenData() : loft(midmem), foundationGeom(true) {}

public:
  struct Loft
  {
    struct PtAttr
    {
      enum
      {
        TYPE_NORMAL,
        TYPE_REL_TO_GND,
        TYPE_MOVE_TO_MIN,
        TYPE_MOVE_TO_MAX,
        TYPE_GROUP_REL_TO_GND,
      };
      unsigned type : 4, attr : 4, grpId : 16, matId : 8;
    };

    Point2 offset;
    int subdivCount, shapeSubdivCount;
    real uSize, vTile;
    unsigned int closed : 1, flipUV : 1, extrude : 1, cullcw : 1;
    Tab<SimpleString> matNames;
    SmallTab<Point4, MidmemAlloc> shapePts;
    SmallTab<PtAttr, MidmemAlloc> shapePtsAttr;
    real minStep, maxStep, curvatureStrength, maxHerr, maxHillHerr;
    real htTestStep;
    unsigned flags;
    Point3 normalsDir;
    float aboveHt;
    real maxMappingDistortionThreshold;
    bool integralMappingLength;
    bool followHills, followHollows;
    bool roadBhv;
    bool makeDelaunayPtCloud;
    bool waterSurface, waterSurfaceExclude;
    bool storeSegs;
    int loftLayerOrder;
    int stage;
    float roadMaxAbsAng, roadMaxInterAng;
    float roadTestWidth;
    float marginAtStart = 0, marginAtEnd = 0, zeroOpacityDistAtEnds = 0;
    Point2 randomOpacityMulAlong, randomOpacityMulAcross;
    SimpleString layerTag;

    Loft() :
      subdivCount(4),
      shapeSubdivCount(4),
      uSize(51),
      vTile(1),
      closed(false),
      extrude(false),
      cullcw(false),
      offset(0, 0),
      flipUV(false),
      integralMappingLength(false),
      aboveHt(1.0),
      htTestStep(1.5f),
      followHills(false),
      followHollows(false),
      makeDelaunayPtCloud(false),
      waterSurface(false),
      waterSurfaceExclude(false),
      storeSegs(false),
      roadBhv(false),
      roadMaxAbsAng(45.0f),
      roadMaxInterAng(30.0f),
      roadTestWidth(0),
      loftLayerOrder(0),
      stage(0),
      randomOpacityMulAlong(1, 0),
      randomOpacityMulAcross(1, 0),
      maxHerr(1.0),
      maxHillHerr(0.01),
      minStep(3.f),
      maxStep(25.f),
      curvatureStrength(15.f),
      normalsDir(0, 1, 0),
      matNames(midmem)
    {}
  };

  Tab<Loft> loft;
  bool foundationGeom;
  SmallTab<IDagorEdCustomCollider *, MidmemAlloc> colliders;
  unsigned collFilter;
};
