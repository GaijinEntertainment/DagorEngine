#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_grpMgr.h>
#include "assetCreate.h"
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>

bool DagorAssetMgr::mountBuiltGameRes(const char *mount_folder_name, const DataBlock &skip_types)
{
  if (!folders.size())
  {
    folders.push_back(new DagorAssetFolder(-1, "Root", NULL));
    perFolderStartAssetIdx.push_back(0);
  }

  int s_asset_num = assets.size();
  int nsid = nspaceNames.addNameId("gameres");
  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(0, mount_folder_name, "::"));
  folders[fidx]->flags &= ~(DagorAssetFolder::FLG_EXPORT_ASSETS | DagorAssetFolder::FLG_SCAN_ASSETS |
                            DagorAssetFolder::FLG_SCAN_FOLDERS | DagorAssetFolder::FLG_INHERIT_RULES);
  folders[0]->subFolderIdx.push_back(fidx);
  perFolderStartAssetIdx.push_back(assets.size());

  post_msg(*msgPipe, msgPipe->NOTE, "mounting built gameRes to %s...", mount_folder_name);

  Tab<String> names(tmpmem);
  ::get_loaded_game_resource_names(names);


  Tab<bool> exp_types_mask(tmpmem);
  exp_types_mask.resize(getAssetTypesCount());
  mem_set_0(exp_types_mask);

  int nid = skip_types.getNameId("type"), atype;
  for (int i = 0; i < skip_types.paramCount(); i++)
    if (skip_types.getParamType(i) == DataBlock::TYPE_STRING && skip_types.getParamNameId(i) == nid)
    {
      atype = getAssetTypeId(skip_types.getStr(i));
      if (atype != -1)
        exp_types_mask[atype] = true;
    }

  atype = typeNames.getNameId("fx");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("fx", fidx, nsid, names, EffectGameResClassId);

  atype = typeNames.getNameId("rendInst");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("rendInst", fidx, nsid, names, RendInstGameResClassId);

  int dm_start = assets.size();
  atype = typeNames.getNameId("dynModel");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("dynModel", fidx, nsid, names, DynModelGameResClassId);
  int dm_end = assets.size();

  atype = typeNames.getNameId("animChar");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("animChar", fidx, nsid, names, CharacterGameResClassId);

  atype = typeNames.getNameId("skeleton");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("skeleton", fidx, nsid, names, GeomNodeTreeGameResClassId);

  atype = typeNames.getNameId("physObj");
  if (atype != -1 && !exp_types_mask[atype])
    fillGameResFolder("physObj", fidx, nsid, names, PhysObjGameResClassId);

  updateGlobUniqueFlags();

  // specify refernce skeleton for dynModels
  atype = typeNames.getNameId("skeleton");
  for (int i = dm_start; i < dm_end; i++)
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

void DagorAssetMgr::fillGameResFolder(const char *type_name, int root_fidx, int nsid, dag::ConstSpan<String> names,
  unsigned res_classid)
{
  int type = typeNames.getNameId(type_name);
  if (type == -1)
    return;

  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(root_fidx, type_name, "::"));
  folders[fidx]->flags &= ~(DagorAssetFolder::FLG_EXPORT_ASSETS | DagorAssetFolder::FLG_SCAN_ASSETS |
                            DagorAssetFolder::FLG_SCAN_FOLDERS | DagorAssetFolder::FLG_INHERIT_RULES);
  perFolderStartAssetIdx.push_back(assets.size());
  folders[root_fidx]->subFolderIdx.push_back(fidx);

  DagorAssetPrivate *ca = NULL;

  for (int i = 0; i < names.size(); i++)
  {
    if (::get_resource_type_id(names[i]) != res_classid)
      continue;

    if (!ca)
      ca = new DagorAssetPrivate(*this);

    ca->setNames(assetNames.addNameId(names[i]), nsid, true);
    if (perTypeNameIds[type].addInt(ca->getNameId()))
    {
      ca->setAssetData(fidx, -1, type);
      assets.push_back(ca);
      ca = NULL;
    }
    else
      post_msg(*msgPipe, msgPipe->WARNING, "duplicate asset %s of type <%s> in namespace %s", ca->getName(), typeNames.getName(type),
        nspaceNames.getName(nsid));
  }

  del_it(ca);
}
