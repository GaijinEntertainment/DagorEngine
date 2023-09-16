#pragma once

#include <propPanel2/comWnd/treeview_panel.h>
#include <util/dag_simpleString.h>
#include <winGuiWrapper/wgw_timer.h>

class DagorAsset;
class DagorAssetMgr;
class IGenEditorPlugin;


struct ImageIndex
{
  ImageIndex() {}

  ImageIndex(const char *name, int index) : mName(name), mIndex(index) {}

  SimpleString mName;
  int mIndex;
};


class AvTree : public TreeViewWindow, public ITimerCallBack
{
public:
  AvTree(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]);
  ~AvTree();

  void fill(DagorAssetMgr *mgr, const DataBlock &set_blk);
  int getImageIndex(const char *image_file_name);
  bool markExportedTree(TTreeNode *n, int flags);
  bool isFolderExportable(TLeafHandle parent, bool *exported = NULL);

  void saveTreeData(DataBlock &blk);
  void loadSelectedItem();

  void resetFilter();
  void getFilterAssetTypes(Tab<int> &types);
  void setFilterAssetTypes(const Tab<int> &types);
  SimpleString getFilterString();
  void setFilterString(const char *filter);

  void searchNext(const char *text, bool forward);
  bool selectAsset(const DagorAsset *a);

  static const int IS_DAGOR_ASSET_FOLDER = 1;

protected:
  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual long onKeyDown(int pcb_id, PropertyContainerControlBase *panel, unsigned v_key);
  virtual bool handleNodeFilter(TTreeNodeInfo &node);

  // ITimerCallBack
  virtual void update() override;

  Tab<int> mSelectedTypesID;
  SimpleString mFilterString;

private:
  TLeafHandle addGroup(int folder_idx, TLeafHandle parent, const DataBlock *blk);
  TLeafHandle addEntry(const DagorAsset *asset, TLeafHandle parent, bool selected);

  void scanOpenTree(TLeafHandle parent, DataBlock *blk);

  void setTypeFilterToAll();
  void setCaptionFilterButton();
  void onFilterEditBoxChanged();

private:
  DagorAssetMgr *mDAMgr;
  Tab<ImageIndex> mImages;
  TLeafHandle mFirstSel;
  WinTimer filterTimer;
};

static inline const class DagorAssetFolder *get_dagor_asset_folder(void *v)
{
  return (((uintptr_t)v) & AvTree::IS_DAGOR_ASSET_FOLDER) ? (DagorAssetFolder *)((uintptr_t)v & ~1) : NULL;
}
