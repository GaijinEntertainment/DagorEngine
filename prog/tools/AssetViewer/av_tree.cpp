// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_tree.h"
#include "assetBuildCache.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <util/dag_string.h>
#include <propPanel/propPanel.h>

static TEXTUREID folder_textureId = BAD_TEXTUREID;
static TEXTUREID folder_packed_textureId = BAD_TEXTUREID;
static TEXTUREID folder_changed_textureId = BAD_TEXTUREID;
static Tab<TEXTUREID> asset_changed_textureId(inimem);


void AvTree::loadIcons(DagorAssetMgr &asset_mgr)
{
  folder_textureId = PropPanel::load_icon("folder");
  folder_packed_textureId = PropPanel::load_icon("folder_packed");
  folder_changed_textureId = PropPanel::load_icon("folder_changed");

  asset_changed_textureId.resize(asset_mgr.getAssetTypesCount());
  for (int i = 0; i < asset_mgr.getAssetTypesCount(); i++)
    asset_changed_textureId[i] = PropPanel::load_icon(String(0, "asset_%s_changed", asset_mgr.getAssetTypeName(i)));
}


bool AvTree::isFolderExportable(const AllAssetsTree &tree, PropPanel::TLeafHandle parent, bool *exported)
{
  for (int i = 0; i < tree.getChildrenCount(parent); ++i)
  {
    PropPanel::TLeafHandle child = tree.getChild(parent, i);
    void *data = tree.getItemData(child);
    if (get_dagor_asset_folder(data)) // folder
    {
      if (isFolderExportable(tree, child))
        return true;
    }
    else // asset
    {
      DagorAsset *a = (DagorAsset *)data;
      if (a && ::is_asset_exportable(a))
      {
        if (exported)
          *exported = true;
        return true;
      }
    }
  }

  if (exported)
    *exported = false;

  return false;
}


bool AvTree::markExportedTree(AllAssetsTree &tree, PropPanel::TLeafHandle parent, int flags)
{
  bool changed = false;

  const int childCount = tree.getChildrenCount(parent);
  for (int i = 0; i < childCount; ++i)
  {
    PropPanel::TLeafHandle child = tree.getChild(parent, i);
    PropPanel::TTreeNode *childTreeNode = tree.getItemNode(child);

    if (const DagorAssetFolder *g = get_dagor_asset_folder(childTreeNode->userData))
    {
      TEXTUREID imidx = (g->flags & g->FLG_EXPORT_ASSETS) ? folder_packed_textureId : folder_textureId;

      if (markExportedTree(tree, child, flags))
        imidx = folder_changed_textureId;

      if (imidx != childTreeNode->icon && childTreeNode->item)
        tree.changeItemImage(child, imidx);
    }
    else if (DagorAsset *a = (DagorAsset *)childTreeNode->userData)
    {
      TEXTUREID imidx;

      if (::is_asset_exportable(a) && (a->testUserFlags(flags) != flags))
      {
        changed = true;
        imidx = asset_changed_textureId[a->getType()];
      }
      else
      {
        imidx = AssetSelectorCommon::getAssetTypeIcon(a->getType());
      }

      if (imidx != childTreeNode->icon && childTreeNode->item)
        tree.changeItemImage(child, imidx);
    }
  }

  return changed;
}
