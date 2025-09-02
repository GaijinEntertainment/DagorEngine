// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"
#include <propPanel/control/container.h>
#include <propPanel/imguiHelper.h>
#include <stdio.h>

namespace PropPanel
{

class ExtensiblePropertyControl : public ContainerPropertyControl
{
public:
  ExtensiblePropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT; }

  int getIntValue() const override
  {
    const int result = buttonStatus;
    buttonStatus = EXT_BUTTON_NONE;
    return result;
  }

  void setIntValue(int value) override { buttonFlags = value; }

  void setTextValue(const char value[]) override { controlCaption = value; }

  int getCaptionValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  void setEnabled(bool enabled) override
  {
    controlEnabled = enabled;

    for (PropertyControlBase *control : mControlArray)
      control->setEnabled(enabled);
  }

  bool isRealContainer() override { return false; }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    if (mControlArray.empty())
    {
      char finalCaption[128];
      snprintf(finalCaption, sizeof(finalCaption), "+ %s", controlCaption.c_str());

      // Use full width.
      const ImVec2 size(ImGui::GetContentRegionAvail().x, 0.0f);

      ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
      const bool buttonPressed = ImGui::Button(finalCaption, size);
      ImGui::PopStyleVar();

      if (buttonPressed)
        handleButtonClick(EXT_BUTTON_APPEND);
    }
    else
    {
      // This is a layout table, we do not need the default padding.
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

      // Here we create a table to decrease the width of the normal controls and put the menu button after them.
      // "t" stands for table. It could be anything.
      if (ImGui::BeginTable("##t", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit))
      {
        const float menuButtonWidth = ImGui::GetFrameHeight(); // Simply use a square button.
        const float columnPadding = ImGui::GetStyle().ItemSpacing.x;
        const float column2Width = columnPadding + menuButtonWidth;
        const float column1Width = ImGui::GetContentRegionAvail().x - column2Width;

        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, column1Width);
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, column2Width);

        ImGui::TableNextColumn();
        ContainerPropertyControl::updateImgui();

        ImGui::TableNextColumn();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columnPadding);
        makeMenuButton(menuButtonWidth);

        ImGui::EndTable();
      }

      ImGui::PopStyleVar();
    }
  }

protected:
  int isButtonFlagEnabled(int item_id) const { return (buttonFlags & (1 << item_id)) != 0; }

  void handleButtonClick(int item_id)
  {
    buttonStatus = item_id;
    message_queue.sendDelayedOnWcClick(*this, mId);
  }

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
    handleButtonClick(item_id);
  }

  String controlCaption;
  bool controlEnabled = true;
  mutable int buttonStatus = EXT_BUTTON_NONE;
  int buttonFlags = 0;
};

} // namespace PropPanel