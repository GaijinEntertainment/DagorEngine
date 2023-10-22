#pragma once


#include <de3_landClassData.h>
#include <assets/assetChangeNotify.h>
#include <util/dag_fastStrMap.h>

class DataBlock;
class IAssetUpdateNotify;


class SharedLandClassAssetData : public landclass::AssetData
{
public:
  int getRefCount() const { return refCount; }
  void addRef() { refCount++; }
  bool delRef()
  {
    refCount--;
    if (refCount <= 0)
    {
      delete this;
      return true;
    }
    return false;
  }

  bool loadAsset(const DataBlock &blk);

  static void clearAsset(AssetData &a);

public:
  SimpleString assetName;
  int assetNameId = -1, nameId = -1;

protected:
  int refCount = 0;

  static bool loadTiledEntities(landclass::TiledEntities &tiled, const DataBlock &blk);
  static bool loadPlantedEntities(landclass::PlantedEntities &planted, const DataBlock &blk);
  static bool loadGrassEntities(landclass::GrassEntities &grass, const DataBlock &blk);
  static bool loadColorImageData(landclass::ColorImagesData &detTex, const DataBlock &blk);
  static bool loadGenGeomData(landclass::PolyGeomGenData &genGeom, const DataBlock &blk);
  static void loadGenGeomDataMapping(landclass::PolyGeomGenData::Mapping &m, const DataBlock &blk);

  static landclass::DensityMap *loadDensityMap(const char *tif_fname, int &w, int &h);
  static bool loadGrassDensity(landclass::GrassDensity &gd, const DataBlock &blk);

public:
  static const char *loadingAssetFname;
};


class LandClassAssetMgr : public IDagorAssetChangeNotify
{
public:
  LandClassAssetMgr() : assets(midmem), assetsUu(midmem), hlist(midmem) {}

  landclass::AssetData *getAsset(const char *asset_name);
  landclass::AssetData *addRefAsset(landclass::AssetData *data);
  void releaseAsset(landclass::AssetData *data);
  const char *getAssetName(landclass::AssetData *data);

  void compact();

  void addNotifyClient(IAssetUpdateNotify *n);
  void delNotifyClient(IAssetUpdateNotify *n);

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type);
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type);

protected:
  FastStrMap nameMap;
  Tab<SharedLandClassAssetData *> assets;
  Tab<int> assetsUu;
  Tab<IAssetUpdateNotify *> hlist;
};
