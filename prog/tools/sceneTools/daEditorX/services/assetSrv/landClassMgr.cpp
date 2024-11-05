// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landClassMgr.h"
#include "flags.h"
#include <de3_bitMaskMgr.h>
#include <de3_assetService.h>
#include <de3_colorRangeService.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <oldEditor/de_interface.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/binDumpHierBitmap.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_hierGrid.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <landMesh/lmeshRenderer.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>


landclass::AssetData::AssetData() :
  tiled(NULL), planted(NULL), grass(NULL), csgGen(NULL), detTex(NULL), indicesTex(NULL), colorImages(NULL), genGeom(NULL), layerIdx(0)
{
  mem_set_0(physMatId);
  mem_set_0(indexedPhysMatId);
}
landclass::AssetData::~AssetData() { SharedLandClassAssetData::clearAsset(*this); }

void SharedLandClassAssetData::clearAsset(landclass::AssetData &a)
{
  del_it(a.tiled);
  if (a.planted)
  {
    for (int i = 0; i < a.planted->dmPool.size(); i++)
      del_it(a.planted->dmPool[i]);
    del_it(a.planted);
  }
  if (a.grass)
  {
    for (int i = 0; i < a.grass->data.size(); i++)
      del_it(a.grass->data[i].densityMap);
    del_it(a.grass);
  }
  del_it(a.detTex);
  del_it(a.indicesTex);
  del_it(a.colorImages);
  del_it(a.genGeom);
  destroy_it(a.csgGen);
}


landclass::AssetData *LandClassAssetMgr::getAsset(const char *asset_name)
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

  int asset_type = DAEDITOR3.getAssetTypeId("land");
  DagorAsset *asset = DAEDITOR3.getAssetByName(asset_name, asset_type);
  SharedLandClassAssetData *lcad = new (midmem) SharedLandClassAssetData;

  if (asset)
  {
    SharedLandClassAssetData::loadingAssetFname = asset_name;
    if (lcad->loadAsset(asset->props))
      ; // asset loaded normally
    else
      DAEDITOR3.conError("errors while loading asset: %s", asset_name);
    SharedLandClassAssetData::loadingAssetFname = NULL;
    lcad->assetName = asset->getName();
    lcad->assetNameId = asset->getNameId();
    asset->getMgr().subscribeUpdateNotify(this, -1, -1);
  }
  else
    DAEDITOR3.conError("can't find landclass asset: %s", asset_name);

  assets[id] = lcad;
  assets[id]->nameId = id;
  lcad->addRef();
  return lcad;
}

landclass::AssetData *LandClassAssetMgr::addRefAsset(landclass::AssetData *data)
{
  for (int i = assets.size() - 1; i >= 0; i--)
    if (assets[i] == data)
    {
      assets[i]->addRef();
      return data;
    }
  return NULL;
}

void LandClassAssetMgr::releaseAsset(landclass::AssetData *data)
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
const char *LandClassAssetMgr::getAssetName(landclass::AssetData *data)
{
  if (!data)
    return nullptr;
  int id = static_cast<SharedLandClassAssetData *>(data)->nameId;
  if (id < 0 || id >= nameMap.strCount() || assets[id] != data)
    return nullptr;
  return nameMap.getStrSlow(id);
}

void LandClassAssetMgr::compact() {}

void LandClassAssetMgr::onAssetRemoved(int asset_name_id, int asset_type)
{
  if (asset_type != DAEDITOR3.getAssetTypeId("land"))
    return;

  for (int i = 0; i < assets.size(); i++)
    if (assets[i] && assets[i]->assetNameId == asset_name_id)
    {
      SharedLandClassAssetData::clearAsset(*assets[i]);

      // notify clients
      for (int j = 0; j < hlist.size(); j++)
        hlist[j]->onLandClassAssetChanged(assets[i]);
      return;
    }
}
void LandClassAssetMgr::onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
{
  const int landAssetType = DAEDITOR3.getAssetTypeId("land");

  if (asset_type == asset.getMgr().getTexAssetTypeId())
  {
    IDagorAssetRefProvider *refProvider = asset.getMgr().getAssetRefProvider(landAssetType);
    if (!refProvider)
      return;

    for (SharedLandClassAssetData *landClassAssetData : assets)
    {
      DagorAsset *landClassAsset = DAEDITOR3.getAssetByName(landClassAssetData->assetName, landAssetType);
      if (!landClassAsset)
        continue;

      const dag::ConstSpan<IDagorAssetRefProvider::Ref> refs = refProvider->getAssetRefs(*landClassAsset);
      for (const IDagorAssetRefProvider::Ref &ref : refs)
        if (ref.getAsset() == &asset)
        {
          for (IAssetUpdateNotify *notify : hlist)
            notify->onLandClassAssetTexturesChanged(landClassAssetData);
          return;
        }
    }

    return;
  }

  if (asset_type != landAssetType)
    return;

  for (int i = 0; i < assets.size(); i++)
    if (assets[i] && assets[i]->assetNameId == asset_name_id)
    {
      SharedLandClassAssetData::clearAsset(*assets[i]);

      // reload asset data
      SharedLandClassAssetData::loadingAssetFname = asset.getName();
      if (assets[i]->loadAsset(asset.props))
        ; // asset loaded normally
      else
        DAEDITOR3.conError("errors while re-loading asset: %s", asset.getName());
      SharedLandClassAssetData::loadingAssetFname = NULL;

      // notify clients
      for (int j = 0; j < hlist.size(); j++)
        hlist[j]->onLandClassAssetChanged(assets[i]);
      return;
    }
}

void LandClassAssetMgr::addNotifyClient(IAssetUpdateNotify *n)
{
  for (int i = 0; i < hlist.size(); i++)
    if (hlist[i] == n)
      return;
  hlist.push_back(n);
}

void LandClassAssetMgr::delNotifyClient(IAssetUpdateNotify *n)
{
  for (int i = 0; i < hlist.size(); i++)
    if (hlist[i] == n)
    {
      erase_items(hlist, i, 1);
      return;
    }
}

bool SharedLandClassAssetData::loadAsset(const DataBlock &blk)
{
  splineClassAssetName = blk.getStr("splineClassAssetName", "");
  mem_set_0(physMatId);
  mem_set_0(indexedPhysMatId);
  if (const char *mat = blk.getStr("physMat", NULL))
  {
    if ((physMatId[0] = PhysMat::getMaterialId(mat)) == PHYSMAT_INVALID)
      DAEDITOR3.conError("bad physMat <%s> in asset: %s", mat, loadingAssetFname);
    for (int i = 0; i < indexedPhysMatId.size(); i++)
      indexedPhysMatId[i] = PhysMat::getMaterialId(mat);
  }

  const DataBlock *b;
  bool ok = true;
  if (blk.getBlockByName("resources"))
  {
    tiled = new (midmem) landclass::TiledEntities;
    if (!loadTiledEntities(*tiled, blk))
      ok = false;
  }

  if (blk.getBlockByName("obj_plant_generate"))
  {
    planted = new (midmem) landclass::PlantedEntities;
    if (!loadPlantedEntities(*planted, blk))
      ok = false;
  }

  b = blk.getBlockByName("grass");
  if (b)
  {
    grass = new (midmem) landclass::GrassEntities;
    if (!loadGrassEntities(*grass, blk))
      ok = false;
    if (!grass->data.size())
      del_it(grass);
  }

  b = blk.getBlockByName("physMatIndices");
  if (b)
  {
    const DataBlock &detailsBlk = *blk.getBlockByName("physMatIndices");
    for (int bi = 0; bi < detailsBlk.blockCount(); ++bi)
    {
      const DataBlock &detTexBlk = *detailsBlk.getBlock(bi);
      const int biomeIndex = detTexBlk.getInt("index", 0);
      const char *mat = detTexBlk.getStr("physMat", NULL);
      indexedPhysMatId[biomeIndex] = PhysMat::getMaterialId(mat);
      if (PhysMat::getMaterialId(mat) == PHYSMAT_INVALID)
        DAEDITOR3.conError("bad physMat <%s> in asset: %s", mat, loadingAssetFname);
    }
  }

  b = blk.getBlockByName("detail");
  if (b)
  {
    if (const char *mat = b->getStr("physMatRed", NULL))
      if ((physMatId[1 + 0] = PhysMat::getMaterialId(mat)) == PHYSMAT_INVALID)
        DAEDITOR3.conError("bad detail/physMatRed <%s> in asset: %s", mat, loadingAssetFname);
    if (const char *mat = b->getStr("physMatGreen", NULL))
      if ((physMatId[1 + 1] = PhysMat::getMaterialId(mat)) == PHYSMAT_INVALID)
        DAEDITOR3.conError("bad detail/physMatGreen <%s> in asset: %s", mat, loadingAssetFname);
    if (const char *mat = b->getStr("physMatBlue", NULL))
      if ((physMatId[1 + 2] = PhysMat::getMaterialId(mat)) == PHYSMAT_INVALID)
        DAEDITOR3.conError("bad detail/physMatBlue <%s> in asset: %s", mat, loadingAssetFname);
    if (const char *mat = b->getStr("physMatBlack", NULL))
      if ((physMatId[1 + 3] = PhysMat::getMaterialId(mat)) == PHYSMAT_INVALID)
        DAEDITOR3.conError("bad detail/physMatBlack <%s> in asset: %s", mat, loadingAssetFname);

    if (b->getStr("splattingmap", nullptr))
    {
      detTex = new (midmem) DataBlock(*b);
    }

    if (const DataBlock *lcTextures = b->getBlockByName("landClassTextures"))
    {
      if (lcTextures->getStr("indices"))
      {
        indicesTex = new (midmem) DataBlock(*lcTextures);
        detTex = new (midmem) DataBlock(*blk.getBlockByName("detail"));
        if (const DataBlock *detailsBlk = detTex->getBlockByName("details"))
        {
          const char *defaultGroupName = detailsBlk->getBlockByNameEx("scheme")->getStr("detail_group", nullptr);
          int physMatForDefaultGroup = indexedPhysMatId[0];
          const int detailGroupNameId = detailsBlk->getNameId("detail_group");
          ska::flat_hash_map<eastl::string, int> detailGroupPhysMats;
          for (int i = 0; i < detailsBlk->blockCount(); ++i)
          {
            const DataBlock *detailGroup = detailsBlk->getBlock(i);
            if (detailGroup->getBlockNameId() != detailGroupNameId)
              continue;
            const char *detailGroupName = detailGroup->getStr("name", nullptr);
            if (const char *detailGroupPhysMat = detailGroup->getStr("physMat", nullptr))
              detailGroupPhysMats[eastl::string(detailGroupName)] = PhysMat::getMaterialId(detailGroupPhysMat);
          }
          // If there is at least one phys mat defined for group we switch on per group phys mats.
          if (!detailGroupPhysMats.empty())
          {
            const int detailNameId = detailsBlk->getNameId("detail");
            for (int i = 0, detail_idx = 0; i < detailsBlk->blockCount(); ++i)
            {
              const DataBlock *detail = detailsBlk->getBlock(i);
              if (detail->getBlockNameId() != detailNameId)
                continue;
              const char *detailGroup = detail->getStr("detail_group", defaultGroupName);
              indexedPhysMatId[detail_idx++] = detailGroup ? detailGroupPhysMats[eastl::string(detailGroup)] : physMatForDefaultGroup;
            }
          }
        }
      }
    }
  }

  b = blk.getBlockByName("images");
  if (b)
  {
    colorImages = new (midmem) landclass::ColorImagesData;
    if (!loadColorImageData(*colorImages, *b))
      ok = false;
  }

  b = blk.getBlockByName("geometry_generation");
  if (b && b->getBool("generate", true))
  {
    genGeom = new (midmem) landclass::PolyGeomGenData;
    if (!loadGenGeomData(*genGeom, *b))
      ok = false;
  }
  layerIdx = blk.getInt("layerIdx", blk.getBool("secondaryLayer", false) ? 1 : 0);
  editableMasks = blk.getBlockByNameEx("detail")->getBool("editable", false);

  if (const char *nm = blk.getStr("gen_CSG", NULL))
  {
    DagorAsset *a = DAEDITOR3.getAssetByName(nm, DAEDITOR3.getAssetTypeId("csg"));
    if (!a)
      DAEDITOR3.conError("cannot find asset <%s>, type=csg", nm);
    csgGen = a ? DAEDITOR3.createEntity(*a, true) : NULL;
    if (!csgGen)
      DAEDITOR3.conError("cannot create entity for asset <%s>, type=csg", nm);
  }
  return ok;
}

bool SharedLandClassAssetData::loadTiledEntities(landclass::TiledEntities &tiled, const DataBlock &blk)
{
  IColorRangeService *colRangeSrv = EDITORCORE->queryEditorInterface<IColorRangeService>();
  int rendInst_atype = DAEDITOR3.getAssetTypeId("rendInst");
  tiled.riOnly = true;

  tiled.data.clear();
  BBox2 area;

  area.lim[0] = Point2(0, 0);
  area.lim[1] = blk.getBlockByNameEx("detail")->getPoint2("size", Point2(0, 0));

  area.lim[0] = blk.getPoint2("landBBoxLim0", area.lim[0]);
  area.lim[1] = blk.getPoint2("landBBoxLim1", area.lim[1]);
  tiled.setSeedToEntities = blk.getBool("setSeedToEntities", false);

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
    const DataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != resNid)
      continue;

    Point2 offset = dataBlk.getPoint2("offset", Point2(0, 0));
    const char *orientTypeStr = dataBlk.getStr("orientation", NULL);
    if (orientTypeStr)
      if (stricmp(orientTypeStr, "world") != 0 && stricmp(orientTypeStr, "normal") != 0)
      {
        DAEDITOR3.conError("undefined orientation=<%s> in asset: %s", orientTypeStr, loadingAssetFname);
        orientTypeStr = NULL;
      }

    for (int i = 0; i < dataBlk.blockCount(); i++)
    {
      const DataBlock &objBlk = *dataBlk.getBlock(i);
      if (objBlk.getBlockNameId() != objNid)
        continue;

      tm = objBlk.getTm("matrix", TMatrix::IDENT);
      if (!(area & Point2(tm.getcol(3).x, tm.getcol(3).z)))
        continue;

      const char *asset_name = objBlk.getStr("name", NULL);
      const char *obj_orient = objBlk.getStr("orientation", orientTypeStr);
      unsigned orientType = landclass::SingleEntityPlaces::ORIENT_WORLD;

      if (!obj_orient || stricmp(obj_orient, "world") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_WORLD;
      else if (stricmp(obj_orient, "world_xz") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_WORLD_XZ;
      else if (stricmp(obj_orient, "normal") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_NORMAL;
      else if (stricmp(obj_orient, "normal_xz") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_NORMAL_XZ;
      else
        DAEDITOR3.conError("undefined orientation=<%s> in asset: %s", obj_orient, loadingAssetFname);

      int idx = -1;
      for (int k = tiled.data.size() - 1; k >= 0; k--)
        if (strcmp(asset_name, tiled.data[k].resName) == 0 && tiled.data[k].orientType == orientType)
        {
          idx = k;
          break;
        }

      if (idx < 0)
      {
        DagorAsset *a = DAEDITOR3.getGenObjAssetByName(asset_name);
        IObjEntity *ent = a ? DAEDITOR3.createEntity(*a, true) : NULL;
        if (!ent)
        {
          int nid = missingAssetNames.addNameId(asset_name);
          if (nid == missingAssetNames.nameCount() - 1)
            DAEDITOR3.conError("invalid object '%s' in %s", asset_name, loadingAssetFname);
          ent = DAEDITOR3.createInvalidEntity(true);
          if (!ent)
            continue;
        }
        if (ent->getAssetTypeId() != rendInst_atype)
          tiled.riOnly = false;

        idx = append_items(tiled.data, 1);
        tiled.data[idx].resName = asset_name;
        tiled.data[idx].orientType = orientType;
        tiled.data[idx].entity = ent;
        tiled.data[idx].yOffset = dataBlk.getPoint2("yOffset", dataBlk.getPoint2("offset_y", Point2(0, 0)));
        tiled.data[idx].rseed = dataBlk.getInt("rseed", 123);
        tiled.data[idx].entity->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile"));

        if (!load_multipoint_data(tiled.data[idx].mpRec, dataBlk, BBox3()))
          DAEDITOR3.conError("asset %s: unknown multipoint placement type", asset_name);
      }

      Point3 pos = tm.getcol(3) + Point3(offset.x, 0, offset.y);

      pos.x -= floor((pos.x - area.lim[0].x) / tile_x) * tile_x;
      pos.z -= floor((pos.z - area.lim[0].y) / tile_z) * tile_z;
      tm.setcol(3, pos);

      tiled.data[idx].tm.push_back(tm);
    }

    const DataBlock &colBlk = *dataBlk.getBlockByNameEx("colors");
    E3DCOLOR dc0 = colBlk.getE3dcolor("colorFrom", dataBlk.getE3dcolor("colorFrom", 0x80808080U));
    E3DCOLOR dc1 = colBlk.getE3dcolor("colorTo", dataBlk.getE3dcolor("colorTo", 0x80808080U));
    unsigned col_idx = colRangeSrv ? colRangeSrv->addColorRange(dc0, dc1) : IColorRangeService::IDX_GRAY;
    for (int k = 0; k < tiled.data.size(); k++)
      tiled.data[k].colorRangeIdx = col_idx;

    for (int i = 0; i < colBlk.blockCount(); i++)
    {
      const DataBlock &b = *colBlk.getBlock(i);
      const char *objName = b.getBlockName();
      const char *obj_orient = b.getStr("orientation", orientTypeStr);
      unsigned orientType = landclass::SingleEntityPlaces::ORIENT_WORLD;

      if (!obj_orient || stricmp(obj_orient, "world") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_WORLD;
      else if (stricmp(obj_orient, "world_xz") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_WORLD_XZ;
      else if (stricmp(obj_orient, "normal") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_NORMAL;
      else if (stricmp(obj_orient, "normal_xz") == 0)
        orientType = landclass::SingleEntityPlaces::ORIENT_NORMAL_XZ;
      else
        DAEDITOR3.conError("undefined orientation=<%s> in asset: %s", obj_orient, loadingAssetFname);

      for (int k = 0; k < tiled.data.size(); k++)
        if (strcmp(objName, tiled.data[k].resName) == 0 && tiled.data[k].orientType == orientType)
        {
          tiled.data[k].colorRangeIdx = colRangeSrv
                                          ? colRangeSrv->addColorRange(b.getE3dcolor("colorFrom", dc0), b.getE3dcolor("colorTo", dc1))
                                          : IColorRangeService::IDX_GRAY;
          objName = NULL;
          break;
        }
      if (objName)
        DAEDITOR3.conError("failed to find tiled entity <%s>/%d to set color range %p-%p", objName, orientType,
          b.getE3dcolor("colorFrom", dc0), b.getE3dcolor("colorTo", dc1));
    }

    // for ( int k = tiled.data.size()-1; k >=0; k -- )
    //   debug ( "use entity: %s, e=%p", (char*)tiled.data[k].resName, tiled.data[k].entity);
  }

  // justity tile box
  if (float_nonzero(area.lim[0].x) || float_nonzero(area.lim[0].y))
  {
    Point2 offset = area.lim[0];
    for (int k = tiled.data.size() - 1; k >= 0; k--)
      for (int i = 0; i < tiled.data[k].tm.size(); i++)
      {
        Point3 pos = tiled.data[k].tm[i].getcol(3);
        pos.x -= offset.x;
        pos.z -= offset.y;
        pos.x -= floor(pos.x / tile_x) * tile_x;
        pos.z -= floor(pos.z / tile_z) * tile_z;
        tiled.data[k].tm[i].setcol(3, pos);
      }
  }

  if (DAGORED2)
    tiled.colliders = DAGORED2->loadColliders(blk, tiled.collFilter);
  tiled.aboveHt = blk.getReal("placeAboveHt", 1.0);
  tiled.useYpos = blk.getBool("useYpos", false);
  tiled.tile = Point2(tile_x, tile_z);
  return true;
}

bool SharedLandClassAssetData::loadPlantedEntities(landclass::PlantedEntities &planted, const DataBlock &blk_source)
{
  IColorRangeService *colRangeSrv = EDITORCORE->queryEditorInterface<IColorRangeService>();
  int rendInst_atype = DAEDITOR3.getAssetTypeId("rendInst");
  planted.riOnly = true;

  OAHashNameMap<true> names, rNames;
  OAHashNameMap<true> dmNames;
  Tab<IPoint2> dmSz(tmpmem);

  DataBlock blk = blk_source;
  int genNid = blk.getNameId("obj_plant_generate");
  int objNid = blk.getNameId("object");

  for (int blki = 0; blki < blk.blockCount(); ++blki)
  {
    DataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != genNid)
      continue;
    const int densmap = dataBlk.getNameId("densityMap");
    if (densmap == -1)
      continue;
    int firstDenseMapIdx = -1;
    for (int i = 0; i < dataBlk.paramCount(); ++i)
    {
      if (dataBlk.getParamNameId(i) != densmap)
        continue;
      firstDenseMapIdx = i;
      break;
    }
    for (int i = dataBlk.paramCount() - 1; i > firstDenseMapIdx; --i)
    {
      if (dataBlk.getParamNameId(i) != densmap)
        continue;
      DataBlock *secondaryDensityMapData = blk.addNewBlock(&dataBlk, "obj_plant_generate");
      secondaryDensityMapData->removeParam("densityMap");
      secondaryDensityMapData->setStr("densityMap", dataBlk.getStr(i));
      dataBlk.removeParam(i);
    }
  }

  for (int blki = 0; blki < blk.blockCount(); ++blki)
  {
    const DataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != genNid)
      continue;

    landclass::SingleGenEntityGroup &group = planted.data.push_back();

    group.density = dataBlk.getReal("density", 1);
    group.objOverlapRatio = dataBlk.getReal("intersection", 1);
    group.rseed = dataBlk.getInt("rseed", 12345);
    Point2 common_yOffset = dataBlk.getPoint2("yOffset", dataBlk.getPoint2("offset_y", Point2(0, 0)));
    group.sumWeight = 0;
    if (DAGORED2)
      group.colliders = DAGORED2->loadColliders(dataBlk, group.collFilter);
    group.aboveHt = dataBlk.getReal("placeAboveHt", 1.0);
    group.colorRangeIdx = colRangeSrv ? colRangeSrv->addColorRange(dataBlk.getE3dcolor("colorFrom", 0x80808080U),
                                          dataBlk.getE3dcolor("colorTo", 0x80808080U))
                                      : IColorRangeService::IDX_GRAY;
    group.setSeedToEntities = dataBlk.getBool("setSeedToEntities", blk.getBool("setSeedToEntities", false));

    const char *densmap = dataBlk.getStr("densityMap", NULL);
    group.densMapCellSz = Point2(0, 0);
    group.densMapSize = dataBlk.getPoint2("mapSize", Point2(32, 32));
    group.densMapOfs = dataBlk.getPoint2("mapOfs", Point2(0, 0));

    if (densmap)
    {

      int dmid = dmNames.addNameId(densmap);
      if (dmid >= dmSz.size())
      {
        planted.dmPool.push_back(NULL);
        dmSz.push_back(IPoint2(1, 1));
        G_ASSERT(planted.dmPool.size() == dmid + 1 || dmSz.size() == dmid + 1);

        planted.dmPool[dmid] = loadDensityMap(densmap, dmSz[dmid].x, dmSz[dmid].y);
      }

      group.densityMap = planted.dmPool[dmid];

      group.densMapCellSz.x = group.densMapSize.x / dmSz[dmid].x;
      group.densMapCellSz.y = group.densMapSize.y / dmSz[dmid].y;
    }

    for (int i = 0; i < dataBlk.blockCount(); i++)
    {
      const DataBlock &objBlk = *dataBlk.getBlock(i);
      if (objBlk.getBlockNameId() != objNid)
        continue;

      const char *asset_name = objBlk.getStr("name", NULL);
      // debug("name=%s, type=%s, cls=%d", resname, objBlk.getStr("type", NULL), cls);
      if (!asset_name)
        continue;

      landclass::SingleGenEntityGroup::Obj &obj = group.obj.push_back();
      if (names.addNameId(asset_name) >= planted.ent.size())
      {
        DagorAsset *a = DAEDITOR3.getGenObjAssetByName(asset_name);
        IObjEntity *e = a ? DAEDITOR3.createEntity(*a, true) : NULL;
        if (!e)
        {
          DAEDITOR3.conError("invalid object '%s' in %s", asset_name, loadingAssetFname);
          e = DAEDITOR3.createInvalidEntity(true);

          if (!e)
          {
            group.obj.pop_back();
            continue;
          }
        }
        if (e->getAssetTypeId() != rendInst_atype)
          planted.riOnly = false;
        e->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("mask_tile"));
        obj.entityIdx = rNames.addNameId(asset_name);
        planted.ent.push_back(e);
        G_ASSERT(planted.ent.size() == obj.entityIdx + 1);
      }
      else
        obj.entityIdx = rNames.getNameId(asset_name);

      if (obj.entityIdx == -1)
      {
        group.obj.pop_back();
        continue;
      }

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
        debug("incorrect weight for %s: %.3f", asset_name, obj.weight);
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

      if (!load_multipoint_data(obj.mpRec, objBlk, BBox3()))
        DAEDITOR3.conError("asset %s: unknown multipoint placement type", asset_name);
    }

    if (!group.obj.size())
    {
      DAEDITOR3.conError("no valid objects in %s, block %d", loadingAssetFname, blki);
      planted.data.pop_back();
    }
  }
  if (!planted.data.size())
    DAEDITOR3.conError("no planted object groups in %s", loadingAssetFname);

  return true;
}

bool SharedLandClassAssetData::loadGrassEntities(landclass::GrassEntities &grass, const DataBlock &blk)
{
  int nid = blk.getNameId("grass");
  bool ok = true;
  for (int blki = 0; blki < blk.blockCount(); ++blki)
  {
    const DataBlock &dataBlk = *blk.getBlock(blki);
    if (dataBlk.getBlockNameId() != nid)
      continue;

    if (!loadGrassDensity(grass.data.push_back(), dataBlk))
    {
      grass.data.pop_back();
      ok = false;
    }
  }
  return ok;
}

bool SharedLandClassAssetData::loadGrassDensity(landclass::GrassDensity &gd, const DataBlock &blk)
{
  gd.density = blk.getReal("density", 1);
  gd.rseed = blk.getInt("rseed", 12345);

  const char *densmap = blk.getStr("densityMap", NULL);
  gd.densMapCellSz = Point2(0, 0);
  gd.densMapSize = blk.getPoint2("mapSize", Point2(32, 32));
  gd.densMapOfs = blk.getPoint2("mapOfs", Point2(0, 0));

  const char *resname = blk.getStr("grassAsset", NULL);
  if (!resname)
    return false;

  DagorAsset *a = DAEDITOR3.getAssetByName(resname, DAEDITOR3.getAssetTypeId("grass"));
  gd.entity = a ? DAEDITOR3.createEntity(*a, true) : NULL;
  if (!gd.entity)
  {
    DAEDITOR3.conError("invalid grass '%s' in %s", resname, loadingAssetFname);
    return false;
  }

  if (gd.entity)
    gd.entity->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("mask_tile"));

  if (densmap)
  {
    int w, h;
    gd.densityMap = loadDensityMap(densmap, w, h);

    gd.densMapCellSz.x = gd.densMapSize.x / w;
    gd.densMapCellSz.y = gd.densMapSize.y / h;
  }

  return true;
}

bool SharedLandClassAssetData::loadColorImageData(landclass::ColorImagesData &ci, const DataBlock &blk)
{
  ci.images.resize(blk.blockCount());
  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    landclass::ColorImagesData::Image &img = ci.images[i];
    const char *mt = b.getStr("mappingType", "hmap");

    img.name = b.getBlockName();
    img.fname = b.getStr("image", NULL);
    if (strncmp(img.fname, "./", 2) == 0)
      img.fname = String(260, "*%s", (char *)img.fname);

    if (stricmp(mt, "hmap") == 0)
      img.mappingType = 0;
    else if (stricmp(mt, "hmap%") == 0)
      img.mappingType = 1;
    else if (stricmp(mt, "world") == 0)
      img.mappingType = 2;
    else if (stricmp(mt, "vertU") == 0)
      img.mappingType = 3;
    else if (stricmp(mt, "vertV") == 0)
      img.mappingType = 4;
    else
    {
      DAEDITOR3.conError("asset %s: unknown mappingType=<%s>", loadingAssetFname, mt);
      clear_and_shrink(ci.images);
      return false;
    }

    img.tile = b.getPoint2("tile", Point2(100, 100));
    img.offset = b.getPoint2("offset", Point2(0, 0));
    img.clampU = b.getBool("clampU", false);
    img.clampV = b.getBool("clampV", false);
    img.flipU = b.getBool("flipU", false);
    img.flipV = b.getBool("flipV", false);
    img.swapUV = b.getBool("swapUV", false);
  }
  return true;
}

bool SharedLandClassAssetData::loadGenGeomData(landclass::PolyGeomGenData &genGeom, const DataBlock &blk)
{
  int nid = blk.getNameId("UV");
  const DataBlock *b;

  genGeom.foundationGeom = blk.getBool("foundationGeom", true);
  genGeom.stage = blk.getInt("stage", 0);
  genGeom.layerTag = blk.getStr("layerTag", NULL);
  genGeom.mainMat = blk.getStr("main_mat", NULL);
  genGeom.borderMat = blk.getStr("border_mat", NULL);
  genGeom.borderWidth = blk.getReal("border_width", 0.0);
  genGeom.borderEdges = blk.getInt("border_edges", 1);
  genGeom.flags = readFlags(blk.getBlockByName("node_flags"), genGeom.normalsDir);
  genGeom.waterSurface = blk.getBool("waterSurface", false);
  genGeom.waterSurfaceExclude = blk.getBool("waterSurfaceExclude", false);
  if (genGeom.waterSurfaceExclude)
    genGeom.waterSurface = true;

  if (genGeom.borderEdges < 1)
    return false;

  b = blk.getBlockByName("main_UV");
  if (b)
    for (int i = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
        loadGenGeomDataMapping(genGeom.mainUV.push_back(), *b->getBlock(i));

  if (genGeom.waterSurface)
  {
    genGeom.borderMat = NULL;
    genGeom.borderWidth = 0;
    return true;
  }

  b = blk.getBlockByName("border_UV");
  if (b)
    for (int i = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
        loadGenGeomDataMapping(genGeom.borderUV.push_back(), *b->getBlock(i));
  return true;
}

void SharedLandClassAssetData::loadGenGeomDataMapping(landclass::PolyGeomGenData::Mapping &m, const DataBlock &blk)
{
  m.chanIdx = blk.getInt("channel", 1);
  m.offset = blk.getPoint2("offset", Point2(0, 0));
  m.type = stricmp(blk.getStr("type", "planarY"), "spline") == 0 ? m.TYPE_SPLINE : m.TYPE_PLANAR;

  m.size = blk.getPoint2("size", Point2(1, 1));
  m.swapUV = blk.getBool("swapUV", false);
  m.rotationW = blk.getReal("rotationW", 0);
  m.tileU = blk.getReal("tileU", 1);
  m.sizeV = blk.getReal("sizeV", 1);
  m.autotilelength = blk.getBool("autotilelength", false);
}

landclass::DensityMap *SharedLandClassAssetData::loadDensityMap(const char *tif_fname, int &w, int &h)
{
  IBitMaskImageMgr *bmImgMgr = EDITORCORE->queryEditorInterface<IBitMaskImageMgr>();
  landclass::DensityMap *map = NULL;
  w = 1;
  h = 1;

  DagorAsset *a = DAEDITOR3.getAssetByName(tif_fname, DAEDITOR3.getAssetTypeId("tifMask"));
  if (!a)
  {
    DAEDITOR3.conError("cant find asset <%s:tifMask>", tif_fname);
    return NULL;
  }

  IBitMaskImageMgr::BitmapMask img;
  if (bmImgMgr && bmImgMgr->loadImage(img, NULL, a->getTargetFilePath()))
  {
    if (img.getBitsPerPixel() != 1 && img.getBitsPerPixel() != 8)
      debug("density map <%s> must be 1 or 8 bpp; it is %dx%d %dbpp", tif_fname, img.getWidth(), img.getHeight(),
        img.getBitsPerPixel());
    else if ((img.getWidth() & 15) | (img.getHeight() & 15))
      debug("density map <%s> size must multiple of 16; it is %dx%d %dbpp", tif_fname, img.getWidth(), img.getHeight(),
        img.getBitsPerPixel());
    else
    {
      struct Bitmap1
      {
        Bitmap1(const IBitMaskImageMgr::BitmapMask &_img) : img(_img), ymax(img.getHeight() - 1) {}
        int getW() const { return img.getWidth(); }
        int getH() const { return img.getHeight(); }
        int get(int x, int y) const { return img.getMaskPixel1(x, ymax - y); }
        const IBitMaskImageMgr::BitmapMask &img;
        int ymax;
      };
      struct Bitmap8
      {
        Bitmap8(const IBitMaskImageMgr::BitmapMask &_img) : img(_img), ymax(img.getHeight() - 1) {}
        int getW() const { return img.getWidth(); }
        int getH() const { return img.getHeight(); }
        int get(int x, int y) const { return img.getMaskPixel8(x, ymax - y) > 127; }
        const IBitMaskImageMgr::BitmapMask &img;
        int ymax;
      };

      mkbindump::BinDumpSaveCB lcwr(8 << 20, _MAKE4C('PC'), false);
      if (img.getBitsPerPixel() == 1)
        mkbindump::build_ro_hier_bitmap(lcwr, Bitmap1(img), 4, 3);
      else if (img.getBitsPerPixel() == 8)
        mkbindump::build_ro_hier_bitmap(lcwr, Bitmap8(img), 4, 3);

      MemoryLoadCB mcrd(lcwr.getMem(), false);
      map = RoHugeHierBitMap2d<4, 3>::create(mcrd);
      w = map->getW();
      h = map->getH();
    }

    bmImgMgr->destroyImage(img);
  }
  else
    DAEDITOR3.conError("can't load density map <%s> from %s", tif_fname, a->getTargetFilePath());

  return map;
}

const char *SharedLandClassAssetData::loadingAssetFname = NULL;
