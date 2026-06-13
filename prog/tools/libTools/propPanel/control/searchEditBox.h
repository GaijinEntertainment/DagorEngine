// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "editBox.h"

#include <propPanel/toastManager.h>

namespace PropPanel
{

class SearchEditBoxPropertyControl : public EditBoxPropertyControl
{
public:
  SearchEditBoxPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    EditBoxPropertyControl(id, event_handler, parent, x, y, w, h, caption, false, false)
  {
    iconMain = load_icon("search");
    iconClear = load_icon("close_editor");
  }

  ~SearchEditBoxPropertyControl() override { removeToast(); }

  void setButtonPictureValues(const char *fname) override { iconMain = load_icon(fname); }

  void setErrorMessageToast(const char *message) override
  {
    // Do not recreate the toast message if it still fully visible and has the same error message to prevent blinking.
    const ToastMessage *toastMessage = get_toast_message(toastId);
    if (toastMessage && !toastMessage->isFadeoutStarted() && toastMessage->text == message)
      return;

    removeToast();
    errorMessage = message;
    toastCreationRequested = !errorMessage.empty();
  }

private:
  void updateImguiInput(const char *input_label, bool &text_changed, bool &deactivated_after_edit) override
  {
    const bool patchColors = valueHighlightColor == ColorOverride::EDIT_BOX_SEARCH_TEXT_SET_BACKGROUND;
    if (patchColors)
    {
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
      ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
    }

    ImRect inputRect;
    text_changed = ImguiHelper::searchInput(this, input_label, placeholderText, controlValue, iconMain, iconClear, &textInputFocused,
      nullptr, &inputRect, &deactivated_after_edit);

    if (patchColors)
      ImGui::PopStyleColor(2);

    if (toastCreationRequested)
    {
      toastCreationRequested = false;
      createToast(getToastPosition(inputRect));
    }

    if (!textInputFocused)
      removeToast();
  }

  IPoint2 getToastPosition(const ImRect &edit_box_rect) const
  {
    // Very rough estimation.
    ImVec2 toastSize = ImGui::CalcTextSize(errorMessage);
    toastSize.x += ImGui::GetStyle().FramePadding.x + ImguiHelper::getFontSizedIconSize().x;
    toastSize.y += ImGui::GetStyle().FramePadding.y;

    const ImRect allowedRect = ImGui::GetCurrentWindow()->Viewport->GetMainRect();

    ImRect avoidRect = edit_box_rect;
    avoidRect.Expand(ImGui::GetStyle().ItemSpacing);

    ImGuiDir lastDir = ImGuiDir_Down;
    const ImVec2 position = ImGui::FindBestWindowPosForPopupEx(edit_box_rect.GetBL(), toastSize, &lastDir, allowedRect, avoidRect,
      ImGuiPopupPositionPolicy_Tooltip);
    return IPoint2(position.x, position.y);
  }

  void createToast(const IPoint2 &toast_position)
  {
    removeToast();

    ToastMessage toastMessage;
    toastMessage.type = ToastType::Alert;
    toastMessage.text = errorMessage;
    toastMessage.disableFadeOutAnimation();
    toastMessage.keepVisibleIndefinitely();
    toastMessage.setHideOnMouseMove(true);
    toastMessage.setPosition(toast_position, ImVec2(0.0f, 0.0f));
    toastId = set_toast_message(toastMessage);
  }

  void removeToast()
  {
    if (toastId != ToastMessage::INVALID_ID)
    {
      remove_toast_message(toastId);
      toastId = ToastMessage::INVALID_ID;
    }
  }

  IconId iconMain = IconId::Invalid;
  IconId iconClear = IconId::Invalid;
  String errorMessage;
  uint64_t toastId = ToastMessage::INVALID_ID;
  bool toastCreationRequested = false;
};

} // namespace PropPanel