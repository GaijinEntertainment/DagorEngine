// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>

#include <assets/assetHlp.h>
namespace dabuildcache
{
bool invalidate_asset(const DagorAsset &, bool) { G_ASSERT_RETURN(false, false); }
} // namespace dabuildcache

#include <assets/asset.h>
#include <util/dag_string.h>
const char *DagorAsset::getName() const { G_ASSERT_RETURN(false, nullptr); }
const char *DagorAsset::getTypeStr() const { G_ASSERT_RETURN(false, nullptr); }
String DagorAsset::getSrcFilePath() const { G_ASSERT_RETURN(false, {}); }

#include <assets/assetMgr.h>
DagorAsset *DagorAssetMgr::findAsset(const char *, int) const { G_ASSERT_RETURN(false, nullptr); }
DagorAsset *DagorAssetMgr::findAsset(const char *) const { G_ASSERT_RETURN(false, nullptr); }
dag::ConstSpan<int> DagorAssetMgr::getFilteredAssets(dag::ConstSpan<int>, int) const { G_ASSERT_RETURN(false, {}); }

#include <libTools/dagFileRW/dagMatRemapUtil.h>
bool load_scene(const char *, DagData &, bool) { G_ASSERT_RETURN(false, false); }

#include "../main/assetStatus.h"
AssetLoadingStatus get_texture_status(const char *) { G_ASSERT_RETURN(false, AssetLoadingStatus::NotLoaded); }


const DagorAssetMgr *get_asset_manager() { G_ASSERT_RETURN(false, nullptr); }
AssetLoadingStatus get_asset_status(DagorAsset &, bool) { G_ASSERT_RETURN(false, AssetLoadingStatus::NotLoaded); }

#include <assets/assetRefs.h>
const Tab<IDagorAssetRefProvider::Ref> &get_asset_dependencies(DagorAsset &)
{
  G_ASSERT(0);
  static Tab<IDagorAssetRefProvider::Ref> refs;
  return refs;
}
