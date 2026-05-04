// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <assetsGui/av_assetBrowser.h>

#include <3d/dag_texMgr.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assetsGui/av_client.h>
#include <gui/dag_imguiUtil.h>
#include <libTools/util/hdpiUtil.h>
#include <propPanel/control/contextMenu.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>

#include <imgui/imgui_internal.h>

void AssetBrowser::BrowserItem::release()
{
  if (assetThumbnail.has_value())
  {
    release_managed_tex(assetThumbnail.value());
    assetThumbnail.reset();
  }
}

void AssetBrowser::BrowserItem::setAsset(DagorAsset &in_asset)
{
  release();

  asset = &in_asset;
}

TEXTUREID AssetBrowser::BrowserItem::getThumbnailTexture()
{
  TEXTUREID textureId;
  if (assetThumbnail.has_value())
  {
    textureId = assetThumbnail.value();
  }
  else if (asset->getType() == asset->getMgr().getTexAssetTypeId())
  {
    textureId = add_managed_texture(String(64, "%s*", asset->getName()));
    acquire_managed_tex(textureId);
    assetThumbnail = textureId;
  }
  else
  {
    textureId = BAD_TEXTUREID;
    assetThumbnail = textureId;
  }

  if (textureId == BAD_TEXTUREID || !prefetch_and_check_managed_texture_loaded(textureId))
    return BAD_TEXTUREID;

  return textureId;
}

void AssetBrowser::Layout::update(int item_count, int image_size, float available_width)
{
  const ImVec2 itemSpacing(hdpi::_pxS(ITEM_SPACING), hdpi::_pxS(ITEM_SPACING));

  verticalSpaceBetweenImageAndLabel = hdpi::_pxS(VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL);
  selectionBorderSize = hdpi::_pxS(SELECTION_BORDER_SIZE);
  windowPadding = hdpi::_pxS(WINDOW_PADDING);

  itemImageSize = ImVec2(image_size, image_size);
  itemSize = ImVec2(itemImageSize.x + (selectionBorderSize * 2),
    itemImageSize.y + verticalSpaceBetweenImageAndLabel + ImGui::GetCurrentContext()->FontSize + (selectionBorderSize * 2));
  itemStep = ImVec2(itemSize.x + itemSpacing.x, itemSize.y + itemSpacing.y);
  // Use available width with itemSpacing.x added to it because there is no spacing after the last column.
  columnCount = max((int)((available_width + itemSpacing.x - (windowPadding * 2.0f)) / itemStep.x), 1);
  lineCount = (item_count + columnCount - 1) / columnCount;
}

AssetBrowser::AssetBrowser(IAssetBrowserHost &host) : assetBrowserHost(host) { settingsIcon = PropPanel::load_icon("settings"); }

AssetBrowser::~AssetBrowser() { clearTexturesDrawnThisFrame(); }

bool AssetBrowser::isTheSameAssetList(dag::Span<DagorAsset *> assets) const
{
  if (assets.size() != browserItems.size())
    return false;

  for (int i = 0; i < assets.size(); ++i)
    if (assets[i] != browserItems[i].asset)
      return false;

  return true;
}

void AssetBrowser::setAssets(dag::Span<DagorAsset *> assets, DagorAsset *selected_asset)
{
  // If the asset list is the same then we can keep the texture references.
  // (It prevents flashing when texture streaming is using alwaysRelease:b=yes.)
  if (!isTheSameAssetList(assets))
  {
    clearItems();
    for (DagorAsset *asset : assets)
    {
      if (asset->getType() == asset->getMgr().getTexAssetTypeId())
      {
        browserItems.push_back(BrowserItem());
        BrowserItem &browserItem = browserItems.back();
        browserItem.setAsset(*asset);
      }
    }
  }

  selectedAsset = selected_asset;

  if (!ignoreEnsureVisibleRequest)
  {
    ensureVisibleRequested = true;
    focusItemRequested = true;
  }
}

int AssetBrowser::getThumbnailImageSize() const { return floorf(requestedImageSize); }

void AssetBrowser::setThumbnailImageSize(int size)
{
  requestedImageSize = clamp(size, Layout::IMAGE_SIZE_MINIMUM, Layout::IMAGE_SIZE_MAXIMUM);
}

void AssetBrowser::clearItems()
{
  for (BrowserItem &browserItem : browserItems)
    browserItem.release();
  browserItems.clear();

  selectedAsset = nullptr;
  ensureVisibleRequested = false;
  focusItemRequested = false;
}

void AssetBrowser::clearTexturesDrawnThisFrame()
{
  for (TEXTUREID textureId : texturesDrawnThisFrame)
    release_managed_tex(textureId);
  texturesDrawnThisFrame.clear();
}

void AssetBrowser::applySelectionRequests(ImGuiMultiSelectIO &multi_select_io)
{
  for (ImGuiSelectionRequest &request : multi_select_io.Requests)
  {
    if (request.Type == ImGuiSelectionRequestType_SetAll)
    {
      G_ASSERT(!request.Selected);
      selectedAsset = nullptr;
    }
    else if (request.Type == ImGuiSelectionRequestType_SetRange)
    {
      G_ASSERT(request.RangeFirstItem == request.RangeLastItem);
      if (request.RangeFirstItem >= 0 && request.RangeFirstItem < browserItems.size())
        selectedAsset = browserItems[request.RangeFirstItem].asset;
    }
  }
}

bool AssetBrowser::invisibleSelectable(bool selected, ImGuiButtonFlags button_flags, const ImVec2 &size_arg)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  const ImGuiID id = window->GetID("");
  ImVec2 size = ImGui::CalcItemSize(size_arg, 0.0f, 0.0f);
  const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
  ImGui::ItemSize(size);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  ImGui::MultiSelectItemHeader(id, &selected, &button_flags);

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, button_flags);

  ImGui::MultiSelectItemFooter(id, &selected, &pressed);

  // Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with
  // gamepad/keyboard.
  if (pressed || (hovered && (button_flags & ImGuiSelectableFlags_SetNavIdOnHover)))
  {
    if (!g.NavHighlightItemUnderNav && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
    {
      ImGui::SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, ImGui::WindowRectAbsToRel(window, bb)); // (bb == NavRect)
      g.NavCursorVisible = false;
    }
  }

  return pressed;
}

void AssetBrowser::drawItemTexture(ImDrawList &draw_list, TEXTUREID texture_id, const ImVec2 &pos)
{
  if (texture_id == BAD_TEXTUREID)
    return;

  BaseTexture *baseTexture = D3dResManagerData::getBaseTex(texture_id);
  if (!baseTexture)
    return;

  TextureInfo textureInfo;
  baseTexture->getinfo(textureInfo);
  if (textureInfo.w == 0 || textureInfo.h == 0)
    return;

  // Keep reference to drawn textures till imgui_render() to prevent crash.
  // (A crash could happen when selecting an asset in the browser. That leads to clearItems() being called, that
  // releases the textures, while they already have been queued to ImGui for drawing.)
  acquire_managed_tex(texture_id);
  texturesDrawnThisFrame.push_back(texture_id);

  ImVec2 itemImageOffset(0.0f, 0.0f);
  ImVec2 itemImageSize(layout.itemImageSize);
  if (textureInfo.w > textureInfo.h)
  {
    itemImageSize.y = (int)(layout.itemImageSize.y * (float(textureInfo.h) / textureInfo.w));
    itemImageOffset.y = (int)((layout.itemImageSize.y - itemImageSize.y) * 0.5f);
  }
  else if (textureInfo.h > textureInfo.w)
  {
    itemImageSize.x = (int)(layout.itemImageSize.x * (float(textureInfo.w) / textureInfo.h));
    itemImageOffset.x = (int)((layout.itemImageSize.x - itemImageSize.x) * 0.5f);
  }

  const ImTextureID imTextureId = ImGuiDagor::EncodeTexturePtr<ImTextureID>(baseTexture);
  draw_list.AddImage(imTextureId, pos + itemImageOffset, pos + itemImageOffset + itemImageSize);
}

void AssetBrowser::drawItem(BrowserItem &browser_item, const ImVec2 &pos, int item_index)
{
  const bool selected = browser_item.asset == selectedAsset;

  ImGui::SetCursorScreenPos(pos);

  if (focusItemRequested && selected && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() &&
      !ImGui::GetIO().WantTextInput)
  {
    focusItemRequested = false;
    ImGui::SetKeyboardFocusHere(); // Set the focus to the selected item, so keyboard navigation will start from the selection.
    ImGui::SetWindowFocus();       // This is only needed to focus the child window that contains the asset browser items by default.
  }

  ImGui::SetNextItemSelectionUserData(item_index);

  ImGui::PushID(browser_item.asset);
  const bool browserItemClicked = invisibleSelectable(selected, ImGuiButtonFlags_None, layout.itemSize);
  // The check for NavHighlightItemUnderNav is needed to prevent displaying the item as hovered when navigating with keyboard.
  const bool hovered = ImGui::IsItemHovered() && !ImGui::GetCurrentContext()->NavHighlightItemUnderNav;
  const bool browserItemDoubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
  const bool browserItemRightClicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  ImGui::PopID();

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  if (hovered || selected)
  {
    const ImVec2 posMax(pos.x + layout.itemSize.x, pos.y + layout.itemSize.y);

    if (hovered)
    {
      const ImU32 bgColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::ASSET_BROWSER_ITEM_BACKGROUND_HOVERED);
      ImGui::RenderFrame(pos, posMax, bgColor, true, ImGui::GetStyle().FrameRounding);
    }
    else
    {
      const ImU32 bgColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::ASSET_BROWSER_ITEM_BACKGROUND_SELECTED);
      drawList->AddRectFilled(pos, posMax, bgColor, ImGui::GetStyle().FrameRounding);
    }
  }

  // Draw a white background for images with alpha.
  const ImVec2 imagePosMin(pos.x + layout.selectionBorderSize, pos.y + layout.selectionBorderSize);
  const ImVec2 imagePosMax(imagePosMin.x + layout.itemImageSize.x, imagePosMin.y + layout.itemImageSize.y);
  drawList->AddRectFilled(imagePosMin, imagePosMax, IM_COL32(255, 255, 255, 255));

  drawItemTexture(*drawList, browser_item.getThumbnailTexture(), imagePosMin);

  const float fontHeight = ImGui::GetCurrentContext()->FontSize;
  const ImVec2 textPosMin(imagePosMin.x, imagePosMax.y + layout.verticalSpaceBetweenImageAndLabel);
  const ImVec2 textPosMax(textPosMin.x + layout.itemImageSize.x, textPosMin.y + fontHeight);
  const float clipPosX = textPosMax.x;
  ImGui::RenderTextEllipsis(drawList, textPosMin, ImVec2(clipPosX, textPosMax.y), clipPosX, browser_item.asset->getName(), nullptr,
    nullptr);

  PropPanel::set_previous_imgui_control_tooltip(browser_item.asset, browser_item.asset->getName());

  if (ensureVisibleRequested && selected)
  {
    ensureVisibleRequested = false;
    ImGui::SetScrollHereY();
  }

  if (browserItemClicked || browserItemDoubleClicked || browserItemRightClicked)
  {
    ignoreEnsureVisibleRequest = true;
    assetBrowserHost.assetBrowserOnAssetSelected(browser_item.asset, browserItemDoubleClicked);
    ignoreEnsureVisibleRequest = false;

    if (browserItemRightClicked)
      assetBrowserHost.assetBrowserOnContextMenu(browser_item.asset);
  }
}

void AssetBrowser::drawItems()
{
  clearTexturesDrawnThisFrame();

  if (!ImGui::BeginChild("assets", ImVec2(0.0f, 0.0f), ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
  {
    ImGui::EndChild();
    return;
  }

  const float availableWidth = ImGui::GetContentRegionAvail().x;
  layout.update(browserItems.size(), getThumbnailImageSize(), availableWidth);

  // Calculate and store start position.
  ImVec2 startPosition = ImGui::GetCursorScreenPos();
  startPosition = ImVec2(startPosition.x + layout.windowPadding, startPosition.y + layout.windowPadding);
  ImGui::SetCursorScreenPos(startPosition);

  ImGuiMultiSelectIO *multiSelectIo = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_SingleSelect | ImGuiMultiSelectFlags_NavWrapX,
    selectedAsset ? 1 : 0, browserItems.size());
  G_ASSERT(multiSelectIo);
  applySelectionRequests(*multiSelectIo);

  const int columnCount = layout.columnCount;
  ImGuiListClipper clipper;
  clipper.Begin(layout.lineCount, layout.itemStep.y);

  if (ensureVisibleRequested)
  {
    auto it = eastl::find_if(browserItems.begin(), browserItems.end(),
      [this](const BrowserItem &browser_item) { return browser_item.asset == selectedAsset; });

    if (it != browserItems.end())
      clipper.IncludeItemByIndex((it - browserItems.begin()) / columnCount);
  }

  if (multiSelectIo->RangeSrcItem != -1)
    clipper.IncludeItemByIndex(multiSelectIo->RangeSrcItem / columnCount);

  while (clipper.Step())
  {
    for (int lineIndex = clipper.DisplayStart; lineIndex < clipper.DisplayEnd; lineIndex++)
    {
      const int itemMinIndexForCurrentLine = lineIndex * columnCount;
      const int itemMaxIndexForCurrentLine = min((lineIndex + 1) * columnCount, (int)browserItems.size());
      for (int itemIndex = itemMinIndexForCurrentLine; itemIndex < itemMaxIndexForCurrentLine; ++itemIndex)
      {
        const ImVec2 position(startPosition.x + (itemIndex % layout.columnCount) * layout.itemStep.x,
          startPosition.y + lineIndex * layout.itemStep.y);

        drawItem(browserItems[itemIndex], position, itemIndex);
      }
    }
  }
  clipper.End();

  multiSelectIo = ImGui::EndMultiSelect();
  G_ASSERT(multiSelectIo);
  applySelectionRequests(*multiSelectIo);

  // Zooming with Ctrl+Wheel.
  if (ImGui::IsWindowAppearing())
    zoomWheelAccumulator = 0.0f;
  if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f && ImGui::IsKeyDown(ImGuiMod_Ctrl) && !ImGui::IsAnyItemActive())
  {
    zoomWheelAccumulator += ImGui::GetIO().MouseWheel;
    if (fabsf(zoomWheelAccumulator) >= 1.0f)
    {
      requestedImageSize *= powf(1.1f, (float)(int)zoomWheelAccumulator);
      requestedImageSize = clamp(requestedImageSize, (float)Layout::IMAGE_SIZE_MINIMUM, (float)Layout::IMAGE_SIZE_MAXIMUM);
      zoomWheelAccumulator -= (int)zoomWheelAccumulator;
      ensureVisibleRequested = true;
    }
  }

  ImGui::Dummy(ImVec2(0.0f, 0.0f));
  ImGui::EndChild();
}

void AssetBrowser::showSettingsPanel(const char *popup_id)
{
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
  const bool popupIsOpen = ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove);
  ImGui::PopStyleColor();

  if (!popupIsOpen)
  {
    settingsPanelOpen = false;
    return;
  }

  ImGui::TextUnformatted("Thumbnail size");

  int imageSize = getThumbnailImageSize();
  const bool changed = ImGui::SliderInt("##imageSize", &imageSize, Layout::IMAGE_SIZE_MINIMUM, Layout::IMAGE_SIZE_MAXIMUM);
  if (changed)
  {
    requestedImageSize = imageSize;
    ensureVisibleRequested = true;
  }

  ImGui::EndPopup();
}

void AssetBrowser::updateImgui()
{
  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);
  const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
  const float spaceXBeforeButton = max(ImGui::GetContentRegionAvail().x - settingsButtonSize.x, 0.0f);
  ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + spaceXBeforeButton, cursorPos.y));

  const char *popupId = "settingsPopup";
  const bool settingsButtonPressed =
    PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsIcon, fontSizedIconSize, settingsPanelOpen);
  PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Settings");

  if (settingsPanelOpen)
    showSettingsPanel(popupId);

  if (settingsButtonPressed)
  {
    ImGui::OpenPopup(popupId);
    settingsPanelOpen = true;
  }

  drawItems();

  if (contextMenu && !contextMenu->updateImgui())
    contextMenu.reset();

  if (ImGui::Shortcut(WINDOW_TOGGLE_HOTKEY))
    assetBrowserHost.assetBrowserSetOpen(!assetBrowserHost.assetBrowserIsOpen());
}

PropPanel::IMenu &AssetBrowser::createContextMenu()
{
  contextMenu.reset(new PropPanel::ContextMenu());
  return *contextMenu;
}
