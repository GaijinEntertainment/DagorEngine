// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_view.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_assetTypeFilterButtonGroupControl.h>
#include <assetsGui/av_assetTypeFilterControl.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_selObjDlg.h>

#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <sepGui/wndMenuInterface.h>

#include <imgui/imgui.h>

using PropPanel::TLeafHandle;

//==============================================================================
// AssetBaseView
//==============================================================================
AssetBaseView::AssetBaseView(IAssetBaseViewClient *client, IAssetSelectorContextMenuHandler *menu_event_handler) :
  eh(client), menuEventHandler(menu_event_handler), curMgr(NULL), curFilter(midmem), filter(midmem), canNotifySelChange(true)
{
  assetsTree.reset(new AllAssetsTree(this));
  assetTypeFilterControl.reset(new AssetTypeFilterControl());
  closeIcon = (ImTextureID)((unsigned)PropPanel::load_icon("close_editor"));
  searchIcon = (ImTextureID)((unsigned)PropPanel::load_icon("search"));
  settingsIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_default"));
  settingsOpenIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_active"));
}


AssetBaseView::~AssetBaseView() {}


//==============================================================================

void AssetBaseView::connectToAssetBase(DagorAssetMgr *mgr, dag::ConstSpan<int> allowed_type_indexes, const DataBlock *expansion_state)
{
  curMgr = mgr;

  filter = allowed_type_indexes;
  curFilter = filter;
  allowedTypes.clear();
  shownTypes.clear();

  if (mgr)
  {
    allowedTypes.resize(mgr->getAssetTypesCount());
    shownTypes.resize(mgr->getAssetTypesCount());

    for (int i = 0; i < mgr->getAssetTypesCount(); ++i)
    {
      const bool allowed = eastl::find(allowed_type_indexes.begin(), allowed_type_indexes.end(), i) != allowed_type_indexes.end();
      allowedTypes[i] = shownTypes[i] = allowed;
    }

    fill(expansion_state);
    onShownTypeFilterChanged();
  }
}


//==============================================================================
void AssetBaseView::fill(const DataBlock *expansion_state)
{
  if (!curMgr)
    return;

  if (!curMgr->getRootFolder())
    return;

  assetsTree->fill(*curMgr, expansion_state);
}


//==============================================================================
DagorAsset *AssetBaseView::getSelectedAsset() const { return assetsTree->getSelectedAsset(); }


DagorAssetFolder *AssetBaseView::getSelectedAssetFolder() const { return assetsTree->getSelectedAssetFolder(); }


const char *AssetBaseView::getSelObjName()
{
  const DagorAsset *asset = assetsTree->getSelectedAsset();
  if (asset)
    selectionBuffer = SelectAssetDlg::getAssetNameWithTypeIfNeeded(*asset, filter);
  else
    selectionBuffer.clear();

  return selectionBuffer.c_str();
}


void AssetBaseView::onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel)
{
  if (assetsTree->isFolder(new_sel))
    return;

  eh->onAvSelectAsset(getSelectedAsset(), getSelObjName());
}

void AssetBaseView::onTvDClick(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle node)
{
  if (assetsTree->isFolder(node))
    return;

  eh->onAvAssetDblClick(getSelectedAsset(), getSelObjName());
}

//==============================================================================

bool AssetBaseView::selectAsset(const DagorAsset &asset, bool reset_filter_if_needed)
{
  bool selected = assetsTree->selectAsset(&asset);

  if (!selected && reset_filter_if_needed)
  {
    resetFilter();
    assetsTree->refilter();
    selected = assetsTree->selectAsset(&asset);
  }

  return selected;
}

bool AssetBaseView::selectObjInBase(const char *_name)
{
  DagorAsset *asset = nullptr;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (!type)
    asset = curMgr->findAsset(_name, curFilter);
  else
  {
    int asset_type = curMgr->getAssetTypeId(type + 1);
    if (asset_type != -1)
      for (int i = 0; i < curFilter.size(); i++)
        if (curFilter[i] == asset_type)
        {
          asset = curMgr->findAsset(String::mk_sub_str(name, type), asset_type);
          break;
        }
  }

  if (asset)
  {
    assetsTree->selectAsset(asset);
    eh->onAvSelectAsset(asset, _name);
    return true;
  }

  return false;
}


void AssetBaseView::saveTreeData(DataBlock &blk)
{
  const String originalTextToSearch = textToSearch;
  const Tab<int> originalCurFilter = curFilter;
  const Tab<bool> originalShownTypes = shownTypes;

  resetFilter();
  assetsTree->refilter();
  assetsTree->saveTreeData(blk);

  textToSearch = originalTextToSearch;
  curFilter = originalCurFilter;
  shownTypes = originalShownTypes;

  assetsTree->setSearchText(textToSearch);
  assetsTree->setShownTypes(shownTypes);
  assetsTree->refilter();
}


void AssetBaseView::getTreeNodesExpand(Tab<bool> &exps)
{
  clear_and_shrink(exps);
  TLeafHandle leaf, root;
  leaf = root = assetsTree->getRoot();
  exps.push_back(assetsTree->isOpen(leaf));
  while ((leaf = assetsTree->getNextNode(leaf, true)) && (leaf != root))
    exps.push_back(assetsTree->isOpen(leaf));
}


void AssetBaseView::setTreeNodesExpand(dag::ConstSpan<bool> exps)
{
  if (!exps.size())
    return;

  TLeafHandle leaf, root;
  leaf = root = assetsTree->getRoot();
  if (exps[0] && leaf)
    assetsTree->expand(leaf);
  else
    assetsTree->collapse(leaf);

  int i = 0;
  while ((leaf = assetsTree->getNextNode(leaf, true)) && (leaf != root) && (++i < exps.size()))
    if (exps[i])
      assetsTree->expand(leaf);
    else
      assetsTree->collapse(leaf);
}


void AssetBaseView::setFilterStr(const char *str)
{
  textToSearch = str;
  assetsTree->setSearchText(str);
  assetsTree->refilter();
}


void AssetBaseView::resetFilter()
{
  textToSearch.clear();
  curFilter = filter;
  shownTypes = allowedTypes;

  assetsTree->setSearchText("");
  assetsTree->setShownTypes(allowedTypes);
}


void AssetBaseView::setShownAssetTypeIndexes(dag::ConstSpan<int> shown_type_indexes)
{
  if (shown_type_indexes.size() == curFilter.size() && mem_eq(shown_type_indexes, curFilter.data()))
    return;

  curFilter.clear();

  for (bool &shown : shownTypes)
    shown = false;

  for (int type : shown_type_indexes)
    if (type >= 0 && type < allowedTypes.size() && allowedTypes[type])
    {
      curFilter.push_back(type);
      shownTypes[type] = true;
    }

  assetsTree->setShownTypes(shownTypes);
  assetsTree->refilter();
}


void AssetBaseView::addCommonMenuItems(PropPanel::IMenu &menu)
{
  menu.addSeparator(ROOT_MENU_ITEM);
  menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFilePathMenuItem, "File path");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");
  menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
}


bool AssetBaseView::onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree)
{
  return menuEventHandler->onAssetSelectorContextMenu(tree_base_window, tree);
}

void AssetBaseView::onShownTypeFilterChanged()
{
  assetsTree->setShownTypes(shownTypes);
  assetsTree->refilter();
}

void AssetBaseView::showSettingsPanel(const char *popup_id)
{
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
  const bool popupIsOpen = ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove);
  ImGui::PopStyleColor();

  if (!popupIsOpen)
  {
    settingsPanelOpen = false;
    return;
  }

  ImGui::TextUnformatted("Select filter");

  if (assetTypeFilterControl->updateImgui(*curMgr, make_span(allowedTypes), make_span(shownTypes)))
    onShownTypeFilterChanged();

  ImGui::EndPopup();
}

void AssetBaseView::updateImgui(float control_height)
{
  ImGui::PushID(this);

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x));

  bool inputFocused;
  ImGuiID inputId;
  const bool searchInputChanged = PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Filter and search",
    textToSearch, searchIcon, closeIcon, &inputFocused, &inputId);

  PropPanel::set_previous_imgui_control_tooltip((const void *)inputId, AssetSelectorCommon::searchTooltip);

  if (inputFocused)
  {
    if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_None, inputId) ||
        ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_None, inputId))
    {
      // The ImGui text input control reacts to Enter by making it inactive. Keep it active.
      ImGui::SetActiveID(inputId, ImGui::GetCurrentWindow());

      assetsTree->selectNextItem();
    }
    else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Enter, ImGuiInputFlags_None, inputId) ||
             ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_None, inputId))
    {
      ImGui::SetActiveID(inputId, ImGui::GetCurrentWindow());
      assetsTree->selectNextItem(/*forward = */ false);
    }
  }

  if (searchInputChanged)
  {
    assetsTree->setSearchText(textToSearch);
    assetsTree->refilter();
  }

  ImGui::SameLine();
  const char *popupId = "settingsPopup";
  const ImTextureID settingsButtonIcon = settingsPanelOpen ? settingsOpenIcon : settingsIcon;
  bool settingsButtonPressed =
    PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsButtonIcon, fontSizedIconSize, settingsPanelOpen);
  PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), "Settings");

  if (settingsPanelOpen)
    showSettingsPanel(popupId);

  if (AssetTypeFilterButtonGroupControl::updateImgui(*curMgr, allowedTypes, make_span(shownTypes), settingsButtonPressed))
    onShownTypeFilterChanged();

  if (settingsButtonPressed)
  {
    ImGui::OpenPopup(popupId);
    settingsPanelOpen = true;
  }

  const char *expandAllTitle = "Expand all";
  const char *collapseAllTitle = "Collapse all";
  const ImVec2 expandAllSize = PropPanel::ImguiHelper::getButtonSize(expandAllTitle);
  const ImVec2 collapseAllSize = PropPanel::ImguiHelper::getButtonSize(collapseAllTitle);
  const ImVec2 expandButtonRegionAvailable = ImGui::GetContentRegionAvail();
  const ImVec2 expandButtonCursorPos = ImGui::GetCursorPos();

  ImGui::SetCursorPosX(
    expandButtonCursorPos.x + expandButtonRegionAvailable.x - (expandAllSize.x + ImGui::GetStyle().ItemSpacing.x + collapseAllSize.x));
  if (ImGui::Button(expandAllTitle))
    assetsTree->expandRecursive(assetsTree->getRoot(), true);

  ImGui::SameLine();
  ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x - collapseAllSize.x);
  if (ImGui::Button(collapseAllTitle))
    assetsTree->expandRecursive(assetsTree->getRoot(), false);

  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
    PropPanel::focus_helper.requestFocus(&searchInputFocusId);

  assetsTree->updateImgui();

  ImGui::PopID();
}
