// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "group.h"
#include "../imageHelper.h"
#include "../messageQueueInternal.h"
#include "../tooltipHelper.h"
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <propPanel/iconWithNameAndSize.h>

namespace PropPanel
{

class ExtGroupPropertyControl : public GroupPropertyControl
{
public:
  ExtGroupPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[], const char *menu_button_icon = nullptr, const char *menu_button_tooltip = nullptr) :
    GroupPropertyControl(event_handler, parent, id, x, y, w, h, caption), menuButtonTooltip(menu_button_tooltip)
  {
    menuButtonIcon.setFileName(menu_button_icon);
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }

  int getIntValue() const override
  {
    const int result = buttonStatus;
    buttonStatus = EXT_BUTTON_NONE;
    return result;
  }

  void setIntValue(int value) override { buttonFlags = value; }

  void setButtonPictureValues(const char *fname) override { menuButtonIcon.setFileName(fname); }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void updateImgui() override
  {
    // NOTE: if you modify this then you might have to modify the code in GroupPropertyControl too!

    // SetNextItemOpen does nothing if SkipItems is set (and CollapsingHeader also always returns false if SkipItems
    // is set), so to avoid unintended openness change we do nothing too if SkipItems is set.
    if (ImGui::GetCurrentWindow()->SkipItems)
      return;

    const bool wasMinimized = isMinimized();
    if (wasMinimized)
      ImGui::PushFont(imgui_get_bold_font(), 0.0f);

    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_Header, getOverriddenColor(valueHighlightColor));

    ImGui::SetNextItemOpen(!wasMinimized);
    setMinimized(!ImGui::CollapsingHeader(getStringCaption(), ImGuiTreeNodeFlags_AllowOverlap));
    setFocusToPreviousImGuiControlAndScrollToItsTopIfRequested();
    setPreviousImguiControlTooltip();
    showJumpToGroupContextMenuOnRightClick();

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    if (wasMinimized)
      ImGui::PopFont();

    const ImVec2 headerButtonTopLeft = ImGui::GetItemRectMin();
    const ImVec2 headerButtonBottomRight = ImGui::GetItemRectMax();

    makeMenuButton(headerButtonBottomRight.x, headerButtonTopLeft.y);

    if (!isMinimized())
    {
      addVerticalSpaceAfterControl();

      // The indentation set in the style is too big but I did not want to hardcode another value here.
      const float indent = floorf(ImGui::GetStyle().IndentSpacing / 2.0f);
      ImGui::Indent(indent);
      ContainerPropertyControl::updateImgui();
      ImGui::Unindent(indent);

      addVerticalSpaceAfterControl();

      const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      drawHalfFrame(headerButtonTopLeft.x, headerButtonBottomRight.y, headerButtonBottomRight.x, cursorPos.y);
      ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));
      ImGui::Dummy(ImVec2(0.0f, 0.0f)); // Prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
    }
  }

private:
  int isButtonFlagEnabled(int item_id) const { return (buttonFlags & (1 << item_id)) != 0; }

  void makeMenuButtonInternal(const ImVec2 &menu_button_size, float menu_button_vertical_frame_padding)
  {
    const IconId iconId = menuButtonIcon.getIconId();
    bool buttonClicked;

    const ImVec2 newFramePadding(ImGui::GetStyle().FramePadding.x, menu_button_vertical_frame_padding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, newFramePadding);

    if (iconId == IconId::Invalid)
    {
      const ImVec2 finalSize(menu_button_size.x + (newFramePadding.x * 2.0f), menu_button_size.y + (newFramePadding.y * 2.0f));
      buttonClicked = ImGui::Button("...", finalSize);
    }
    else
    {
      const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);
      buttonClicked = ImGui::ImageButton("ib", icon, menu_button_size);
    }

    ImGui::PopStyleVar();

    tooltip_helper.setPreviousImguiControlTooltip((const void *)((uintptr_t)ImGui::GetItemID()), menuButtonTooltip.c_str(),
      menuButtonTooltip.c_str() + menuButtonTooltip.length());

    if (isButtonFlagEnabled(EXT_BUTTON_SINGLE_ACTION))
    {
      if (buttonClicked)
      {
        buttonStatus = EXT_BUTTON_SINGLE_ACTION;
        message_queue.sendDelayedOnWcClick(*this, mId);
      }
      return;
    }

    // With nullptr as the parameter this uses the previous ID, so the ID of the ... button.
    if (!ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
      return;

    if (isButtonFlagEnabled(EXT_BUTTON_RENAME))
    {
      if (ImGui::MenuItem("Rename"))
        handlePopupButtonClick(EXT_BUTTON_RENAME);

      ImGui::Separator();
    }

    if (isButtonFlagEnabled(EXT_BUTTON_INSERT))
      if (ImGui::MenuItem("Insert"))
        handlePopupButtonClick(EXT_BUTTON_INSERT);

    if (isButtonFlagEnabled(EXT_BUTTON_REMOVE))
      if (ImGui::MenuItem("Remove"))
        handlePopupButtonClick(EXT_BUTTON_REMOVE);

    if (isButtonFlagEnabled(EXT_BUTTON_UP) || isButtonFlagEnabled(EXT_BUTTON_DOWN))
    {
      ImGui::Separator();

      if (isButtonFlagEnabled(EXT_BUTTON_UP))
        if (ImGui::MenuItem("Move up"))
          handlePopupButtonClick(EXT_BUTTON_UP);

      if (isButtonFlagEnabled(EXT_BUTTON_DOWN))
        if (ImGui::MenuItem("Move down"))
          handlePopupButtonClick(EXT_BUTTON_DOWN);
    }

    ImGui::EndPopup();
  }

  void makeMenuButton(float header_button_right_x, float header_button_top_y)
  {
    const float fontSize = ImGui::GetFontSize();
    const ImVec2 menuButtonSize = ImVec2(fontSize, fontSize); // Simply use a square button.
    const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
    const float menuButtonWidthWithFramePadding = menuButtonSize.x + (framePadding.x * 2.0f);

    // Have some space above and below the button to make it more clear that it is within the minimize/maximize button.
    const float menuButtonVerticalFramePadding = floorf(framePadding.y * 0.5f);

    // Vertically offset the button to make it centered within the minimize/maximize button.
    const float menuButtonVerticalOffset = (framePadding.y - menuButtonVerticalFramePadding);

    // Extrusion amount that TreeNodeBehavior uses when ImGuiTreeNodeFlags_Framed is set.
    const float headerButtonExtrusionMaxX = IM_TRUNC(ImGui::GetCurrentWindow()->WindowPadding.x * 0.5f);

    const float menuButtonX = header_button_right_x - headerButtonExtrusionMaxX - menuButtonWidthWithFramePadding;
    const float menuButtonY = header_button_top_y + menuButtonVerticalOffset;

    const ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(menuButtonX, menuButtonY));
    makeMenuButtonInternal(menuButtonSize, menuButtonVerticalFramePadding);
    ImGui::SetCursorScreenPos(originalCursorPos);
  }

  void handlePopupButtonClick(int item_id)
  {
    ImGui::CloseCurrentPopup();
    buttonStatus = item_id;
    message_queue.sendDelayedOnWcClick(*this, mId);
  }

  mutable int buttonStatus = EXT_BUTTON_NONE;
  int buttonFlags = 0;
  IconWithName menuButtonIcon;
  SimpleString menuButtonTooltip;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel