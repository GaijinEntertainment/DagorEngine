#pragma once


#include <assetsGui/av_view.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <generic/dag_span.h>
#include <sepGui/wndMenuInterface.h>
#include <util/dag_oaHashNameMap.h>

class DagorAsset;
class DagorAssetMgr;

class SelectAssetDlg : public CDialogWindow, public IAssetBaseViewClient, public ITreeViewEventHandler, public IMenuEventHandler
{
public:
  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, const char *caption, const char *sel_btn_caption, const char *reset_btn_caption,
    dag::ConstSpan<int> type_filter, int x, int y, unsigned w, unsigned h);

  SelectAssetDlg(void *phandle, DagorAssetMgr *mgr, IAssetBaseViewClient *cli, const char *caption, dag::ConstSpan<int> type_filter,
    int x, int y, unsigned w, unsigned h);

  ~SelectAssetDlg();

  virtual bool onOk();
  virtual bool onCancel() { return true; }

  virtual int closeReturn() { return DIALOG_ID_CLOSE; }

  void selectObj(const char *name) { view->selectObjInBase(name); }
  const char *getSelObjName();

  void getTreeNodesExpand(Tab<bool> &exps) { view->getTreeNodesExpand(exps); }
  void setTreeNodesExpand(dag::ConstSpan<bool> exps) { view->setTreeNodesExpand(exps); }
  void setFilterStr(const char *str) { view->setFilterStr(str); }
  const char *getFilterStr() const { return view->getFilterStr(); }

  bool changeFilters(DagorAssetMgr *_mgr, dag::ConstSpan<int> type_filter);
  void addAssetToRecentlyUsed(const char *asset_name);

  virtual void resizeWindow(unsigned w, unsigned h, bool internal = false);

  // IAssetBaseViewClient

  virtual void onAvClose() {}
  virtual void onAvAssetDblClick(const char *asset_name);
  virtual void onAvSelectAsset(const char *asset_name) {}
  virtual void onAvSelectFolder(const char *asset_folder_name) {}

private:
  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel2 *panel) override;

  // ITreeViewEventHandler
  virtual void onTvDClick(TreeBaseWindow &tree, TLeafHandle node) override;
  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel) override;
  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu) override;

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id) override;

  void commonConstructor(dag::ConstSpan<int> filter);
  void fillFavoritesTree();
  String getSelectedFavorite();
  int getRecentlyUsedImageIndex(const DagorAsset &asset);
  void fillRecentlyUsedTree();
  String getSelectedRecentlyUsed();

  static TLeafHandle getTreeItemByName(TreeBaseWindow &tree, const String &name);
  static void selectTreeItemByName(TreeBaseWindow &tree, const String &name);

  IAssetBaseViewClient *client;
  AssetBaseView *view;
  DagorAssetMgr *mgr;
  TreeBaseWindow *favoritesTree;
  FastNameMapEx favoritesTreeImageMap;
  dag::Vector<int> favoritesTreeAllowedTypes;
  TreeBaseWindow *recentlyUsedTree;
  FastNameMapEx recentlyUsedTreeImageMap;
  dag::Vector<int> recentlyUsedTreeAllowedTypes;
  String selectionBuffer;
};
