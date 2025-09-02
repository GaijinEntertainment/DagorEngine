// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

namespace PropPanel
{
enum class IconId : int;
}

class DagorAsset;
class DagorAssetFolder;
class DagorAssetMgr;

class AssetSelectorCommon
{
public:
  static void initialize(const DagorAssetMgr &asset_mgr);

  static void revealInExplorer(const DagorAsset &asset);
  static void revealInExplorer(const DagorAssetFolder &folder);
  static void copyAssetFilePathToClipboard(const DagorAsset &asset);
  static void copyAssetFolderPathToClipboard(const DagorAsset &asset);
  static void copyAssetFolderPathToClipboard(const DagorAssetFolder &folder);
  static void copyAssetNameToClipboard(const DagorAsset &asset);
  static void copyAssetFolderNameToClipboard(const DagorAssetFolder &folder);
  static const DagorAsset *getAssetByName(const DagorAssetMgr &asset_mgr, const char *_name, dag::ConstSpan<int> asset_types);
  static PropPanel::IconId getAssetTypeIcon(int type) { return assetTypeIcons[type]; }
  static dag::ConstSpan<int> getAllAssetTypeIndexes() { return allAssetTypeIndexes; }

  static const char *const searchTooltip;

private:
  static dag::Vector<PropPanel::IconId> assetTypeIcons;
  static dag::Vector<int> allAssetTypeIndexes;
};
