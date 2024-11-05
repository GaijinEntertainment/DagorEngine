// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "group.h"
#include "../messageQueueInternal.h"
#include <propPanel/constants.h>

namespace PropPanel
{

class ExtGroupPropertyControl : public GroupPropertyControl
{
public:
  ExtGroupPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    GroupPropertyControl(event_handler, parent, id, x, y, w, h, caption)
  {}

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }

  virtual int getIntValue() const override
  {
    const int result = buttonStatus;
    buttonStatus = EXT_BUTTON_NONE;
    return result;
  }

  virtual void setIntValue(int value) override { buttonFlags = value; }

  virtual void updateImgui() override
  {
    // NOTE: if you modify this then you might have to modify the code in GroupPropertyControl too!

    // SetNextItemOpen does nothing if SkipItems is set (and CollapsingHeader also always returns false if SkipItems
    // is set), so to avoid unintended openness change we do nothing too if SkipItems is set.
    if (ImGui::GetCurrentWindow()->SkipItems)
      return;

    const float menuButtonWidth = ImGui::GetFrameHeight(); // Simply use a square button.
    const float spaceBetweenControls = hdpi::_pxS(Constants::SPACE_BETWEEN_BUTTONS_IN_COMBINED_CONTROL);
    const float headerWidth = ImGui::GetContentRegionAvail().x - spaceBetweenControls - menuButtonWidth;

    const bool wasMinimized = isMinimized();
    if (wasMinimized)
      ImGui::PushFont(imgui_get_bold_font());

    ImGui::SetNextItemOpen(!wasMinimized);
    setMinimized(!ImguiHelper::collapsingHeaderWidth(getStringCaption(), headerWidth));
    setFocusToPreviousImGuiControlAndScrollToItsTopIfRequested();
    showJumpToGroupContextMenuOnRightClick();

    if (wasMinimized)
      ImGui::PopFont();

    const ImVec2 headerButtonTopLeft = ImGui::GetItemRectMin();
    const ImVec2 headerButtonBottomRight = ImGui::GetItemRectMax();

    ImGui::SameLine(0.0f, spaceBetweenControls);
    makeMenuButton(menuButtonWidth);

    if (!isMinimized())
    {
      addVerticalSpaceAfterControl();

      // The indentation set in the style is too big but I did not want to hardcode another value here.
      const float indent = floorf(ImGui::GetStyle().IndentSpacing / 2.0f);
      ImGui::Indent(indent);
      ContainerPropertyControl::updateImgui();
      ImGui::Unindent(indent);

      addVerticalSpaceAfterControl();

      // Draw the half frame.
      const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      drawList->AddLine(ImVec2(headerButtonTopLeft.x, headerButtonBottomRight.y), ImVec2(headerButtonTopLeft.x, cursorPos.y),
        Constants::GROUP_BORDER_COLOR);
      drawList->AddLine(ImVec2(headerButtonTopLeft.x, cursorPos.y),
        ImVec2(headerButtonTopLeft.x + headerWidth + spaceBetweenControls + menuButtonWidth, cursorPos.y),
        Constants::GROUP_BORDER_COLOR);

      ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));

      // This is here to prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
      ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
  }

private:
  int isButtonFlagEnabled(int item_id) const { return (buttonFlags & (1 << item_id)) != 0; }

  void makeMenuButton(float menuButtonWidth)
  {
    ImGui::Button("...", ImVec2(menuButtonWidth, 0.0f));

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

  void handlePopupButtonClick(int item_id)
  {
    ImGui::CloseCurrentPopup();
    buttonStatus = item_id;
    message_queue.sendDelayedOnWcClick(*this, mId);
  }

  mutable int buttonStatus = EXT_BUTTON_NONE;
  int buttonFlags = 0;
};

} // namespace PropPanel