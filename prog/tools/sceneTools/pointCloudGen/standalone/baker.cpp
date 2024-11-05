// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "baker.h"
#include "assets/asset.h"
#include "assets/assetMgr.h"
#include "debug/dag_assert.h"
#include "generic/dag_span.h"
#include "options.h"
#include "pointCloudGen.h"

bool interrupted = false;
namespace plod
{

Baker::Baker(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *asset_manager, ILogWriter *log_writer,
  const AppOptions &options) :
  PointCloudGenerator(app_dir, app_blk, asset_manager, log_writer), options(options)
{
  rendinstTypeId = asset_manager->getAssetTypeId("rendinst");
  rendinstRefProfider = asset_manager->getAssetRefProvider(rendinstTypeId);
}


bool Baker::isAssetSupported(DagorAsset *asset)
{
  if (!PointCloudGenerator::isAssetSupported(asset))
    return false;

  const auto plodNameId = asset->props.getNameId("plod");
  for (int i = 0; const auto &block = asset->props.getBlock(i); i++)
    if (block->getBlockNameId() == plodNameId)
      return true;

  return false;
}

void Baker::gatherAssetsFromList(dag::Vector<DagorAsset *> &assets, dag::ConstSpan<plod::String> asset_names)
{
  for (const auto &name : asset_names)
  {
    DagorAsset *asset = assetMgr->findAsset(name.c_str());
    if (!asset)
      conerror("Requested asset could not be found: %s", name.c_str());
    else
    {
      if (!isAssetSupported(asset))
        conerror("Requested asset not supported: %s", name.c_str());
      else
        assets.push_back(asset);
    }
  }
}

void Baker::gatherAssetsFromPackages(dag::Vector<DagorAsset *> &assets, dag::ConstSpan<plod::String> packages)
{
  const auto allAssets = assetMgr->getAssets();
  for (auto &asset : allAssets)
    if (isAssetSupported(asset) && eastl::count(packages.begin(), packages.end(), asset->getDestPackName()) > 0)
      assets.push_back(asset);
}

void Baker::gatherAllAssets(dag::Vector<DagorAsset *> &assets)
{
  for (auto *asset : assetMgr->getAssets())
    if (isAssetSupported(asset))
      assets.push_back(asset);
}

int Baker::generatePLODs()
{
  dag::Vector<DagorAsset *> assets{};
  if (!options.assetsToBuild.empty())
    gatherAssetsFromList(assets, options.assetsToBuild);
  else if (!options.packsToBuild.empty())
    gatherAssetsFromPackages(assets, options.packsToBuild);
  else
    gatherAllAssets(assets);

  conlog("Total assets for processing : %d", assets.size());

  uint32_t processed = 0;
  const auto processAsset = [&](DagorAsset *asset) {
    plod::String msg{};
    msg.sprintf("[%u/%u] Processing asset %s", ++processed, assets.size(), asset->getName());
    conlog(msg.c_str());
    if (!options.dryMode)
      generate(asset);
  };

  for (const auto &asset : assets)
  {
    if (interrupted)
      return 1;
    processAsset(asset);
  }

  return 0;
}

} // namespace plod
