// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include "riGen/landClassData.h"
#include "riGen/riGenData.h"

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <shaders/dag_rendInstRes.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <supp/dag_alloca.h>


#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }

bool rendinst::render::tmInstColored = false;

class LandClassGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId, refCount;
    eastl::unique_ptr<rendinst::gen::land::AssetData> landData;
  };

  eastl::vector<ResData> resData;


  int findRes(int res_id) const
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameRes(GameResource *res) const
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].landData.get() == (rendinst::gen::land::AssetData *)res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return rendinst::gen::land::HUID_LandClass; }

  const char *getResClassName() override { return "LandClass"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findGameRes(res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0 || !interlocked_acquire_load(resData[id].landData->riResResolved))
      return nullptr;

    resData[id].refCount++;

    return (GameResource *)resData[id].landData.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findGameRes(resource);
    if (id < 0)
      return false;

    resData[id].refCount++;
    return true;
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    if (resData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(resData[id].resId, name);
      logerr("Attempt to release %s resource %s with 0 refcount", getResClassName(), name);
      return;
    }

    resData[id].refCount--;
  }

  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }

  bool releaseGameResource(GameResource *resource) override
  {
    int id = findGameRes(resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool /*force*/, bool /*once*/) override
  {
    bool result = false;
    for (int i = resData.size() - 1; i >= 0; --i)
    {
      if (resData[i].refCount != 0)
        continue;
      if (get_refcount_game_resource_pack_by_resid(resData[i].resId) > 0)
        continue;

      resData.erase(resData.begin() + i);
      result = true;
    }

    return result;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
    }

    String name;
    get_game_resource_name(res_id, name);
    auto landData = eastl::make_unique<rendinst::gen::land::AssetData>();
    landData->loadingAssetFname = name;
    landData->load(cb);
    landData->loadingAssetFname = nullptr;

    WinAutoLock lock2(cs);
    resData.push_back(ResData{res_id, 0, eastl::move(landData)});
  }


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    rendinst::gen::land::AssetData *ld;
    {
      WinAutoLock lock(cs);
      int id = findRes(res_id);
      if (id < 0)
      {
        String name;
        get_game_resource_name(res_id, name);
        logerr("no LandClass resource %s", name.str());
        return;
      }
      ld = resData[id].landData.get();
    }
    if (ld->riRes.size() * 2 != num_refs)
    {
      String name;
      get_game_resource_name(res_id, name);
      logerr("LandClass resource %s refs mismatch %d != %d", name.str(), ld->riRes.size() * 2, num_refs);
      ld->clear();
      return;
    }
    if (interlocked_acquire_load(ld->riResResolved))
      return;

    Tab<bool> riPosInst(tmpmem);
    Tab<bool> riPaletteRotation(tmpmem);
    riPosInst.resize(num_refs / 2);
    mem_set_0(riPosInst);
    riPaletteRotation.resize(num_refs / 2);
    mem_set_0(riPaletteRotation);
    ld->riResName.resize(num_refs / 2);
    for (int i = 0; i < num_refs / 2; i++)
    {
      String ri_name;
      get_game_resource_name(ref_ids[i * 2], ri_name);
      ld->riRes[i] = (RenderableInstanceLodsResource *)::get_game_resource(ref_ids[i * 2]);
      ld->riResName[i] = str_dup(ri_name, strmem);
      ld->riCollRes[i] = nullptr;
      if (!ld->riRes[i])
      {
        String name;
        get_game_resource_name(res_id, name);
        G_ASSERT_LOG(ld->riRes[i] != nullptr, "LandClass resource %s failed to resolve ref[%d]=%d <%s>", name.str(), i, ref_ids[i * 2],
          ri_name.str());
        ld->riRes[i] = rendinst::get_stub_res();
        continue;
      }
      ld->riCollRes[i] = (CollisionResource *)::get_game_resource(ref_ids[i * 2 + 1]);
      if (!ld->riCollRes[i] && ref_ids[i * 2 + 1] >= 0)
      {
        String name;
        get_game_resource_name(res_id, name);
        String name2;
        get_game_resource_name(ref_ids[i * 2 + 1], name2);
        logerr("LandClass resource %s failed to resolve ref[%d]=%d <%s>", name.str(), i, ref_ids[i * 2 + 1], name2.str());
      }
      if (ld->riCollRes[i])
        ld->riCollRes[i]->collapseAndOptimize(USE_GRID_FOR_RI);
      else if (ld->riRes[i]->hasImpostor())
        logwarn("missing tree-RI-collRes: ri=%s", ri_name);
      riPosInst[i] = ld->riRes[i]->hasImpostor();
      riPaletteRotation[i] = ld->riRes[i]->isBakedImpostor();
    }
    ld->updatePosInst(riPosInst);
    ld->updatePaletteRotation(riPaletteRotation);

    interlocked_release_store(ld->riResResolved, 1);
  }

  void reset() override { resData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resData, resId, refCount)
};

static InitOnDemand<LandClassGameResFactory> land_gameres_factory;


rendinst::gen::land::AssetData::AssetData() :
  tiled(nullptr), planted(nullptr), riRes(midmem), riCollRes(midmem), riColPair(midmem), loadingAssetFname(nullptr), riResResolved(0)
{
  memset(hash, 0, sizeof(hash));
}

void rendinst::gen::land::AssetData::load(IGenLoad &_crd)
{
  clear();

  unsigned fmt = 0;
  _crd.beginBlock(&fmt);
  IGenLoad &crd = (fmt == btag_compr::ZSTD)
                    ? ptr_to_ref(new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(_crd, _crd.getBlockRest()))
                    : ptr_to_ref(new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(_crd, _crd.getBlockRest()));
  RoDataBlock *blk = RoDataBlock::load(crd);

  detailData.setFrom(*blk->getBlockByNameEx("detail"));
  layerIdx = blk->getInt("layerIdx", blk->getBool("secondaryLayer", false) ? 1 : 0);

  riRes.resize(blk->getInt("riResCount", 0));
  mem_set_0(riRes);
  riCollRes.resize(riRes.size());
  mem_set_0(riCollRes);
  clear_and_resize(riPosInst, riRes.size());
  mem_set_0(riPosInst);
  clear_and_resize(riPaletteRotation, riRes.size());
  mem_set_0(riPaletteRotation);
  riColPair.assign(riRes.size() * 2, 0x80808080U);

  if (blk->getBlockByName("resources"))
  {
    tiled = new (midmem) rendinst::gen::land::TiledEntities;
    if (!loadTiledEntities(*blk))
      logerr("failed to load tiled entities properly");
  }

  if (blk->getBlockByName("obj_plant_generate"))
  {
    planted = new (midmem) rendinst::gen::land::PlantedEntities;

    OAHashNameMap<true> dm_nm;
    Tab<IPoint2> dm_sz(tmpmem);

    dm_sz.resize(crd.readInt());
    planted->dmPool.resize(dm_sz.size());
    for (int i = 0; i < planted->dmPool.size(); i++)
    {
      SimpleString s;
      crd.readString(s);
      G_VERIFY(dm_nm.addNameId(s) == i);

      planted->dmPool[i] = RoHugeHierBitMap2d<4, 3>::create(crd);
      dm_sz[i].x = planted->dmPool[i]->getW();
      dm_sz[i].y = planted->dmPool[i]->getH();
    }

    if (!loadPlantedEntities(*blk, dm_nm, dm_sz))
      logerr("failed to load planted entities properly");
  }
  crd.ceaseReading();
  crd.~IGenLoad();
  _crd.endBlock();

  memset(hash, 0, sizeof(hash));
  if (blk->getBool("hasHash", false))
  {
    _crd.beginBlock();
    G_ASSERT(_crd.getBlockRest() == sizeof(hash));
    _crd.read(hash, sizeof(hash));
    _crd.endBlock();

    unsigned *h = (unsigned *)hash;
    debug("landClass-hash: %08X%08X%08X%08X%08X %s", h[0], h[1], h[2], h[3], h[4], loadingAssetFname);
    (void)h;
  }

  delete blk;
}
void rendinst::gen::land::AssetData::updatePosInst(dag::ConstSpan<bool> _riPosInst)
{
  riPosInst = _riPosInst;
  if (tiled)
    for (int i = 0; i < tiled->data.size(); i++)
      if (tiled->data[i].entityIdx >= 0)
        tiled->data[i].posInst = riPosInst[tiled->data[i].entityIdx];
  if (planted)
    for (int i = 0; i < planted->data.size(); i++)
      for (int j = 0; j < planted->data[i].obj.size(); j++)
        if (planted->data[i].obj[j].entityIdx >= 0)
          planted->data[i].obj[j].posInst = riPosInst[planted->data[i].obj[j].entityIdx];
  for (int i = 0; i < riPosInst.size(); i++)
    if (!riPosInst[i] && !rendinst::render::tmInstColored)
      riColPair[i * 2 + 0] = riColPair[i * 2 + 1] = 0x80808080;
}
void rendinst::gen::land::AssetData::updatePaletteRotation(dag::ConstSpan<bool> _riPaletteRotation)
{
  riPaletteRotation = _riPaletteRotation;
  if (tiled)
    for (int i = 0; i < tiled->data.size(); i++)
      if (tiled->data[i].entityIdx >= 0)
        tiled->data[i].paletteRotation = riPaletteRotation[tiled->data[i].entityIdx];
  if (planted)
    for (int i = 0; i < planted->data.size(); i++)
      for (int j = 0; j < planted->data[i].obj.size(); j++)
        if (planted->data[i].obj[j].entityIdx >= 0)
          planted->data[i].obj[j].paletteRotation = riPaletteRotation[planted->data[i].obj[j].entityIdx];
}

void rendinst::gen::land::AssetData::clear()
{
  del_it(tiled);
  del_it(planted);
  clear_and_shrink(riColPair);
  for (int i = 0; i < riRes.size(); i++)
    ::release_game_resource((GameResource *)riRes[i]);
  for (int i = 0; i < riCollRes.size(); i++)
    ::release_game_resource((GameResource *)riCollRes[i]);
  clear_and_shrink(riRes);
  clear_and_shrink(riCollRes);
  clear_all_ptr_items(riResName);
  interlocked_release_store(riResResolved, 0);
}

void rendinst::gen::land::AssetData::updateRiColor(int entId, E3DCOLOR from, E3DCOLOR to)
{
  if (entId < 0)
    return;
  if (riColPair[entId * 2 + 0] != from || riColPair[entId * 2 + 1] != to)
  {
    if (riColPair[entId * 2] != 0x80808080U || riColPair[entId * 2 + 1] != 0x80808080U)
      logwarn("changed color for res#%d from %08X:%08X to %08X:%08X", entId, riColPair[entId * 2].u, riColPair[entId * 2 + 1].u,
        from.u, to.u);
    riColPair[entId * 2 + 0] = from;
    riColPair[entId * 2 + 1] = to;
  }
}

bool rendinst::gen::land::AssetData::loadTiledEntities(const RoDataBlock &blk)
{
  tiled->data.clear();
  BBox2 area;

  area.lim[0] = blk.getPoint2("landBBoxLim0", Point2(0, 0));
  area.lim[1] = blk.getPoint2("landBBoxLim1", Point2(0, 0));

  if (area.isempty())
    return false;
  float tile_x = area.lim[1].x - area.lim[0].x;
  float tile_z = area.lim[1].y - area.lim[0].y;

  int resNid = blk.getNameId("resources");
  int objNid = blk.getNameId("object");
  TMatrix tm;
  OAHashNameMap<true> missingAssetNames;

  for (int blki = 0; blki < blk.blockCount(); ++blki)
  {
    const RoDataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != resNid)
      continue;

    Point2 offset = dataBlk.getPoint2("offset", Point2(0, 0));
    const char *orientTypeStr = dataBlk.getStr("orientation", nullptr);
    if (orientTypeStr)
      if (stricmp(orientTypeStr, "world") != 0 && stricmp(orientTypeStr, "normal") != 0)
      {
        logerr("undefined orientation=<%s> in asset: %s", orientTypeStr, loadingAssetFname);
        orientTypeStr = nullptr;
      }

    for (int i = 0; i < dataBlk.blockCount(); i++)
    {
      const RoDataBlock &objBlk = *dataBlk.getBlock(i);
      if (objBlk.getBlockNameId() != objNid)
        continue;

      tm = objBlk.getTm("matrix", TMatrix::IDENT);
      if (!(area & Point2(tm.getcol(3).x, tm.getcol(3).z)))
        continue;

      int asset_idx = objBlk.getInt("riResId", -1);
      const char *obj_orient = objBlk.getStr("orientation", orientTypeStr);
      unsigned orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD;

      if (!obj_orient || stricmp(obj_orient, "world") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD;
      else if (stricmp(obj_orient, "world_xz") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD_XZ;
      else if (stricmp(obj_orient, "normal") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_NORMAL;
      else if (stricmp(obj_orient, "normal_xz") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_NORMAL_XZ;
      else
        logerr("undefined orientation=<%s> in asset: %s", obj_orient, loadingAssetFname);

      int idx = -1;
      for (int k = tiled->data.size() - 1; k >= 0; k--)
        if (asset_idx == tiled->data[k].entityIdx && tiled->data[k].orientType == orientType)
        {
          idx = k;
          break;
        }

      if (idx < 0)
      {
        idx = append_items(tiled->data, 1);
        tiled->data[idx].orientType = orientType;
        tiled->data[idx].entityIdx = asset_idx;
        tiled->data[idx].posInst = false;
        tiled->data[idx].paletteRotation = false;
        tiled->data[idx].yOffset = dataBlk.getPoint2("yOffset", dataBlk.getPoint2("offset_y", Point2(0, 0)));
        tiled->data[idx].rseed = dataBlk.getInt("rseed", 123);

        if (!load_multipoint_data(tiled->data[idx].mpRec, dataBlk))
          logerr("asset #%d: unknown multipoint placement type", asset_idx);
      }

      Point3 pos = tm.getcol(3) + Point3(offset.x, 0, offset.y);

      pos.x -= floor((pos.x - area.lim[0].x) / tile_x) * tile_x;
      pos.z -= floor((pos.z - area.lim[0].y) / tile_z) * tile_z;
      tm.setcol(3, pos);

      tiled->data[idx].tm.push_back(tm);
    }

    const RoDataBlock &colBlk = *dataBlk.getBlockByNameEx("colors");
    E3DCOLOR dc0 = colBlk.getE3dcolor("colorFrom", dataBlk.getE3dcolor("colorFrom", 0x80808080U));
    E3DCOLOR dc1 = colBlk.getE3dcolor("colorTo", dataBlk.getE3dcolor("colorTo", 0x80808080U));
    for (int k = 0; k < tiled->data.size(); k++)
      updateRiColor(tiled->data[k].entityIdx, dc0, dc1);

    for (int i = 0; i < colBlk.blockCount(); i++)
    {
      const RoDataBlock &b = *colBlk.getBlock(i);
      int objId = atoi(b.getBlockName());
      const char *obj_orient = b.getStr("orientation", orientTypeStr);
      unsigned orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD;

      if (!obj_orient || stricmp(obj_orient, "world") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD;
      else if (stricmp(obj_orient, "world_xz") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_WORLD_XZ;
      else if (stricmp(obj_orient, "normal") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_NORMAL;
      else if (stricmp(obj_orient, "normal_xz") == 0)
        orientType = rendinst::gen::land::SingleEntityPlaces::ORIENT_NORMAL_XZ;
      else
        logerr("undefined orientation=<%s> in asset: %s", obj_orient, loadingAssetFname);

      for (int k = 0; k < tiled->data.size(); k++)
        if (tiled->data[k].entityIdx == objId && tiled->data[k].orientType == orientType)
        {
          updateRiColor(tiled->data[k].entityIdx, b.getE3dcolor("colorFrom", dc0), b.getE3dcolor("colorTo", dc1));
          objId = -1;
          break;
        }
      if (objId > 0)
        logerr("failed to find tiled entity #%d/%d to set color range %08X-%08X", objId, orientType, b.getE3dcolor("colorFrom", dc0).u,
          b.getE3dcolor("colorTo", dc1).u);
    }
  }

  // justity tile box
  if (float_nonzero(area.lim[0].x) || float_nonzero(area.lim[0].y))
  {
    Point2 offset = area.lim[0];
    for (int k = tiled->data.size() - 1; k >= 0; k--)
      for (int i = 0; i < tiled->data[k].tm.size(); i++)
      {
        Point3 pos = tiled->data[k].tm[i].getcol(3);
        pos.x -= offset.x;
        pos.z -= offset.y;
        pos.x -= floor(pos.x / tile_x) * tile_x;
        pos.z -= floor(pos.z / tile_z) * tile_z;
        tiled->data[k].tm[i].setcol(3, pos);
      }
  }

  tiled->aboveHt = blk.getReal("placeAboveHt", 1.0);
  tiled->useYpos = blk.getBool("useYpos", false);
  tiled->tile = Point2(tile_x, tile_z);
  return true;
}

bool rendinst::gen::land::AssetData::loadPlantedEntities(const RoDataBlock &blk, const OAHashNameMap<true> &dmNames,
  dag::ConstSpan<IPoint2> dmSz)
{
  int genNid = blk.getNameId("obj_plant_generate");
  int objNid = blk.getNameId("object");

  OAHashNameMap<true> names, rNames;

  for (int blki = 0; blki < blk.blockCount(); ++blki)
  {
    const RoDataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != genNid)
      continue;

    rendinst::gen::land::SingleGenEntityGroup &group = planted->data.push_back();

    group.density = dataBlk.getReal("density", 1);
    group.objOverlapRatio = dataBlk.getReal("intersection", 1);
    group.rseed = dataBlk.getInt("rseed", 12345);
    Point2 common_yOffset = dataBlk.getPoint2("yOffset", dataBlk.getPoint2("offset_y", Point2(0, 0)));
    group.sumWeight = 0;
    group.aboveHt = dataBlk.getReal("placeAboveHt", 1.0);
    E3DCOLOR dc0 = dataBlk.getE3dcolor("colorFrom", 0x80808080U), dc1 = dataBlk.getE3dcolor("colorTo", 0x80808080U);

    const char *densmap = dataBlk.getStr("densityMap", nullptr);
    group.densMapCellSz = Point2(0, 0);
    group.densMapSize = dataBlk.getPoint2("mapSize", Point2(32, 32));
    group.densMapOfs = dataBlk.getPoint2("mapOfs", Point2(0, 0));

    if (densmap)
    {
      int dmid = dmNames.getNameId(densmap);
      if (dmid >= 0)
      {
        group.densityMap = planted->dmPool[dmid];
        group.densMapCellSz.x = group.densMapSize.x / dmSz[dmid].x;
        group.densMapCellSz.y = group.densMapSize.y / dmSz[dmid].y;

        static constexpr int CSZ = rendinst::gen::land::DensMapLeaf::SZ;
        float density = group.density * group.densMapCellSz.x * group.densMapCellSz.y / 100.0 * CSZ * CSZ;
        if (group.density > 0 && density < 1.0 / 64.0 && group.densMapCellSz.x * group.densMapCellSz.y < 1)
        {
#if DAGOR_DBGLEVEL > 0
          logerr(
            "%s.obj_plant_generate[%d]: density=%g or density map \"%s\" cell size=%gx%g is too low (per block density=%g < 1/64)",
            loadingAssetFname, blki, group.density, densmap, group.densMapCellSz.x, group.densMapCellSz.y, density);
#endif
          group.density = 0; //== prevent generating
        }
      }
    }

    for (int i = 0; i < dataBlk.blockCount(); i++)
    {
      const RoDataBlock &objBlk = *dataBlk.getBlock(i);
      if (objBlk.getBlockNameId() != objNid)
        continue;

      int asset_idx = objBlk.getInt("riResId", -1);

      rendinst::gen::land::SingleGenEntityGroup::Obj &obj = group.obj.push_back();
      obj.entityIdx = asset_idx;
      obj.posInst = false;
      obj.paletteRotation = false;
      updateRiColor(asset_idx, dc0, dc1);

      const char *obj_orient = objBlk.getStr("orientation", "world");
      if (!obj_orient || stricmp(obj_orient, "world") == 0)
        obj.orientType = obj.ORIENT_WORLD;
      else if (stricmp(obj_orient, "world_xz") == 0)
        obj.orientType = obj.ORIENT_WORLD_XZ;
      else if (stricmp(obj_orient, "normal") == 0)
        obj.orientType = obj.ORIENT_NORMAL;
      else if (stricmp(obj_orient, "normal_xz") == 0)
        obj.orientType = obj.ORIENT_NORMAL_XZ;
      else
        logerr("undefined orientation=<%s> in asset: %s", obj_orient, loadingAssetFname);

      obj.weight = objBlk.getReal("weight", 1);
      if (obj.weight < 0)
      {
        debug("incorrect weight for #%d: %.3f", asset_idx, obj.weight);
        obj.weight = 0;
        return false;
      }
      group.sumWeight += obj.weight;

      obj.rotX = objBlk.getPoint2("rot_x", Point2(0, 0)) * DEG_TO_RAD;
      obj.rotY = objBlk.getPoint2("rot_y", Point2(0, 0)) * DEG_TO_RAD;
      obj.rotZ = objBlk.getPoint2("rot_z", Point2(0, 0)) * DEG_TO_RAD;
      obj.scale = objBlk.getPoint2("scale", Point2(1, 0));
      obj.yScale = objBlk.getPoint2("yScale", Point2(1, 0));
      obj.yOffset = objBlk.getPoint2("yOffset", objBlk.getPoint2("offset_y", common_yOffset));

      if (!load_multipoint_data(obj.mpRec, objBlk))
        logerr("asset #%d: unknown multipoint placement type", asset_idx);
    }

    if (!group.obj.size())
    {
      logerr("no valid objects in %s, block %d", loadingAssetFname, blki);
      planted->data.pop_back();
    }
  }
  if (!planted->data.size())
    logerr("no planted object groups in %s", loadingAssetFname);

  return true;
}

void rendinst::register_land_gameres_factory()
{
  land_gameres_factory.demandInit();
  add_factory(land_gameres_factory);
}
namespace rendinst
{
void rt_rigen_free_unused_land_classes()
{
  if (land_gameres_factory)
    land_gameres_factory->freeUnusedResources(false, false);
}
} // namespace rendinst

const DataBlock &rendinst::get_detail_data(void *land_cls_res)
{
  return reinterpret_cast<rendinst::gen::land::AssetData *>(land_cls_res)->detailData;
}
int rendinst::get_landclass_data_hash(void *land_cls_res, void *buf, int buf_sz)
{
  return reinterpret_cast<rendinst::gen::land::AssetData *>(land_cls_res)->getHash(buf, buf_sz);
}

rendinst::gen::land::AssetData rendinst::gen::land::AssetData::emptyLandStub;
