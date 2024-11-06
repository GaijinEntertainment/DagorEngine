#pragma once


#include <assetsGui/av_client.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <generic/dag_tab.h>

namespace PropPanel
{
class IMenu;
}
class DagorAssetMgr;
class IMenuEventHandler;

struct AssetTreeRec
{
  PropPanel::TLeafHandle handle;
  int index;
};

//==============================================================================
// AssetBaseView
//==============================================================================

class AssetBaseView : public PropPanel::ITreeViewEventHandler
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
  SimpleString getFilterStr() const { return mTreeList->getFilterStr(); }
  dag::ConstSpan<int> getFilter() const { return filter; }
  dag::ConstSpan<int> getCurFilter() const { return curFilter; }

  // ITreeViewEventHandler

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  virtual void onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) override;
  virtual void onTvListDClick(PropPanel::TreeBaseWindow &tree, int index) override;
  virtual void onTvAssetTypeChange(PropPanel::TreeBaseWindow &tree, const Tab<String> &vals) override;
  virtual bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override { return true; }
  virtual bool onTvListContextMenu(PropPanel::TreeBaseWindow &tree, PropPanel::IListBoxInterface &list_box) override;

  // If control_height is 0 then it will use the entire available height.
  void updateImgui(float control_height = 0.0f);

  static void addCommonMenuItems(PropPanel::IMenu &menu);

private:
  Tab<int> filter;
  Tab<int> curFilter;
  Tab<AssetTreeRec> mAssetList;

  PropPanel::TLeafHandle addAsset(const char *name, PropPanel::TLeafHandle parent, int idx);

  IAssetBaseViewClient *eh;
  IMenuEventHandler *menuEventHandler;
  DagorAssetMgr *curMgr;

  PropPanel::TreeListWindow *mTreeList;

  bool canNotifySelChange;
  bool doShowEmptyGroups;

  void fill(dag::ConstSpan<int> types);
  void fillGroups(PropPanel::TLeafHandle parent, int folder_idx, dag::ConstSpan<int> only_folders);

  void fillObjects(PropPanel::TLeafHandle parent);

  char mSelBuf[64];
};
