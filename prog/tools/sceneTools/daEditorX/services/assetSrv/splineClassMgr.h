#pragma once


#include <de3_splineClassData.h>
#include <assets/assetChangeNotify.h>
#include <math/dag_bezierPrec.h>
#include <util/dag_fastStrMap.h>

class DataBlock;
class IAssetUpdateNotify;
class Mesh;


class SharedSplineClassAssetData : public splineclass::AssetData
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
  int assetNameId = -1, nameId = -1;

protected:
  int refCount = 0;

  static bool loadRoadData(splineclass::RoadData &planted, const DataBlock &blk);

  static bool loadGenEntities(splineclass::GenEntities &planted, const DataBlock &blk);
  static bool loadLoftGeomData(splineclass::LoftGeomGenData &genGeom, const DataBlock &blk);

public:
  static const char *loadingAssetFname;
};


class SplineClassAssetMgr : public IDagorAssetChangeNotify
{
public:
  SplineClassAssetMgr() : assets(midmem), assetsUu(midmem), hlist(midmem) {}

  splineclass::AssetData *getAsset(const char *asset_name);
  splineclass::AssetData *addRefAsset(splineclass::AssetData *data);
  void releaseAsset(splineclass::AssetData *data);
  const char *getAssetName(splineclass::AssetData *data);

  void compact();

  void addNotifyClient(IAssetUpdateNotify *n);
  void delNotifyClient(IAssetUpdateNotify *n);

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type);
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type);

  static void build2DSpline(BezierSpline2d &spline, dag::ConstSpan<Point4> spts, bool closed);
  static void createLoftMesh(Mesh &mesh, const splineclass::LoftGeomGenData::Loft &loft, const BezierSplinePrec3d &path,
    int path_subdiv_count, const BezierSplinePrec2d &shape, dag::ConstSpan<Point4> spts, int shape_subdiv_count, bool extrude,
    bool place_on_collision, float above_ht, dag::ConstSpan<splineclass::LoftGeomGenData::Loft::PtAttr> pt_attr, int start_seg,
    int end_seg, float min_step, float max_step, float curvature, float max_h_err, float max_hill_h_err, bool follow_hills,
    bool follow_hollows, float ht_test_step, bool road_bhv, float road_max_abs_ang, float road_max_inter_ang, float road_test_wd,
    float scale_tc_along, int select_mat, dag::ConstSpan<splineclass::Attr> splineScales, float zero_opac_fore_end,
    float zero_opac_back_end, Tab<splineclass::SegData> *out_loftSeg);

protected:
  FastStrMap nameMap;
  Tab<SharedSplineClassAssetData *> assets;
  Tab<int> assetsUu;
  Tab<IAssetUpdateNotify *> hlist;
};
