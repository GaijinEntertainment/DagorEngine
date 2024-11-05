// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/treeviewPanel.h>
#include <util/dag_simpleString.h>
#include <winGuiWrapper/wgw_timer.h>

class DagorAsset;
class DagorAssetMgr;
class IGenEditorPlugin;


struct ImageIndex
{
  ImageIndex() {}

  ImageIndex(const char *name, TEXTUREID id) : mName(name), mId(id) {}

  SimpleString mName;
  TEXTUREID mId;
};


class AvTree : public PropPanel::TreeViewWindow, public ITimerCallBack
{
public:
  AvTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]);
  ~AvTree();

  void fill(DagorAssetMgr *mgr, const DataBlock &set_blk);
  bool markExportedTree(PropPanel::TTreeNode *n, int flags);
  bool isFolderExportable(PropPanel::TLeafHandle parent, bool *exported = NULL);

  void saveTreeData(DataBlock &blk);
  void loadSelectedItem();

  void resetFilter();
  void getFilterAssetTypes(Tab<int> &types);
  void setFilterAssetTypes(const Tab<int> &types);
  SimpleString getFilterString();
  void setFilterString(const char *filter);

  void searchNext(const char *text, bool forward);
  bool selectAsset(const DagorAsset *a);

  virtual void updateImgui(float control_height = 0.0f) override;

  static const int IS_DAGOR_ASSET_FOLDER = 1;

protected:
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual long onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key);

  virtual bool handleNodeFilter(const PropPanel::TTreeNode &node) override;

  // ITimerCallBack
  virtual void update() override;

  TEXTUREID getIconTextureId(const char *image_file_name);

  Tab<int> mSelectedTypesID;
  SimpleString mFilterString;
  bool filterHasWildcardCharacter = false;

private:
  PropPanel::TLeafHandle addGroup(int folder_idx, PropPanel::TLeafHandle parent, const DataBlock *blk);
  PropPanel::TLeafHandle addEntry(const DagorAsset *asset, PropPanel::TLeafHandle parent, bool selected);

  void scanOpenTree(PropPanel::TLeafHandle parent, DataBlock *blk);

  void setTypeFilterToAll();
  void setCaptionFilterButton();
  void onFilterEditBoxChanged();

private:
  eastl::unique_ptr<PropPanel::ContainerPropertyControl> mPanelFS;
  DagorAssetMgr *mDAMgr;
  Tab<ImageIndex> mImages;
  PropPanel::TLeafHandle mFirstSel;
  WinTimer filterTimer;
  float panelLastHeight = 0.0f;
};

static inline const class DagorAssetFolder *get_dagor_asset_folder(void *v)
{
  return (((uintptr_t)v) & AvTree::IS_DAGOR_ASSET_FOLDER) ? (DagorAssetFolder *)((uintptr_t)v & ~1) : NULL;
}
