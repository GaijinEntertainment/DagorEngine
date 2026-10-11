// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "multiPointData.h"

#include <landMesh/landDetile.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_simpleString.h>
#include <util/dag_roHugeHierBitMap2d.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_e3dColor.h>


class IGenLoad;
class RenderableInstanceLodsResource;
class CollisionResource;


namespace rendinst::gen::land
{
typedef RoHugeHierBitMap2d<4, 3> DensityMap;
typedef DensityMap::L2Bmp::L1Bmp DensMapLeaf;

class SingleEntityPlaces;
class SingleGenEntityGroup;

class TiledEntities;
class PlantedEntities;

class AssetData;


static constexpr unsigned HUID_LandClass = 0x03FB59C4u; // LandClass
} // namespace rendinst::gen::land


class rendinst::gen::land::AssetData
{
public:
  TiledEntities *tiled;
  PlantedEntities *planted;

  Tab<RenderableInstanceLodsResource *> riRes;
  Tab<CollisionResource *> riCollRes;
  Tab<E3DCOLOR> riColPair;
  SmallTab<bool, MidmemAlloc> riPosInst;
  SmallTab<bool, MidmemAlloc> riPaletteRotation;
  Tab<char *> riResName;

  int layerIdx;
  volatile int riResResolved;
  const char *loadingAssetFname;
  DataBlock detailData;
  LandDetile detile; // the tile texture's de-tiling; tiled and planted follow it
  land_detile::Cluster detileClusterTable[LAND_DETILE_MAX_CLUSTERS];

  char hash[20];

public:
  AssetData();
  ~AssetData() { clear(); }

  void load(IGenLoad &crd);
  void clear();

  void updatePosInst(dag::ConstSpan<bool> riPosInst);
  void updatePaletteRotation(dag::ConstSpan<bool> riPaletteRotation);

  int getHash(void *buf, int buf_sz)
  {
    if (buf_sz < sizeof(hash))
      return -(int)sizeof(hash);
    memcpy(buf, hash, sizeof(hash));
    return (int)sizeof(hash);
  }

  static AssetData emptyLandStub;

protected:
  void updateRiColor(int entId, E3DCOLOR from, E3DCOLOR to);

  bool loadTiledEntities(const RoDataBlock &blk);
  bool loadPlantedEntities(const RoDataBlock &blk, const OAHashNameMap<true> &dm_nm, dag::ConstSpan<IPoint2> dm_sz);
};


class rendinst::gen::land::SingleEntityPlaces
{
public:
  SingleEntityPlaces() : entityIdx(-1), tm(midmem) {}
  SingleEntityPlaces(SingleEntityPlaces &&) = default;
  ~SingleEntityPlaces() { clear(); }

  void clear() { tm.clear(); }

public:
  enum
  {
    ORIENT_WORLD,
    ORIENT_NORMAL,
    ORIENT_NORMAL_XZ,
    ORIENT_WORLD_XZ
  };

  Tab<TMatrix> tm;
  land_detile::TiledSpots byX; // built only for a de-tiled landclass
  int entityIdx;

  Point2 yOffset;
  unsigned rseed;
  unsigned orientType;
  bool posInst;
  bool paletteRotation;

  rendinst::gen::MpPlacementRec mpRec;
};


class rendinst::gen::land::SingleGenEntityGroup
{
public:
  SingleGenEntityGroup() : obj(midmem), density(0), densityMap(nullptr) {}

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

    rendinst::gen::MpPlacementRec mpRec;
    bool posInst;
    bool paletteRotation;
  };

  float density;
  float objOverlapRatio;
  unsigned rseed;
  float sumWeight;
  Tab<Obj> obj;

  DensityMap *densityMap;
  const land_detile::LeafOccupancy *densityLeaves = nullptr; // only for a de-tiled landclass
  Point2 densMapCellSz, densMapSize, densMapOfs;

  float aboveHt;
};


class rendinst::gen::land::TiledEntities
{
public:
  TiledEntities() : data(midmem) {}

public:
  Tab<SingleEntityPlaces> data;
  Point2 tile;
  float aboveHt;
  bool useYpos;
  const LandDetile *detile = nullptr;                       // the owning AssetData's; null or off = the plain tile grid
  const land_detile::Cluster *detileClusterTable = nullptr; // AssetData's; null = the walk computes its own
};


class rendinst::gen::land::PlantedEntities
{
public:
  PlantedEntities() : data(midmem), dmPool(midmem) {}
  ~PlantedEntities() { clear(); }

  void clear()
  {
    data.clear();
    clear_all_ptr_items(dmPool);
  }

public:
  Tab<SingleGenEntityGroup> data;
  Tab<DensityMap *> dmPool;
  dag::Vector<land_detile::LeafOccupancy> dmLeaves;         // dmPool's leaf bits, only for a de-tiled landclass
  const LandDetile *detile = nullptr;                       // the owning AssetData's, set at load; off = the plain density grid
  const land_detile::Cluster *detileClusterTable = nullptr; // AssetData's; null = the walk computes its own
};
