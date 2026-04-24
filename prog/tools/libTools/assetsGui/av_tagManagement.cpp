// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <assetsGui/av_ids.h>
#include <assetsGui/av_tagManagement.h>

#include <assets/asset.h>
#include <assets/assetHlp.h>

#include <osApiWrappers/dag_localConv.h>

#include <util/dag_fastIntList.h>

#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <propPanel/toastManager.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <propPanel/messageQueue.h>
#include <propPanel/colors.h>

#include <imgui/imgui_internal.h>


using hdpi::_pxScaled;

AssetTagManager::AssetTagManager(const DagorAsset *_a) : lastSelectedAsset(_a) { resetInput(); }

static bool is_not_alnum_under(char c) { return !(isalpha(c) || isdigit(c) || (c == '_')); }

static bool is_tag_name_valid(const eastl::string &name)
{
  return eastl::find_if(name.begin(), name.end(), is_not_alnum_under) == name.end();
}

static bool tags_equal(const eastl::string &a, const eastl::string &b) { return dd_stricmp(a.c_str(), b.c_str()) == 0; }

static void calcFramedLabelWithCloseButtonSize(const char *label, ImVec2 &size, float &close_pos_x)
{
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec2 padding = style.FramePadding;
  const ImVec2 labelSize = ImGui::CalcTextSize(label);
  const float closeSize = ImGui::GetCurrentContext()->FontSize;
  close_pos_x = labelSize.x + style.ItemSpacing.x;
  size = ImVec2(close_pos_x + closeSize, max(labelSize.y, closeSize));
}

static bool framedLabelWithCloseButton(const char *label, ImVec2 &size, float &close_pos_x, const char *close_tooltip = nullptr)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec2 padding = style.FramePadding;
  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);
  ImGui::ItemSize(bb);

  const ImGuiID id = window->GetID(label);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  // If we simply used ImGuiItemFlags_AllowOverlap then the cursor would turn to text input cursor on the entire control
  // with the exception of the close button. So we only allow overlap if the cursor is over the close button. Hacky...
  const ImVec2 closeButtonPos = bb.Min + padding + ImVec2(close_pos_x, 0);
  const float closeButtonSize = ImGui::GetCurrentContext()->FontSize;
  if (ImGui::IsMouseHoveringRect(closeButtonPos, closeButtonPos + ImVec2(closeButtonSize, closeButtonSize)))
    ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
  else
    ImGui::ItemHoverable(bb, id, ImGuiItemFlags_None);

  ImGui::RenderNavCursor(bb, id);

  // Render frame
  const ImU32 backgroundColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::TAG_BACKGROUND);
  window->DrawList->AddRectFilled(bb.Min, bb.Max, backgroundColor, style.FrameRounding);
  if (ImGui::IsItemHovered() && !ImGui::IsItemFocused())
  {
    const ImU32 borderColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::TAG_BORDER);
    window->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max + ImVec2(1, 1), borderColor, style.FrameRounding, 0, 2.0f);
    window->DrawList->AddRect(bb.Min, bb.Max, borderColor, style.FrameRounding, 0, 2.0f);
  }

  // Render text
  const ImU32 textColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::FILTER_TEXT);
  window->DrawList->AddText(bb.Min + padding, textColor, label);

  // Close button
  ImGui::PushStyleColor(ImGuiCol_Text, textColor);
  ImGui::PushStyleColor(ImGuiCol_Button, backgroundColor);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, backgroundColor);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, backgroundColor);
  ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4(0, 0, 0, 0));
  const ImGuiID closeButtonId = ImGui::GetIDWithSeed("#CLOSE", nullptr, id);
  const bool closed = ImGui::CloseButton(closeButtonId, closeButtonPos);
  ImGui::PopStyleColor(5);

  if (close_tooltip)
    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), close_tooltip);

  return closed;
}

bool AssetTagManager::updateImgui()
{
  ImGui::PushID(this);

  ImGui::PushFont(imgui_get_bold_font(), 0.0f);
  if (lastSelectedAsset)
  {
    ImGui::TextUnformatted(lastSelectedAsset->getName());
  }
  else
  {
    ImGui::Text("--No assets selected--");
  }
  ImGui::PopFont();

  // Use full width by default, but for layout reasons always substruct the hint height!
  ImVec2 size = ImGui::GetContentRegionAvail();
  size.y -= ImGui::GetTextLineHeightWithSpacing();
  const bool inputWasActive = inputLastActive;

  if (inputDisabled)
    ImGui::BeginDisabled();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, ImGui::GetStyle().FrameBorderSize);
  bool childVisible = ImGui::BeginChildEx(nullptr, ImGui::GetID("##tagFrame"), size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
  if (!childVisible)
  {
    ImGui::EndChild();

    if (inputDisabled)
      ImGui::EndDisabled();
  }
  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(1);

  if (lastSelectedAsset)
  {
    const FastIntList &tags = assettags::getTagIds(*lastSelectedAsset);
    if (!tags.empty())
    {
      ImGuiID closeId = ImGui::GetID("Close");
      ImVec2 pos = ImGui::GetCursorScreenPos();
      const float closeSize = ImGui::GetCurrentContext()->FontSize;
      pos.x += (ImGui::GetContentRegionAvail().x - closeSize);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
      bool removeTags = ImGui::CloseButton(closeId, pos);
      ImGui::PopStyleColor(1);
      PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Remove all tags");
      const float spacedCloseSize = ImGui::GetStyle().ItemSpacing.x * 2 + closeSize;

      if (removeTags)
      {
        PropPanel::request_delayed_callback(*this);
      }

      FastIntList tagsToDel;
      int lineIndex = 0;
      for (int i = 0; i < tags.size(); ++i)
      {
        int tagId = tags[i];
        const char *tagName = assettags::getTagName(tagId);
        ImVec2 tagButtonSize;
        float tagButtonClosePos;
        calcFramedLabelWithCloseButtonSize(tagName, tagButtonSize, tagButtonClosePos);

        if (lineIndex > 0)
          ImGui::SameLine();

        if ((ImGui::GetContentRegionAvail().x - spacedCloseSize) <= tagButtonSize.x && i > 0)
        {
          ImGui::NewLine();
          lineIndex = 0;
        }

        if (framedLabelWithCloseButton(tagName, tagButtonSize, tagButtonClosePos, "Remove tag"))
          tagsToDel.addInt(tagId);

        lineIndex++;
      }
      if (!tagsToDel.empty())
        assettags::delTagIds(*lastSelectedAsset, tagsToDel);
    }
  }

  ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_None;
  if (inputDisabled)
  {
    ImGui::BeginDisabled();
    inputFlags |= ImGuiInputTextFlags_ReadOnly;
  }

  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  const bool hovered = ImGui::ItemHoverable(window->Rect(), window->ID, g.LastItemData.ItemFlags);
  if (hovered)
  {
    ImGui::GetCurrentContext()->MouseCursor = ImGuiMouseCursor_TextInput;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      ImGui::SetKeyboardFocusHere();
  }

  if (inputTagNames.size() > 0)
  {
    // Draw background error rects
    ImU32 invalidColor = PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::TAG_ERROR_BACKGROUND);
    ImVec2 clipRectMin = window->DC.CursorPos;
    ImVec2 clipRectMax = window->DC.CursorPos + ImGui::GetContentRegionAvail();
    window->DrawList->PushClipRect(clipRectMin, clipRectMax);
    for (int i = 0; i < inputTagNames.size(); ++i)
    {
      G_ASSERT(inputTagNameValidity.size() == inputTagNames.size());
      if (inputTagNameValidity[i])
        continue;

      G_ASSERT(inputTagNamePositions.size() == inputTagNames.size());
      G_ASSERT(inputTagNameSizes.size() == inputTagNames.size());
      const ImVec2 startPos = window->DC.CursorPos + inputTagNamePositions[i];
      const ImRect bb(startPos, startPos + inputTagNameSizes[i]);
      window->DrawList->AddRectFilled(bb.Min, bb.Max, invalidColor, 0.0f);
    }
    window->DrawList->PopClipRect();
  }

  ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

  const bool textChanged = ImGuiDagor::InputTextWithHint(AssetTagInputLabel, inputHint, &inputText, inputFlags);

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);

  if (inputDisabled)
    ImGui::EndDisabled();

  bool scrollXChanged = false;
  inputLastActive = ImGui::IsItemActive();
  if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(AssetTagInputLabel)))
  {
    if (forceReloadInputBuf)
    {
      inputText.clear();
      forceReloadInputBuf = false;
      inputState->ReloadUserBufAndMoveToEnd();
    }

    if (inputLastActive)
    {
      if (lastScrollX != inputState->Scroll.x)
      {
        lastScrollX = inputState->Scroll.x;
        scrollXChanged = true;
      }
    }
    else if (inputWasActive)
    {
      lastScrollX = 0.0f;
      scrollXChanged = true;
    }
  }

  if ((inputWasActive && textChanged) || scrollXChanged)
  {
    allTagsValid = true;
    allTagsReallyNew = true;

    // Split input tag names
    inputTagNames.clear();
    inputTagNameSizes.clear();
    inputTagNamePositions.clear();
    inputTagNameValidity.clear();
    if (inputText.size() > 0)
    {
      eastl_size_t lb = inputText.find('\n');
      if (lb != eastl::string::npos)
        inputText.erase(lb);

      eastl_size_t start = 0;
      eastl_size_t end = 0;
      while ((start = inputText.find_first_not_of(' ', end)) != eastl::string::npos)
      {
        end = inputText.find(' ', start);
        eastl::string tagName = inputText.substr(start, end - start);
        inputTagNames.push_back(tagName);
        inputTagNameSizes.push_back(ImGui::CalcTextSize(tagName.c_str()));
        ImVec2 pos = ImGui::CalcTextSize(inputText.substr(0, start).c_str());
        pos.x -= lastScrollX;
        pos.y -= ImGui::GetCurrentContext()->FontSize;
        inputTagNamePositions.push_back(pos);
        inputTagNameValidity.push_back(true);
      }
    }

    if (lastSelectedAsset)
    {
      // Check if tags are valid and unique/new
      const FastIntList &tagIds = assettags::getTagIds(*lastSelectedAsset);
      FastIntList inputTagIds;
      Tab<const char *> newTagNames;
      auto validity = inputTagNameValidity.begin();
      for (auto it = inputTagNames.begin(); it != inputTagNames.end(); ++it)
      {
        bool &valid = *validity;
        validity++;

        // Skip tag-names with invalid characters
        const eastl::string &inputTagName = *it;
        if (!is_tag_name_valid(inputTagName))
        {
          allTagsValid = false;
          valid = false;
          continue;
        }

        // Skip tag-name if already processed previously
        if (eastl::count(inputTagNames.begin(), it, inputTagName, tags_equal) > 0)
        {
          allTagsReallyNew = false;
          valid = false;
          continue;
        }

        // Skip tag-names already added to asset and process new names
        const char *tagstr = inputTagName.c_str();
        const int inputTagId = assettags::getTagNameId(tagstr);
        if (inputTagId > -1)
        {
          if (tagIds.hasInt(inputTagId))
          {
            allTagsReallyNew = false;
            valid = false;
            continue;
          }
          inputTagIds.addInt(inputTagId);
        }
        else
        {
          newTagNames.push_back(tagstr);
        }
      }

      if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
      {
        for (auto newTagName : newTagNames)
          inputTagIds.addInt(assettags::addTagName(newTagName));

        assettags::addTagIds(*lastSelectedAsset, inputTagIds);

        if (!allTagsValid || !allTagsReallyNew)
        {
          PropPanel::ToastMessage message;
          message.type = PropPanel::ToastType::Alert;
          message.text = "Symbols were not generated into tag!";
          message.fadeoutOnlyOnMouseLeave = true;
          message.setToMousePos(Point2(0.5f, 0.5f));
          PropPanel::set_toast_message(message);

          ImGui::SetKeyboardFocusHere(-1);
        }
        else
        {
          ImGui::ClearActiveID();
        }

        resetInput();
      }
    }
  }

  if (childVisible)
  {
    ImGui::EndChild();

    if (inputDisabled)
      ImGui::EndDisabled();
  }

  if (inputWasActive)
  {
    if (allTagsValid && allTagsReallyNew)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::FILTER_TEXT));
      ImGui::Text("Use only A-Z, a-z, 0-9 and underscores _");
      ImGui::PopStyleColor();
    }
    else if (!allTagsValid)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TAG_ERROR_TEXT));
      ImGui::Text("Invalid characters used. Use only A-Z, a-z, 0-9 and underscores _");
      ImGui::PopStyleColor();
    }
    else
    {
      ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TAG_ERROR_TEXT));
      ImGui::Text("This tag already exists. Please enter a unique tag");
      ImGui::PopStyleColor();
    }
  }

  ImGui::PopID();

  return !ImGui::Shortcut(WINDOW_TOGGLE_HOTKEY);
}

void AssetTagManager::onImguiDelayedCallback(void *data)
{
  PropPanel::DialogWindow dialog(nullptr, hdpi::_pxScaled(280), hdpi::_pxScaled(120), "Unsaved Changes");
  PropPanel::ContainerPropertyControl *panel = dialog.getPanel();
  panel->createStatic(0, "All tags will be removed.\nWould you like to remove all created tags?");
  dialog.setDialogButtonText(PropPanel::DIALOG_ID_OK, "Yes, confirm");
  dialog.setDialogButtonText(PropPanel::DIALOG_ID_CANCEL, "No, cancel");

  if (dialog.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    if (lastSelectedAsset)
    {
      const FastIntList tags = assettags::getTagIds(*lastSelectedAsset);
      assettags::delTagIds(*lastSelectedAsset, tags);
    }
  }
}

void AssetTagManager::onAvSelectAsset(DagorAsset *asset, const char *)
{
  lastSelectedAsset = asset;
  resetInput();
}

void AssetTagManager::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *)
{
  lastSelectedAsset = nullptr;
  resetInput();
}

void AssetTagManager::resetInput()
{
  inputText.clear();
  forceReloadInputBuf = true;
  if (ImGui::GetCurrentWindowRead() != nullptr)
  {
    if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(AssetTagInputLabel)))
    {
      forceReloadInputBuf = false;
      inputState->ReloadUserBufAndMoveToEnd();
    }
  }
  inputHint = "";
  lastScrollX = 0;

  inputTagNames.clear();
  inputTagNameSizes.clear();
  inputTagNamePositions.clear();
  inputTagNameValidity.clear();

  if (lastSelectedAsset)
  {
    inputDisabled = false;
    const char *type = lastSelectedAsset->getTypeStr();
    if (strcmp(type, "collision") == 0)
    {
      inputHint = "Tags cannot be created for collisions";
      inputDisabled = true;
    }
    else
    {
      if (assettags::getTagIds(*lastSelectedAsset).size() == 0)
        inputHint = "Enter tags separated by space";
    }
  }
  else
  {
    inputHint = "Select asset for editing tags";
    inputDisabled = true;
  }

  allTagsValid = true;
  allTagsReallyNew = true;
}


AssetTagsDlg::AssetTagsDlg(void *phandle, const DagorAsset *selected_asset) :
  DialogWindow(phandle, _pxScaled(DEFAULT_MINIMUM_WIDTH), _pxScaled(DEFAULT_MINIMUM_HEIGHT), ""), mTagManager(selected_asset)
{
  setCaption("Tag Management##tagManagementModal");

  showButtonPanel(false);
  setManualModalSizingEnabled();

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel && "AssetTagsDlg : No panel with controls");

  panel->createCustomControlHolder(AssetsGuiIds::TagManager, this);

  if (!hasEverBeenShown())
    setWindowSize(IPoint2(hdpi::_pxS(DEFAULT_WIDTH), hdpi::_pxS(DEFAULT_HEIGHT)));
  centerWindowToMousePos();
}

void AssetTagsDlg::customControlUpdate(int id)
{
  if (!mTagManager.updateImgui())
    clickDialogButton(PropPanel::DIALOG_ID_CLOSE);
}
