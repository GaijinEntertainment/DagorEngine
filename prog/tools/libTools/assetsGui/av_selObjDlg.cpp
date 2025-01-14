// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_selObjDlg.h>

#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_favoritesTab.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_recentlyUsedTab.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetUtils.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_shellExecute.h>
#include <util/dag_globDef.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/propPanel.h>

#include <imgui/imgui.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <Shlobj.h>
#endif

SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, const char *caption, const char *sel_btn_caption,
  const char *reset_btn_caption, dag::ConstSpan<int> filter, int x, int y, unsigned w, unsigned h) :

  DialogWindow(phandle, x, y, w > 0 ? _pxActual(w) : _pxScaled(DEFAULT_MINIMUM_WIDTH),
    h > 0 ? _pxActual(h) : _pxScaled(SelectAssetDlg::DEFAULT_MINIMUM_HEIGHT), caption),
  mgr(_mgr),
  view(nullptr)
{
  client = this;

  PropPanel::ContainerPropertyControl *_bp = buttonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  _bp->setCaption(PropPanel::DIALOG_ID_OK, sel_btn_caption);
  _bp->setCaption(PropPanel::DIALOG_ID_CANCEL, reset_btn_caption);

  commonConstructor(filter);
}


SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, IAssetBaseViewClient *cli, const char *caption,
  dag::ConstSpan<int> filter, int x, int y, unsigned w, unsigned h) :

  DialogWindow(phandle, x, y, w > 0 ? _pxActual(w) : _pxScaled(DEFAULT_MINIMUM_WIDTH),
    h > 0 ? _pxActual(h) : _pxScaled(SelectAssetDlg::DEFAULT_MINIMUM_HEIGHT), caption),
  mgr(_mgr),
  view(NULL)
{
  client = cli ? cli : this;

  PropPanel::ContainerPropertyControl *_bp = buttonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  showButtonPanel(false);

  commonConstructor(filter);
}


void SelectAssetDlg::commonConstructor(dag::ConstSpan<int> filter)
{
  AssetSelectorCommon::initialize(*mgr);

  folderTextureId = PropPanel::load_icon("folder");

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel && "SelectAssetDlg::commonConstructor : No panel with controls");

  PropPanel::ContainerPropertyControl *tabPanel = panel->createTabPanel(AssetsGuiIds::TabPanel, "");
  PropPanel::ContainerPropertyControl *allTabPage = tabPanel->createTabPage(AssetsGuiIds::AllPage, "All");

  view = new AssetBaseView(client, this);

  PropPanel::ContainerPropertyControl *favoritesTabPage = tabPanel->createTabPage(AssetsGuiIds::FavoritesPage, "Favorites");
  PropPanel::ContainerPropertyControl *recentlyUsedTabPage = tabPanel->createTabPage(AssetsGuiIds::RecentlyUsedPage, "Recently used");

  G_ASSERT(view);

  view->connectToAssetBase(mgr, filter);

  panel->createCustomControlHolder(AssetsGuiIds::CustomControlHolder, this);
}


DagorAsset *SelectAssetDlg::getSelectedAsset() const
{
  const int currentPage = const_cast<SelectAssetDlg *>(this)->getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage == AssetsGuiIds::FavoritesPage)
    return favoritesTab ? favoritesTab->getSelectedAsset() : nullptr;
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage)
    return recentlyUsedTab ? recentlyUsedTab->getSelectedAsset() : nullptr;

  return view->getSelectedAsset();
}


const char *SelectAssetDlg::getSelObjName()
{
  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage == AssetsGuiIds::FavoritesPage)
  {
    selectionBuffer = getSelectedFavorite();
    return selectionBuffer;
  }
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage)
  {
    selectionBuffer = getSelectedRecentlyUsed();
    return selectionBuffer;
  }

  return view->getSelObjName();
}


bool SelectAssetDlg::changeFilters(DagorAssetMgr *_mgr, dag::ConstSpan<int> type_filter)
{
  if (view->checkFiltersEq(type_filter))
    return false;
  mgr = _mgr;
  view->connectToAssetBase(mgr, type_filter);
  if (favoritesTab)
    favoritesTab->onAllowedTypesChanged(type_filter);
  if (recentlyUsedTab)
    recentlyUsedTab->onAllowedTypesChanged(type_filter);
  return true;
}


SelectAssetDlg::~SelectAssetDlg() { del_it(view); }


bool SelectAssetDlg::onOk()
{
  AssetSelectorGlobalState::addRecentlyUsed(String(getSelObjName()));

  return true;
}


void SelectAssetDlg::onAvAssetDblClick(DagorAsset *asset, const char *obj_name) { clickDialogButton(PropPanel::DIALOG_ID_OK); }


int SelectAssetDlg::onMenuItemClick(unsigned id)
{
  if (id == AssetsGuiIds::AddToFavoritesMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      addAssetToFavorites(*asset);
  }
  else if (id == AssetsGuiIds::GoToAssetInSelectorMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      goToAsset(*asset);
  }
  else if (id == AssetsGuiIds::CopyAssetFilePathMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      AssetSelectorCommon::copyAssetFilePathToClipboard(*asset);
  }
  else if (id == AssetsGuiIds::CopyAssetFolderPathMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      AssetSelectorCommon::copyAssetFolderPathToClipboard(*asset);
    else if (const DagorAssetFolder *folder = view->getSelectedAssetFolder())
      AssetSelectorCommon::copyAssetFolderPathToClipboard(*folder);
  }
  else if (id == AssetsGuiIds::CopyAssetNameMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      AssetSelectorCommon::copyAssetNameToClipboard(*asset);
    else if (const DagorAssetFolder *folder = view->getSelectedAssetFolder())
      AssetSelectorCommon::copyAssetFolderNameToClipboard(*folder);
  }
  else if (id == AssetsGuiIds::RevealInExplorerMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      AssetSelectorCommon::revealInExplorer(*asset);
    else if (const DagorAssetFolder *folder = view->getSelectedAssetFolder())
      AssetSelectorCommon::revealInExplorer(*folder);
  }
  else if (id == AssetsGuiIds::ExpandChildrenMenuItem || id == AssetsGuiIds::CollapseChildrenMenuItem)
  {
    AllAssetsTree &tree = view->getTree();
    const PropPanel::TLeafHandle selectedItem = tree.getSelectedItem();
    if (tree.isFolder(selectedItem))
    {
      const bool expand = id == AssetsGuiIds::ExpandChildrenMenuItem;
      const int childCount = tree.getChildrenCount(selectedItem);
      for (int i = 0; i < childCount; ++i)
        tree.expandRecursive(tree.getChild(selectedItem, i), expand);
    }
  }

  return 0;
}

bool SelectAssetDlg::onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree)
{
  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage != AssetsGuiIds::AllPage)
    return false;

  PropPanel::IMenu &menu = tree.createContextMenu();
  menu.setEventHandler(this);

  if (const DagorAssetFolder *folder = view->getSelectedAssetFolder())
  {
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::ExpandChildrenMenuItem, "Expand children");
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CollapseChildrenMenuItem, "Collapse children");
    menu.addSeparator(ROOT_MENU_ITEM);

    menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
    menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
    menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");

    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
  }
  else
  {
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
    AssetBaseView::addCommonMenuItems(menu);
  }

  return true;
}

void SelectAssetDlg::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == AssetsGuiIds::TabPanel)
  {
    const int newPage = panel->getInt(pcb_id);
    if (newPage == AssetsGuiIds::FavoritesPage)
    {
      if (!favoritesTab)
      {
        favoritesTab.reset(new FavoritesTab(*this, *mgr));
        favoritesTab->onAllowedTypesChanged(view->getFilter());
        favoritesTabNeedsRefilling = false;
      }

      if (favoritesTabNeedsRefilling)
      {
        favoritesTabNeedsRefilling = false;
        favoritesTab->fillTree();
      }
    }
    else if (newPage == AssetsGuiIds::RecentlyUsedPage)
    {
      if (!recentlyUsedTab)
      {
        recentlyUsedTab.reset(new RecentlyUsedTab(*this, *mgr));
        recentlyUsedTab->onAllowedTypesChanged(view->getFilter());
        recentlyUsedTabNeedsRefilling = false;
      }

      if (recentlyUsedTabNeedsRefilling)
      {
        recentlyUsedTabNeedsRefilling = false;
        recentlyUsedTab->fillTree();
      }
    }

    client->onAvSelectAsset(getSelectedAsset(), getSelObjName());
  }
}

String SelectAssetDlg::getSelectedFavorite()
{
  const DagorAsset *asset = favoritesTab ? favoritesTab->getSelectedAsset() : nullptr;
  return asset ? getAssetNameWithTypeIfNeeded(*asset) : String();
}

void SelectAssetDlg::addAssetToFavorites(const DagorAsset &asset)
{
  AssetSelectorGlobalState::addFavorite(asset.getNameTypified());
  favoritesTabNeedsRefilling = true;
}

void SelectAssetDlg::addAssetToRecentlyUsed(const char *asset_name)
{
  const String assetName(asset_name);
  if (!AssetSelectorGlobalState::addRecentlyUsed(assetName))
    return;

  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage == AssetsGuiIds::RecentlyUsedPage)
    recentlyUsedTab->fillTree();
  else
    recentlyUsedTabNeedsRefilling = true;
}

void SelectAssetDlg::goToAsset(const DagorAsset &asset)
{
  getPanel()->setInt(AssetsGuiIds::TabPanel, AssetsGuiIds::AllPage);
  selectObj(asset.getNameTypified());
}

void SelectAssetDlg::onSelectionChanged(const DagorAsset *asset)
{
  if (asset)
    client->onAvSelectAsset(const_cast<DagorAsset *>(asset), getAssetNameWithTypeIfNeeded(*asset));
}

void SelectAssetDlg::onSelectionDoubleClicked(const DagorAsset *asset)
{
  if (asset)
    client->onAvAssetDblClick(const_cast<DagorAsset *>(asset), getAssetNameWithTypeIfNeeded(*asset));
}

String SelectAssetDlg::getSelectedRecentlyUsed()
{
  DagorAsset *asset = recentlyUsedTab ? recentlyUsedTab->getSelectedAsset() : nullptr;
  return asset ? getAssetNameWithTypeIfNeeded(*asset) : String();
}

String SelectAssetDlg::getAssetNameWithTypeIfNeeded(const DagorAsset &asset, dag::ConstSpan<int> allowed_type_indexes)
{
  if (asset.getMgr().isAssetNameUnique(asset, allowed_type_indexes))
    return String(asset.getName());
  else
    return asset.getNameTypified();
}

String SelectAssetDlg::getAssetNameWithTypeIfNeeded(const DagorAsset &asset)
{
  return getAssetNameWithTypeIfNeeded(asset, view->getFilter());
}

PropPanel::TLeafHandle SelectAssetDlg::getTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name)
{
  PropPanel::TLeafHandle rootItem = tree.getRoot();
  PropPanel::TLeafHandle item = rootItem;
  while (true)
  {
    if (tree.getItemName(item) == name)
      return item;

    item = tree.getNextNode(item, true);
    if (!item || item == rootItem)
      return nullptr;
  }
}

void SelectAssetDlg::selectTreeItemByName(PropPanel::TreeBaseWindow &tree, const String &name)
{
  PropPanel::TLeafHandle item = getTreeItemByName(tree, name);
  if (item)
    tree.setSelectedItem(item);
}

const DagorAsset *SelectAssetDlg::getAssetByName(const char *_name, dag::ConstSpan<int> asset_types) const
{
  if (!_name || !_name[0])
    return nullptr;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (type)
  {
    const int assetType = mgr->getAssetTypeId(type + 1);
    if (assetType == -1)
      return nullptr;

    for (int i = 0; i < asset_types.size(); i++)
      if (assetType == asset_types[i])
        return mgr->findAsset(String::mk_sub_str(name, type), assetType);

    return nullptr;
  }

  return mgr->findAsset(name, asset_types);
}

void SelectAssetDlg::customControlUpdate(int id)
{
  const float panelHeight = getButtonPanelHeight();
  float regionAvailableY = ImGui::GetContentRegionAvail().y;
  if (panelHeight > 0.0f)
    regionAvailableY -= panelHeight + ImGui::GetStyle().ItemSpacing.y;

  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage == AssetsGuiIds::AllPage)
    view->updateImgui(regionAvailableY);
  else if (currentPage == AssetsGuiIds::FavoritesPage && favoritesTab)
    favoritesTab->updateImgui();
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage && recentlyUsedTab)
    recentlyUsedTab->updateImgui();
}
