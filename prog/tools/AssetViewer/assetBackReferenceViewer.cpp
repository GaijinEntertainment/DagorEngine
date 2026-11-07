// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assetBackReferenceViewer.h"
#include <assetsGui/av_assetSelectorCommon.h>
#include <winGuiWrapper/wgw_dialogs.h>

AssetBackReferenceViewer::AllAssetRefsHashType &AssetBackReferenceViewer::getAllAssetRefs()
{
  static AllAssetRefsHashType allAssetRefs; // Getting references for rendInsts is slow, so we cache the results.
  return allAssetRefs;
}

bool AssetBackReferenceViewer::ensureAllAssetRefsCollected(DagorAsset &asset)
{
  static bool allAssetRefsCollected = false;

  if (!allAssetRefsCollected)
  {
    if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Show references to asset",
          "Asset Viewer can stop responding while it collects asset references.\n"
          "Later searches will be fast until Asset Viewer restarts.\n\nContinue?") != wingw::MB_ID_YES)
      return false;

    collectAllAssetReferences(getAllAssetRefs(), asset.getMgr(), EDITORCORE->getConsole());
    allAssetRefsCollected = true;
  }

  return true;
}

void AssetBackReferenceViewer::setAsset(DagorAsset *asset)
{
  assetNameTypified = asset ? asset->getNameTypified() : String();
  setWindowSubtitle(asset ? asset->getName() : "");
  setWindowSubtitleIcon(asset ? AssetSelectorCommon::getAssetTypeIcon(asset->getType()) : PropPanel::IconId::Invalid);
  clearResults();

  if (asset)
    getBackReferences(*asset, getAllAssetRefs());

  fillResultsList();
}
