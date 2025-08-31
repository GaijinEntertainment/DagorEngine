// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_assetService.h>
#include <de3_landClassData.h>
#include <de3_interface.h>
#include <assets/asset.h>
#include "landClassMgr.h"
#include "splineClassMgr.h"
#include <winGuiWrapper/wgw_dialogs.h>
#include <3d/dag_materialData.h>
#include <math/dag_Point4.h>
#include <math/dag_mesh.h>

static LandClassAssetMgr lcaMgr;
static SplineClassAssetMgr scaMgr;

class GenericAssetService : public IAssetService
{
public:
  landclass::AssetData *getLandClassData(const char *asset_name) override { return lcaMgr.getAsset(asset_name); }
  landclass::AssetData *addRefLandClassData(landclass::AssetData *data) override { return data ? lcaMgr.addRefAsset(data) : NULL; }
  void releaseLandClassData(landclass::AssetData *data) override { lcaMgr.releaseAsset(data); }
  const char *getLandClassDataName(landclass::AssetData *data) override { return lcaMgr.getAssetName(data); }

  splineclass::AssetData *getSplineClassData(const char *asset_name) override { return scaMgr.getAsset(asset_name); }
  splineclass::AssetData *addRefSplineClassData(splineclass::AssetData *data) override
  {
    return data ? scaMgr.addRefAsset(data) : NULL;
  }
  void releaseSplineClassData(splineclass::AssetData *data) override { scaMgr.releaseAsset(data); }
  const char *getSplineClassDataName(splineclass::AssetData *data) override { return scaMgr.getAssetName(data); }

  bool isLoftCreatable(const splineclass::LoftGeomGenData *genGeom, int loft_idx) override
  {
    if (!genGeom || loft_idx < 0 || loft_idx >= genGeom->loft.size())
      return false;
    const splineclass::LoftGeomGenData::Loft &loft = genGeom->loft[loft_idx];
    if (loft.shapePts.size() < 2)
      return false;
    return true;
  }
  bool createLoftMesh(Mesh &mesh, const splineclass::LoftGeomGenData *genGeom, int loft_idx, BezierSpline3d &path, int start_seg,
    int end_seg, bool place_on_collision, float scale_tc_along, int select_mat, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<splineclass::SegData> *out_loftSeg, const char *asset_name, float zero_opac_fore_end, float zero_opac_back_end,
    float path_start_margin, float path_end_margin) override
  {
    if (!genGeom || loft_idx < 0 || loft_idx >= genGeom->loft.size())
      return false;
    const splineclass::LoftGeomGenData::Loft &loft = genGeom->loft[loft_idx];
    if (loft.shapePts.size() < 2)
      return false;

    BezierSpline2d shape;
    SplineClassAssetMgr::build2DSpline(shape, loft.shapePts, loft.closed);
    for (int i = 0; i < shape.segs.size(); ++i)
      shape.segs[i].sk[0] += loft.offset;

    SplineClassAssetMgr::createLoftMesh(mesh, loft, ::toPrecSpline(path), loft.subdivCount, ::toPrecSpline(shape), loft.shapePts,
      loft.shapeSubdivCount, loft.extrude, place_on_collision, loft.aboveHt, loft.shapePtsAttr, start_seg, end_seg, loft.minStep,
      loft.maxStep, loft.curvatureStrength, loft.maxHerr, loft.maxHillHerr, loft.followHills, loft.followHollows, loft.htTestStep,
      loft.roadBhv, loft.roadMaxAbsAng, loft.roadMaxInterAng, loft.roadTestWidth, scale_tc_along, select_mat, splineScales,
      zero_opac_fore_end, zero_opac_back_end, path_start_margin, path_end_margin, out_loftSeg);

    if (
      mesh.face.size() != mesh.tface[0].size() || mesh.face.size() != mesh.tface[1].size() || mesh.face.size() != mesh.tface[2].size())
    {
      DAEDITOR3.conError("<%s>: invalid mesh", asset_name);
      return false;
    }

    for (int i = 0; i < mesh.vert.size(); ++i)
      if (check_nan(mesh.vert[i].x) || check_nan(mesh.vert[i].y) || check_nan(mesh.vert[i].z))
      {
        wingw::message_box(wingw::MBS_EXCL, "Internal spline error", "\"%s\": loft object's geometry is invalid.", asset_name);
        return false;
      }

    return true;
  }

  MaterialData *getMaterialData(const char *asset_name) override
  {
    const char *physmat_suffix = strrchr(asset_name, '@');
    DagorAsset *a = DAEDITOR3.getAssetByName(physmat_suffix ? String::mk_sub_str(asset_name, physmat_suffix).c_str() : asset_name,
      DAEDITOR3.getAssetTypeId("mat"));
    if (!a)
      physmat_suffix = nullptr, a = DAEDITOR3.getAssetByName(asset_name, DAEDITOR3.getAssetTypeId("mat"));
    if (!a)
      return NULL;

    MaterialData *md = new (tmpmem) MaterialData;

    md->className = a->props.getStr("class_name", "simple");
    md->matScript = a->props.getStr("script", "");
    if (physmat_suffix)
      md->matName = String::mk_str_cat(a->getName(), physmat_suffix).c_str();
    else
      md->matName = a->getName();
    md->flags = a->props.getInt("flags", 0);
    md->mat.power = a->props.getReal("power", 0);
    md->mat.diff.set_xyzw(a->props.getPoint4("diff", Point4(1, 1, 1, 1)));
    md->mat.amb.set_xyzw(a->props.getPoint4("amb", Point4(1, 1, 1, 1)));
    md->mat.spec.set_xyzw(a->props.getPoint4("spec", Point4(1, 1, 1, 1)));
    md->mat.emis.set_xyzw(a->props.getPoint4("emis", Point4(0, 0, 0, 0)));

    const DataBlock *blk;

    Tab<SimpleString> textures(tmpmem);

    blk = a->props.getBlockByNameEx("textures");

    String texBuf;
    for (int i = 0; i < MAXMATTEXNUM; i++)
    {
      texBuf.printf(16, "tex%d", i);
      const char *texName = blk->getStr(texBuf, NULL);
      if (!texName)
        continue;

      while (textures.size() < i)
        textures.push_back();

      textures.push_back() = DAEDITOR3.resolveTexAsset(texName);
      if (textures.back().empty())
        DAEDITOR3.conError("can't find tex[%d] <%s> for mat <%s>", i, texName, asset_name);
    }

    memset(md->mtex, 0xFF, sizeof(md->mtex));
    for (int ti = 0; ti < MAXMATTEXNUM; ++ti)
    {
      bool badTex = false;

      if (ti < textures.size() && textures[ti])
        md->mtex[ti] = ::add_managed_texture(textures[ti]);

      if (badTex)
        md->mtex[ti] = BAD_TEXTUREID;
    }

    return md;
  }

  void subscribeUpdateNotify(IAssetUpdateNotify *notify, bool lndcls, bool splcls) override
  {
    if (lndcls)
      lcaMgr.addNotifyClient(notify);
    if (splcls)
      scaMgr.addNotifyClient(notify);
  }
  void unsubscribeUpdateNotify(IAssetUpdateNotify *notify, bool lndcls, bool splcls) override
  {
    if (lndcls)
      lcaMgr.delNotifyClient(notify);
    if (splcls)
      scaMgr.delNotifyClient(notify);
  }
};

static GenericAssetService srv;

void *get_generic_asset_service() { return &srv; }
