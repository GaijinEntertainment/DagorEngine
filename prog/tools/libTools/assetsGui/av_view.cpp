// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_view.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetHlp.h>
#include <assets/assetFolder.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_assetTypeFilterButtonGroupControl.h>
#include <assetsGui/av_assetTypeFilterControl.h>
#include <assetsGui/av_globalState.h>
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

#include <imgui/imgui.h>

using PropPanel::ROOT_MENU_ITEM;
using PropPanel::TLeafHandle;

//==============================================================================
// AssetBaseView
//==============================================================================
AssetBaseView::AssetBaseView(IAssetBaseViewClient *client, IAssetSelectorContextMenuHandler *menu_event_handler,
  IAssetBrowserHost &asset_browser_host) :
  eh(client),
  menuEventHandler(menu_event_handler),
  assetBrowserHost(asset_browser_host),
  curMgr(NULL),
  curFilter(midmem),
  filter(midmem),
  canNotifySelChange(true)
{
  assetsTree.reset(new AllAssetsTree(this));
  assetTypeFilterControl.reset(new AssetTypeFilterControl());
  closeIcon = PropPanel::load_icon("close_editor");
  searchIcon = PropPanel::load_icon("search");
  settingsIcon = PropPanel::load_icon("filter_default");
  settingsOpenIcon = PropPanel::load_icon("filter_active");
  tagsToggleIcon = PropPanel::load_icon("tag");
  assetBrowserToggleIcon = PropPanel::load_icon("table");
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


void AssetBaseView::getFilteredAssetsFromTheCurrentFolder(dag::Vector<DagorAsset *> &assets) const
{
  assetsTree->getFilteredAssetsFromTheCurrentFolder(assets);
}


void AssetBaseView::onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel)
{
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
    refilterAssetsTree();
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


void AssetBaseView::saveTreeData(DataBlock &blk) { assetsTree->saveTreeData(blk); }


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


void AssetBaseView::expandNodesFromSelectionTillRoot() { assetsTree->expandTillRoot(assetsTree->getSelectedItem()); }


void AssetBaseView::setFilterStr(const char *str)
{
  textToSearch = str;
  assetsTree->setSearchText(str);
  refilterAssetsTree();
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
  refilterAssetsTree();
}


void AssetBaseView::addCommonMenuItems(PropPanel::IMenu &menu, bool add_tags)
{
  menu.addSeparator(ROOT_MENU_ITEM);
  if (AssetSelectorGlobalState::getMoveCopyToSubmenu())
  {
    menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
    menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFilePathMenuItem, "File path");
    menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
    menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");
    if (add_tags)
      menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetTagsMenuItem, "Tags");
  }
  else
  {
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetFilePathMenuItem, "Copy file path");
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Copy folder path");
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetNameMenuItem, "Copy name");
    if (add_tags)
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetTagsMenuItem, "Copy tags");
  }
  menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
}


bool AssetBaseView::onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree)
{
  PropPanel::IMenu &menu = tree.createContextMenu();
  DagorAsset *asset = getSelectedAsset();
  DagorAssetFolder *assetFolder = getSelectedAssetFolder();
  return menuEventHandler->onAssetSelectorContextMenu(menu, asset, assetFolder);
}

void AssetBaseView::refilterAssetsTree()
{
  assetsTree->refilter();
  assetBrowserHost.assetBrowserFill();
}

void AssetBaseView::onShownTypeFilterChanged()
{
  assetsTree->setShownTypes(shownTypes);
  refilterAssetsTree();
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

void AssetBaseView::setAssetTagsView(IAssetTagsView *asset_tags_view) { assetTagsView = asset_tags_view; }

void AssetBaseView::setTagsToggled(bool toggled) { tagsToggled = toggled; }

// "Re-implementing" the imgui collapsing header function for two reasons:
// - The "label_end" property of the TreeNodeBehavior is not accessible in the original function
//   which is needed in this case to indicate active tag filters as a toggleable "*" suffix
// - The close button of the collapsing header needs a custom (and properly working) tooltip
static bool collapsingHeader(const char *label, const char *label_end, bool *p_visible)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  if (p_visible && !*p_visible)
    return false;

  ImGuiID id = window->GetID(label);
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_CollapsingHeader;
  if (p_visible)
    flags |= ImGuiTreeNodeFlags_AllowOverlap | (ImGuiTreeNodeFlags)ImGuiTreeNodeFlags_ClipLabelForTrailingButton;
  bool isOpen = ImGui::TreeNodeBehavior(id, flags, label, label_end);
  if (p_visible != NULL)
  {
    ImGuiContext &g = *ImGui::GetCurrentContext();
    ImGuiLastItemData lastItemBackup = g.LastItemData;

    float buttonSize = g.FontSize;
    float buttonX = ImMax(g.LastItemData.Rect.Min.x, g.LastItemData.Rect.Max.x - g.Style.FramePadding.x - buttonSize);
    float buttonY = g.LastItemData.Rect.Min.y + g.Style.FramePadding.y;
    ImGuiID closeButtonId = ImGui::GetIDWithSeed("#CLOSE", NULL, id);
    if (ImGui::CloseButton(closeButtonId, ImVec2(buttonX, buttonY)))
      *p_visible = false;
    PropPanel::set_previous_imgui_control_tooltip(closeButtonId, "Reset all");

    g.LastItemData = lastItemBackup;
  }

  return isOpen;
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

  PropPanel::set_previous_imgui_control_tooltip(inputId, AssetSelectorCommon::searchTooltip);

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
    refilterAssetsTree();
  }

  ImGui::SameLine();
  const char *popupId = "settingsPopup";
  const PropPanel::IconId settingsButtonIcon = settingsPanelOpen ? settingsOpenIcon : settingsIcon;
  bool settingsButtonPressed =
    PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsButtonIcon, fontSizedIconSize, settingsPanelOpen);
  PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Settings");

  if (settingsPanelOpen)
    showSettingsPanel(popupId);

  if (AssetTypeFilterButtonGroupControl::updateImgui(*curMgr, allowedTypes, make_span(shownTypes), settingsButtonPressed))
    onShownTypeFilterChanged();

  if (settingsButtonPressed)
  {
    ImGui::OpenPopup(popupId);
    settingsPanelOpen = true;
  }

  bool tagFilterChanged = false;
  if (curMgr != nullptr)
  {
    int version = assettags::getVersion();
    if (tagsVersion != version)
    {
      tagsVersion = version;
      tagFilterChanged = true;
      // Tag references may have been deleted...
      for (int i = tagFilter.size() - 1; i >= 0; --i)
      {
        const int tagId = tagFilter[i];
        if (assettags::getTagRefCount(tagId) <= 0)
          tagFilter.delInt(tagId);
      }
    }

    if (assettags::getReferencedTagCount() > 0)
    {
      // Disable window-padding (use indent for the tag toggles instead)
      // and cache it for restoring right after collapsing header!
      ImGuiContext &g = *ImGui::GetCurrentContext();
      ImGuiWindow *window = g.CurrentWindow;
      float tagIndent = 0.0f;
      ImVec2 windowPadding = window->WindowPadding;
      if (window->WindowPadding != ImVec2(0, 0))
      {
        window->WindowPadding = ImVec2(0, 0);
        tagIndent = IM_TRUNC(windowPadding.x * 0.5f);
      }
      else if (window->ParentWindow)
      {
        tagIndent = IM_TRUNC(window->ParentWindow->WindowPadding.x * 0.5f);
      }

      bool keepTags = true;
      const char *tagFilters = "Tag Filters *";
      const char *tagFiltersEnd = tagFilter.size() == 0 ? tagFilters + 11 : nullptr;
      if (collapsingHeader(tagFilters, tagFiltersEnd, &keepTags))
      {
        window->WindowPadding = windowPadding;
        ImGui::Indent(tagIndent);

        // Cache the positions here for later group half-frame drawing
        const ImVec2 buttonTopLeft = g.LastItemData.Rect.Min;
        const ImVec2 buttonBottomRight = g.LastItemData.Rect.Max;

        int lineIndex = 0;
        const int count = assettags::getCount();
        for (int i = 0; i < count; ++i)
        {
          if (assettags::getTagRefCount(i) <= 0)
          {
            if (tagFilter.delInt(i))
              tagFilterChanged = true;
            continue;
          }

          const char *tagName = assettags::getTagName(i);
          const ImVec2 tagButtonSize = PropPanel::ImguiHelper::getButtonSize(tagName);

          if (lineIndex > 0)
            ImGui::SameLine();

          if (ImGui::GetContentRegionAvail().x < tagButtonSize.x && i > 0)
          {
            ImGui::NewLine();
            lineIndex = 0;
          }

          ImGui::PushStyleColor(ImGuiCol_Border, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TAG_BORDER));

          ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TAG_FILTER_BACKGROUND));

          ImU32 colTextFilter = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::TAG_FILTER_TEXT);
          bool filterForTag = tagFilter.hasInt(i);
          if (PropPanel::ImguiHelper::toggleButtonWithDragSelection(tagName, &filterForTag, 2.0f, colTextFilter))
          {
            if (filterForTag)
              tagFilter.addInt(i);
            else
              tagFilter.delInt(i);
            tagFilterChanged = true;
          }

          ImGui::PopStyleColor(4);

          lineIndex++;
        }

        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        PropPanel::ImguiHelper::drawHalfFrame(buttonTopLeft.x, buttonBottomRight.y, buttonBottomRight.x, cursorPos.y);
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));
        ImGui::Dummy(ImVec2(0.0f, 0.0f)); // Prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().

        ImGui::Unindent(tagIndent);
      }

      window->WindowPadding = windowPadding;

      if (!keepTags)
      {
        tagFilterChanged = tagFilter.size() > 0;
        tagFilter.reset();
      }
    }
  }
  if (tagFilterChanged)
  {
    assetsTree->setTagFilter(tagFilter);
    refilterAssetsTree();
  }

  if (assetTagsView != nullptr)
    tagsToggled = (assetTagsView->getVisibleTagManager() != nullptr);

  G_STATIC_ASSERT(AssetTagManager::WINDOW_TOGGLE_HOTKEY == (ImGuiMod_Ctrl | ImGuiKey_T));
  const bool tagsToggleClicked = PropPanel::ImguiHelper::imageCheckButtonWithBackground("tagsToggle", tagsToggleIcon,
    fontSizedIconSize, tagsToggled, "Tag Management (Ctrl+T)");

  ImGui::SameLine();

  G_STATIC_ASSERT(AssetBrowser::WINDOW_TOGGLE_HOTKEY == (ImGuiMod_Ctrl | ImGuiKey_B));
  const bool assetBrowserToggleClicked = PropPanel::ImguiHelper::imageCheckButtonWithBackground("assetBrowserToggle",
    assetBrowserToggleIcon, fontSizedIconSize, assetBrowserHost.assetBrowserIsOpen(), "Asset Browser (Ctrl+B)");

  if (tagsToggleClicked || ImGui::Shortcut(AssetTagManager::WINDOW_TOGGLE_HOTKEY))
  {
    tagsToggled = !tagsToggled;
    if (assetTagsView != nullptr)
      assetTagsView->showTagManager(tagsToggled);
  }

  if (assetBrowserToggleClicked || ImGui::Shortcut(AssetBrowser::WINDOW_TOGGLE_HOTKEY))
    assetBrowserHost.assetBrowserSetOpen(!assetBrowserHost.assetBrowserIsOpen());

  ImGui::SameLine();

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
