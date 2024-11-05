// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dialogManagerInternal.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/constants.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

DialogManager dialog_manager;

void DialogManager::showDialog(DialogWindow &dialog)
{
  // Already shown modeless dialogs are brought to the top, modals are not.
  G_ASSERT(!dialog.isModal() || eastl::find(dialogStack.begin(), dialogStack.end(), &dialog) == dialogStack.end());

  G_ASSERT(eastl::find(dialogsToShow.begin(), dialogsToShow.end(), &dialog) == dialogsToShow.end());
  dialogsToShow.push_back(&dialog);
}

void DialogManager::hideDialog(DialogWindow &dialog)
{
  auto it = eastl::find(dialogStack.begin(), dialogStack.end(), &dialog);
  if (it != dialogStack.end())
  {
    *it = nullptr;
    needsCompaction = true;
  }

  it = eastl::find(dialogsToShow.begin(), dialogsToShow.end(), &dialog);
  if (it != dialogsToShow.end())
    dialogsToShow.erase(it);
}

void DialogManager::showQueuedDialogs()
{
  for (DialogWindow *dialog : dialogsToShow)
  {
    G_ASSERT(dialog);

    if (dialog->isModal())
    {
      G_ASSERT(eastl::find(dialogStack.begin(), dialogStack.end(), dialog) == dialogStack.end());
    }
    else
    {
      // If a modeless dialog is already shown then bring it to the top.
      auto it = eastl::find(dialogStack.begin(), dialogStack.end(), dialog);
      if (it != dialogStack.end())
      {
        *it = nullptr;
        needsCompaction = true;
      }
    }

    dialogStack.push_back(dialog);
  }

  dialogsToShow.clear();
}

void DialogManager::compactDialogStack()
{
  for (int i = 0; i < dialogStack.size(); ++i)
  {
    if (!dialogStack[i])
    {
      dialogStack.erase(dialogStack.begin() + i);
      --i;
    }
  }
}

void DialogManager::renderModalDialog(DialogWindow &dialog, int stack_index)
{
  ImGui::PushID(&dialog);

  // We always use auto sizing here.
  bool useAutoSizeForTheCurrentFrame;
  dialog.beforeUpdateImguiDialog(useAutoSizeForTheCurrentFrame);

  // Change the color of the dialog title and the close button for the daEditorX Classic style.
  ImGui::PushStyleColor(ImGuiCol_Text, Constants::DIALOG_TITLE_COLOR);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

  // Using ImGuiCol_WindowBg here because modal and modeless dialogs must have the same color.
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

  const bool dimModalBackground = dialog.isModalBackgroundDimmingEnabled();
  if (!dimModalBackground)
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, IM_COL32(0, 0, 0, 0));

  ImGui::OpenPopup(dialog.getCaption());
  bool initiallyOpen = true;
  const ImGuiWindowFlags windowFlags = dialog.isManualModalSizingEnabled() ? ImGuiWindowFlags_None : ImGuiWindowFlags_AlwaysAutoResize;
  const bool isOpen = ImGui::BeginPopupModal(dialog.getCaption(), &initiallyOpen, windowFlags);

  ImGui::PopStyleColor(dimModalBackground ? 3 : 4);

  if (isOpen)
  {
    dialog.updateImguiDialog();

    if ((stack_index + 1) < dialogStack.size())
      renderDialog(stack_index + 1);

    ImGui::EndPopup();
  }
  else
  {
    dialog.clickDialogButton(DIALOG_ID_CLOSE);
  }

  ImGui::PopID();
}

void DialogManager::renderModelessDialog(DialogWindow &dialog)
{
  ImGui::PushID(&dialog);

  bool useAutoSizeForTheCurrentFrame;
  dialog.beforeUpdateImguiDialog(useAutoSizeForTheCurrentFrame);

  // Change the color of the dialog title and the close button for the daEditorX Classic style.
  ImGui::PushStyleColor(ImGuiCol_Text, Constants::DIALOG_TITLE_COLOR);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

  // This is hacky, but ImGui::SetNextWindowContentSize(ImVec2(0.0f, 0.0f)) did not work.
  const ImGuiWindowFlags flags = useAutoSizeForTheCurrentFrame ? ImGuiWindowFlags_AlwaysAutoResize : ImGuiWindowFlags_None;

  bool isOpen = true;
  const bool renderContents = ImGui::Begin(dialog.getCaption(), &isOpen, flags);

  ImGui::PopStyleColor(2);

  if (renderContents)
  {
    // Auto focus the dialog.
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        !ImGui::IsAnyItemActive() && !ImGui::GetIO().WantTextInput)
      ImGui::FocusWindow(ImGui::GetCurrentWindow(), ImGuiFocusRequestFlags_RestoreFocusedChild);

    dialog.updateImguiDialog();
  }

  ImGui::End();

  if (!isOpen)
    dialog.clickDialogButton(DIALOG_ID_CLOSE);

  ImGui::PopID();
}

void DialogManager::renderDialog(int stack_index)
{
  G_ASSERT(stack_index >= 0 && stack_index < dialogStack.size());

  for (int i = stack_index; i < dialogStack.size(); ++i)
  {
    DialogWindow *dialog = dialogStack[i];
    if (!dialog)
      continue;

    if (dialog->isModal())
    {
      // Modal dialog must be rendered hierarchically.
      renderModalDialog(*dialog, i);
      break;
    }
    else
    {
      // Modless dialog need no special handling, they can be rendered after each other.
      renderModelessDialog(*dialog);
    }
  }
}

void DialogManager::updateImgui()
{
  G_ASSERT(!updating);
  updating = true;

  if (!dialogsToShow.empty())
    showQueuedDialogs();

  if (needsCompaction)
  {
    compactDialogStack();
    needsCompaction = false;
  }

  if (!dialogStack.empty())
    renderDialog(0);

  G_ASSERT(updating);
  updating = false;
}

void render_dialogs() { dialog_manager.updateImgui(); }

} // namespace PropPanel