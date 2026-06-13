// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetSearchResultsWindow.h"

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <coolConsole/coolConsole.h>
#include <de3_interface.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_interface.h>

#include <EASTL/hash_map.h>

class AssetBackReferenceViewer : public AssetSearchResultsWindow
{
public:
  AssetBackReferenceViewer() : AssetSearchResultsWindow("Asset back references") { addColumnTitle("Asset name"); }

  void setAsset(DagorAsset *asset)
  {
    static AllAssetRefsHashType allAssetRefs; // Getting references for rendInsts is slow, so we cache the results.

    assetNameTypified = asset ? asset->getNameTypified() : String();
    setWindowSubtitle(asset ? asset->getName() : "");
    setWindowSubtitleIcon(asset ? AssetSelectorCommon::getAssetTypeIcon(asset->getType()) : PropPanel::IconId::Invalid);
    clearResults();

    if (asset)
    {
      if (allAssetRefs.empty())
        collectAllAssetReferences(allAssetRefs, asset->getMgr(), EDITORCORE->getConsole());

      getBackReferences(*asset, allAssetRefs);
    }

    fillResultsList();
  }

private:
  struct AllAssetRefsElement
  {
    Tab<IDagorAssetRefProvider::Ref> refs;
  };

  typedef eastl::hash_map<const DagorAsset *, AllAssetRefsElement> AllAssetRefsHashType;

  void saveResultsToBlk(DataBlock &blk) const override
  {
    blk.addStr("asset", assetNameTypified);

    AssetSearchResultsWindow::saveResultsToBlk(blk);
  }

  bool loadResultsFromBlk(const DataBlock &blk, const char *path) override
  {
    assetNameTypified = blk.getStr("asset", "");
    if (assetNameTypified.empty())
    {
      logerr("Loading results from \"%s\" has failed. It does not contain the asset name.", path);
      return false;
    }

    DagorAsset *asset = DAEDITOR3.getAssetByName(assetNameTypified);
    if (!asset)
    {
      logerr("Loading results from \"%s\" has failed. Asset \"%s\" cannot be found.", path, assetNameTypified);
      return false;
    }

    if (!AssetSearchResultsWindow::loadResultsFromBlk(blk, path))
      return false;

    setAsset(asset);
    setPinned(true); // Pin it because the loaded asset like does not match the currently selected asset in the Assets Tree.

    return true;
  }

  static void collectAllAssetReferencesInternal(AllAssetRefsHashType &all_asset_refs, dag::ConstSpan<DagorAsset *> assets,
    CoolConsole &console)
  {
    int assetsWithoutRefProvider = 0;

    for (int i = 0; i < assets.size(); ++i)
    {
      DagorAsset *currentAsset = assets[i];
      G_ASSERT(all_asset_refs.find(currentAsset) == all_asset_refs.end());

      IDagorAssetRefProvider *refProvider = currentAsset->getMgr().getAssetRefProvider(currentAsset->getType());
      if (refProvider)
      {
        AllAssetRefsElement assetRefs;
        refProvider->getAssetRefs(*currentAsset, assetRefs.refs);
        all_asset_refs.emplace(currentAsset, assetRefs);
      }
      else
      {
        ++assetsWithoutRefProvider;
      }

      console.incDone();
    }

    if (assetsWithoutRefProvider > 0)
      DAEDITOR3.conError("\nFound %d assets without ref provider. The results won't be 100%% accurate.", assetsWithoutRefProvider);
  }

  static void collectAllAssetReferences(AllAssetRefsHashType &all_asset_refs, DagorAssetMgr &asset_mgr, CoolConsole &console)
  {
    EcAutoBusy autoBusy;
    dag::ConstSpan<DagorAsset *> assets = asset_mgr.getAssets();

    const bool consoleWasOpen = console.isVisible();
    console.showConsole(true);

    DAEDITOR3.conNote("Processing %d assets", assets.size());
    console.startLog();
    console.setTotal(assets.size());
    console.startProgress();

    collectAllAssetReferencesInternal(all_asset_refs, assets, console);

    const bool hasErrorsOrWarnings = console.hasErrors() || console.hasWarnings();
    console.endLogAndProgress();

    if (!consoleWasOpen && !hasErrorsOrWarnings)
      console.hideConsole();
  }

  void getBackReferences(DagorAsset &asset, const AllAssetRefsHashType &all_asset_refs)
  {
    dag::Vector<const DagorAsset *> referencingAssets;

    for (AllAssetRefsHashType::const_iterator i = all_asset_refs.begin(); i != all_asset_refs.end(); ++i)
      for (const IDagorAssetRefProvider::Ref &ref : i->second.refs)
        if (ref.getAsset() == &asset)
          referencingAssets.emplace_back(i->first);

    eastl::sort(referencingAssets.begin(), referencingAssets.end(), assetCompareFunction);

    DAEDITOR3.conNote("\nAssets referencing <%s>:", asset.getNameTypified().c_str());
    for (const DagorAsset *currentAsset : referencingAssets)
    {
      DAEDITOR3.conNote("  %s", currentAsset->getNameTypified().c_str());

      addResult(*currentAsset);
    }
  }

  static bool assetCompareFunction(const DagorAsset *a, const DagorAsset *b)
  {
    const int result = stricmp(a->getNameTypified().c_str(), b->getNameTypified().c_str());
    if (result == 0)
      return a < b;
    return result < 0;
  }

  String assetNameTypified;
};
