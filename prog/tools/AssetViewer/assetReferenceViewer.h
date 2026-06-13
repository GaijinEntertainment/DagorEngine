// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetSearchResultsWindow.h"

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <de3_interface.h>

class AssetReferenceViewer : public AssetSearchResultsWindow
{
public:
  AssetReferenceViewer() : AssetSearchResultsWindow("Asset references")
  {
    addColumnTitle("Asset name");
    addColumnTitle("Broken");
    addColumnTitle("External");
    addColumnTitle("Optional");
  }

  void setAsset(DagorAsset *asset)
  {
    assetNameTypified = asset ? asset->getNameTypified() : String();
    setWindowSubtitle(asset ? asset->getName() : "");
    setWindowSubtitleIcon(asset ? AssetSelectorCommon::getAssetTypeIcon(asset->getType()) : PropPanel::IconId::Invalid);
    clearResults();

    if (asset)
      getReferences(*asset);

    fillResultsList();
  }

private:
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

  void getReferences(DagorAsset &asset)
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

      addResult(eastl::move(outputSearchResult));
    }
  }

  String assetNameTypified;
};
