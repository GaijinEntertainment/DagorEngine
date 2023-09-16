#pragma once


#include <assetsGui/av_client.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <generic/dag_tab.h>

class DagorAssetMgr;
class IMenuEventHandler;

struct AssetTreeRec
{
  TLeafHandle handle;
  int index;
};

//==============================================================================
// AssetBaseView
//==============================================================================

class AssetBaseView : public ITreeViewEventHandler
{
public:
  AssetBaseView(IAssetBaseViewClient *client, IMenuEventHandler *menu_event_handler, void *handle, int l, int t, int w, int h);
  ~AssetBaseView();

  const char *getSelGroupName();
  const char *getSelObjName();
  int getSelGroupId();

  inline const DagorAssetMgr *getCurAssetBase() { return curMgr; }

  void connectToAssetBase(DagorAssetMgr *mgr);
  void reloadBase();

  bool selectObjInBase(const char *name);
  // void selectObj(const char* name);

  void getTreeNodesExpand(Tab<bool> &exps);
  void setTreeNodesExpand(dag::ConstSpan<bool> exps);

  void resize(unsigned w, unsigned h);

  // call this function BEFORE connectToLib()
  inline void setFilter(int type)
  {
    filter.resize(1);
    filter[0] = type;
  }
  inline void setFilter(dag::ConstSpan<int> types) { filter = types; }
  inline void clearFilter() { clear_and_shrink(filter); }
  inline bool checkFiltersEq(dag::ConstSpan<int> types) { return types.size() == filter.size() && mem_eq(types, filter.data()); }
  inline void showEmptyGroups(bool show = true) { doShowEmptyGroups = show; }
  void setFilterStr(const char *str) { mTreeList->setFilterStr(str); }
  const char *getFilterStr() const { return mTreeList->getFilterStr(); }
  dag::ConstSpan<int> getCurFilter() const { return curFilter; }

  // ITreeViewEventHandler

  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel);
  virtual void onTvListSelection(TreeBaseWindow &tree, int index);
  virtual void onTvListDClick(TreeBaseWindow &tree, int index);
  virtual void onTvAssetTypeChange(TreeBaseWindow &tree, const Tab<String> &vals);
  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu) { return true; };
  virtual bool onTvListContextMenu(TreeBaseWindow &tree, int index, IMenu &menu) override;

private:
  Tab<int> filter;
  Tab<int> curFilter;
  Tab<AssetTreeRec> mAssetList;

  TLeafHandle addAsset(const char *name, TLeafHandle parent, int idx);

  IAssetBaseViewClient *eh;
  IMenuEventHandler *menuEventHandler;
  DagorAssetMgr *curMgr;

  TreeListWindow *mTreeList;

  bool canNotifySelChange;
  bool doShowEmptyGroups;

  void fill(dag::ConstSpan<int> types);
  void fillGroups(TLeafHandle parent, int folder_idx, dag::ConstSpan<int> only_folders);

  void fillObjects(TLeafHandle parent);

  char mSelBuf[64];
};
