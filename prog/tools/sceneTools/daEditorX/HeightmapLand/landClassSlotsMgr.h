// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_hierGrid.h>
#include <de3_rendInstGen.h>
#include <util/dag_simpleString.h>
#include <util/dag_stdint.h>

namespace landclass
{
class AssetData;
class SingleEntityPool;
} // namespace landclass

template <class T>
class MapStorage;
struct HmapBitLayerDesc;
class IAssetUpdateNotify;
class IObjEntity;


class LandClassSlotsManager
{
  class TiledEntityHolder
  {
  public:
    struct Tile
    {
      enum
      {
        TILED,
        PLANTED,
        POOL_COUNT
      };

      // manually managed pool arrays
      dag::Span<landclass::SingleEntityPool> pool[POOL_COUNT];
      SmallTab<IObjEntity *, MidmemAlloc> grass;
      bool discarded;

      Tile() : discarded(true) {}
      ~Tile()
      {
        clear(TILED);
        clear(PLANTED);
        clearGrass();
      }

      void clear(int pool_type);
      void clearGrass();
      void resize(int pool_type, int count);

      void beginGenerate(int pool_type);
      void endGenerate(int pool_type);

      void clearObjects(int pool_type)
      {
        beginGenerate(pool_type);
        endGenerate(pool_type);
      }
      void clearAllObjects()
      {
        clearObjects(TILED);
        clearObjects(PLANTED);
        clearGrass();
      }

      bool isPoolEmpty(int pool_type);
    };

    HierGrid2<Tile, 0> tiles;

  public:
    TiledEntityHolder();

    void reset() { tiles.clear(); }
    Tile *getTile(int tx, int ty) const { return tiles.get_leaf(tx, ty); }
    Tile &createTile(int tx, int ty) { return *tiles.create_leaf(tx, ty); }
  };

  struct PerLayer
  {
    struct Rec
    {
      landclass::AssetData *landClass;
      rendinst::gen::land::AssetData *rigenLand;
      EditableHugeBitMap2d *rigenBm;
      SimpleString name;
      TiledEntityHolder entHolder;
      int tmpBmpIdx;
      unsigned changed : 1, grassLayer : 1;

      Rec(const char *nm) : name(nm), landClass(NULL), tmpBmpIdx(-1), changed(true), rigenLand(NULL), rigenBm(NULL) {}
      ~Rec() { clearData(); }

      void reload();
      void clearData();
    };

    Tab<Rec *> cls;

    PerLayer() : cls(midmem) {}
    ~PerLayer() { clear_all_ptr_items(cls); }
  };


  SmallTab<PerLayer *, MidmemAlloc> layers;
  float grid2world, world0x, world0y;
  int tileSize;
  int outer_border = 0;
  dag::ConstSpan<HmapBitLayerDesc> landClsLayer;

public:
  LandClassSlotsManager(int layer_count);
  ~LandClassSlotsManager() { clear(); }

  landclass::AssetData *setLandClass(int layer_idx, int landclass_idx, const char *asset_name);
  landclass::AssetData *getLandClass(int layer_idx, int landclass_idx);
  void reloadLandClass(int layer_idx, int landclass_idx);

  void reinitRIGen(bool reset_lc_cache = false);

  void onLandSizeChanged(float grid_step, float ofs_x, float ofs_y, dag::ConstSpan<HmapBitLayerDesc> land_cls_layer);

  void onLandRegionChanged(int x0, int y0, int x1, int y1, MapStorage<uint32_t> &landClsMap, bool all_classes = true,
    bool dont_regenerate = false, bool finished = true);

  void regenerateObjects(MapStorage<uint32_t> &landClsMap, bool only_changed = true);

  static landclass::AssetData *getAsset(const char *asset_name);
  static void releaseAsset(landclass::AssetData *asset);

  static void subscribeLandClassUpdateNotify(IAssetUpdateNotify *n);
  static void unsubscribeLandClassUpdateNotify(IAssetUpdateNotify *n);

  void clear();

  void exportEntityGenDataToFile(MapStorage<uint32_t> &land_cls_map, unsigned target_code);
  void exportEntityGenDataLayer(MapStorage<uint32_t> &land_cls_map, unsigned target_code, const DataBlock &appBlk, const char *base,
    DataBlock &fileBlk, int layer_idx);

protected:
  static void __stdcall LandClassSlotsManager::reinitRIGenCallBack(void *arg)
  {
    reinterpret_cast<LandClassSlotsManager *>(arg)->reinitRIGen(true);
  }
};
