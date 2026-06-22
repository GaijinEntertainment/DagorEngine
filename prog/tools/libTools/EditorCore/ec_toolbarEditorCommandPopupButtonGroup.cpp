// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_toolbarEditorCommandPopupButtonGroup.h"

#include "ec_editorCommand.h"
#include "ec_toolbarEditorCommandToggleButton.h"

#include <propPanel/iconWithNameAndSize.h>
#include <propPanel/imguiHelper.h>

static constexpr float POPUP_SELECTION_ALPHA = 0.5f;
static constexpr float TOOLBAR_GROUP_POPUP_TRIGGER_DURATION_SEC = 0.5f;

ToolbarEditorCommandPopupButtonGroup::ToolbarEditorCommandPopupButtonGroup(int id, PropPanel::ControlEventHandler *event_handler,
  ContainerPropertyControl *parent) :
  ContainerPropertyControl(id, event_handler, parent)
{}

void ToolbarEditorCommandPopupButtonGroup::addControl(PropertyControlBase *pcontrol, bool new_line)
{
  G_ASSERT(pcontrol);
  G_ASSERT(pcontrol->getID() > 0);
  G_ASSERT(pcontrol->queryInterface<ToolbarEditorCommandToggleButton>());
  ContainerPropertyControl::addControl(pcontrol, new_line);

  if (activeButtonId == 0)
    activeButtonId = pcontrol->getID();
}

void ToolbarEditorCommandPopupButtonGroup::updateImgui()
{
  // Update the active button because we do not receive a notification when the button's toggle state changes from the outside (for
  // example by pressing a hotkey). Alternatively ToolbarEditorCommandToggleButton::setBoolValue() could notify the parent (us).
  for (PropertyControlBase *control : mControlArray)
  {
    ToolbarEditorCommandToggleButton *button = control->queryInterface<ToolbarEditorCommandToggleButton>();
    G_ASSERT(button);
    if (button->getBoolValue())
    {
      activeButtonId = button->getID();
      break;
    }
  }

  for (PropertyControlBase *control : mControlArray)
    if (control->getID() == activeButtonId)
    {
      ToolbarEditorCommandToggleButton *button = control->queryInterface<ToolbarEditorCommandToggleButton>();
      G_ASSERT(button);

      ImGuiID buttonId;
      bool buttonActive;
      ImRect buttonRect;
      button->toolbarToggleButtonUpdateImgui(ImDrawFlags_RoundCornersAll, &buttonId, &buttonActive, &buttonRect);

      if (mControlArray.size() > 1)
      {
        const ImVec2 rightBottom = buttonRect.GetBR();
        const float size = floorf(buttonRect.GetWidth() * 0.3f);
        ImGui::GetWindowDrawList()->AddTriangleFilled(rightBottom, ImVec2(rightBottom.x - size, rightBottom.y),
          ImVec2(rightBottom.x, rightBottom.y - size), ImGui::GetColorU32(ImGuiCol_ResizeGrip));

        updateImguiPopupButtonGroup(*button, buttonId, buttonActive);
      }
    }
}

void ToolbarEditorCommandPopupButtonGroup::updateImguiPopupContents(ToolbarEditorCommandToggleButton &active_button)
{
  const int activeButtonSize = active_button.getIcon().getLoadedSize();
  if (activeButtonSize <= 0) // Safety check only. This should not happen.
    return;

  ToolbarEditorCommandToggleButton *clickedButton = nullptr;
  for (PropertyControlBase *control : mControlArray)
  {
    ImGui::PushID(control);

    ToolbarEditorCommandToggleButton *button = control->queryInterface<ToolbarEditorCommandToggleButton>();
    G_ASSERT(button);

    const bool disabled = !button->isEnabled();
    if (disabled)
      ImGui::BeginDisabled();

    // The styling matches ToolbarToggleButtonPropertyControl.
    const bool checked = button->getBoolValue();
    if (checked)
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    else
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(PropPanel::ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER));

    PropPanel::IconWithNameAndSize &iconWithNameAndSize = button->getIcon();
    const PropPanel::IconId iconId = iconWithNameAndSize.getIconId(activeButtonSize);
    const char *displayName = getCommandDisplayName(button->getCommandId());
    const ImVec2 size(ImGui::GetContentRegionAvail().x, 0.0f);
    PropPanel::ImguiHelper::imageButtonWithText(displayName, iconId, ImVec2(activeButtonSize, activeButtonSize), size);

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered() && !disabled && !clickedButton)
      clickedButton = button;

    ImGui::PopStyleColor(2);

    if (checked)
      ImGui::PopStyleColor();
    else
      ImGui::PopStyleVar();

    if (disabled)
      ImGui::EndDisabled();

    // Using item ID for tooltip ID instead of button's address to prevent conflict with the button on the toolbar.
    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), button->getTooltipString().begin(),
      button->getTooltipString().end());

    ImGui::PopID();
  }

  if (clickedButton)
  {
    popupOpen = false;
    clickedButton->setBoolValue(true);
    clickedButton->sendOnClickNotification();
  }
}

void ToolbarEditorCommandPopupButtonGroup::updateImguiPopupButtonGroup(ToolbarEditorCommandToggleButton &active_button,
  ImGuiID button_id, bool button_active)
{
  const char *popupId = "toolbarButtonGroupPopup";
  if (!popupOpen)
  {
    if (!button_active || !ImGui::IsMouseDown(ImGuiMouseButton_Left, button_id))
      return;

    if (ImGui::GetIO().MouseDownDuration[ImGuiMouseButton_Left] < TOOLBAR_GROUP_POPUP_TRIGGER_DURATION_SEC)
      return;

    popupOpen = true;
    ImGui::OpenPopup(popupId);
  }

  if (ImGui::BeginPopup(popupId))
  {
    updateImguiPopupContents(active_button);
    ImGui::EndPopup();
  }
  else
  {
    popupOpen = false;
  }
}

const char *ToolbarEditorCommandPopupButtonGroup::getCommandDisplayName(const char *command_id)
{
  const EditorCommand *command = ec_editor_commands.getCommand(command_id);
  if (!command)
    return command_id;

  const char *displayName = command->getDisplayName();
  return *displayName ? displayName : command_id;
}
