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
                       public IAssetSelectorContextMenuHandler,
                       public IAssetSelectorFavoritesRecentlyUsedHost,
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
  DagorAsset *getSelectedAsset() const;
  const char *getSelObjName();

  void getTreeNodesExpand(Tab<bool> &exps) { view->getTreeNodesExpand(exps); }
  void setTreeNodesExpand(dag::ConstSpan<bool> exps) { view->setTreeNodesExpand(exps); }
  void setFilterStr(const char *str) { view->setFilterStr(str); }
  const String &getFilterStr() const { return view->getFilterStr(); }

  bool changeFilters(DagorAssetMgr *_mgr, dag::ConstSpan<int> type_filter);
  void addAssetToRecentlyUsed(const char *asset_name);

  const DagorAsset *getAssetByName(const char *_name, dag::ConstSpan<int> asset_types) const;

  const DagorAssetMgr *getAssetMgr() const { return mgr; }

  // IAssetBaseViewClient

  virtual void onAvClose() override {}
  virtual void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override;
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override {}
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override {}

  // IAssetSelectorFavoritesRecentlyUsedHost

  virtual dag::ConstSpan<int> getAllowedTypes() const override { return view->getFilter(); }
  virtual void addAssetToFavorites(const DagorAsset &asset) override;
  virtual void goToAsset(const DagorAsset &asset) override;
  virtual void onSelectionChanged(const DagorAsset *asset) override;
  virtual void onSelectionDoubleClicked(const DagorAsset *asset) override;

  TEXTUREID getFolderTextureId() const { return folderTextureId; }

  static String getAssetNameWithTypeIfNeeded(const DagorAsset &asset, dag::ConstSpan<int> allowed_type_indexes);

private:
  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IAssetSelectorContextMenuHandler
  virtual bool onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

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
