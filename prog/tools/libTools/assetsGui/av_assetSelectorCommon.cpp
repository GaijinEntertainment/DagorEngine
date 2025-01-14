// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_assetSelectorCommon.h>

#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assets/assetUtils.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/propPanel.h>

dag::Vector<TEXTUREID> AssetSelectorCommon::assetTypeIcons;
dag::Vector<int> AssetSelectorCommon::allAssetTypeIndexes;

const char *const AssetSelectorCommon::searchTooltip =
  "Filter and search assets.\n\n"
  "Searching for multiple words are supported. Enter the words separated by space.\n"
  "The asset will be listed if it contains all the words. The order of the words does not matter.\n"
  "For example \"stone bridge\" (without the quotes) will match assets that contain both stone and bridge.\n"
  "So it will match both african_stone_bridge_a and bridge_stone_a_cmp.\n\n"
  "Wildcard characters are supported. \"*\" matches zero or more characters, \"?\" matches exactly one character.\n"
  "Examples:\n"
  "fireplace* will match assets that start with fireplace.\n"
  "*cmp will match assets that end with cmp.\n"
  "fireplace*cmp will match assets that start with fireplace and end with cmp.\n"
  "car?e* will match assets that start with car followed by any single character then \"e\" and can end with\n"
  "anything. So it will match career_stuff_a and carpet01.\n\n"
  "You can combine wildcard search and multiple word search. For example \"african* bridge\" will search for\n"
  "assets that start with african and contain bridge anywhere.\n\n"
  "Hotkeys:\n"
  "- Up arrow: go to the previous match\n"
  "- Down arrow: go to the next match\n"
  "- Shift+Enter: go to the previous match\n"
  "- Enter: go to the next match";

void AssetSelectorCommon::initialize(const DagorAssetMgr &asset_mgr)
{
  assetTypeIcons.resize(asset_mgr.getAssetTypesCount());
  for (int i = 0; i < asset_mgr.getAssetTypesCount(); ++i)
    assetTypeIcons[i] = PropPanel::load_icon(String(0, "asset_%s", asset_mgr.getAssetTypeName(i)));

  allAssetTypeIndexes.resize(asset_mgr.getAssetTypesCount());
  for (int i = 0; i < asset_mgr.getAssetTypesCount(); ++i)
    allAssetTypeIndexes[i] = i;
}

void AssetSelectorCommon::revealInExplorer(const DagorAsset &asset) { dag_reveal_in_explorer(&asset); }

void AssetSelectorCommon::revealInExplorer(const DagorAssetFolder &folder) { dag_reveal_in_explorer(&folder); }

void AssetSelectorCommon::copyAssetFilePathToClipboard(const DagorAsset &asset)
{
  String text(asset.isVirtual() ? asset.getTargetFilePath() : asset.getSrcFilePath());
  clipboard::set_clipboard_ansi_text(make_ms_slashes(text));
}

void AssetSelectorCommon::copyAssetFolderPathToClipboard(const DagorAsset &asset)
{
  String text(asset.getFolderPath());
  clipboard::set_clipboard_ansi_text(make_ms_slashes(text));
}

void AssetSelectorCommon::copyAssetFolderPathToClipboard(const DagorAssetFolder &folder)
{
  String text(folder.folderPath);
  clipboard::set_clipboard_ansi_text(make_ms_slashes(text));
}

void AssetSelectorCommon::copyAssetNameToClipboard(const DagorAsset &asset) { clipboard::set_clipboard_ansi_text(asset.getName()); }

void AssetSelectorCommon::copyAssetFolderNameToClipboard(const DagorAssetFolder &folder)
{
  clipboard::set_clipboard_ansi_text(folder.folderName);
}

const DagorAsset *AssetSelectorCommon::getAssetByName(const DagorAssetMgr &asset_mgr, const char *_name,
  dag::ConstSpan<int> asset_types)
{
  if (!_name || !_name[0])
    return nullptr;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (type)
  {
    const int assetType = asset_mgr.getAssetTypeId(type + 1);
    if (assetType == -1)
      return nullptr;

    for (int i = 0; i < asset_types.size(); i++)
      if (assetType == asset_types[i])
        return asset_mgr.findAsset(String::mk_sub_str(name, type), assetType);

    return nullptr;
  }

  return asset_mgr.findAsset(name, asset_types);
}
