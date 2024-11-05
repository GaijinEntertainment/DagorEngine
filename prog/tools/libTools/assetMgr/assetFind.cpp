// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include "assetMgrPrivate.h"
#include <util/dag_fastIntList.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_cpuFreq.h>

inline eastl::vector<DagorAssetMgr::AssetMap>::const_iterator DagorAssetMgr::findAssetByName(unsigned name) const
{
  auto cbegin = amap.cbegin(), cend = amap.cend();
  auto ci = eastl::binary_search_i(cbegin, cend, AssetMap{name, 0, 0});
  if (ci != cend)
    while (ci > cbegin && name == (ci - 1)->name)
      ci--;
  return ci;
}

DagorAsset *DagorAssetMgr::findAsset(const char *name) const
{
  if (!name || !name[0])
    return NULL;

  int name_id = assetNames.getNameId(name);
  if (name_id == -1)
    return NULL;

  if (amap.size() != assets.size())
    rebuildAssetMap();
  auto ci = findAssetByName(name_id);
  if (ci != amap.cend())
    return assets[ci->idx];
  return NULL;
}

DagorAsset *DagorAssetMgr::findAsset(const char *name, int type_id) const
{
  if (!name || !name[0])
    return NULL;

  int name_id = assetNames.getNameId(name);
  if (name_id == -1)
    return NULL;

  if (amap.size() != assets.size())
    rebuildAssetMap();
  for (auto ci = findAssetByName(name_id), cend = amap.cend(); ci != cend && ci->name == name_id; ci++)
    if (ci->type == type_id)
      return assets[ci->idx];
  return NULL;
}

DagorAsset *DagorAssetMgr::findAsset(const char *name, dag::ConstSpan<int> types) const
{
  if (types.size() == 1)
    return findAsset(name, types[0]);

  if (!name || !name[0] || types.size() < 1)
    return NULL;

  int name_id = assetNames.getNameId(name);
  if (name_id == -1)
    return NULL;

  if (amap.size() != assets.size())
    rebuildAssetMap();
  for (auto ci = findAssetByName(name_id), cend = amap.cend(); ci != cend && ci->name == name_id; ci++)
    for (int j = 0; j < types.size(); j++)
      if (ci->type == types[j])
        return assets[ci->idx];
  return NULL;
}

void DagorAssetMgr::getFolderAssetIdxRange(int folder_idx, int &start_idx, int &end_idx) const
{
  G_ASSERT(folder_idx >= 0 && folder_idx < folders.size());

  start_idx = perFolderStartAssetIdx[folder_idx];
  end_idx = folder_idx + 1 < folders.size() ? perFolderStartAssetIdx[folder_idx + 1] : assets.size();
}

bool DagorAssetMgr::isAssetNameUnique(const DagorAsset &asset, dag::ConstSpan<int> types)
{
  if (asset.isGloballyUnique())
    return true;

  int cnt = 0;
  for (int i = 0; i < types.size(); i++)
    if (types[i] >= 0 && types[i] < perTypeNameIds.size())
      if (perTypeNameIds[types[i]].hasInt(asset.getNameId()))
      {
        cnt++;
        if (cnt > 1)
          return false;
      }
  return true;
}

void DagorAssetMgr::rebuildAssetMap() const
{
  int t0 = get_time_msec();
  amap.resize(assets.size());
  for (int i = 0; i < assets.size(); i++)
    amap[i].name = assets[i]->getNameId(), amap[i].type = assets[i]->getType(), amap[i].idx = i;
  eastl::sort(amap.begin(), amap.end(), [](const AssetMap &a, const AssetMap &b) {
    if (a.name != b.name)
      return a.name < b.name;
    if (a.idx != b.idx)
      return a.idx < b.idx;
    return a.type < b.type;
  });
  debug("rebuilt assetMap (used for faster searches) for %d msec (%d assets)", get_time_msec() - t0, assets.size());
}
