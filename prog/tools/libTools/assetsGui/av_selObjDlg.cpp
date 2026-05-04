// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_selObjDlg.h>

#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_favoritesTab.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_recentlyUsedTab.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetHlp.h>
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
using PropPanel::ROOT_MENU_ITEM;

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <Shlobj.h>
#endif

class SelectAssetDlgAssetBrowser : public PropPanel::DialogWindow, public PropPanel::ICustomControl
{
public:
  explicit SelectAssetDlgAssetBrowser(IAssetBrowserHost &host, const char *caption) :
    DialogWindow(nullptr, _pxScaled(125), _pxScaled(250), caption), assetBrowser(host)
  {
    showButtonPanel(false);
    propertiesPanel->createCustomControlHolder(CustomControlHolderId, this);

    if (!hasEverBeenShown())
      setWindowSize(IPoint2(hdpi::_pxS(220), (int)(ImGui::GetIO().DisplaySize.y * 0.8f)));
  }

  PropPanel::IMenu &createContextMenu() { return assetBrowser.createContextMenu(); }
  void setAssets(dag::Span<DagorAsset *> assets, DagorAsset *selected_asset) { assetBrowser.setAssets(assets, selected_asset); }
  int getThumbnailImageSize() const { return assetBrowser.getThumbnailImageSize(); }
  void setThumbnailImageSize(int size) { assetBrowser.setThumbnailImageSize(size); }

private:
  // PropPanel::ICustomControl
  void customControlUpdate(int id) override
  {
    G_ASSERT(id == CustomControlHolderId);
    assetBrowser.updateImgui();
  }

  static constexpr int CustomControlHolderId = 1;

  AssetBrowser assetBrowser;
};

SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, const char *caption, const char *sel_btn_caption,
  const char *reset_btn_caption, dag::ConstSpan<int> filter, const char *asset_browser_caption) :

  DialogWindow(phandle, _pxScaled(DEFAULT_MINIMUM_WIDTH), _pxScaled(SelectAssetDlg::DEFAULT_MINIMUM_HEIGHT), caption),
  client(nullptr),
  mgr(_mgr),
  view(nullptr)
{
  PropPanel::ContainerPropertyControl *_bp = buttonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  _bp->setCaption(PropPanel::DIALOG_ID_OK, sel_btn_caption);
  _bp->setCaption(PropPanel::DIALOG_ID_CANCEL, reset_btn_caption);

  commonConstructor(filter, asset_browser_caption);
}


SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, IAssetBaseViewClient *cli, const char *caption,
  dag::ConstSpan<int> filter, const char *asset_browser_caption) :

  DialogWindow(phandle, _pxScaled(DEFAULT_MINIMUM_WIDTH), _pxScaled(SelectAssetDlg::DEFAULT_MINIMUM_HEIGHT), caption),
  client(cli),
  mgr(_mgr),
  view(NULL)
{
  PropPanel::ContainerPropertyControl *_bp = buttonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  showButtonPanel(false);

  commonConstructor(filter, asset_browser_caption);
}


void SelectAssetDlg::commonConstructor(dag::ConstSpan<int> filter, const char *asset_browser_caption)
{
  AssetSelectorCommon::initialize(*mgr);

  folderTextureId = PropPanel::load_icon("folder");

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel && "SelectAssetDlg::commonConstructor : No panel with controls");

  PropPanel::ContainerPropertyControl *tabPanel = panel->createTabPanel(AssetsGuiIds::TabPanel, "");
  PropPanel::ContainerPropertyControl *allTabPage = tabPanel->createTabPage(AssetsGuiIds::AllPage, "All");

  view = new AssetBaseView(this, this, *this);

  PropPanel::ContainerPropertyControl *favoritesTabPage = tabPanel->createTabPage(AssetsGuiIds::FavoritesPage, "Favorites");
  PropPanel::ContainerPropertyControl *recentlyUsedTabPage = tabPanel->createTabPage(AssetsGuiIds::RecentlyUsedPage, "Recently used");

  G_ASSERT(view);

  view->connectToAssetBase(mgr, filter);

  panel->createCustomControlHolder(AssetsGuiIds::CustomControlHolder, this);

  if (!hasEverBeenShown())
    setWindowSize(IPoint2(hdpi::_pxS(DEFAULT_WIDTH), (int)(ImGui::GetIO().DisplaySize.y * 0.8f)));

  if (!asset_browser_caption)
    asset_browser_caption = "Asset Browser##AssetBrowserModal";
  assetBrowserDlg.reset(new SelectAssetDlgAssetBrowser(*this, asset_browser_caption));
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

void SelectAssetDlg::assetBrowserFill()
{
  if (!assetBrowserIsOpen())
    return;

  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  dag::Vector<DagorAsset *> assets;
  if (currentPage == AssetsGuiIds::AllPage)
    view->getFilteredAssetsFromTheCurrentFolder(assets);
  else if (currentPage == AssetsGuiIds::FavoritesPage && favoritesTab)
    favoritesTab->getFilteredAssetsFromTheCurrentFolder(assets);
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage && recentlyUsedTab)
    recentlyUsedTab->getFilteredAssetsFromTheCurrentFolder(assets);

  assetBrowserDlg->setAssets(make_span(assets), getSelectedAsset());
}

bool SelectAssetDlg::assetBrowserIsOpen() const { return assetBrowserDlg && assetBrowserDlg->isVisible(); }

void SelectAssetDlg::assetBrowserSetOpen(bool open)
{
  if (!assetBrowserDlg || assetBrowserDlg->isVisible() == open)
    return;

  if (open)
  {
    if (!assetBrowserDlg->hasEverBeenShown())
      assetBrowserDlg->positionBesideWindow(getCaption());

    assetBrowserDlg->show();
    assetBrowserFill();
  }
  else
  {
    assetBrowserDlg->hide();
  }
}

void SelectAssetDlg::assetBrowserOnAssetSelected(DagorAsset *asset, bool double_clicked)
{
  if (!asset)
    return;

  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);
  if (currentPage == AssetsGuiIds::AllPage)
    view->selectAsset(*asset, true);
  else if (currentPage == AssetsGuiIds::FavoritesPage && favoritesTab)
    favoritesTab->setSelectedAsset(asset);
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage && recentlyUsedTab)
    recentlyUsedTab->setSelectedAsset(asset);

  if (double_clicked)
    onAvAssetDblClick(const_cast<DagorAsset *>(asset), getAssetNameWithTypeIfNeeded(*asset));
}

void SelectAssetDlg::assetBrowserOnContextMenu(DagorAsset *asset)
{
  // Because the context menu click handlers work with the selected tree item we must ensure that the asset matches the selection.
  if (!assetBrowserDlg || !asset || asset != getSelectedAsset())
    return;

  PropPanel::IMenu &menu = assetBrowserDlg->createContextMenu();
  onAssetSelectorContextMenu(menu, asset, nullptr);
}

void SelectAssetDlg::setAssetTagsView(IAssetTagsView *asset_tags_view)
{
  if (view)
    view->setAssetTagsView(asset_tags_view);
}


AssetTagManager *SelectAssetDlg::getTagManager()
{
  if (view)
  {
    IAssetTagsView *tagsView = view->getAssetTagsView();
    if (tagsView)
      return tagsView->getVisibleTagManager();
  }
  return nullptr;
}


AssetTagManager *SelectAssetDlg::getVisibleTagManager() { return tagsDlg != nullptr ? tagsDlg->getTagManager() : nullptr; }


void SelectAssetDlg::showTagManager(bool show) { PropPanel::request_delayed_callback(*this); }


void SelectAssetDlg::onImguiDelayedCallback(void *data)
{
  DagorAsset *selectedAsset = getSelectedAsset();
  AssetTagsDlg dlg(0, selectedAsset);
  dlg.getTagManager()->onAvSelectAsset(selectedAsset, getSelObjName());
  tagsDlg = &dlg;
  dlg.showDialog();
  tagsDlg = nullptr;
}


void SelectAssetDlg::onAvClose()
{
  AssetTagManager *tagManager = getTagManager();
  if (tagManager)
    tagManager->onAvClose();

  if (client)
    client->onAvClose();
}


void SelectAssetDlg::onAvAssetDblClick(DagorAsset *asset, const char *obj_name)
{
  if (!asset)
    return;

  lastSelectedAsset = asset;

  AssetTagManager *tagManager = getTagManager();
  if (tagManager)
    tagManager->onAvAssetDblClick(asset, obj_name);

  if (client)
    client->onAvAssetDblClick(asset, obj_name);
  else
    clickDialogButton(PropPanel::DIALOG_ID_OK);
}


void SelectAssetDlg::onAvSelectAsset(DagorAsset *asset, const char *asset_name)
{
  if (asset)
  {
    lastSelectedAsset = asset;

    AssetTagManager *tagManager = getTagManager();
    if (tagManager)
      tagManager->onAvSelectAsset(asset, asset_name);

    if (client)
      client->onAvSelectAsset(asset, asset_name);
  }

  assetBrowserFill();
}


void SelectAssetDlg::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name)
{
  AssetTagManager *tagManager = getTagManager();
  if (tagManager)
    tagManager->onAvSelectFolder(asset_folder, asset_folder_name);

  if (client)
    client->onAvSelectFolder(asset_folder, asset_folder_name);
}


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
  else if (id == AssetsGuiIds::CopyAssetTagsMenuItem)
  {
    if (const DagorAsset *asset = view->getSelectedAsset())
      AssetSelectorCommon::copyAssetTagsToClipboard(*asset);
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

bool SelectAssetDlg::onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder)
{
  const int currentPage = getPanel()->getInt(AssetsGuiIds::TabPanel);

  if (currentPage == AssetsGuiIds::AllPage)
  {
    menu.setEventHandler(this);

    if (asset_folder)
    {
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::ExpandChildrenMenuItem, "Expand children");
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CollapseChildrenMenuItem, "Collapse children");
      menu.addSeparator(ROOT_MENU_ITEM);

      if (AssetSelectorGlobalState::getMoveCopyToSubmenu())
      {
        menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
        menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
        menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");
      }
      else
      {
        menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Copy folder path");
        menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetNameMenuItem, "Copy name");
      }

      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
    }
    else if (asset)
    {
      const bool hasTags = assettags::getTagIds(*asset).size() > 0;
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
      AssetBaseView::addCommonMenuItems(menu, hasTags);
    }

    return true;
  }
  else if (currentPage == AssetsGuiIds::FavoritesPage && favoritesTab)
  {
    favoritesTab->fillContextMenu(menu, asset, asset_folder);
    return true;
  }
  else if (currentPage == AssetsGuiIds::RecentlyUsedPage && recentlyUsedTab)
  {
    recentlyUsedTab->fillContextMenu(menu, asset);
    return true;
  }

  return false;
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
        favoritesTab->fillTree(lastSelectedAsset);
      }
      else
      {
        favoritesTab->setSelectedAsset(lastSelectedAsset);
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
        recentlyUsedTab->fillTree(lastSelectedAsset);
      }
      else
      {
        recentlyUsedTab->setSelectedAsset(lastSelectedAsset);
      }
    }
    else if (newPage == AssetsGuiIds::AllPage)
    {
      if (lastSelectedAsset)
        view->selectAsset(*lastSelectedAsset, true);
    }

    assetBrowserFill();
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
    recentlyUsedTab->fillTree(recentlyUsedTab->getSelectedAsset());
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
  String name;
  if (asset)
    name = getAssetNameWithTypeIfNeeded(*asset);

  onAvSelectAsset(const_cast<DagorAsset *>(asset), name);
}

void SelectAssetDlg::onSelectionDoubleClicked(const DagorAsset *asset)
{
  if (asset)
    onAvAssetDblClick(const_cast<DagorAsset *>(asset), getAssetNameWithTypeIfNeeded(*asset));
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

void SelectAssetDlg::saveAssetBrowserSettings(DataBlock &blk) const
{
  blk.setInt("thumbnailImageSize", assetBrowserDlg->getThumbnailImageSize());
  blk.setBool("open", assetBrowserIsOpen());
}

void SelectAssetDlg::loadAssetBrowserSettings(DataBlock &blk)
{
  assetBrowserDlg->setThumbnailImageSize(blk.getInt("thumbnailImageSize", assetBrowserDlg->getThumbnailImageSize()));
  assetBrowserOpeningRequested = blk.getBool("open", assetBrowserOpeningRequested);
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

void SelectAssetDlg::updateImguiDialog()
{
  DialogWindow::updateImguiDialog();

  // Delay the displaying of the asset browser dialog because its position is based on the asset selector dialog's position and size.
  if (assetBrowserOpeningRequested && !ImGui::IsWindowAppearing())
  {
    assetBrowserOpeningRequested = false;
    assetBrowserSetOpen(true);
  }
}

eastl::unique_ptr<SelectAssetDlg> assets_gui_create_asset_selector_dialog(const SelectAssetDlgOptions &options, DagorAssetMgr &mgr)
{
  eastl::unique_ptr<SelectAssetDlg> dlg(
    new SelectAssetDlg(nullptr, &mgr, options.dialogCaption.empty() ? "Select asset" : options.dialogCaption,
      options.selectButtonCaption.empty() ? "Select asset" : options.selectButtonCaption,
      options.resetButtonCaption.empty() ? "Reset asset" : options.resetButtonCaption, options.selectableAssetTypes));

  dlg->setAssetTagsView(dlg.get());
  dlg->setManualModalSizingEnabled();

  if (!options.initiallySelectedAsset.empty())
    dlg->selectObj(options.initiallySelectedAsset);

  if (options.openAllGroups)
  {
    Tab<bool> gr;
    dlg->getTreeNodesExpand(gr);
    mem_set_ff(gr);
    dlg->setTreeNodesExpand(gr);
  }

  if (!options.filter.empty())
    dlg->setFilterStr(options.filter);

  if (!options.positionBesideWindow.empty() && !dlg->hasEverBeenShown())
    dlg->positionBesideWindow(options.positionBesideWindow);

  return dlg;
}
