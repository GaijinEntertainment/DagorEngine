#pragma once


#include <assetsGui/av_client.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/propPanel.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_fastIntList.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class IMenu;
}
class AllAssetsTree;
class AssetTypeFilterControl;
class DagorAssetMgr;
class AssetTagManager;
class IAssetBrowserHost;

#ifndef ImTextureID
typedef void *ImTextureID;
#endif

//==============================================================================
// IAssetTagsView
//==============================================================================

class IAssetTagsView
{
public:
  virtual AssetTagManager *getVisibleTagManager() = 0;
  virtual void showTagManager(bool show) = 0;
};

//==============================================================================
// AssetBaseView
//==============================================================================

class AssetBaseView : public PropPanel::ITreeViewEventHandler
{
public:
  AssetBaseView(IAssetBaseViewClient *client, IAssetSelectorContextMenuHandler *menu_event_handler,
    IAssetBrowserHost &asset_browser_host);
  ~AssetBaseView();

  DagorAsset *getSelectedAsset() const;
  DagorAssetFolder *getSelectedAssetFolder() const;
  const char *getSelObjName();
  void getFilteredAssetsFromTheCurrentFolder(dag::Vector<DagorAsset *> &assets) const;

  DagorAssetMgr *getCurAssetBase() { return curMgr; }

  AllAssetsTree &getTree() { return *assetsTree; }
  const AllAssetsTree &getTree() const { return *assetsTree; }

  void connectToAssetBase(DagorAssetMgr *mgr, dag::ConstSpan<int> allowed_type_indexes, const DataBlock *expansion_state = nullptr);

  bool selectAsset(const DagorAsset &asset, bool reset_filter_if_needed);
  bool selectObjInBase(const char *name);

  void saveTreeData(DataBlock &blk);
  void getTreeNodesExpand(Tab<bool> &exps);
  void setTreeNodesExpand(dag::ConstSpan<bool> exps);
  void expandNodesFromSelectionTillRoot();

  inline bool checkFiltersEq(dag::ConstSpan<int> types) { return types.size() == filter.size() && mem_eq(types, filter.data()); }
  void setFilterStr(const char *str);
  const String &getFilterStr() const { return textToSearch; }
  dag::ConstSpan<int> getFilter() const { return filter; }

  void setShownAssetTypeIndexes(dag::ConstSpan<int> shown_type_indexes);
  dag::ConstSpan<int> getShownAssetTypeIndexes() const { return curFilter; }

  void resetFilter();

  // ITreeViewEventHandler

  void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  void onTvDClick(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle node) override;
  bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

  IAssetTagsView *getAssetTagsView() const { return assetTagsView; }
  void setAssetTagsView(IAssetTagsView *asset_tags_view);

  bool getTagsToggled() const { return tagsToggled; }
  void setTagsToggled(bool toggled);

  // If control_height is 0 then it will use the entire available height.
  void updateImgui(float control_height = 0.0f);

  static void addCommonMenuItems(PropPanel::IMenu &menu, bool add_tags = false);

private:
  Tab<int> filter;
  Tab<int> curFilter;
  Tab<bool> allowedTypes;
  Tab<bool> shownTypes;
  FastIntList tagFilter;

  IAssetBaseViewClient *eh;
  IAssetSelectorContextMenuHandler *menuEventHandler;
  IAssetBrowserHost &assetBrowserHost;
  DagorAssetMgr *curMgr;

  eastl::unique_ptr<AllAssetsTree> assetsTree;
  eastl::unique_ptr<AssetTypeFilterControl> assetTypeFilterControl;

  bool canNotifySelChange;

  void fill(const DataBlock *expansion_state);

  void refilterAssetsTree();
  void onShownTypeFilterChanged();
  void showSettingsPanel(const char *popup_id);

  String textToSearch;
  String selectionBuffer;
  PropPanel::IconId closeIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId searchIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId settingsIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId settingsOpenIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId tagsToggleIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId assetBrowserToggleIcon = PropPanel::IconId::Invalid;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;

  IAssetTagsView *assetTagsView = nullptr;
  bool tagsToggled = false;
  int tagsVersion = 0;
};
