#pragma once


#include <assetsGui/av_view.h>
#include <drv/3d/dag_resId.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/customControl.h>
#include <generic/dag_span.h>
#include <sepGui/wndMenuInterface.h>
#include <util/dag_oaHashNameMap.h>
#include <EASTL/unique_ptr.h>

class DagorAsset;
class DagorAssetMgr;
class FavoritesTab;
class RecentlyUsedTab;

class SelectAssetDlg : public PropPanel::DialogWindow,
                       public IAssetBaseViewClient,
                       public IMenuEventHandler,
                       public PropPanel::ICustomControl
{
public:
  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, const char *caption, const char *sel_btn_caption, const char *reset_btn_caption,
    dag::ConstSpan<int> type_filter, int x = 0, int y = 0, unsigned w = 0, unsigned h = 0);

  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, IAssetBaseViewClient *cli, const char *caption, dag::ConstSpan<int> type_filter,
    int x = 0, int y = 0, unsigned w = 0, unsigned h = 0);

  ~SelectAssetDlg();

  virtual bool onOk() override;
  virtual bool onCancel() override { return true; }

  virtual int closeReturn() override { return PropPanel::DIALOG_ID_CLOSE; }

  void selectObj(const char *name) { view->selectObjInBase(name); }
  const char *getSelObjName();

  void getTreeNodesExpand(Tab<bool> &exps) { view->getTreeNodesExpand(exps); }
  void setTreeNodesExpand(dag::ConstSpan<bool> exps) { view->setTreeNodesExpand(exps); }
  void setFilterStr(const char *str) { view->setFilterStr(str); }
  SimpleString getFilterStr() const { return view->getFilterStr(); }
  dag::ConstSpan<int> getAllowedTypes() const { return view->getFilter(); }

  bool changeFilters(DagorAssetMgr *_mgr, dag::ConstSpan<int> type_filter);
  void addAssetToFavorites(const DagorAsset &asset);
  void addAssetToRecentlyUsed(const char *asset_name);
  void goToAsset(const DagorAsset &asset);

  void onFavoriteSelectionChanged(const DagorAsset *asset);
  void onFavoriteSelectionDoubleClicked(const DagorAsset *asset);
  void onRecentlyUsedSelectionChanged(const DagorAsset *asset);
  void onRecentlyUsedSelectionDoubleClicked(const DagorAsset *asset);

  const DagorAsset *getAssetByName(const char *_name, dag::ConstSpan<int> asset_types) const;

  const DagorAssetMgr *getAssetMgr() const { return mgr; }

  // IAssetBaseViewClient

  virtual void onAvClose() override {}
  virtual void onAvAssetDblClick(const char *asset_name) override;
  virtual void onAvSelectAsset(const char *asset_name) override {}
  virtual void onAvSelectFolder(const char *asset_folder_name) override {}

  TEXTUREID getFolderTextureId() const { return folderTextureId; }

  static void getAssetImageName(String &image_name, const DagorAsset *asset);
  static void revealInExplorer(const DagorAsset &asset);
  static void copyAssetFilePathToClipboard(const DagorAsset &asset);
  static void copyAssetFolderPathToClipboard(const DagorAsset &asset);
  static void copyAssetNameToClipboard(const DagorAsset &asset);
  static bool matchesSearchText(const char *haystack, const char *needle);

private:
  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id) override;

  // PropPanel::ICustomControl
  virtual void customControlUpdate(int id) override;

  void commonConstructor(dag::ConstSpan<int> filter);
  String getSelectedFavorite();
  String getSelectedRecentlyUsed();
  String getAssetNameWithTypeIfNeeded(const DagorAsset &asset);

  static PropPanel::TLeafHandle getTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name);
  static void selectTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name);

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 450;

  IAssetBaseViewClient *client;
  AssetBaseView *view;
  DagorAssetMgr *mgr;
  eastl::unique_ptr<FavoritesTab> favoritesTab;
  eastl::unique_ptr<RecentlyUsedTab> recentlyUsedTab;
  String selectionBuffer;
  TEXTUREID folderTextureId = BAD_TEXTUREID;
  bool favoritesTabNeedsRefilling = true;
  bool recentlyUsedTabNeedsRefilling = true;
};
