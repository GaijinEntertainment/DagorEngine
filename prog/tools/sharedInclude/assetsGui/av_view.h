#pragma once


#include <assetsGui/av_client.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class IMenu;
}
class AllAssetsTree;
class AssetTypeFilterControl;
class DagorAssetMgr;

#ifndef ImTextureID
typedef void *ImTextureID;
#endif

//==============================================================================
// AssetBaseView
//==============================================================================

class AssetBaseView : public PropPanel::ITreeViewEventHandler
{
public:
  AssetBaseView(IAssetBaseViewClient *client, IAssetSelectorContextMenuHandler *menu_event_handler);
  ~AssetBaseView();

  DagorAsset *getSelectedAsset() const;
  DagorAssetFolder *getSelectedAssetFolder() const;
  const char *getSelObjName();

  DagorAssetMgr *getCurAssetBase() { return curMgr; }

  AllAssetsTree &getTree() { return *assetsTree; }
  const AllAssetsTree &getTree() const { return *assetsTree; }

  void connectToAssetBase(DagorAssetMgr *mgr, dag::ConstSpan<int> allowed_type_indexes, const DataBlock *expansion_state = nullptr);

  bool selectAsset(const DagorAsset &asset, bool reset_filter_if_needed);
  bool selectObjInBase(const char *name);

  void saveTreeData(DataBlock &blk);
  void getTreeNodesExpand(Tab<bool> &exps);
  void setTreeNodesExpand(dag::ConstSpan<bool> exps);

  inline bool checkFiltersEq(dag::ConstSpan<int> types) { return types.size() == filter.size() && mem_eq(types, filter.data()); }
  void setFilterStr(const char *str);
  const String &getFilterStr() const { return textToSearch; }
  dag::ConstSpan<int> getFilter() const { return filter; }

  void setShownAssetTypeIndexes(dag::ConstSpan<int> shown_type_indexes);
  dag::ConstSpan<int> getShownAssetTypeIndexes() const { return curFilter; }

  void resetFilter();

  // ITreeViewEventHandler

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  virtual void onTvDClick(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle node) override;
  virtual bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

  // If control_height is 0 then it will use the entire available height.
  void updateImgui(float control_height = 0.0f);

  static void addCommonMenuItems(PropPanel::IMenu &menu);

private:
  Tab<int> filter;
  Tab<int> curFilter;
  Tab<bool> allowedTypes;
  Tab<bool> shownTypes;

  IAssetBaseViewClient *eh;
  IAssetSelectorContextMenuHandler *menuEventHandler;
  DagorAssetMgr *curMgr;

  eastl::unique_ptr<AllAssetsTree> assetsTree;
  eastl::unique_ptr<AssetTypeFilterControl> assetTypeFilterControl;

  bool canNotifySelChange;

  void fill(const DataBlock *expansion_state);

  void fillObjects(PropPanel::TLeafHandle parent);

  void onShownTypeFilterChanged();
  void showSettingsPanel(const char *popup_id);

  String textToSearch;
  String selectionBuffer;
  ImTextureID closeIcon = nullptr;
  ImTextureID searchIcon = nullptr;
  ImTextureID settingsIcon = nullptr;
  ImTextureID settingsOpenIcon = nullptr;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;
};
