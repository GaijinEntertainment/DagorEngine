#include <assetsGui/av_selObjDlg.h>
#include <assetsGui/av_globalState.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <util/dag_globDef.h>
#include <propPanel2/comWnd/panel_window.h>
#include "av_ids.h"


SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, const char *caption, const char *sel_btn_caption,
  const char *reset_btn_caption, dag::ConstSpan<int> filter, int x, int y, unsigned w, unsigned h) :

  CDialogWindow(phandle, x, y, w, h, caption), mgr(_mgr), view(NULL)
{
  client = this;

  PropertyContainerControlBase *_bp = mButtonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  buttonsToWidth();

  _bp->setCaption(DIALOG_ID_OK, sel_btn_caption);
  _bp->setCaption(DIALOG_ID_CANCEL, reset_btn_caption);

  commonConstructor(filter);
}


SelectAssetDlg::SelectAssetDlg(void *phandle, DagorAssetMgr *_mgr, IAssetBaseViewClient *cli, const char *caption,
  dag::ConstSpan<int> filter, int x, int y, unsigned w, unsigned h) :

  CDialogWindow(phandle, x, y, w, h, caption), mgr(_mgr), view(NULL)
{
  client = cli ? cli : this;

  PropertyContainerControlBase *_bp = mButtonsPanel->getContainer();
  G_ASSERT(_bp && "SelectAssetDlg::SelectAssetDlg : No panel with buttons");

  showButtonPanel(false);

  commonConstructor(filter);
}


void SelectAssetDlg::commonConstructor(dag::ConstSpan<int> filter)
{
  PropertyContainerControlBase *panel = getPanel();
  G_ASSERT(panel && "SelectAssetDlg::commonConstructor : No panel with controls");

  const int verticalInterval = panel->getClientHeight(); // For an empty panel getClientHeight equals with getVerticalInterval.
  PropertyContainerControlBase *tabPanel = panel->createTabPanel(AssetsGuiIds::TabPanel, "");
  PropertyContainerControlBase *allTabPage = tabPanel->createTabPage(AssetsGuiIds::AllPage, "All");

  const int tabPageHeight =
    panel->getHeight() - panel->getClientHeight() - tabPanel->getClientHeight() - allTabPage->getClientHeight() - verticalInterval;

  PropertyControlBase *placeholder = allTabPage->createPlaceholder(AssetsGuiIds::AssetBaseViewPlaceholder, tabPageHeight);
  view = new AssetBaseView(client, this, allTabPage->getWindowHandle(), placeholder->getX(), placeholder->getY(),
    placeholder->getWidth(), tabPageHeight);

  PropertyContainerControlBase *favoritesTabPage = tabPanel->createTabPage(AssetsGuiIds::FavoritesPage, "Favorites");
  placeholder = favoritesTabPage->createPlaceholder(AssetsGuiIds::FavoritesTreePlaceholder, tabPageHeight);
  favoritesTree = new TreeBaseWindow(this, favoritesTabPage->getWindowHandle(), placeholder->getX(), placeholder->getY(),
    placeholder->getWidth(), tabPageHeight, "", /*icons_show = */ true);

  PropertyContainerControlBase *recentlyUsedTabPage = tabPanel->createTabPage(AssetsGuiIds::RecentlyUsedPage, "Recently used");
  placeholder = recentlyUsedTabPage->createPlaceholder(AssetsGuiIds::RecentlyUsedTreePlaceholder, tabPageHeight);
  recentlyUsedTree = new TreeBaseWindow(this, recentlyUsedTabPage->getWindowHandle(), placeholder->getX(), placeholder->getY(),
    placeholder->getWidth(), tabPageHeight, "", /*icons_show = */ true);

  G_ASSERT(view);

  view->setFilter(filter);
  view->showEmptyGroups(false);
  view->connectToAssetBase(mgr);
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
  fillFavoritesTree();
  fillRecentlyUsedTree();
  return true;
}


void SelectAssetDlg::resizeWindow(unsigned w, unsigned h, bool internal)
{
  __super::resizeWindow(w, h, false);

  PropertyContainerControlBase *_pp = getPanel();
  G_ASSERT(_pp && "SelectAssetDlg::SelectAssetDlg : No panel with controls");

  view->resize(_pp->getWidth(), _pp->getHeight());
  buttonsToWidth();
}


SelectAssetDlg::~SelectAssetDlg()
{
  del_it(view);
  del_it(favoritesTree);
  del_it(recentlyUsedTree);
}


bool SelectAssetDlg::onOk()
{
  AssetSelectorGlobalState::addRecentlyUsed(String(getSelObjName()));

  return true;
}


void SelectAssetDlg::onAvAssetDblClick(const char *obj_name) { click(DIALOG_ID_OK); }


int SelectAssetDlg::onMenuItemClick(unsigned id)
{
  if (id == AssetsGuiIds::AddToFavoritesMenuItem)
  {
    const String assetName(getSelObjName());
    G_ASSERT(!assetName.empty());
    AssetSelectorGlobalState::addFavorite(assetName);

    // Mark the favorites tree dirty.
    favoritesTreeAllowedTypes.clear();
  }
  else if (id == AssetsGuiIds::RemoveFromFavoritesMenuItem)
  {
    const String assetName(getSelectedFavorite());
    if (!assetName.empty())
    {
      favoritesTree->removeItem(favoritesTree->getSelectedItem());
      AssetSelectorGlobalState::removeFavorite(assetName);
    }
  }
  else if (id == AssetsGuiIds::GoToAssetInSelectorMenuItem)
  {
    const String assetName(getSelObjName());
    if (!assetName.empty())
    {
      getPanel()->setInt(AssetsGuiIds::TabPanel, AssetsGuiIds::AllPage);
      selectObj(assetName);
    }
  }

  return 0;
}

void SelectAssetDlg::onChange(int pcb_id, PropPanel2 *panel)
{
  if (pcb_id == AssetsGuiIds::TabPanel)
  {
    const int newPage = panel->getInt(pcb_id);
    if (newPage == AssetsGuiIds::FavoritesPage)
      fillFavoritesTree();
    else if (newPage == AssetsGuiIds::RecentlyUsedPage)
      fillRecentlyUsedTree();

    client->onAvSelectAsset(getSelObjName());
  }
}

void SelectAssetDlg::onTvDClick(TreeBaseWindow &tree, TLeafHandle node)
{
  if (&tree == favoritesTree)
  {
    const TLeafHandle selectedNode = favoritesTree->getSelectedItem();
    if (selectedNode)
    {
      const String assetName = favoritesTree->getItemName(selectedNode);
      client->onAvAssetDblClick(assetName);
    }
  }
  else if (&tree == recentlyUsedTree)
  {
    const TLeafHandle selectedNode = recentlyUsedTree->getSelectedItem();
    if (selectedNode)
    {
      const String assetName = recentlyUsedTree->getItemName(selectedNode);
      client->onAvAssetDblClick(assetName);
    }
  }
}

void SelectAssetDlg::onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel)
{
  if (&tree == favoritesTree && getPanel()->getInt(AssetsGuiIds::TabPanel) == AssetsGuiIds::FavoritesPage)
    client->onAvSelectAsset(getSelectedFavorite());
  else if (&tree == recentlyUsedTree && getPanel()->getInt(AssetsGuiIds::TabPanel) == AssetsGuiIds::RecentlyUsedPage)
    client->onAvSelectAsset(getSelectedRecentlyUsed());
}

bool SelectAssetDlg::onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu)
{
  if (&tree == favoritesTree)
  {
    menu.setEventHandler(this);
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::GoToAssetInSelectorMenuItem, "Go to asset");
    menu.addSeparator(ROOT_MENU_ITEM);
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RemoveFromFavoritesMenuItem, "Remove from favorites");
    return true;
  }
  else if (&tree == recentlyUsedTree)
  {
    menu.setEventHandler(this);
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::GoToAssetInSelectorMenuItem, "Go to asset");
    return true;
  }

  return false;
}

void SelectAssetDlg::fillFavoritesTree()
{
  dag::ConstSpan<int> allowedTypes = view->getCurFilter();
  if (allowedTypes.size() == favoritesTreeAllowedTypes.size() && mem_eq(allowedTypes, favoritesTreeAllowedTypes.data()))
    return;

  favoritesTreeAllowedTypes = allowedTypes;

  const TLeafHandle selectedIem = favoritesTree->getSelectedItem();
  const String selectedItemName = selectedIem ? favoritesTree->getItemName(selectedIem) : String();

  favoritesTree->clear();

  dag::Vector<String> sortedFavorites = AssetSelectorGlobalState::getFavorites();
  eastl::sort(sortedFavorites.begin(), sortedFavorites.end(), [](const String &a, const String &b) { return strcmp(a, b) <= 0; });

  String imageName;
  for (const String &assetName : sortedFavorites)
  {
    const DagorAsset *asset = mgr->findAsset(assetName);

    if (asset && eastl::find(allowedTypes.begin(), allowedTypes.end(), asset->getType()) == allowedTypes.end())
      continue;

    if (asset)
      imageName.printf(64, "asset_%s", asset->getTypeStr());
    else
      imageName = "unknown";

    int iconIndex = favoritesTreeImageMap.getNameId(imageName);
    if (iconIndex < 0)
    {
      const String imagePath(256, "%s%s%s", p2util::get_icon_path(), imageName, ".bmp");
      favoritesTree->addImage(imagePath);
      iconIndex = favoritesTreeImageMap.addNameId(imageName);
    }

    favoritesTree->addItem(assetName, iconIndex);
  }

  if (selectedIem)
    selectTreeItemByName(*favoritesTree, selectedItemName);
}

String SelectAssetDlg::getSelectedFavorite()
{
  const TLeafHandle selectedNode = favoritesTree->getSelectedItem();
  if (selectedNode)
    return favoritesTree->getItemName(selectedNode);

  return String();
}

int SelectAssetDlg::getRecentlyUsedImageIndex(const DagorAsset &asset)
{
  const char *assetTypeName = asset.getTypeStr();
  const int iconIndex = recentlyUsedTreeImageMap.getNameId(assetTypeName);
  if (iconIndex >= 0)
    return iconIndex;

  const String imagePath(256, "%sasset_%s%s", p2util::get_icon_path(), assetTypeName, ".bmp");
  recentlyUsedTree->addImage(imagePath);
  return recentlyUsedTreeImageMap.addNameId(assetTypeName);
}

void SelectAssetDlg::fillRecentlyUsedTree()
{
  dag::ConstSpan<int> allowedTypes = view->getCurFilter();
  if (allowedTypes.size() == recentlyUsedTreeAllowedTypes.size() && mem_eq(allowedTypes, recentlyUsedTreeAllowedTypes.data()))
    return;

  recentlyUsedTreeAllowedTypes = allowedTypes;

  const TLeafHandle selectedIem = recentlyUsedTree->getSelectedItem();
  const String selectedItemName = selectedIem ? recentlyUsedTree->getItemName(selectedIem) : String();

  recentlyUsedTree->clear();

  for (int i = AssetSelectorGlobalState::getRecentlyUsed().size() - 1; i >= 0; --i)
  {
    const String &assetName = AssetSelectorGlobalState::getRecentlyUsed()[i];
    const DagorAsset *asset = mgr->findAsset(assetName);

    if (!asset || eastl::find(allowedTypes.begin(), allowedTypes.end(), asset->getType()) == allowedTypes.end())
      continue;

    recentlyUsedTree->addItem(assetName, getRecentlyUsedImageIndex(*asset));
  }

  if (selectedIem)
    selectTreeItemByName(*recentlyUsedTree, selectedItemName);
}

void SelectAssetDlg::addAssetToRecentlyUsed(const char *asset_name)
{
  const String assetName(asset_name);
  if (!AssetSelectorGlobalState::addRecentlyUsed(assetName))
    return;

  dag::ConstSpan<int> allowedTypes = view->getCurFilter();
  if (allowedTypes.size() != recentlyUsedTreeAllowedTypes.size() || !mem_eq(allowedTypes, recentlyUsedTreeAllowedTypes.data()))
  {
    fillRecentlyUsedTree();
    return;
  }

  const TLeafHandle selectedIem = recentlyUsedTree->getSelectedItem();
  const String selectedItemName = selectedIem ? recentlyUsedTree->getItemName(selectedIem) : String();

  TLeafHandle item = getTreeItemByName(*recentlyUsedTree, assetName);
  if (item)
  {
    // Don't bother if it's already the most recent in the tree.
    if (recentlyUsedTree->getChild(nullptr, 0) == item)
      return;

    recentlyUsedTree->removeItem(item);
  }

  const DagorAsset *asset = mgr->findAsset(assetName);
  if (!asset || eastl::find(allowedTypes.begin(), allowedTypes.end(), asset->getType()) == allowedTypes.end())
    return;

  recentlyUsedTree->addItemAsFirst(assetName, getRecentlyUsedImageIndex(*asset));

  if (selectedIem)
    selectTreeItemByName(*recentlyUsedTree, selectedItemName);
}

String SelectAssetDlg::getSelectedRecentlyUsed()
{
  const TLeafHandle selectedNode = recentlyUsedTree->getSelectedItem();
  if (selectedNode)
    return recentlyUsedTree->getItemName(selectedNode);

  return String();
}

TLeafHandle SelectAssetDlg::getTreeItemByName(TreeBaseWindow &tree, const String &name)
{
  TLeafHandle rootItem = tree.getRoot();
  TLeafHandle item = rootItem;
  while (true)
  {
    if (tree.getItemName(item) == name)
      return item;

    item = tree.getNextNode(item, true);
    if (!item || item == rootItem)
      return nullptr;
  }
}

void SelectAssetDlg::selectTreeItemByName(TreeBaseWindow &tree, const String &name)
{
  TLeafHandle item = getTreeItemByName(tree, name);
  if (item)
    tree.setSelectedItem(item);
}