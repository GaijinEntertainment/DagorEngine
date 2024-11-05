// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_grpMgr.h>
#include "assetCreate.h"
#include <util/dag_fastIntList.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

struct DagorAssetMgr::ClassidToAssetMap
{
  int atype;
  unsigned classId;
};

bool DagorAssetMgr::mountBuiltGameResEx(const DataBlock &list, const DataBlock &skip_types)
{
  if (!folders.size())
  {
    folders.push_back(new DagorAssetFolder(-1, "Root", NULL));
    perFolderStartAssetIdx.push_back(0);
  }
  struct GameResAssetDesc
  {
    const char *atype;
    unsigned classId;
  };
  static GameResAssetDesc graDesc[] = {
    {"fx", EffectGameResClassId},
    {"rendInst", RendInstGameResClassId},
    {"dynModel", DynModelGameResClassId},
    {"animChar", CharacterGameResClassId},
    {"skeleton", GeomNodeTreeGameResClassId},
    {"physObj", PhysObjGameResClassId},
    {"collision", CollisionGameResClassId},
    {"rndGrass", RndGrassGameResClassId},
    {"tex", 0},
  };

  Tab<ClassidToAssetMap> map(tmpmem);
  int nid = skip_types.getNameId("type");
  int nsid = nspaceNames.addNameId("gameres");

  for (int j = 0; j < sizeof(graDesc) / sizeof(graDesc[0]); j++)
  {
    int atype = getAssetTypeId(graDesc[j].atype);
    if (atype < 0)
      continue;
    for (int i = 0; i < skip_types.paramCount(); i++)
      if (skip_types.getParamType(i) == DataBlock::TYPE_STRING && skip_types.getParamNameId(i) == nid)
        if (strcmp(graDesc[j].atype, skip_types.getStr(i)) == 0)
        {
          atype = -1;
          break;
        }

    if (atype >= 0)
    {
      ClassidToAssetMap &m = map.push_back();
      m.atype = atype;
      m.classId = graDesc[j].classId;
    }
  }

  int s_asset_num = assets.size();
  post_msg(*msgPipe, msgPipe->NOTE, "mounting built gameRes (%d types)", map.size());
  fillGameResFolder(0, nsid, list, map);

  updateGlobUniqueFlags();

  // specify reference skeleton for dynModels
  int atype = typeNames.getNameId("skeleton");
  int atype2 = typeNames.getNameId("dynModel");
  if (atype >= 0 && atype2 >= 0)
    for (int i = s_asset_num; i < assets.size(); i++)
      if (assets[i]->getType() == atype2)
      {
        String gn_name(256, "%s_skeleton", assets[i]->getName());
        int nm_len = i_strlen(assets[i]->getName());
        if (findAsset(gn_name, atype))
          assets[i]->props.setStr("ref_skeleton", gn_name);
        else if (strncmp(assets[i]->getName(), "low_", 4) == 0 && findAsset(&gn_name[4], atype))
          assets[i]->props.setStr("ref_skeleton", &gn_name[4]);
        else if (nm_len > 6 && strncmp(assets[i]->getName() + nm_len - 6, "_model", 6) == 0)
        {
          gn_name.printf(256, "%.*s_skeleton", nm_len - 6, assets[i]->getName());
          if (findAsset(gn_name, atype))
            assets[i]->props.setStr("ref_skeleton", gn_name);
        }
        else if (nm_len > 5 && strncmp(assets[i]->getName() + nm_len - 5, "_dmg2", 5) == 0)
        {
          gn_name.printf(256, "%.*s_skeleton", nm_len - 1, assets[i]->getName());
          if (findAsset(gn_name, atype))
            assets[i]->props.setStr("ref_skeleton", gn_name);
        }
      }

  for (int i = s_asset_num; i < assets.size(); i++)
    assets[i]->props.compact();
  post_msg(*msgPipe, msgPipe->NOTE, "added %d gameRes assets", assets.size() - s_asset_num);
  return true;
}

void DagorAssetMgr::fillGameResFolder(int pfidx, int nsid, const DataBlock &folderBlk, dag::ConstSpan<ClassidToAssetMap> map)
{
  DagorAssetPrivate *ca = NULL;

  assets.reserve(folderBlk.paramCount());
  for (int i = 0; i < folderBlk.paramCount(); i++)
  {
    if (folderBlk.getParamType(i) != folderBlk.TYPE_INT)
      continue;

    int atype = -1, classId = folderBlk.getInt(i);
    for (int j = 0; j < map.size(); j++)
      if (map[j].classId == classId)
      {
        atype = map[j].atype;
        break;
      }
    if (atype < 0)
      continue;

    if (!ca)
      ca = new DagorAssetPrivate(*this);

    ca->setNames(assetNames.addNameId(folderBlk.getParamName(i)), nsid, true);
    if (perTypeNameIds[atype].addInt(ca->getNameId()))
    {
      ca->setAssetData(pfidx, -1, atype);
      assets.push_back(ca);
      ca = NULL;
    }
    else
      post_msg(*msgPipe, msgPipe->WARNING, "duplicate asset %s of type <%s> in namespace %s", ca->getName(), typeNames.getName(atype),
        nspaceNames.getName(nsid));
  }
  del_it(ca);

  for (int i = 0; i < folderBlk.blockCount(); i++)
  {
    const DataBlock &b = *folderBlk.getBlock(i);
    String fpath(260, "%s/%s", folders[pfidx]->folderPath.str(), b.getBlockName());
    dd_simplify_fname_c(fpath);
    int fidx = folders.size();
    folders.push_back(new DagorAssetFolder(pfidx, b.getBlockName(), fpath));

    folders[fidx]->flags &= ~(DagorAssetFolder::FLG_EXPORT_ASSETS | DagorAssetFolder::FLG_SCAN_ASSETS |
                              DagorAssetFolder::FLG_SCAN_FOLDERS | DagorAssetFolder::FLG_INHERIT_RULES);

    int s_asset_idx = assets.size();
    perFolderStartAssetIdx.push_back(assets.size());
    fillGameResFolder(fidx, nsid, b, map);
    if (assets.size() == s_asset_idx)
    {
      debug("remove empty subtree %s", fpath);
      erase_ptr_items(folders, fidx, folders.size() - fidx);
      perFolderStartAssetIdx.resize(fidx);
    }
    else
      folders[pfidx]->subFolderIdx.push_back(fidx);
  }
}
