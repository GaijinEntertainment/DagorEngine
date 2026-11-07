// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <assetsGui/av_assetBrowser.h>

#include <3d/dag_texMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_client.h>
#include <assetsGui/av_thumbnailRenderer.h>
#include <drv/3d/dag_resetDevice.h>
#include <gui/dag_imguiUtil.h>
#include <libTools/util/hdpiUtil.h>
#include <math/dag_mathBase.h>
#include <propPanel/control/contextMenu.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>

#include <EASTL/algorithm.h>
#include <imgui/imgui_internal.h>
#include <ska_hash_map/flat_hash_map2.hpp>

static IAssetThumbnailRenderer *thumbnail_renderer = nullptr;
static int thumbnail_render_frame = -1;

static bool is_label_line_break_char(char c) { return c == '_' || c == ' ' || c == '-' || c == '.'; }

// ImGui's word wrap does not break at '_', the usual word separator in asset names.
static const char *get_label_line_end(ImFont &font, float font_size, float width, const char *line_start, const char *text_end)
{
  const char *fitEnd = text_end;
  font.CalcTextSizeA(font_size, width, 0.0f, line_start, text_end, &fitEnd);
  if (fitEnd >= text_end)
    return text_end;

  for (const char *c = fitEnd - 1; c > line_start; --c)
    if (is_label_line_break_char(*c))
      return c + 1;

  // Show at least one character on each line, even if it does not fit.
  if (fitEnd == line_start)
    return line_start + ImTextCountUtf8BytesFromChar(line_start, text_end);

  return fitEnd;
}

static int get_label_line_count(ImFont &font, float font_size, float width, int max_line_count, const char *text)
{
  const char *textEnd = text + strlen(text);
  const char *lineStart = text;
  int lineCount = 1;
  for (; lineCount < max_line_count; ++lineCount)
  {
    lineStart = get_label_line_end(font, font_size, width, lineStart, textEnd);
    if (lineStart >= textEnd)
      break;
  }

  return lineCount;
}

// Wrap the text to at most line_count centered lines and put an ellipsis at the end of the last line if the text does not fit.
static void render_wrapped_text_with_ellipsis(ImDrawList &draw_list, const ImVec2 &pos, float width, int line_count, const char *text)
{
  ImFont *font = draw_list._Data->Font;
  const float fontSize = draw_list._Data->FontSize;
  const char *textEnd = text + strlen(text);
  const char *lineStart = text;
  const ImVec2 centerAlign(0.5f, 0.0f);

  for (int lineIndex = 0; lineIndex < line_count && lineStart < textEnd; ++lineIndex)
  {
    const ImVec2 linePosMin(pos.x, pos.y + lineIndex * fontSize);
    const ImVec2 linePosMax(pos.x + width, linePosMin.y + fontSize);
    const char *lineEnd = get_label_line_end(*font, fontSize, width, lineStart, textEnd);

    // RenderTextEllipsis() cannot align, but a line with an ellipsis fills the width anyway.
    if (lineIndex == (line_count - 1) && lineEnd < textEnd)
    {
      ImGui::RenderTextEllipsis(&draw_list, linePosMin, linePosMax, linePosMax.x, lineStart, textEnd, nullptr);
      break;
    }

    ImGui::RenderTextClippedEx(&draw_list, linePosMin, linePosMax, lineStart, lineEnd, nullptr, centerAlign);
    lineStart = lineEnd;
  }
}

void AssetBrowser::BrowserItem::release()
{
  if (assetThumbnail.has_value())
  {
    release_managed_tex(assetThumbnail.value());
    assetThumbnail.reset();
  }
}

const char *AssetBrowser::BrowserItem::getName() const { return asset ? asset->getName() : folder->folderName.c_str(); }

TEXTUREID AssetBrowser::BrowserItem::getThumbnailTexture()
{
  const bool isTexture = asset->getType() == asset->getMgr().getTexAssetTypeId();

  if (!assetThumbnail.has_value())
  {
    if (isTexture)
    {
      const TEXTUREID textureId = add_managed_texture(String(64, "%s*", asset->getName()));
      if (acquire_managed_tex(textureId))
      {
        assetThumbnail = textureId;
      }
      else
      {
        // The factory could not make the texture, so it would never be loaded.
        release_managed_tex(textureId);
        assetThumbnail = BAD_TEXTUREID;
      }
    }
    else if (!thumbnail_renderer)
    {
      assetThumbnail = BAD_TEXTUREID;
    }
    else
    {
      // One blocking render per frame in all Asset Browsers, so the window updates between them. Replace this when moving
      // rendering to a separate thread.
      const int frame = ImGui::GetFrameCount();
      if (thumbnail_render_frame == frame)
        return BAD_TEXTUREID;

      // Count every renderThumbnail() call, also one that returns no thumbnail, because it can load a resource
      // synchronously and block.
      thumbnail_render_frame = frame;

      assetThumbnail = thumbnail_renderer->renderThumbnail(*asset, Layout::IMAGE_SIZE_MAXIMUM);
      if (!assetThumbnail.has_value())
        return BAD_TEXTUREID;
    }
  }

  const TEXTUREID textureId = assetThumbnail.value();
  if (textureId == BAD_TEXTUREID || (isTexture && !prefetch_and_check_managed_texture_loaded(textureId)))
    return BAD_TEXTUREID;

  return textureId;
}

int AssetBrowser::Layout::getLabelPadding() { return hdpi::_pxS(SELECTION_BORDER_SIZE) / 4; }

float AssetBrowser::Layout::getLabelWidth(int image_size)
{
  return image_size + (hdpi::_pxS(SELECTION_BORDER_SIZE) * 2) - (getLabelPadding() * 2);
}

void AssetBrowser::Layout::update(int item_count, int image_size, float available_width, int label_line_count)
{
  const ImVec2 itemSpacing(hdpi::_pxS(ITEM_SPACING), hdpi::_pxS(ITEM_SPACING));

  verticalSpaceBetweenImageAndLabel = hdpi::_pxS(VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL);
  selectionBorderSize = hdpi::_pxS(SELECTION_BORDER_SIZE);
  windowPadding = hdpi::_pxS(WINDOW_PADDING);
  labelPadding = getLabelPadding();
  labelWidth = getLabelWidth(image_size);
  labelLineCount = label_line_count;

  itemImageSize = ImVec2(image_size, image_size);
  const float labelHeight = ImGui::GetCurrentContext()->FontSize * labelLineCount;
  itemSize = ImVec2(itemImageSize.x + (selectionBorderSize * 2),
    itemImageSize.y + verticalSpaceBetweenImageAndLabel + labelHeight + (selectionBorderSize * 2));
  itemStep = ImVec2(itemSize.x + itemSpacing.x, itemSize.y + itemSpacing.y);
  // Use available width with itemSpacing.x added to it because there is no spacing after the last column.
  columnCount = max((int)((available_width + itemSpacing.x - (windowPadding * 2.0f)) / itemStep.x), 1);
  lineCount = (item_count + columnCount - 1) / columnCount;
}

AssetBrowser::AssetBrowser(IAssetBrowserHost &host) : assetBrowserHost(host)
{
  settingsIcon = PropPanel::load_icon("settings");
  folderIcon = PropPanel::load_icon("folder");
}

AssetBrowser::~AssetBrowser()
{
  clearItems();
  clearTexturesDrawnThisFrame();
}

bool AssetBrowser::isTheSameItemList(dag::ConstSpan<BrowserItem> items) const
{
  if (items.size() != browserItems.size())
    return false;

  for (int i = 0; i < items.size(); ++i)
    if (items[i].asset != browserItems[i].asset || items[i].folder != browserItems[i].folder)
      return false;

  return true;
}

bool AssetBrowser::isSelected(const BrowserItem &browser_item) const
{
  return browser_item.asset ? browser_item.asset == selectedAsset : browser_item.folder == selectedFolder;
}

bool AssetBrowser::isContextMenuOpen() const { return contextMenu != nullptr; }

bool AssetBrowser::canShowAsset(const DagorAsset &asset) const
{
  return asset.getType() == asset.getMgr().getTexAssetTypeId() ||
         (thumbnail_renderer && thumbnail_renderer->canRenderThumbnail(asset));
}

void AssetBrowser::updateShownItems()
{
  dag::Vector<BrowserItem> shownItems;
  if (showFolders)
  {
    for (const DagorAssetFolder *subFolder : allSubFolders)
    {
      BrowserItem &browserItem = shownItems.push_back();
      browserItem.folder = subFolder;
    }
  }

  for (DagorAsset *asset : allAssets)
  {
    if (showTypesWithoutThumbnailSupport || canShowAsset(*asset))
    {
      BrowserItem &browserItem = shownItems.push_back();
      browserItem.asset = asset;
    }
  }

  // If the item list is the same then we can keep the thumbnails.
  // (It prevents flashing when texture streaming is using alwaysRelease:b=yes, and rendering thumbnails again.)
  if (!isTheSameItemList(make_span_const(shownItems)))
  {
    // Keep the thumbnails of the assets that stay shown, because each render blocks.
    ska::flat_hash_map<const DagorAsset *, BrowserItem *> oldItems;
    oldItems.reserve(browserItems.size());
    for (BrowserItem &browserItem : browserItems)
      if (browserItem.asset)
        oldItems.emplace(browserItem.asset, &browserItem);

    for (BrowserItem &browserItem : shownItems)
    {
      auto it = oldItems.find(browserItem.asset);
      if (it != oldItems.end())
      {
        browserItem.assetThumbnail = it->second->assetThumbnail;
        it->second->assetThumbnail.reset();
      }
    }

    clearItems();
    browserItems = eastl::move(shownItems);
    labelLineCountLabelWidth = -1.0f;
  }
}

void AssetBrowser::setAssets(dag::Span<DagorAsset *> assets, dag::Span<DagorAssetFolder *> sub_folders, DagorAsset *selected_asset,
  const DagorAssetMgr *asset_mgr, const DagorAssetFolder *folder)
{
  folderPath.clear();
  if (asset_mgr)
    for (const DagorAssetFolder *pathFolder = folder; pathFolder; pathFolder = asset_mgr->getFolderPtr(pathFolder->parentIdx))
      folderPath.push_back(pathFolder);
  eastl::reverse(folderPath.begin(), folderPath.end());

  allAssets.assign(assets.begin(), assets.end());
  allSubFolders.assign(sub_folders.begin(), sub_folders.end());
  updateShownItems();

  selectedAsset = selected_asset;
  selectedFolder = nullptr;
  ensureVisibleRequested = focusItemRequested = !ignoreEnsureVisibleRequest;
}

int AssetBrowser::getThumbnailImageSize() const { return floorf(requestedImageSize); }

void AssetBrowser::setThumbnailImageSize(int size)
{
  requestedImageSize = clamp(size, Layout::IMAGE_SIZE_MINIMUM, Layout::IMAGE_SIZE_MAXIMUM);
}

void AssetBrowser::setShownItemsOption(bool &option, bool value)
{
  if (value == option)
    return;

  option = value;
  updateShownItems();
  ensureVisibleRequested = true;
}

void AssetBrowser::setShowTypesWithoutThumbnailSupport(bool show) { setShownItemsOption(showTypesWithoutThumbnailSupport, show); }

void AssetBrowser::setShowFolders(bool show) { setShownItemsOption(showFolders, show); }

void AssetBrowser::clearItems()
{
  for (BrowserItem &browserItem : browserItems)
    browserItem.release();
  browserItems.clear();
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
      selectedFolder = nullptr;
    }
    else if (request.Type == ImGuiSelectionRequestType_SetRange)
    {
      G_ASSERT(request.RangeFirstItem == request.RangeLastItem);
      if (request.RangeFirstItem >= 0 && request.RangeFirstItem < browserItems.size())
      {
        selectedAsset = browserItems[request.RangeFirstItem].asset;
        selectedFolder = browserItems[request.RangeFirstItem].folder;
      }
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

void AssetBrowser::drawItemIcon(ImDrawList &draw_list, PropPanel::IconId icon_id, const ImVec2 &pos)
{
  const ImTextureID imTextureId = PropPanel::get_im_texture_id_from_icon_id(icon_id);
  if (!imTextureId)
    return;

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const ImVec2 iconSize(min(fontSizedIconSize.x, layout.itemImageSize.x), min(fontSizedIconSize.y, layout.itemImageSize.y));
  const ImVec2 iconPos(pos.x + (int)((layout.itemImageSize.x - iconSize.x) * 0.5f),
    pos.y + (int)((layout.itemImageSize.y - iconSize.y) * 0.5f));
  draw_list.AddImage(imTextureId, iconPos, iconPos + iconSize);
}

void AssetBrowser::drawLoadingIndicator(ImDrawList &draw_list, const ImVec2 &pos)
{
  // Smaller than 0.5 to make it look as large as the type icons, and to keep the stroke inside the icon area.
  constexpr float RADIUS_PER_ICON_SIZE = 0.4f;

  constexpr float STROKE_WIDTH_PER_RADIUS = 0.25f;
  constexpr float ARC_DEGREES = 270.0f;
  constexpr float TURNS_PER_SECOND = 1.0f;

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const float radius =
    min(min(fontSizedIconSize.x, layout.itemImageSize.x), min(fontSizedIconSize.y, layout.itemImageSize.y)) * RADIUS_PER_ICON_SIZE;
  const ImVec2 center(pos.x + layout.itemImageSize.x * 0.5f, pos.y + layout.itemImageSize.y * 0.5f);

  // The fmod keeps the angle small. A float angle that grows with the uptime loses precision and makes the arc stutter.
  const float startAngle = fmod(ImGui::GetTime() * TURNS_PER_SECOND, 1.0) * TWOPI;

  draw_list.PathArcTo(center, radius, startAngle, startAngle + DegToRad(ARC_DEGREES));
  draw_list.PathStroke(ImGui::GetColorU32(ImGuiCol_Text), ImDrawFlags_None, max(radius * STROKE_WIDTH_PER_RADIUS, 1.0f));
}

const DagorAssetFolder *AssetBrowser::drawItem(BrowserItem &browser_item, const ImVec2 &pos, int item_index)
{
  const bool selected = isSelected(browser_item);

  ImGui::SetCursorScreenPos(pos);

  if (focusItemRequested && selected && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() &&
      !ImGui::GetIO().WantTextInput)
  {
    focusItemRequested = false;
    ImGui::SetKeyboardFocusHere(); // Set the focus to the selected item, so keyboard navigation will start from the selection.
    ImGui::SetWindowFocus();       // This is only needed to focus the child window that contains the asset browser items by default.
  }

  ImGui::SetNextItemSelectionUserData(item_index);

  ImGui::PushID(browser_item.getId());
  const bool browserItemClicked = invisibleSelectable(selected, ImGuiButtonFlags_None, layout.itemSize);
  const bool activatedWithKeyboard = ImGui::GetCurrentContext()->NavActivatePressedId == ImGui::GetItemID();
  // ImGui activates items only while the navigation cursor is visible, and a mouse click hides it.
  const bool activatedWithHiddenNavCursor =
    browser_item.folder && !ImGui::GetCurrentContext()->NavCursorVisible && ImGui::IsItemFocused() &&
    (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter) || ImGui::Shortcut(ImGuiKey_Space));
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

  const ImVec2 imagePosMin(pos.x + layout.selectionBorderSize, pos.y + layout.selectionBorderSize);
  const ImVec2 imagePosMax(imagePosMin.x + layout.itemImageSize.x, imagePosMin.y + layout.itemImageSize.y);
  const bool canShowThumbnail = browser_item.asset && canShowAsset(*browser_item.asset);
  const TEXTUREID thumbnailTextureId = canShowThumbnail ? browser_item.getThumbnailTexture() : BAD_TEXTUREID;
  if (thumbnailTextureId == BAD_TEXTUREID)
  {
    // The type icons are made for the theme background, so a white background could hide them.
    drawList->AddRect(imagePosMin, imagePosMax, ImGui::GetColorU32(ImGuiCol_Border));

    if (canShowThumbnail && !browser_item.isThumbnailFailed())
    {
      drawLoadingIndicator(*drawList, imagePosMin);
    }
    else
    {
      const PropPanel::IconId iconId =
        browser_item.asset ? AssetSelectorCommon::getAssetTypeIcon(browser_item.asset->getType()) : folderIcon;
      drawItemIcon(*drawList, iconId, imagePosMin);
    }
  }
  else
  {
    // Draw a white background for images with alpha.
    drawList->AddRectFilled(imagePosMin, imagePosMax, IM_COL32(255, 255, 255, 255));
    drawItemTexture(*drawList, thumbnailTextureId, imagePosMin);
  }

  const char *name = browser_item.getName();
  const ImVec2 textPos(pos.x + layout.labelPadding, imagePosMax.y + layout.verticalSpaceBetweenImageAndLabel);
  render_wrapped_text_with_ellipsis(*drawList, textPos, layout.labelWidth, layout.labelLineCount, name);

  PropPanel::set_previous_imgui_control_tooltip(browser_item.getId(), name);

  if (ensureVisibleRequested && selected)
  {
    ensureVisibleRequested = false;
    ImGui::SetScrollHereY();
  }

  if (browser_item.folder)
  {
    // Keyboard navigation also presses the item, so open the folder only on keyboard activation (Enter or Space), not
    // on navigation.
    if (browserItemDoubleClicked || (browserItemClicked && activatedWithKeyboard) || activatedWithHiddenNavCursor)
      return browser_item.folder;
  }
  else if (browserItemClicked || browserItemDoubleClicked || browserItemRightClicked)
  {
    ignoreEnsureVisibleRequest = true;
    assetBrowserHost.assetBrowserOnAssetSelected(browser_item.asset, browserItemDoubleClicked);
    ignoreEnsureVisibleRequest = false;

    if (browserItemRightClicked)
      assetBrowserHost.assetBrowserOnContextMenu(browser_item.asset);
  }

  return nullptr;
}

// All rows use the same line count, so ImGuiListClipper can still be used.
void AssetBrowser::updateLabelLineCount(float label_width)
{
  const float fontSize = ImGui::GetFontSize();
  if (label_width == labelLineCountLabelWidth && fontSize == labelLineCountFontSize)
    return;

  labelLineCountLabelWidth = label_width;
  labelLineCountFontSize = fontSize;
  labelLineCount = 1;
  for (const BrowserItem &browserItem : browserItems)
  {
    const int currentItemLabelLineCount =
      get_label_line_count(*ImGui::GetFont(), fontSize, label_width, Layout::LABEL_LINE_COUNT_MAXIMUM, browserItem.getName());

    labelLineCount = max(labelLineCount, currentItemLabelLineCount);
    if (labelLineCount == Layout::LABEL_LINE_COUNT_MAXIMUM)
      break;
  }
}

void AssetBrowser::drawItems()
{
  if (!ImGui::BeginChild("assets", ImVec2(0.0f, 0.0f), ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
  {
    ImGui::EndChild();
    return;
  }

  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const int imageSize = getThumbnailImageSize();
  updateLabelLineCount(Layout::getLabelWidth(imageSize));
  layout.update(browserItems.size(), imageSize, availableWidth, labelLineCount);

  // Calculate and store start position.
  ImVec2 startPosition = ImGui::GetCursorScreenPos();
  startPosition = ImVec2(startPosition.x + layout.windowPadding, startPosition.y + layout.windowPadding);
  ImGui::SetCursorScreenPos(startPosition);

  ImGuiMultiSelectIO *multiSelectIo = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_SingleSelect | ImGuiMultiSelectFlags_NavWrapX,
    (selectedAsset || selectedFolder) ? 1 : 0, browserItems.size());
  G_ASSERT(multiSelectIo);
  applySelectionRequests(*multiSelectIo);

  const int columnCount = layout.columnCount;
  ImGuiListClipper clipper;
  clipper.Begin(layout.lineCount, layout.itemStep.y);

  if (ensureVisibleRequested)
  {
    auto it = eastl::find_if(browserItems.begin(), browserItems.end(),
      [this](const BrowserItem &browser_item) { return isSelected(browser_item); });

    if (it != browserItems.end())
    {
      clipper.IncludeItemByIndex((it - browserItems.begin()) / columnCount);
    }
    else
    {
      // Otherwise the request would stay, and scroll to a later selection that the host did not make.
      ensureVisibleRequested = false;
      focusItemRequested = false;
    }
  }

  if (multiSelectIo->RangeSrcItem != -1)
    clipper.IncludeItemByIndex(multiSelectIo->RangeSrcItem / columnCount);

  const DagorAssetFolder *folderToOpen = nullptr;
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

        if (const DagorAssetFolder *activatedFolder = drawItem(browserItems[itemIndex], position, itemIndex))
          folderToOpen = activatedFolder;
      }
    }
  }
  clipper.End();

  multiSelectIo = ImGui::EndMultiSelect();
  G_ASSERT(multiSelectIo);
  applySelectionRequests(*multiSelectIo);

  // Opening a folder changes browserItems, so do it after the loop.
  if (folderToOpen)
    assetBrowserHost.assetBrowserOnFolderSelected(*folderToOpen);

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

void AssetBrowser::drawFolderPath(float available_width)
{
  if (folderPath.empty())
    return;

  const char *separator = "/";
  const char *ellipsis = "...";
  const float separatorWidth = ImGui::CalcTextSize(separator).x;
  const float ellipsisWidth = ImGui::CalcTextSize(ellipsis).x + separatorWidth;

  // Like in Windows Explorer, leave out the first folders when the path does not fit.
  int firstShownIndex = folderPath.size() - 1;
  float width = ImGui::CalcTextSize(folderPath.back()->folderName).x;
  for (; firstShownIndex > 0; --firstShownIndex)
  {
    const float widthWithPrevious = width + separatorWidth + ImGui::CalcTextSize(folderPath[firstShownIndex - 1]->folderName).x;
    const float neededEllipsisWidth = firstShownIndex > 1 ? ellipsisWidth : 0.0f;
    if ((widthWithPrevious + neededEllipsisWidth) > available_width)
      break;

    width = widthWithPrevious;
  }

  const ImVec2 clipMin = ImGui::GetCursorScreenPos();
  ImGui::PushClipRect(clipMin, ImVec2(clipMin.x + available_width, clipMin.y + ImGui::GetFrameHeight()), true);
  ImGui::AlignTextToFramePadding();

  // Selecting a folder changes folderPath, so do it after the loop.
  const DagorAssetFolder *clickedFolder = nullptr;

  // Keep the link to the parent folder even if it clips the innermost folder name, to make it possible to go up.
  if (firstShownIndex > 0)
  {
    const DagorAssetFolder *hiddenFolder = folderPath[firstShownIndex - 1];
    if (PropPanel::ImguiHelper::textLinkWithoutUnderline(ellipsis))
      clickedFolder = hiddenFolder;
    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), hiddenFolder->folderName);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(separator);
    ImGui::SameLine(0.0f, 0.0f);
  }

  for (int i = firstShownIndex; i < folderPath.size(); ++i)
  {
    if (i > firstShownIndex)
    {
      ImGui::SameLine(0.0f, 0.0f);
      ImGui::TextUnformatted(separator);
      ImGui::SameLine(0.0f, 0.0f);
    }

    ImGui::PushID(i);
    if (PropPanel::ImguiHelper::textLinkWithoutUnderline(folderPath[i]->folderName))
      clickedFolder = folderPath[i];
    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), folderPath[i]->folderName);
    ImGui::PopID();
  }

  ImGui::PopClipRect();

  if (clickedFolder)
    assetBrowserHost.assetBrowserOnFolderSelected(*clickedFolder);
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

  bool showTypes = showTypesWithoutThumbnailSupport;
  if (ImGui::Checkbox("Show asset types without thumbnail support", &showTypes))
    setShowTypesWithoutThumbnailSupport(showTypes);

  bool showFoldersValue = showFolders;
  if (ImGui::Checkbox("Show folders", &showFoldersValue))
    setShowFolders(showFoldersValue);

  ImGui::NewLine();
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

void AssetBrowser::updateImgui(bool can_submit_contents)
{
  clearTexturesDrawnThisFrame();

  // Handle device reset for non-texture assets. Texture assets are reloaded by the texture factory.
  if (d3dResetCounter != get_d3d_reset_counter())
  {
    d3dResetCounter = get_d3d_reset_counter();
    for (BrowserItem &browserItem : browserItems)
      if (browserItem.asset && browserItem.asset->getType() != browserItem.asset->getMgr().getTexAssetTypeId() &&
          browserItem.assetThumbnail.has_value() && browserItem.assetThumbnail.value() != BAD_TEXTUREID)
        browserItem.release();
  }

  if (can_submit_contents)
  {
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const float spaceXBeforeButton = max(ImGui::GetContentRegionAvail().x - settingsButtonSize.x, 0.0f);
    drawFolderPath(max(spaceXBeforeButton - ImGui::GetStyle().ItemSpacing.x, 0.0f));
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
  }

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

void assets_gui_set_thumbnail_renderer(IAssetThumbnailRenderer *renderer) { thumbnail_renderer = renderer; }

IAssetThumbnailRenderer *assets_gui_get_thumbnail_renderer() { return thumbnail_renderer; }
