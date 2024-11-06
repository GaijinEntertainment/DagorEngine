// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_selObjDlg.h>
#include "av_favoritesTab.h"
#include "av_recentlyUsedTab.h"
#include <assetsGui/av_globalState.h>
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
#include "av_ids.h"

#include <imgui/imgui.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <Shlobj.h>
#include <Shlwapi.h> // StrStrIA
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
  folderTextureId = PropPanel::load_icon("folder");

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel && "SelectAssetDlg::commonConstructor : No panel with controls");

  const hdpi::Px verticalInterval = panel->getClientHeight(); // For an empty panel getClientHeight equals with getVerticalInterval.
  PropPanel::ContainerPropertyControl *tabPanel = panel->createTabPanel(AssetsGuiIds::TabPanel, "");
  PropPanel::ContainerPropertyControl *allTabPage = tabPanel->createTabPage(AssetsGuiIds::AllPage, "All");

  const hdpi::Px tabPageHeight = _pxActual(panel->getHeight()) - panel->getClientHeight() - tabPanel->getClientHeight() -
                                 allTabPage->getClientHeight() - verticalInterval;

  view = new AssetBaseView(client, this, allTabPage->getWindowHandle(), 0, 0, 0, hdpi::_px(tabPageHeight));

  PropPanel::ContainerPropertyControl *favoritesTabPage = tabPanel->createTabPage(AssetsGuiIds::FavoritesPage, "Favorites");
  PropPanel::ContainerPropertyControl *recentlyUsedTabPage = tabPanel->createTabPage(AssetsGuiIds::RecentlyUsedPage, "Recently used");

  G_ASSERT(view);

  view->setFilter(filter);
  view->showEmptyGroups(false);
  view->connectToAssetBase(mgr);

  panel->createCustomControlHolder(AssetsGuiIds::CustomControlHolder, this);
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
  view->setFilter(type_filter);
  mgr = _mgr;
  view->connectToAssetBase(mgr);
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


void SelectAssetDlg::onAvAssetDblClick(const char *obj_name) { clickDialogButton(PropPanel::DIALOG_ID_OK); }


void SelectAssetDlg::revealInExplorer(const DagorAsset &asset) { dag_reveal_in_explorer(&asset); }


void SelectAssetDlg::copyAssetFilePathToClipboard(const DagorAsset &asset)
{
  String text(asset.isVirtual() ? asset.getTargetFilePath() : asset.getSrcFilePath());
  clipboard::set_clipboard_ansi_text(make_ms_slashes(text));
}


void SelectAssetDlg::copyAssetFolderPathToClipboard(const DagorAsset &asset)
{
  String text(asset.getFolderPath());
  clipboard::set_clipboard_ansi_text(make_ms_slashes(text));
}


void SelectAssetDlg::copyAssetNameToClipboard(const DagorAsset &asset) { clipboard::set_clipboard_ansi_text(asset.getName()); }


bool SelectAssetDlg::matchesSearchText(const char *haystack, const char *needle)
{
  if (needle == nullptr || needle[0] == 0)
    return true;

  return haystack && StrStrIA(haystack, needle) != nullptr;
}


int SelectAssetDlg::onMenuItemClick(unsigned id)
{
  if (id == AssetsGuiIds::AddToFavoritesMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      addAssetToFavorites(*asset);
  }
  else if (id == AssetsGuiIds::GoToAssetInSelectorMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      goToAsset(*asset);
  }
  else if (id == AssetsGuiIds::CopyAssetFilePathMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      copyAssetFilePathToClipboard(*asset);
  }
  else if (id == AssetsGuiIds::CopyAssetFolderPathMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      copyAssetFolderPathToClipboard(*asset);
  }
  else if (id == AssetsGuiIds::CopyAssetNameMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      copyAssetNameToClipboard(*asset);
  }
  else if (id == AssetsGuiIds::RevealInExplorerMenuItem)
  {
    const DagorAsset *asset = getAssetByName(getSelObjName(), view->getCurFilter());
    if (asset)
      revealInExplorer(*asset);
  }

  return 0;
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
        recentlyUsedTab.reset(new RecentlyUsedTab(*this));
        recentlyUsedTab->onAllowedTypesChanged(view->getFilter());
        recentlyUsedTabNeedsRefilling = false;
      }

      if (recentlyUsedTabNeedsRefilling)
      {
        recentlyUsedTabNeedsRefilling = false;
        recentlyUsedTab->fillTree();
      }
    }

    client->onAvSelectAsset(getSelObjName());
  }
}

String SelectAssetDlg::getSelectedFavorite()
{
  const DagorAsset *asset = favoritesTab ? favoritesTab->getSelectedAsset() : nullptr;
  return asset ? getAssetNameWithTypeIfNeeded(*asset) : String();
}

void SelectAssetDlg::getAssetImageName(String &image_name, const DagorAsset *asset)
{
  if (asset)
    image_name.printf(64, "asset_%s", asset->getTypeStr());
  else
    image_name = "unknown";
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

void SelectAssetDlg::onFavoriteSelectionChanged(const DagorAsset *asset)
{
  if (asset)
    client->onAvSelectAsset(getAssetNameWithTypeIfNeeded(*asset));
}

void SelectAssetDlg::onFavoriteSelectionDoubleClicked(const DagorAsset *asset)
{
  if (asset)
    client->onAvAssetDblClick(getAssetNameWithTypeIfNeeded(*asset));
}

void SelectAssetDlg::onRecentlyUsedSelectionChanged(const DagorAsset *asset) { onFavoriteSelectionChanged(asset); }

void SelectAssetDlg::onRecentlyUsedSelectionDoubleClicked(const DagorAsset *asset) { onFavoriteSelectionDoubleClicked(asset); };

String SelectAssetDlg::getSelectedRecentlyUsed()
{
  DagorAsset *asset = recentlyUsedTab ? recentlyUsedTab->getSelectedAsset() : nullptr;
  return asset ? getAssetNameWithTypeIfNeeded(*asset) : String();
}

String SelectAssetDlg::getAssetNameWithTypeIfNeeded(const DagorAsset &asset)
{
  if (mgr->isAssetNameUnique(asset, view->getCurFilter()))
    return String(asset.getName());
  else
    return asset.getNameTypified();
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
