// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetSearchResultsWindow.h"

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <coolConsole/coolConsole.h>
#include <de3_interface.h>
#include <EditorCore/ec_input.h>

#include <EASTL/hash_map.h>

class AssetBackReferenceViewer
{
public:
  static void show(DagorAsset &asset, CoolConsole &console)
  {
    static AllAssetRefsHashType allAssetRefs; // Getting references for rendInsts is slow, so we cache the results.

    if (allAssetRefs.empty())
      collectAllAssetReferences(allAssetRefs, asset.getMgr(), console);

    asset_search_results_window.reset(new AssetSearchResultsWindow("Asset back references"));
    asset_search_results_window->setWindowSubtitle(asset.getNameTypified());
    asset_search_results_window->addColumnTitle("Asset name");

    getBackReferences(asset, allAssetRefs);

    asset_search_results_window->fillResultsList();
    asset_search_results_window->show();
  }

private:
  struct AllAssetRefsElement
  {
    Tab<IDagorAssetRefProvider::Ref> refs;
  };

  typedef eastl::hash_map<const DagorAsset *, AllAssetRefsElement> AllAssetRefsHashType;

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

  static void getBackReferences(DagorAsset &asset, const AllAssetRefsHashType &all_asset_refs)
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

      asset_search_results_window->addResult(*currentAsset);
    }
  }

  static bool assetCompareFunction(const DagorAsset *a, const DagorAsset *b)
  {
    const int result = stricmp(a->getNameTypified().c_str(), b->getNameTypified().c_str());
    if (result == 0)
      return a < b;
    return result < 0;
  }
};
