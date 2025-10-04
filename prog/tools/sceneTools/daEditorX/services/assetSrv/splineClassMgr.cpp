// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineClassMgr.h"
#include "flags.h"
#include <de3_bitMaskMgr.h>
#include <de3_assetService.h>
#include <de3_colorRangeService.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <oldEditor/de_interface.h>
#include <libTools/util/strUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_hierGrid.h>
#include <generic/dag_carray.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <EASTL/vector_set.h>

extern const char *daeditor3_get_appblk_fname();

class StubSpacerObjEntity : public IObjEntity
{
public:
  StubSpacerObjEntity() : IObjEntity(1000000) {}

  void setTm(const TMatrix &) override {}
  void getTm(TMatrix &tm) const override { tm.identity(); }
  void destroy() override {}

  void *queryInterfacePtr(unsigned /*huid*/) override { return nullptr; }
  BSphere3 getBsph() const override { return BSphere3(Point3(0, 1e6, 0), 1); }
  BBox3 getBbox() const override { return BBox3(Point3(0, 1e6, 0), Point3(0, 1e6, 0)); }

  inline int getAssetTypeId() const { return entityClass; }
  const char *getObjAssetName() const override { return " "; }
};
static StubSpacerObjEntity stubEntity;

splineclass::AssetData::AssetData() : road(NULL), gen(NULL), genGeom(NULL), isCustomJumplink(false)
{
  lmeshHtConstrSweepWidth = sweepWidth = 0.0;
  addFuzzySweepHalfWidth = 0.0;
  sweep2Width = addFuzzySweep2HalfWidth = 0.0;
  navMeshStripeWidth = 0;
}
splineclass::AssetData::~AssetData() { SharedSplineClassAssetData::clearAsset(*this); }

void SharedSplineClassAssetData::clearAsset(splineclass::AssetData &a)
{
  del_it(a.road);
  del_it(a.gen);
  del_it(a.genGeom);
}


splineclass::AssetData *SplineClassAssetMgr::getAsset(const char *asset_name)
{
  int id = nameMap.getStrId(asset_name);

  if (id == -1)
  {
    if (!assetsUu.size())
    {
      id = assets.size();
      assets.push_back(nullptr);
    }
    else
    {
      id = assetsUu.back();
      assetsUu.pop_back();
    }
    G_VERIFY(nameMap.addStrId(asset_name, id) == id);
  }

  if (assets[id])
  {
    assets[id]->addRef();
    return assets[id];
  }

  int asset_type = DAEDITOR3.getAssetTypeId("spline");
  DagorAsset *asset = DAEDITOR3.getAssetByName(asset_name, asset_type);
  SharedSplineClassAssetData *scad = new (midmem) SharedSplineClassAssetData;

  if (asset)
  {
    SharedSplineClassAssetData::loadingAssetFname = asset_name;
    if (scad->loadAsset(asset->props))
      ; // asset loaded normally
    else
      DAEDITOR3.conError("errors while loading asset: %s", asset_name);
    SharedSplineClassAssetData::loadingAssetFname = NULL;
    scad->assetNameId = asset->getNameId();
    asset->getMgr().subscribeUpdateNotify(this, scad->assetNameId, asset_type);
  }
  else
    DAEDITOR3.conError("can't find spline asset: %s", asset_name);

  assets[id] = scad;
  assets[id]->nameId = id;
  scad->addRef();
  return scad;
}

splineclass::AssetData *SplineClassAssetMgr::addRefAsset(splineclass::AssetData *data)
{
  for (int i = assets.size() - 1; i >= 0; i--)
    if (assets[i] == data)
    {
      assets[i]->addRef();
      return data;
    }
  return NULL;
}

void SplineClassAssetMgr::releaseAsset(splineclass::AssetData *data)
{
  for (int i = assets.size() - 1; i >= 0; i--)
    if (assets[i] == data)
    {
      if (assets[i]->delRef())
      {
        assets[i] = NULL;
        assetsUu.push_back(i);
        nameMap.delStrId(i);
      }
      return;
    }
}
const char *SplineClassAssetMgr::getAssetName(splineclass::AssetData *data)
{
  if (!data)
    return nullptr;
  int id = static_cast<SharedSplineClassAssetData *>(data)->nameId;
  if (id < 0 || id >= nameMap.strCount() || assets[id] != data)
    return nullptr;
  return nameMap.getStrSlow(id);
}

void SplineClassAssetMgr::compact() {}

void SplineClassAssetMgr::onAssetRemoved(int asset_name_id, int asset_type)
{
  if (asset_type != DAEDITOR3.getAssetTypeId("spline"))
    return;

  for (int i = 0; i < assets.size(); i++)
    if (assets[i] && assets[i]->assetNameId == asset_name_id)
    {
      SharedSplineClassAssetData::clearAsset(*assets[i]);

      // notify clients
      for (int j = 0; j < hlist.size(); j++)
        hlist[j]->onSplineClassAssetChanged(assets[i]);
      return;
    }
}
void SplineClassAssetMgr::onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
{
  if (asset_type != DAEDITOR3.getAssetTypeId("spline"))
    return;

  for (int i = 0; i < assets.size(); i++)
    if (assets[i] && assets[i]->assetNameId == asset_name_id)
    {
      SharedSplineClassAssetData::clearAsset(*assets[i]);

      // reload asset data
      SharedSplineClassAssetData::loadingAssetFname = asset.getName();
      if (assets[i]->loadAsset(asset.props))
        ; // asset loaded normally
      else
        DAEDITOR3.conError("errors while re-loading asset: %s", asset.getName());
      SharedSplineClassAssetData::loadingAssetFname = NULL;

      // notify clients
      for (int j = 0; j < hlist.size(); j++)
        hlist[j]->onSplineClassAssetChanged(assets[i]);
      return;
    }
}

void SplineClassAssetMgr::addNotifyClient(IAssetUpdateNotify *n)
{
  for (int i = 0; i < hlist.size(); i++)
    if (hlist[i] == n)
      return;
  hlist.push_back(n);
}

void SplineClassAssetMgr::delNotifyClient(IAssetUpdateNotify *n)
{
  for (int i = 0; i < hlist.size(); i++)
    if (hlist[i] == n)
    {
      erase_items(hlist, i, 1);
      return;
    }
}

bool SharedSplineClassAssetData::loadAsset(const DataBlock &blk)
{
  const DataBlock *b;
  bool ok = true;

  b = blk.getBlockByName("road");
  if (b)
  {
    road = new (midmem) splineclass::RoadData;
    if (!loadRoadData(*road, *b))
    {
      delete road;
      road = NULL;

      ok = false;
    }
  }

  if (blk.getBlockByName("obj_generate"))
  {
    gen = new (midmem) splineclass::GenEntities;
    if (!loadGenEntities(*gen, blk))
    {
      delete gen;
      gen = NULL;

      ok = false;
    }
  }

  if (blk.getBlockByName("loft"))
  {
    genGeom = new (midmem) splineclass::LoftGeomGenData;
    if (!loadLoftGeomData(*genGeom, blk))
    {
      delete genGeom;
      genGeom = NULL;

      ok = false;
    }
  }

  sweepWidth = blk.getReal("sweepWidth", road ? road->linesCount * road->lineWidth : 0);
  addFuzzySweepHalfWidth = blk.getReal("addFuzzySweepHalfWidth", 0);
  lmeshHtConstrSweepWidth = blk.getReal("lmeshHtConstraintSweepWidth", sweepWidth * 1.1);
  sweep2Width = blk.getReal("sweep2Width", 0);
  addFuzzySweep2HalfWidth = blk.getReal("addFuzzySweep2HalfWidth", 0);
  navMeshStripeWidth = blk.getReal("navMeshStripeWidth", 0);
  isCustomJumplink = blk.getBool("isCustomJumplink", isCustomJumplink);
  return ok;
}

bool SharedSplineClassAssetData::loadRoadData(splineclass::RoadData &road, const DataBlock &blk)
{
  if (!inited)
    init();
#define LOAD_MAT(name)       road.mat.name = blk.getStr(#name, #name)
#define LOAD_FLG(name)       road.name = blk.getBool(#name, false)
#define LOAD_FLG2(name, def) road.name = blk.getBool(#name, def)
  LOAD_MAT(road);
  LOAD_MAT(sides);
  LOAD_MAT(vertWalls);
  LOAD_MAT(centerBorder);
  LOAD_MAT(hammerSides);
  LOAD_MAT(hammerCenter);
  LOAD_MAT(paintings);
  LOAD_MAT(crossRoad);

  bool sides = blk.getBool("hasSides", false);
  bool side_borders = blk.getBool("hasSideBorders", false);
  bool walls = blk.getBool("hasVertWalls", false);

  LOAD_FLG2(hasLeftSide, sides);
  LOAD_FLG2(hasRightSide, sides);
  LOAD_FLG2(hasLeftSideBorder, side_borders);
  LOAD_FLG2(hasRightSideBorder, side_borders);
  LOAD_FLG2(hasLeftVertWall, walls);
  LOAD_FLG2(hasRightVertWall, walls);
  LOAD_FLG(hasCenterBorder);
  LOAD_FLG(hasHammerSides);
  LOAD_FLG(hasHammerCenter);
  LOAD_FLG(hasPaintings);
  LOAD_FLG(hasStopLine);
  LOAD_FLG(isBridge);
  LOAD_FLG2(isBridgeSupport, true);
  LOAD_FLG(isCustomBridge);
  LOAD_FLG(isBoundingLink);
  LOAD_FLG(flipRoadUV);
  LOAD_FLG(flipCrossRoadUV);
  LOAD_FLG2(buildGeom, true);
  LOAD_FLG2(buildRoadMap, true);
#undef LOAD_FLG2
#undef LOAD_FLG
#undef LOAD_MAT

  road.linesCount = blk.getInt("linesCount", 2);
  road.lineWidth = blk.getReal("lineWidth", 3.0);
  road.crossTurnRadius = blk.getReal("crossTurnRadius", 10.0);
  road.roadRankId = blk.getInt("roadRank", 0);

  road.useDefaults = blk.getBool("useDefaults", false);
  road.sideSlope = blk.getReal("sideScale", 0.5);
  road.wallSlope = blk.getReal("wallScale", 0.8);
  road.curvatureStrength = blk.getReal("curvatureStrength", 15.0);
  road.minStep = blk.getReal("minStep", 3.0);
  if (road.minStep < splineMinStep)
  {
    DAEDITOR3.conError("minStep %.1f < %.1f in road asset <%s>, forced minimal value", road.minStep, splineMinStep, loadingAssetFname);
    road.minStep = splineMinStep;
  }
  road.maxStep = blk.getReal("maxStep", 25.0);

  road.leftWallHeight = blk.getReal("leftWallHeight", -10.0);
  road.rightWallHeight = blk.getReal("rightWallHeight", -10.0);

  road.centerBorderHeight = blk.getReal("centerBorderHeight", 0.1);
  road.leftBorderHeight = blk.getReal("leftBorderHeight", 0.2);
  road.rightBorderHeight = blk.getReal("rightBorderHeight", 0.2);

  road.leftHammerYOffs = blk.getReal("leftHammerYOffs", 0.4);
  road.leftHammerHeight = blk.getReal("leftHammerHeight", 0.2);
  road.rightHammerYOffs = blk.getReal("rightHammerYOffs", 0.4);
  road.rightHammerHeight = blk.getReal("rightHammerHeight", 0.2);
  road.centerHammerYOffs = blk.getReal("centerHammerYOffs", 0.4);
  road.centerHammerHeight = blk.getReal("centerHammerHeight", 0.2);
  road.bridgeRoadThickness = blk.getReal("bridgeRoadThickness", 1.0);
  road.bridgeStandHeight = blk.getReal("bridgeStandHeight", -10.0);

  road.walkablePartVScale = blk.getReal("walkablePartVScale", 10.0);
  road.walkablePartUScale = blk.getReal("walkablePartUScale", -1.0);

  road.crossRoadVScale = blk.getReal("crossRoadVScale", 0.01);
  road.crossRoadUScale = blk.getReal("crossRoadUScale", 0.01);

  road.maxUpdirRot = blk.getReal("maxUpdirRot", 45) * DEG_TO_RAD;
  road.minUpdirRot = blk.getReal("minUpdirRot", 0) * DEG_TO_RAD;
  road.pointRtoUpdirRot = blk.getReal("pointRtoUpdirRot", 20) * DEG_TO_RAD;
  road.maxPointR = blk.getReal("maxPointR", 1);

  for (int i = 0; i < road.MAX_SEP_LINES; i++)
    road.sepLineType[i] = blk.getInt(String(32, "sep%d", i), 0);


  int nid = blk.getNameId("lamp"), lampId = 0;
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
      lampId++;

  clear_and_resize(road.lamps, lampId);
  mem_set_0(road.lamps);

  lampId = 0;
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *blk.getBlock(i);
      splineclass::RoadData::Lamp &lamp = road.lamps[lampId++];

      if (lampId > road.lamps.size())
        break;

      const char *resname = cb.getStr("name", NULL);

      DagorAsset *a = resname ? DAEDITOR3.getGenObjAssetByName(resname) : NULL;
      lamp.entity = a ? DAEDITOR3.createEntity(*a, true) : NULL;
      if (!lamp.entity)
      {
        DAEDITOR3.conError("invalid lamp object '%s' in %s", resname, loadingAssetFname);
        lamp.entity = DAEDITOR3.createInvalidEntity(true);
      }

      if (lamp.entity)
        lamp.entity->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("road_obj"));

      lamp.roadOffset = cb.getReal("roadOffset", 0);
      lamp.eachSize = cb.getReal("eachSize", 0);
      lamp.additionalHeightOffset = cb.getReal("additionalHeightOffset", 0);
      lamp.placementType = cb.getInt("placementType", 0);
      lamp.rotateToRoad = cb.getBool("rotateToRoad", false);
    }

#define LOAD_SHAPE(x)                                          \
  {                                                            \
    const DataBlock *shapeBlk = blk.getBlockByName(#x);        \
    if (shapeBlk)                                              \
    {                                                          \
      road.x = new splineclass::RoadData::RoadShapeRec;        \
      road.x->isClosed = shapeBlk->getBool("isClosed", false); \
      road.x->uScale = shapeBlk->getReal("uScale", 1);         \
      road.x->vScale = shapeBlk->getReal("vScale", 1);         \
      int nid = shapeBlk->getNameId("point");                  \
      for (int i = 0; i < shapeBlk->blockCount(); i++)         \
        if (shapeBlk->getBlock(i)->getBlockNameId() == nid)    \
        {                                                      \
          const DataBlock *db = shapeBlk->getBlock(i);         \
          splineclass::RoadData::RoadShapeRec::ShapePoint p;   \
          p.soft = db->getBool("soft", false);                 \
          p.pos = db->getPoint2("pos", Point2(0, 0));          \
          road.x->points.push_back(p);                         \
        }                                                      \
    }                                                          \
  }

  LOAD_SHAPE(borderShape);
  LOAD_SHAPE(sideShape);
  LOAD_SHAPE(roadShape);
  LOAD_SHAPE(sideHammerShape);
  LOAD_SHAPE(centerHammerShape);
  LOAD_SHAPE(centerBorderShape);
  LOAD_SHAPE(wallShape);
  LOAD_SHAPE(bridgeShape);

#undef LOAD_SHAPE

  DAEDITOR3.conNote("loaded road from asset: %s", loadingAssetFname);
  return true;
}

static void loadCablesAttachmentPoints(splineclass::SingleGenEntityGroup::GenEntityRec::CablePoints &cable_points,
  const DataBlock &parent_node, eastl::vector_set<const DagorAsset *> &visited_assets, const TMatrix &parent_tm = TMatrix::IDENT)
{
  int node_id = parent_node.getNameId("node");
  for (int i = 0, n = parent_node.blockCount(); i < n; ++i)
  {
    const DataBlock &b = *parent_node.getBlock(i);
    if (b.getBlockNameId() != node_id)
      continue;
    const char *res = b.getStr("name", NULL);
    if (res && *res) // skip unnamed generation (it is used to skip object in generation step)
    {
      TMatrix tm = b.getTm("tm", TMatrix::IDENT);
      tm = parent_tm * tm;
      if (String("wire_pivot_in") == res)
        cable_points.in.push_back(tm.getcol(3));
      else if (String("wire_pivot_out") == res)
        cable_points.out.push_back(tm.getcol(3));
      else if (DagorAsset *a = DAEDITOR3.getGenObjAssetByName(res))
        if (visited_assets.find(a) == visited_assets.end()) // to avoid cyclic
        {
          visited_assets.insert(a);
          loadCablesAttachmentPoints(cable_points, a->props, visited_assets, tm);
          visited_assets.erase(a);
        }
    }
    if (b.blockCount())
      loadCablesAttachmentPoints(cable_points, b, visited_assets, parent_tm);
  }
}

bool SharedSplineClassAssetData::loadGenEntities(splineclass::GenEntities &gen, const DataBlock &blk)
{
  IColorRangeService *colRangeSrv = EDITORCORE->queryEditorInterface<IColorRangeService>();
  int objGenNid = blk.getNameId("obj_generate");
  float startPadding = blk.getReal("startPadding", 0);
  float endPadding = blk.getReal("endPadding", 0);
  bool useLoftSegs = blk.getBool("useLoftSegs", false);
  bool checkSweep1 = blk.getBool("checkLandSweepMask", false);
  bool checkSweep2 = blk.getBool("checkSplineSweepMask", true);
  float minGenHt = blk.getReal("minGenHt", -1e6);
  float maxGenHt = blk.getReal("maxGenHt", +1e6);

  for (int blki = 0; blki < blk.blockCount(); ++blki)
    if (blk.getBlock(blki)->getBlockNameId() == objGenNid)
    {
      const DataBlock &gen_b = *blk.getBlock(blki);
      splineclass::SingleGenEntityGroup &sgeg = gen.data.push_back();

      if (DAGORED2)
        sgeg.colliders = DAGORED2->loadColliders(gen_b, sgeg.collFilter);
      sgeg.aboveHt = gen_b.getReal("placeAboveHt", 1.0);
      sgeg.rseed = gen_b.getInt("rseed", 123);
      sgeg.step = gen_b.getPoint2("step", Point2(4, 0));
      sgeg.offset = gen_b.getPoint2("offset", Point2(0, 0));
      sgeg.cableRadius = gen_b.getPoint2("cableRadius", Point2(0.01, 0));
      sgeg.cableSag = gen_b.getPoint2("cableSag", Point2(0.5, 0.3));
      sgeg.cableRaggedDistribution = gen_b.getPoint2("cableRaggedDistribution", Point2(0.0, 0.0));
      sgeg.placeAtPoint = gen_b.getBool("placeAtPoint", false);
      sgeg.placeAtStart = gen_b.getBool("placeAtStart", sgeg.placeAtPoint);
      sgeg.placeAtEnd = gen_b.getBool("placeAtEnd", sgeg.placeAtPoint);
      sgeg.placeAtVeryStart = gen_b.getBool("placeAtVeryStart", false);
      sgeg.placeAtVeryEnd = gen_b.getBool("placeAtVeryEnd", false);
      sgeg.checkSweep1 = gen_b.getBool("checkLandSweepMask", checkSweep1);
      sgeg.checkSweep2 = gen_b.getBool("checkSplineSweepMask", checkSweep2);
      sgeg.startPadding = gen_b.getReal("startPadding", startPadding);
      sgeg.endPadding = gen_b.getReal("endPadding", endPadding);
      sgeg.useLoftSegs = gen_b.getBool("useLoftSegs", useLoftSegs);
      sgeg.minGenHt = gen_b.getReal("minGenHt", minGenHt);
      sgeg.maxGenHt = gen_b.getReal("maxGenHt", maxGenHt);
      sgeg.tightFenceOrient = gen_b.getBool("tightFenceOrient", false);
      sgeg.tightFenceIntegral = sgeg.tightFenceIntegralPerSegment = sgeg.tightFenceIntegralPerCorner = 0;
      sgeg.integralLastEntCount = 0;
      if (const char *type = gen_b.getStr("tightFenceIntegral", NULL))
      {
        sgeg.tightFenceIntegral = 1;
        if (strcmp(type, "segment") == 0)
          sgeg.tightFenceIntegralPerSegment = 1;
        else if (strcmp(type, "corner") == 0)
          sgeg.tightFenceIntegralPerCorner = 1;
        else if (strcmp(type, "spline") == 0)
          sgeg.tightFenceIntegralPerSegment = 0;
        else
          DAEDITOR3.conError("asset %s: unknown tightFenceIntegral %s", loadingAssetFname, type);
      }

      sgeg.colorRangeIdx = colRangeSrv ? colRangeSrv->addColorRange(gen_b.getE3dcolor("colorFrom", 0x80808080U),
                                           gen_b.getE3dcolor("colorTo", 0x80808080U))
                                       : IColorRangeService::IDX_GRAY;
      sgeg.setSeedToEntities = gen_b.getBool("setSeedToEntities", blk.getBool("setSeedToEntities", false));

      sgeg.sumWeight = 0;

      int next_id = 0;
      carray<signed char, 256> tag_to_id;
      mem_set_ff(tag_to_id);

      const char *pattern = gen_b.getStr("genTagSeq", NULL);
      if (pattern && *pattern)
      {
        clear_and_resize(sgeg.genTagSeq, (int)strlen(pattern));
        for (int i = 0; i < sgeg.genTagSeq.size(); i++)
        {
          if (tag_to_id[pattern[i]] == -1)
          {
            tag_to_id[pattern[i]] = next_id;
            next_id++;
          }
          sgeg.genTagSeq[i] = tag_to_id[pattern[i]];
        }
      }
      if (const char *tags = gen_b.getStr("genTagFirst", NULL))
      {
        clear_and_resize(sgeg.genTagFirst, (int)strlen(tags));
        for (int i = 0; i < sgeg.genTagFirst.size(); i++)
        {
          if (tag_to_id[tags[i]] == -1)
          {
            tag_to_id[tags[i]] = next_id;
            next_id++;
          }
          sgeg.genTagFirst[i] = tag_to_id[tags[i]];
        }
      }
      if (const char *tags = gen_b.getStr("genTagLast", NULL))
      {
        clear_and_resize(sgeg.genTagLast, (int)strlen(tags));
        for (int i = 0; i < sgeg.genTagLast.size(); i++)
        {
          if (tag_to_id[tags[i]] == -1)
          {
            tag_to_id[tags[i]] = next_id;
            next_id++;
          }
          sgeg.genTagLast[i] = tag_to_id[tags[i]];
        }
      }

      const char *orient_str = gen_b.getStr("orientation", "fence");
      if (stricmp(orient_str, "spline") == 0)
        sgeg.orientType = sgeg.ORIENT_SPLINE;
      else if (stricmp(orient_str, "spline_up") == 0)
        sgeg.orientType = sgeg.ORIENT_SPLINE_UP;
      else if (stricmp(orient_str, "fence") == 0)
        sgeg.orientType = sgeg.ORIENT_FENCE;
      else if (stricmp(orient_str, "fence_normal") == 0)
        sgeg.orientType = sgeg.ORIENT_FENCE_NORM;
      else if (stricmp(orient_str, "world") == 0)
        sgeg.orientType = sgeg.ORIENT_WORLD;
      else if (stricmp(orient_str, "world_xz") == 0)
        sgeg.orientType = sgeg.ORIENT_WORLD_XZ;
      else if (stricmp(orient_str, "normal") == 0)
        sgeg.orientType = sgeg.ORIENT_NORMAL;
      else if (stricmp(orient_str, "normal_xz") == 0)
        sgeg.orientType = sgeg.ORIENT_NORMAL_XZ;
      else
      {
        DAEDITOR3.conError("asset %s: unknown orient %s", loadingAssetFname, orient_str);
        sgeg.orientType = sgeg.ORIENT_SPLINE;
      }


      int objGenRecNid = gen_b.getNameId("object");
      int objCnt = 0;
      bool tightFenceIntegralEndItems = false;

      for (int blkri = 0; blkri < gen_b.blockCount(); ++blkri)
        if (gen_b.getBlock(blkri)->getBlockNameId() == objGenRecNid)
        {
          const DataBlock &gen_rec_b = *gen_b.getBlock(blkri);
          const char *res = gen_rec_b.getStr("name", NULL);

          if (!res)
            continue;

          DagorAsset *a = DAEDITOR3.getGenObjAssetByName(res);
          IObjEntity *entity = a ? DAEDITOR3.createEntity(*a, true) : NULL;
          if (!entity && res && !*res) // skip empty name (used to generate blanks)
            entity = &stubEntity;
          if (!entity)
          {
            DAEDITOR3.conError("invalid object '%s' in %s", res, loadingAssetFname);
            entity = DAEDITOR3.createInvalidEntity(true);
            if (!entity)
              continue;
          }
          if (!tightFenceIntegralEndItems)
            tightFenceIntegralEndItems = gen_rec_b.getBool("integral_end_entity", false);
          else if (!gen_rec_b.getBool("integral_end_entity", false))
          {
            DAEDITOR3.conError("asset %s: integral_end_entity:b=yes must be specified for ALL end entities in list",
              loadingAssetFname);
            continue;
          }

          splineclass::SingleGenEntityGroup::GenEntityRec &sgegRec = sgeg.genEntRecs.push_back();
          sgegRec.entIdx = gen.ent.size();
          gen.ent.push_back(entity);
          sgegRec.genTag = -1;
          if (a)
          {
            eastl::vector_set<const DagorAsset *> visitedAssets;
            visitedAssets.insert(a);
            loadCablesAttachmentPoints(sgegRec.cablePoints, a->props, visitedAssets);
          }

          if (const char *genTag = gen_rec_b.getStr("genTag", NULL))
          {
            if (strlen(genTag) == 1)
            {
              if (tag_to_id[genTag[0]] == -1)
              {
                tag_to_id[genTag[0]] = next_id;
                next_id++;
              }
              sgegRec.genTag = tag_to_id[genTag[0]];
            }
            else
              DAEDITOR3.conError("asset %s: bad genTag <%s>, should be 1 symbol", loadingAssetFname, genTag);
          }

          sgegRec.weight = gen_rec_b.getReal("weight", 1);
          sgegRec.width = gen_rec_b.getReal("width",
            (entity ? entity->getBbox().width().x : 0) * gen_rec_b.getReal("widthMul", 1.0f) + gen_rec_b.getReal("widthAdd", 0.0f));

          if (tightFenceIntegralEndItems)
            sgeg.integralLastEntCount++;
          else
            sgeg.sumWeight += sgegRec.weight;

          sgegRec.rotX = gen_rec_b.getPoint2("rot_x", Point2(0, 0)) * DEG_TO_RAD;
          sgegRec.rotY = gen_rec_b.getPoint2("rot_y", Point2(0, 0)) * DEG_TO_RAD;
          sgegRec.rotZ = gen_rec_b.getPoint2("rot_z", Point2(0, 0)) * DEG_TO_RAD;
          sgegRec.xOffset = gen_rec_b.getPoint2("offset_x", Point2(0, 0));
          sgegRec.yOffset = gen_rec_b.getPoint2("offset_y", Point2(0, 0));
          sgegRec.zOffset = gen_rec_b.getPoint2("offset_z", Point2(0, 0));
          sgegRec.scale = gen_rec_b.getPoint2("scale", Point2(1, 0));
          sgegRec.yScale = gen_rec_b.getPoint2("yScale", Point2(1, 0));
          Point2 xzScale = gen_rec_b.getPoint2("xzScale", Point2(1, 0));
          sgegRec.scale.set(sgegRec.scale.x * xzScale.x, sgegRec.scale.y * xzScale.x + xzScale.y);
          sgegRec.yScale.set(sgegRec.yScale.x / xzScale.x, sgegRec.yScale.y / xzScale.x);

          if (!load_multipoint_data(sgegRec.mpRec, gen_rec_b, entity->getBbox()))
            DAEDITOR3.conError("asset %s: unknown multipoint placement type", res);

          objCnt++;
        }
      clear_and_resize(sgeg.genTagSumWt, next_id);
      mem_set_0(sgeg.genTagSumWt);
      for (int i = 0; i < sgeg.genEntRecs.size(); i++)
        if (sgeg.genEntRecs[i].genTag >= 0)
          sgeg.genTagSumWt[sgeg.genEntRecs[i].genTag] += sgeg.genEntRecs[i].weight;

      if (!objCnt)
        return false;
    }

  if (gen.data.size() > 0)
  {
    DAEDITOR3.conNote("loaded gen entities from asset: %s", loadingAssetFname);
    return true;
  }
  else
  {
    DAEDITOR3.conError("can't load gen entities from asset: %s", loadingAssetFname);
    return false;
  }
}

bool SharedSplineClassAssetData::loadLoftGeomData(splineclass::LoftGeomGenData &genGeom, const DataBlock &blk)
{
  if (!inited)
    init();
  int loftNid = blk.getNameId("loft");
  int pointNid = blk.getNameId("pt");
  int groupNid = blk.getNameId("group");

  Tab<Point4> shape(tmpmem);
  Tab<splineclass::LoftGeomGenData::Loft::PtAttr> shapeAttr(tmpmem);
  Tab<int> auto_v_pts(tmpmem);
  Tab<float> auto_v(tmpmem);

  genGeom.foundationGeom = blk.getBool("foundationGeom", true);
  float above_ht = blk.getReal("placeAboveHt", 2);
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == loftNid)
    {
      const DataBlock &b_loft = *blk.getBlock(i);
      splineclass::LoftGeomGenData::Loft &loft = genGeom.loft.push_back();
      const char *mat_name = b_loft.getStr("matName", NULL);

      if (!mat_name)
        DAEDITOR3.conError("error: no material in %s, block #%d", loadingAssetFname, i + 1);
      else if (stricmp(mat_name, "#::delaunay_pt_cloud") == 0)
        loft.makeDelaunayPtCloud = true;
      else
        loft.matNames.push_back(SimpleString(mat_name));

      loft.waterSurface = b_loft.getBool("waterSurface", false);
      loft.waterSurfaceExclude = b_loft.getBool("waterSurfaceExclude", false);
      if (loft.waterSurfaceExclude)
        loft.waterSurface = true;

      loft.maxMappingDistortionThreshold = b_loft.getReal("max_mapping_distortion_threshold", 0.16f);
      loft.integralMappingLength = b_loft.getBool("integral_mapping_length", false);
      loft.subdivCount = b_loft.getInt("subdivCount", 4);
      loft.shapeSubdivCount = b_loft.getInt("shapeSubdivCount", 4);
      loft.vTile = b_loft.getReal("vTile", loft.vTile);
      loft.uSize = b_loft.getReal("uSize", loft.uSize);
      loft.extrude = b_loft.getBool("extrude", loft.extrude);
      loft.cullcw = b_loft.getBool("cullcw", loft.cullcw);
      loft.flipUV = b_loft.getBool("flipUV", loft.flipUV);
      loft.offset = b_loft.getPoint2("offset", Point2(0, 0));

      loft.minStep = b_loft.getReal("minStep", 3.f);
      if (loft.minStep < splineMinStep)
      {
        DAEDITOR3.conError("minStep %.1f < %.1f in loft asset <%s>, forced minimal value", loft.minStep, splineMinStep,
          loadingAssetFname);
        loft.minStep = splineMinStep;
      }
      loft.maxStep = b_loft.getReal("maxStep", 25.f);
      loft.curvatureStrength = b_loft.getReal("curvatureStrength", 15.f);
      loft.marginAtStart = b_loft.getReal("marginAtStart", 0.f);
      loft.marginAtEnd = b_loft.getReal("marginAtEnd", 0.f);
      loft.zeroOpacityDistAtEnds = b_loft.getReal("zeroOpacityDistAtEnds", blk.getReal("zeroOpacityDistAtEnds", 0));
      loft.maxHerr = b_loft.getReal("maxHerr", 1.0f);
      loft.maxHillHerr = b_loft.getReal("maxHillHerr", loft.maxHerr);

      loft.htTestStep = b_loft.getReal("htTestStep", loft.minStep / 2);
      loft.followHills = b_loft.getBool("followHills", false);
      loft.followHollows = b_loft.getBool("followHollows", false);

      loft.roadBhv = b_loft.getBool("roadBehavior", false);
      loft.roadMaxAbsAng = b_loft.getReal("roadAbsAng", 45.0f);
      loft.roadMaxInterAng = b_loft.getReal("roadInterAng", 30.0f);
      loft.roadTestWidth = b_loft.getReal("roadTestWidth", 0);
      loft.storeSegs = b_loft.getBool("storeSegsForGen", false);
      loft.loftLayerOrder = b_loft.getInt("loftLayerOrder", 0);

      loft.flags = readFlags(b_loft.getBlockByName("node_flags"), loft.normalsDir);
      loft.flags |= StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;
      loft.aboveHt = b_loft.getReal("placeAboveHt", above_ht);
      loft.stage = b_loft.getInt("stage", 0);
      loft.randomOpacityMulAlong = b_loft.getPoint2("randomOpacityMulAlong", Point2(1, 0));
      loft.randomOpacityMulAcross = b_loft.getPoint2("randomOpacityMulAcross", Point2(1, 0));

      loft.layerTag = b_loft.getStr("layerTag", NULL);

      const DataBlock &shape_b = *b_loft.getBlockByNameEx("shape");

      loft.closed = loft.makeDelaunayPtCloud ? false : shape_b.getBool("closed", loft.closed);

      if (pointNid == -1)
        DAEDITOR3.conError("error: no 'pt' parameters (:p2=x,z, :p3=x,z,S or :p4=x,z,S,V)"
                           " in %s, block #%d",
          loadingAssetFname, i + 1);

      shape.clear();
      shapeAttr.clear();
      auto_v_pts.clear();
      if (shape_b.blockCount() == 0)
      {
        for (int j = 0; j < shape_b.paramCount(); j++)
          if (shape_b.getParamNameId(j) == pointNid)
          {
            if (shape_b.getParamType(j) == DataBlock::TYPE_POINT2)
            {
              Point2 p2 = shape_b.getPoint2(j);
              auto_v_pts.push_back(shape.size());
              shape.push_back(Point4(p2.x, p2.y, 0, 1));
            }
            else if (shape_b.getParamType(j) == DataBlock::TYPE_POINT3)
            {
              Point3 p3 = shape_b.getPoint3(j);
              auto_v_pts.push_back(shape.size());
              shape.push_back(Point4(p3.x, p3.y, p3.z, 1));
            }
            else if (shape_b.getParamType(j) == DataBlock::TYPE_POINT4)
              shape.push_back(shape_b.getPoint4(j));
          }
      }
      else
      {
        // format with group attributes
        splineclass::LoftGeomGenData::Loft::PtAttr attr = {0, 0, 0};
        bool trivial_case = true;

        for (int k = 0; k < shape_b.blockCount(); k++)
          if (shape_b.getBlock(k)->getBlockNameId() == groupNid)
          {
            const DataBlock &pts = *shape_b.getBlock(k);
            memset(&attr, 0, sizeof(attr));
            int mat = 0;
            const char *matName = pts.getStr("matName", NULL);
            if (matName)
            {
              int i;
              for (i = 0; i < loft.matNames.size(); ++i)
                if (stricmp(loft.matNames[i].str(), matName) == 0)
                {
                  mat = i;
                  break;
                }
              if (i == loft.matNames.size())
              {
                mat = loft.matNames.size();
                loft.matNames.push_back(SimpleString(matName));
              }
              trivial_case = false;
            }
            attr.matId = mat;
            if (attr.matId != mat)
            {
              DAEDITOR3.conError("too much materials <%d> in loft asset <%s>", mat + 1, loadingAssetFname);
            }

            const char *attr_type = pts.getStr("type", NULL);
            if (!attr_type)
              attr.type = attr.TYPE_NORMAL;
            else if (stricmp(attr_type, "rel_to_collision") == 0)
              attr.type = attr.TYPE_REL_TO_GND;
            else if (stricmp(attr_type, "group_rel_to_collision") == 0)
              attr.type = attr.TYPE_GROUP_REL_TO_GND;
            else if (stricmp(attr_type, "move_to_min") == 0)
              attr.type = attr.TYPE_MOVE_TO_MIN;
            else if (stricmp(attr_type, "move_to_max") == 0)
              attr.type = attr.TYPE_MOVE_TO_MAX;
            else if (stricmp(attr_type, "normal") == 0)
              attr.type = attr.TYPE_NORMAL;
            else
              DAEDITOR3.conError("undefined point group type <%s> in loft asset <%s>", attr_type, loadingAssetFname);

            attr.attr = pts.getInt("attr", loft.makeDelaunayPtCloud ? 1 : 0);
            int st_idx = shapeAttr.size();
            for (int j = 0; j < pts.paramCount(); j++)
              if (pts.getParamNameId(j) == pointNid)
              {
                if (pts.getParamType(j) == DataBlock::TYPE_POINT2)
                {
                  Point2 p2 = pts.getPoint2(j);
                  auto_v_pts.push_back(shape.size());
                  shape.push_back(Point4(p2.x, p2.y, 0, 1));
                }
                else if (pts.getParamType(j) == DataBlock::TYPE_POINT3)
                {
                  Point3 p3 = pts.getPoint3(j);
                  auto_v_pts.push_back(shape.size());
                  shape.push_back(Point4(p3.x, p3.y, p3.z, 1));
                }
                else if (pts.getParamType(j) == DataBlock::TYPE_POINT4)
                  shape.push_back(pts.getPoint4(j));
              }

            shapeAttr.resize(shape.size());
            for (int j = st_idx; j < shapeAttr.size(); j++)
              shapeAttr[j] = attr;
            attr.grpId++;
            if (!attr.grpId)
            {
              DAEDITOR3.conError("too much groups in loft asset <%s>", loadingAssetFname);
            }
            if (attr.type != attr.TYPE_NORMAL || attr.attr)
              trivial_case = false;
          }

        if (trivial_case)
          shapeAttr.resize(0);
        // propagate material indices from last to first for unclosed mesh.
        // because segments count is points count - 1.
        if (!loft.closed)
          for (int i = 1; i < (int)shapeAttr.size() - 1; ++i)
            shapeAttr[i].matId = shapeAttr[i + 1].matId;
      }

      if (loft.closed && shape.size() < 3)
        DAEDITOR3.conError("error: closed shape must have at least 3 points in %s, block #%d", loadingAssetFname, i + 1);
      else if (!loft.closed && shape.size() < 2 && !loft.makeDelaunayPtCloud)
        DAEDITOR3.conError("error: non-closed shape must have at least 2 points in %s, block #%d", loadingAssetFname, i + 1);
      else if (!loft.closed && shape.size() < 1)
        DAEDITOR3.conError("error: shape must have at least 1 point in %s, block #%d", loadingAssetFname, i + 1);
      else
      {
        for (int j = 0; j < (int)shape.size() - 1; j++)
          if (Point2(shape[j + 1].x - shape[j].x, shape[j + 1].y - shape[j].y).length() < 1e-6)
            DAEDITOR3.conError("error: degenerate shape segment (%d,%d) in %s, block #%d", j, j + 1, loadingAssetFname, i + 1);
      }

      if (auto_v_pts.size())
      {
        float l = 0;
        auto_v.resize(shape.size());

        for (int j = 0; j < (int)shape.size() - 1; j++)
        {
          auto_v[j] = l;
          l += length(Point2(shape[j + 1].x, shape[j + 1].y) - Point2(shape[j].x, shape[j].y));
        }
        auto_v.back() = l;

        if (loft.closed)
          l += length(Point2(shape.back().x, shape.back().y) - Point2(shape[0].x, shape[0].y));

        for (int j = 0; j < auto_v_pts.size(); j++)
          shape[auto_v_pts[j]].w = auto_v[auto_v_pts[j]] / l;
      }

      G_ASSERT(shapeAttr.size() == shape.size() || shapeAttr.size() == 0);
      if (loft.makeDelaunayPtCloud && !loft.closed && shape.size() == 1 && shapeAttr[0].attr)
      {
        shape.resize(2);
        shapeAttr.resize(2);
        shape[1] = shape[0];
        shape[1].x += 1e-4;
        shapeAttr[1] = shapeAttr[0];
        shapeAttr[1].attr = 0;
        if (fabsf(shape[1].x - shape[0].x) <= 0 || fabsf(shape[1].x - shape[0].x) >= 1e-3)
        {
          shape.resize(1);
          shapeAttr.resize(1);
        }
        DAEDITOR3.conNote("processing single-point delaunay shape in %s, block #%d: %s", loadingAssetFname, i + 1,
          shape.size() == 2 ? "success" : "failed(underflow)");
      }

      loft.shapePts = shape;
      loft.shapePtsAttr = shapeAttr;
    }

  if (DAGORED2)
    genGeom.colliders = DAGORED2->loadColliders(blk, genGeom.collFilter);
  DAEDITOR3.conNote("loaded loft (%d shapes) from asset: %s", genGeom.loft.size(), loadingAssetFname);
  return true;
}

void SharedSplineClassAssetData::init()
{
  DataBlock appBlk(daeditor3_get_appblk_fname());
  splineMinStep = appBlk.getBlockByNameEx("assets")->getReal("splineMinStep", splineMinStep);
  inited = true;
}


const char *SharedSplineClassAssetData::loadingAssetFname = NULL;
float SharedSplineClassAssetData::splineMinStep = 0.f;
bool SharedSplineClassAssetData::inited = false;
