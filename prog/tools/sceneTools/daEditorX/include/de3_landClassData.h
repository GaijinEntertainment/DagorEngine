//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_multiPointData.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <util/dag_simpleString.h>
#include <util/dag_roHugeHierBitMap2d.h>
#include <math/dag_e3dColor.h>
#include <math/random/dag_random.h>

class IDagorEdCustomCollider;

struct LandClassDetailInfo;

namespace landclass
{
typedef RoHugeHierBitMap2d<4, 3> DensityMap;
typedef DensityMap::L2Bmp::L1Bmp DensMapLeaf;

class SingleEntityPlaces;
class SingleGenEntityGroup;
class SingleEntityPool;
class GrassDensity;

class TiledEntities;
class PlantedEntities;
class GrassEntities;
class ColorImagesData;
class PolyGeomGenData;

class AssetData;
} // namespace landclass


class landclass::AssetData
{
public:
  TiledEntities *tiled;
  PlantedEntities *planted;
  ColorImagesData *colorImages;
  PolyGeomGenData *genGeom;
  GrassEntities *grass;
  DataBlock *detTex = 0;
  DataBlock *indicesTex = 0;
  IObjEntity *csgGen;
  int layerIdx;
  bool editableMasks;

  SimpleString splineClassAssetName;
  carray<int, 1 + 4> physMatId;     // physmat for whole landclass and for each subchannel of splatting mask
  carray<int, 64> indexedPhysMatId; // physmat for indexed landclass
protected:
  AssetData();
  ~AssetData();
};


class landclass::SingleEntityPlaces
{
public:
  SingleEntityPlaces() : entity(NULL), tm(midmem) {}
  SingleEntityPlaces(const SingleEntityPlaces &) = delete;
  ~SingleEntityPlaces() { clear(); }

  void clear()
  {
    if (entity)
    {
      entity->destroy();
      entity = NULL;
    }
    tm.clear();
  }

public:
  enum
  {
    ORIENT_WORLD,
    ORIENT_NORMAL,
    ORIENT_NORMAL_XZ,
    ORIENT_WORLD_XZ
  };

  Tab<TMatrix> tm;
  IObjEntity *entity;

  Point2 yOffset;
  unsigned rseed;
  unsigned orientType;
  unsigned colorRangeIdx;

  MpPlacementRec mpRec;

  SimpleString resName;
};
DAG_DECLARE_RELOCATABLE(landclass::SingleEntityPlaces);


class landclass::SingleGenEntityGroup
{
public:
  SingleGenEntityGroup() : obj(midmem), density(0), densityMap(NULL), collFilter(0) {}

public:
  struct Obj
  {
    enum
    {
      ORIENT_WORLD,
      ORIENT_NORMAL,
      ORIENT_NORMAL_XZ,
      ORIENT_WORLD_XZ
    };

    float weight;
    short entityIdx;
    short orientType;

    Point2 rotX, rotY, rotZ;
    Point2 scale, yScale;
    Point2 yOffset;

    MpPlacementRec mpRec;
  };

  float density;
  float objOverlapRatio;
  unsigned rseed;
  float sumWeight;
  Tab<Obj> obj;

  DensityMap *densityMap;
  Point2 densMapCellSz, densMapSize, densMapOfs;

  SmallTab<IDagorEdCustomCollider *, MidmemAlloc> colliders;
  unsigned collFilter;
  float aboveHt;
  unsigned colorRangeIdx;
  bool setSeedToEntities = false;
};
DAG_DECLARE_RELOCATABLE(landclass::SingleGenEntityGroup);


class landclass::GrassDensity
{
public:
  GrassDensity() : entity(NULL), density(0), densityMap(NULL) {}
  GrassDensity(const GrassDensity &) = delete;
  ~GrassDensity() { clear(); }

  void clear()
  {
    if (entity)
      entity->destroy();
    entity = NULL;
    density = NULL;
  }

public:
  float density;
  unsigned rseed;
  IObjEntity *entity;

  DensityMap *densityMap;
  Point2 densMapCellSz, densMapSize, densMapOfs;
};
DAG_DECLARE_RELOCATABLE(landclass::GrassDensity);


class landclass::SingleEntityPool
{
public:
  SingleEntityPool() : entPool(midmem), entUsed(0) {}
  SingleEntityPool(const SingleEntityPool &) = delete;
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
DAG_DECLARE_RELOCATABLE(landclass::SingleEntityPool);


class landclass::TiledEntities
{
public:
  TiledEntities() : data(midmem), riOnly(true) {}

public:
  Tab<SingleEntityPlaces> data;
  Point2 tile;
  SmallTab<IDagorEdCustomCollider *, MidmemAlloc> colliders;
  unsigned collFilter;
  float aboveHt;
  bool useYpos;
  bool riOnly;
  bool setSeedToEntities = false;
};


class landclass::PlantedEntities
{
public:
  PlantedEntities() : data(midmem), ent(midmem), dmPool(midmem), riOnly(true) {}
  PlantedEntities(const SingleEntityPool &) = delete;
  ~PlantedEntities() { clear(); }

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
  Tab<DensityMap *> dmPool;
  bool riOnly;
};


class landclass::GrassEntities
{
public:
  GrassEntities() : data(midmem) {}

public:
  Tab<GrassDensity> data;
};


class landclass::ColorImagesData
{
public:
  ColorImagesData() : images(midmem) {}

public:
  struct Image
  {
    SimpleString name, fname;
    int mappingType;
    Point2 tile, offset;
    bool clampU, clampV, flipU, flipV, swapUV;
  };
  Tab<Image> images;
};


class landclass::PolyGeomGenData
{
public:
  PolyGeomGenData() :
    mainUV(midmem),
    borderUV(midmem),
    borderWidth(0),
    borderEdges(1),
    normalsDir(0, 1, 0),
    foundationGeom(true),
    stage(0),
    waterSurface(false),
    waterSurfaceExclude(false)
  {}

public:
  struct Mapping
  {
    enum
    {
      TYPE_PLANAR,
      TYPE_SPLINE
    };

    int chanIdx;
    int type;
    Point2 offset;

    // specific for TYPE_PLANAR
    Point2 size;
    real rotationW;
    bool swapUV;

    // specific for TYPE_SPLINE
    real tileU, sizeV;
    bool autotilelength;
  };

  Point3 normalsDir;
  unsigned flags;
  real borderWidth;
  int borderEdges;
  bool waterSurface, waterSurfaceExclude;
  Tab<Mapping> mainUV, borderUV;
  SimpleString mainMat, borderMat;
  SimpleString layerTag;
  int stage;
  bool foundationGeom;
};
