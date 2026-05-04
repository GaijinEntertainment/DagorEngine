// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetSearchResultsWindow.h"

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <de3_interface.h>

class AssetReferenceViewer
{
public:
  static void show(DagorAsset &asset)
  {
    asset_search_results_window.reset(new AssetSearchResultsWindow("Asset references"));
    asset_search_results_window->setWindowSubtitle(asset.getNameTypified());
    asset_search_results_window->addColumnTitle("Asset name");
    asset_search_results_window->addColumnTitle("Broken");
    asset_search_results_window->addColumnTitle("External");
    asset_search_results_window->addColumnTitle("Optional");

    getReferences(asset);

    asset_search_results_window->fillResultsList();
    asset_search_results_window->show();
  }

private:
  static void getReferences(DagorAsset &asset)
  {
    IDagorAssetRefProvider *refProvider = asset.getMgr().getAssetRefProvider(asset.getType());
    if (!refProvider)
    {
      DAEDITOR3.conError("\nAsset <%s> refs: no refs provider.", asset.getNameTypified().c_str());
      return;
    }

    Tab<IDagorAssetRefProvider::Ref> refs;
    refProvider->getAssetRefs(asset, refs);
    DAEDITOR3.conNote("\nAsset <%s> references %d asset(s):", asset.getNameTypified().c_str(), (int)refs.size());
    for (const IDagorAssetRefProvider::Ref &ref : refs)
    {
      String refName;
      const DagorAsset *refAsset = ref.getAsset();
      if (refAsset)
        refName = refAsset->getNameTypified();
      else if (ref.flags & IDagorAssetRefProvider::RFLG_BROKEN)
        refName = ref.getBrokenRef();

      DAEDITOR3.conNote("  %c[%c%c] %s", ref.flags & IDagorAssetRefProvider::RFLG_BROKEN ? '-' : ' ',
        ref.flags & IDagorAssetRefProvider::RFLG_EXTERNAL ? 'X' : ' ', ref.flags & IDagorAssetRefProvider::RFLG_OPTIONAL ? 'o' : ' ',
        refName.c_str());

      AssetSearchResultsListControl::SearchResult outputSearchResult;
      if (ref.flags & IDagorAssetRefProvider::RFLG_BROKEN)
        outputSearchResult.assetText = ref.getBrokenRef();
      else
        outputSearchResult.asset = ref.getAsset();

      // Some of the reference providers (like AnimCharRefs and EffectRefs) might add empty external references.
      if (!outputSearchResult.asset && outputSearchResult.assetText.empty())
        continue;

      const char *brokenText = (ref.flags & IDagorAssetRefProvider::RFLG_BROKEN) == 0 ? "-" : "Broken";
      outputSearchResult.additionalColumns.emplace_back(brokenText);

      const char *externalText = (ref.flags & IDagorAssetRefProvider::RFLG_EXTERNAL) == 0 ? "-" : "External";
      outputSearchResult.additionalColumns.emplace_back(externalText);

      const char *optionalText = (ref.flags & IDagorAssetRefProvider::RFLG_OPTIONAL) == 0 ? "-" : "Optional";
      outputSearchResult.additionalColumns.emplace_back(optionalText);

      asset_search_results_window->addResult(eastl::move(outputSearchResult));
    }
  }
};
