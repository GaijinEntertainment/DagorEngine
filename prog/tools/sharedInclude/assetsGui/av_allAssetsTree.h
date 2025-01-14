// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_textFilter.h"
#include <dag/dag_vector.h>
#include <propPanel/commonWindow/treeviewPanel.h>

class DagorAsset;
class DagorAssetFolder;
class DagorAssetMgr;

class AllAssetsTree : public PropPanel::TreeBaseWindow
{
public:
  explicit AllAssetsTree(PropPanel::ITreeViewEventHandler *event_handler);

  void fill(DagorAssetMgr &asset_mgr, const DataBlock *expansion_state);
  void refilter();

  void saveTreeData(DataBlock &blk);
  void loadSelectedItem();

  void selectNextItem(bool forward = true);
  bool selectAsset(const DagorAsset *asset);
  void setShownTypes(dag::ConstSpan<bool> new_shown_types);
  void setSearchText(const char *text);

  DagorAsset *getSelectedAsset() const;
  DagorAssetFolder *getSelectedAssetFolder() const;
  bool isFolder(PropPanel::TLeafHandle node) const;

  virtual void updateImgui(float control_height = 0.0f) override;

  static const int IS_DAGOR_ASSET_FOLDER = 1;

protected:
  virtual bool handleNodeFilter(const PropPanel::TTreeNode &node) override;

private:
  PropPanel::TLeafHandle addGroup(int folder_idx, PropPanel::TLeafHandle parent, const DataBlock *blk);
  PropPanel::TLeafHandle addEntry(const DagorAsset *asset, PropPanel::TLeafHandle parent, bool selected);

  void scanOpenTree(PropPanel::TLeafHandle parent, DataBlock *blk);

  DagorAssetMgr *assetMgr = nullptr;
  PropPanel::TLeafHandle firstSel = nullptr;
  dag::Vector<bool> shownTypes;
  AssetsGuiTextFilter textFilter;
};

static inline DagorAssetFolder *get_dagor_asset_folder(void *v)
{
  return (((uintptr_t)v) & AllAssetsTree::IS_DAGOR_ASSET_FOLDER) ? (DagorAssetFolder *)((uintptr_t)v & ~1) : nullptr;
}
