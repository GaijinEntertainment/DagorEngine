//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bezier.h>
#include <generic/dag_tabFwd.h>

namespace landclass
{
class AssetData;
}
namespace splineclass
{
class AssetData;
}

class IAssetUpdateNotify;
class MaterialData;

// forwards to be used in createLoftMesh()
class Mesh;
namespace splineclass
{
class LoftGeomGenData;
struct Attr;
struct SegData;
} // namespace splineclass


class IAssetService
{
public:
  static constexpr unsigned HUID = 0xC2729EFFu; // IAssetService

  //! returns land class asset data associated with name, or NULL on error; on success, increments ref count for asset
  virtual landclass::AssetData *getLandClassData(const char *asset_name) = 0;
  //! increments ref count for asset (if non-NULL) and returns asset
  virtual landclass::AssetData *addRefLandClassData(landclass::AssetData *data) = 0;
  //! decrements ref count for asset, unloads asset when ref count reaches 0
  virtual void releaseLandClassData(landclass::AssetData *data) = 0;
  //! returns name of asset data (allocated earlier with getLandClassData)
  virtual const char *getLandClassDataName(landclass::AssetData *data) = 0;

  //! returns spline class asset data associated with name, or NULL on error; on success, increments ref count for asset
  virtual splineclass::AssetData *getSplineClassData(const char *asset_name) = 0;
  //! increments ref count for asset (if non-NULL) and returns asset
  virtual splineclass::AssetData *addRefSplineClassData(splineclass::AssetData *data) = 0;
  //! decrements ref count for asset, unloads asset when ref count reaches 0
  virtual void releaseSplineClassData(splineclass::AssetData *data) = 0;
  //! returns name of asset data (allocated earlier with getSplineClassData)
  virtual const char *getSplineClassDataName(splineclass::AssetData *data) = 0;

  //! checks validity of loft before using in createLoftMesh()
  virtual bool isLoftCreatable(const splineclass::LoftGeomGenData *data, int loft_idx) = 0;
  //! generates loft geometry using spline class asset data and path
  virtual bool createLoftMesh(Mesh &mesh, const splineclass::LoftGeomGenData *data, int loft_idx, BezierSpline3d &path, int start_seg,
    int end_seg, bool place_on_collision, float scale_tc_along, int select_mat, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<splineclass::SegData> *out_loftSeg, const char *asset_name, float zero_opac_fore_end, float zero_opac_back_end) = 0;

  //! creates and returns MaterialData built from material asset
  virtual MaterialData *getMaterialData(const char *asset_name) = 0;

  virtual void subscribeUpdateNotify(IAssetUpdateNotify *notify, bool lndcls, bool splcls) = 0;
  virtual void unsubscribeUpdateNotify(IAssetUpdateNotify *notify, bool lndcls, bool splcls) = 0;
};


class IAssetUpdateNotify
{
public:
  virtual void onLandClassAssetChanged(landclass::AssetData *data) = 0;
  virtual void onLandClassAssetTexturesChanged(landclass::AssetData *data) = 0;
  virtual void onSplineClassAssetChanged(splineclass::AssetData *data) = 0;
};
