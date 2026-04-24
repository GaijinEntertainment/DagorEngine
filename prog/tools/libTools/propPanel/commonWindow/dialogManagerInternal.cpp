// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dialogManagerInternal.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <gui/dag_imgui.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

DialogManager dialog_manager;
IModalDialogEventHandler *modal_dialog_event_handler = nullptr;

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

static void RenderDropShadowCornerArc(ImDrawList *dl, const ImVec2 &center, float size, int a_side, ImU32 col)
{
  static const int shadowCornerArcSample = IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 4;
  const int aMin = a_side * shadowCornerArcSample;
  const int aMax = aMin + shadowCornerArcSample;
  dl->_PathArcToFastEx(center, size, aMin, aMax, 2);
  ImVector<ImVec2> inner = dl->_Path;
  const int count = inner.Size - 1;
  dl->PathClear();

  dl->_PathArcToFastEx(center, size * 2.0f, aMin, aMax, 2);
  ImVector<ImVec2> outer = dl->_Path;
  dl->PathClear();

  unsigned int vtx_base = dl->_VtxCurrentIdx;
  dl->PrimReserve(count * 6, (count + 1) * 2);

  // Submit vertices of the two arcs
  const ImVec2 uv = dl->_Data->TexUvWhitePixel;
  for (int n = 0; n <= count; n++)
  {
    dl->PrimWriteVtx(inner[n], uv, col);
    dl->PrimWriteVtx(outer[n], uv, 0);
  }

  // Submit a strip of triangles
  for (int n = 0; n < count; n++)
  {
    int base = vtx_base + n * 2;

    // Triangle 1
    dl->PrimWriteIdx((ImDrawIdx)(base));
    dl->PrimWriteIdx((ImDrawIdx)(base + 1));
    dl->PrimWriteIdx((ImDrawIdx)(base + 1 + 2));

    // Triangle 2
    dl->PrimWriteIdx((ImDrawIdx)(base));
    dl->PrimWriteIdx((ImDrawIdx)(base + 1 + 2));
    dl->PrimWriteIdx((ImDrawIdx)(base + 1 + 1));
  }
}

static void RenderDropShadow(float size, ImU8 opacity)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  ImVec2 p = window->Pos;
  ImVec2 s = window->Size;
  ImVec2 m = {p.x + s.x, p.y + s.y};

  // Sadly the title and frame has to be re-rendered in a new command
  // to avoid rounding/precision related artifacts along the corners.
  ImDrawList *wdl = window->DrawList;
  ImDrawList *wdlClone = wdl->CloneOutput();

  wdl->PushClipRectFullScreen();

  ImU32 col = IM_COL32(0, 0, 0, opacity);

  // Shadow sides
  wdl->AddRectFilledMultiColor({p.x + size, p.y - size}, {m.x - size, p.y}, 0, 0, col, col);
  wdl->AddRectFilledMultiColor({p.x - size, p.y + size}, {p.x, m.y - size}, 0, col, col, 0);
  wdl->AddRectFilledMultiColor({m.x, p.y + size}, {m.x + size, m.y - size}, col, 0, 0, col);
  wdl->AddRectFilledMultiColor({p.x + size, m.y}, {m.x - size, m.y + size}, col, col, 0, 0);

  RenderDropShadowCornerArc(wdl, {m.x - size, m.y - size}, size, 0, col);
  RenderDropShadowCornerArc(wdl, {p.x + size, m.y - size}, size, 1, col);
  RenderDropShadowCornerArc(wdl, {p.x + size, p.y + size}, size, 2, col);
  RenderDropShadowCornerArc(wdl, {m.x - size, p.y + size}, size, 3, col);

  wdl->PopClipRect();

  // Re-render the window title and frame.
  wdl->AddDrawCmd();
  for (int i = 0; i < wdlClone->CmdBuffer.Size; ++i)
  {
    ImDrawCmd cmd = wdlClone->CmdBuffer[i];
    cmd.IdxOffset += wdl->IdxBuffer.Size;
    wdl->CmdBuffer.push_back(cmd);
  }
  for (int i = 0; i < wdlClone->IdxBuffer.Size; ++i)
  {
    wdl->IdxBuffer.push_back(wdlClone->IdxBuffer[i]);
  }
  // push_back doesn't update _IdxWritePtr; sync it manually to satisfy ImGui's assertion in AddDrawListToDrawDataEx.
  wdl->_IdxWritePtr = wdl->IdxBuffer.Data + wdl->IdxBuffer.Size;

  IM_DELETE(wdlClone);
}

void DialogManager::renderModalDialog(DialogWindow &dialog, int stack_index)
{
  ImGui::PushID(&dialog);

  // We always use auto sizing here.
  bool useAutoSizeForTheCurrentFrame;
  dialog.beforeUpdateImguiDialog(useAutoSizeForTheCurrentFrame);

  // Change the color of the modal dialog title and the close button for the daEditorX Classic style.
  PropPanel::pushModalWindowColorOverrides();

  // Using ImGuiCol_WindowBg here because modal and modeless dialogs must have the same color.
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

  const bool dimModalBackground = dialog.isModalBackgroundDimmingEnabled();
  if (!dimModalBackground)
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, IM_COL32(0, 0, 0, 0));

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, hdpi::_pxS(PropPanel::Constants::MODAL_WINDOW_ROUNDING));
  const float padding = hdpi::_pxS(PropPanel::Constants::MODAL_WINDOW_PADDING);
  const float windowPadding = padding * 2.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(windowPadding, padding));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding, windowPadding));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, hdpi::_pxS(1));

  ImGui::OpenPopup(dialog.getCaption());
  bool initiallyOpen = true;
  bool *pOpen = dialog.getCloseButtonVisible() ? &initiallyOpen : nullptr;

  ImFont *font = ImGui::GetFont();
  ImGui::PushFont(imgui_get_bold_font(), 0.0f);
  const ImGuiWindowFlags windowFlags = dialog.isManualModalSizingEnabled() ? ImGuiWindowFlags_None : ImGuiWindowFlags_AlwaysAutoResize;
  const bool isOpen = ImGui::BeginPopupModal(dialog.getCaption(), pOpen, windowFlags);
  ImGui::PushFont(font, 0.0f); // Default font has to be re-pushed, because Begin clears internal stacks!

  ImGui::PopStyleVar(4);

  ImGui::PopStyleColor(dimModalBackground ? 1 : 2);

  PropPanel::popModalWindowColorOverrides();

  RenderDropShadow(hdpi::_pxS(8), 64);

  if (isOpen)
  {
    dialog.updateImguiDialog();

    if ((stack_index + 1) < dialogStack.size())
      renderDialog(stack_index + 1);

    ImGui::PopFont(); // Pop default font

    ImGui::EndPopup();
  }
  else
  {
    ImGui::PopFont(); // Pop default font

    dialog.clickDialogButton(DIALOG_ID_CLOSE);
  }

  ImGui::PopFont();

  ImGui::PopID();
}

void DialogManager::renderModelessDialog(DialogWindow &dialog)
{
  ImGui::PushID(&dialog);

  bool useAutoSizeForTheCurrentFrame;
  dialog.beforeUpdateImguiDialog(useAutoSizeForTheCurrentFrame);

  // Change the color of the dialog title and the close button for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();

  // This is hacky, but ImGui::SetNextWindowContentSize(ImVec2(0.0f, 0.0f)) did not work.
  const ImGuiWindowFlags flags = useAutoSizeForTheCurrentFrame ? ImGuiWindowFlags_AlwaysAutoResize : ImGuiWindowFlags_None;

  bool isOpen = true;
  const bool renderContents = ImGui::Begin(dialog.getCaption(), &isOpen, flags);

  PropPanel::popDialogTitleBarColorOverrides();

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

bool DialogManager::isAnyModalDialogOpen() const
{
  for (const DialogWindow *dialog : dialogStack)
    if (dialog && dialog->isModal())
      return true;

  return false;
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

void set_modal_dialog_events(IModalDialogEventHandler *event_handler) { modal_dialog_event_handler = event_handler; }

bool is_any_modal_dialog_open() { return dialog_manager.isAnyModalDialogOpen(); }

} // namespace PropPanel