#pragma once


#include <assetsGui/av_assetBrowser.h>
#include <assetsGui/av_view.h>
#include <assetsGui/av_tagManagement.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/customControl.h>
#include <propPanel/control/menu.h>
#include <generic/dag_span.h>
#include <util/dag_oaHashNameMap.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>

class DagorAsset;
class DagorAssetMgr;
class FavoritesTab;
class RecentlyUsedTab;
class SelectAssetDlgAssetBrowser;

struct SelectAssetDlgOptions
{
  dag::Vector<int> selectableAssetTypes; // default: all asset types are selectable
  String initiallySelectedAsset;         // default: nothing is selected
  String dialogCaption;                  // default: "Select asset"
  String selectButtonCaption;            // default: "Select asset"
  String resetButtonCaption;             // default: "Reset asset"
  String filter;                         // default: no filter
  String positionBesideWindow;           // default: the dialog is centered
  bool openAllGroups = false;            // default: groups are not opened by default
};

class SelectAssetDlg : public PropPanel::DialogWindow,
                       public IAssetBaseViewClient,
                       public IAssetBrowserHost,
                       public IAssetTagsView,
                       public IAssetSelectorContextMenuHandler,
                       public IAssetSelectorFavoritesRecentlyUsedHost,
                       public PropPanel::IMenuEventHandler,
                       public PropPanel::ICustomControl,
                       public PropPanel::IDelayedCallbackHandler
{
public:
  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, const char *caption, const char *sel_btn_caption, const char *reset_btn_caption,
    dag::ConstSpan<int> type_filter, const char *asset_browser_caption = nullptr);

  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, IAssetBaseViewClient *cli, const char *caption, dag::ConstSpan<int> type_filter,
    const char *asset_browser_caption = nullptr);

  ~SelectAssetDlg() override;

  bool onOk() override;
  bool onCancel() override { return true; }

  int closeReturn() override { return PropPanel::DIALOG_ID_CLOSE; }

  void selectObj(const char *name) { view->selectObjInBase(name); }
  DagorAsset *getSelectedAsset() const;
  const char *getSelObjName();

  void getTreeNodesExpand(Tab<bool> &exps) { view->getTreeNodesExpand(exps); }
  void setTreeNodesExpand(dag::ConstSpan<bool> exps) { view->setTreeNodesExpand(exps); }
  void expandNodesFromSelectionTillRoot() { view->expandNodesFromSelectionTillRoot(); }
  void setFilterStr(const char *str) { view->setFilterStr(str); }
  const String &getFilterStr() const { return view->getFilterStr(); }

  bool changeFilters(DagorAssetMgr *_mgr, dag::ConstSpan<int> type_filter);
  void addAssetToRecentlyUsed(const char *asset_name);

  const DagorAsset *getAssetByName(const char *_name, dag::ConstSpan<int> asset_types) const;

  const DagorAssetMgr *getAssetMgr() const { return mgr; }

  void saveAssetBrowserSettings(DataBlock &blk) const;
  void loadAssetBrowserSettings(DataBlock &blk);

  // IAssetBaseViewClient

  void onAvClose() override;
  void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override;
  void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override;

  // IAssetSelectorFavoritesRecentlyUsedHost

  dag::ConstSpan<int> getAllowedTypes() const override { return view->getFilter(); }
  void addAssetToFavorites(const DagorAsset &asset) override;
  void goToAsset(const DagorAsset &asset) override;
  void onSelectionChanged(const DagorAsset *asset) override;
  void onSelectionDoubleClicked(const DagorAsset *asset) override;

  // IAssetTagsView

  AssetTagManager *getVisibleTagManager() override;
  void showTagManager(bool show) override;

  void setAssetTagsView(IAssetTagsView *asset_tags_view);

  PropPanel::IconId getFolderTextureId() const { return folderTextureId; }

  static String getAssetNameWithTypeIfNeeded(const DagorAsset &asset, dag::ConstSpan<int> allowed_type_indexes);

protected:
  // ControlEventHandler
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IAssetSelectorFavoritesRecentlyUsedHost
  IAssetBrowserHost &getAssetBrowserHost() override { return *this; }
  IAssetSelectorContextMenuHandler &getAssetSelectorContextMenuHandler() override { return *this; }

  // IAssetBrowserHost
  void assetBrowserFill() override;
  bool assetBrowserIsOpen() const override;
  void assetBrowserSetOpen(bool open) override;
  void assetBrowserOnAssetSelected(DagorAsset *asset, bool double_clicked) override;
  void assetBrowserOnContextMenu(DagorAsset *asset) override;

  // IAssetSelectorContextMenuHandler
  bool onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder) override;

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  // PropPanel::ICustomControl
  void customControlUpdate(int id) override;

  void commonConstructor(dag::ConstSpan<int> filter, const char *asset_browser_caption);
  String getSelectedFavorite();
  String getSelectedRecentlyUsed();
  String getAssetNameWithTypeIfNeeded(const DagorAsset &asset);

  // IDelayedCallbackHandler
  void onImguiDelayedCallback(void *data) override;

  AssetTagManager *getTagManager();

  void updateImguiDialog() override;

  static PropPanel::TLeafHandle getTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name);
  static void selectTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name);

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 250;
  static const int DEFAULT_WIDTH = 440;

  IAssetBaseViewClient *client;
  AssetBaseView *view;
  DagorAssetMgr *mgr;
  eastl::unique_ptr<FavoritesTab> favoritesTab;
  eastl::unique_ptr<RecentlyUsedTab> recentlyUsedTab;
  eastl::unique_ptr<SelectAssetDlgAssetBrowser> assetBrowserDlg;
  const DagorAsset *lastSelectedAsset = nullptr;
  String selectionBuffer;
  PropPanel::IconId folderTextureId = PropPanel::IconId::Invalid;
  bool favoritesTabNeedsRefilling = true;
  bool recentlyUsedTabNeedsRefilling = true;
  bool assetBrowserOpeningRequested = false;
  AssetTagsDlg *tagsDlg = nullptr;
};

eastl::unique_ptr<SelectAssetDlg> assets_gui_create_asset_selector_dialog(const SelectAssetDlgOptions &options, DagorAssetMgr &mgr);
