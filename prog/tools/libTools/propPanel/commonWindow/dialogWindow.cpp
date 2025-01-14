// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <propPanel/commonWindow/dialogWindow.h>
#include "dialogManagerInternal.h"
#include <propPanel/control/container.h>
#include <propPanel/focusHelper.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <winGuiWrapper/wgw_busy.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <workCycle/dag_workCycle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class DialogButtonsHandler : public PropPanel::ControlEventHandler
{
public:
  explicit DialogButtonsHandler(DialogWindow &in_dialog) : dialog(in_dialog) {}

  virtual void onClick(int pcb_id, ContainerPropertyControl *panel) override
  {
    if (pcb_id == DIALOG_ID_OK && dialog.onOk())
      dialog.hide(pcb_id);
    else if (pcb_id == DIALOG_ID_CANCEL && dialog.onCancel())
      dialog.hide(pcb_id);
    else if (pcb_id == DIALOG_ID_CLOSE && dialog.onClose())
      dialog.hide(dialog.closeReturn());
  }

private:
  DialogWindow &dialog;
};

DialogWindow::DialogWindow(void *phandle, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel) :
  dialogCaption(caption), dialogResult(DIALOG_ID_NONE)
{
  create(_px(w), _px(h), hide_panel);
}


DialogWindow::DialogWindow(void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel) :
  dialogCaption(caption), dialogResult(DIALOG_ID_NONE)
{
  create(_px(w), _px(h), hide_panel);
}

DialogWindow::~DialogWindow()
{
  delete propertiesPanel;
  delete buttonsPanel;
  delete buttonEventHandler;

  if (visible)
    dialog_manager.hideDialog(*this);
}

ContainerPropertyControl *DialogWindow::getPanel() { return propertiesPanel ? propertiesPanel->getContainer() : nullptr; }

void DialogWindow::create(unsigned w, unsigned h, bool hide_panel)
{
  initialWidth = w;
  initialHeight = h;

  G_ASSERT(!propertiesPanel);
  if (!hide_panel)
    propertiesPanel = new ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO);

  buttonEventHandler = new DialogButtonsHandler(*this);
  buttonsPanel = new ContainerPropertyControl(0, buttonEventHandler, nullptr, 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO);
  buttonsPanel->createButton(DIALOG_ID_OK, "Ok");
  buttonsPanel->createButton(DIALOG_ID_CANCEL, "Cancel", true, false);
}

void DialogWindow::show()
{
  modal = false;

  if (initialFocusId == DIALOG_ID_OK || initialFocusId == DIALOG_ID_CANCEL)
    buttonsPanel->setFocusById(initialFocusId);

  // Modals (BeginPopupModal) are centered by ImGui, so this is only needed here.
  if (!moveRequested && (!dockingRequested || dockingRequestNodeId == 0))
    centerWindow();

  if (!visible)
  {
    visible = true;
    dialog_manager.showDialog(*this);
  }
}

int DialogWindow::showDialog()
{
  // We cannot start a modal message loop if we are in an ImGui frame.
  // See the notes at the PropPanel::MessageQueue class.
  if (ImGui::GetCurrentContext()->WithinFrameScope)
  {
    debug_dump_stack();
    wingw::message_box(wingw::MBS_OK | wingw::MBS_EXCL, "Error!",
      "A modal dialog is supposed to show up here!\n\nPlease send the log file to the developers!");
    dialogResult = DIALOG_ID_NONE;
    return dialogResult;
  }

  G_ASSERT(!visible);

  const bool wasBusy = wingw::set_busy(false);

  dialogResult = DIALOG_ID_NONE;
  modal = true;
  visible = true;

  if (initialFocusId == DIALOG_ID_OK || initialFocusId == DIALOG_ID_CANCEL)
  {
    const void *oldControlToFocus = focus_helper.getRequestedControlToFocus();
    buttonsPanel->setFocusById(initialFocusId);

    // Re-request the focus but with the focus rectangle displayed. This allows pressing the button with the Space key.
    const void *newControlToFocus = focus_helper.getRequestedControlToFocus();
    if (newControlToFocus != oldControlToFocus)
      focus_helper.requestFocus(newControlToFocus, /*show_focus_rectangle = */ true);
  }

  dialog_manager.showDialog(*this);

  while (visible)
  {
    d3d::GpuAutoLock gpuLock;

    dagor_work_cycle_flush_pending_frame();
    dagor_draw_scene_and_gui(true, true);
    d3d::update_screen();
    dagor_work_cycle();
  }

  wingw::set_busy(wasBusy);

  return dialogResult;
}

void DialogWindow::hide(int result)
{
  if (result != DIALOG_ID_NONE)
    dialogResult = result;

  if (closeEventHandler)
    closeEventHandler->onClick(-DIALOG_ID_CLOSE, nullptr);

  if (visible)
  {
    visible = false;
    dialog_manager.hideDialog(*this);
  }
}

IPoint2 DialogWindow::getWindowPosition() const
{
  ImGuiWindow *window = ImGui::FindWindowByName(dialogCaption);
  if (!window)
    return IPoint2::ZERO;

  return IPoint2((int)floorf(window->Pos.x), (int)floorf(window->Pos.y));
}

void DialogWindow::setWindowPosition(const IPoint2 &position, const Point2 &pivot)
{
  moveRequested = true;
  moveRequestPosition = Point2(position.x, position.y);
  moveRequestPivot = pivot;
}

IPoint2 DialogWindow::getWindowSize() const
{
  ImGuiWindow *window = ImGui::FindWindowByName(dialogCaption);
  if (!window)
    return IPoint2::ZERO;

  return IPoint2((int)floorf(window->SizeFull.x), (int)floorf(window->SizeFull.y));
}

void DialogWindow::setWindowSize(const IPoint2 &size)
{
  sizingRequested = true;
  sizingRequestSize = Point2(size.x, size.y);
}

void DialogWindow::centerWindow()
{
  moveRequested = true;
  moveRequestPosition = ImGui::GetMainViewport()->GetCenter();
  moveRequestPivot = Point2(0.5f, 0.5f);
}

void DialogWindow::centerWindowToMousePos()
{
  moveRequested = true;
  moveRequestPosition = ImGui::GetMousePos();
  moveRequestPivot = Point2(0.5f, 0.5f);
}

void DialogWindow::autoSize(bool auto_center)
{
  autoSizingRequestedForFrames = 2; // ImGui needs two frames to handle auto sizing.

  if (auto_center)
    centerWindow();
}

void DialogWindow::positionLeftToWindow(const char *window_name, bool use_same_height)
{
  ImGuiWindow *window = ImGui::FindWindowByName(window_name);
  if (!window)
    return;

  moveRequested = true;
  moveRequestPosition = window->Pos;
  moveRequestPivot = Point2(1.0f, 0.0f);

  if (use_same_height)
  {
    sizingRequested = true;
    sizingRequestSize = Point2(0.0f, window->Size.y);
  }
}

void DialogWindow::dockTo(unsigned dock_node_id)
{
  dockingRequested = true;
  dockingRequestNodeId = dock_node_id;
}

int DialogWindow::getScrollPos() const
{
  ImGuiWindow *window = ImGui::FindWindowByName(dialogCaption);
  return window ? (int)floorf(window->Scroll.y) : 0;
}

void DialogWindow::setScrollPos(int pos)
{
  if (pos >= 0)
  {
    scrollingRequestedPositionY = pos;

    // At the first frame the size of the dialog is not yet known, ImGui would throw away our requested scroll position.
    // So set it for two frames.
    scrollingRequestedForFrames = 2;
  }
}

void DialogWindow::clickDialogButton(int id) { buttonEventHandler->onClick(id, nullptr); }

SimpleString DialogWindow::getDialogButtonText(int id) const
{
  if (buttonsPanel)
    return buttonsPanel->getText(id);
  return SimpleString();
}

void DialogWindow::setDialogButtonText(int id, const char *text)
{
  if (buttonsPanel)
    buttonsPanel->setText(id, text);
}

bool DialogWindow::removeDialogButton(int id)
{
  if (buttonsPanel)
    return buttonsPanel->removeById(id);
  return false;
}

void DialogWindow::beforeUpdateImguiDialog(bool &use_auto_size_for_the_current_frame)
{
  const ImVec2 minSize(initialWidth, initialHeight);
  const ImVec2 maxSize(ImGui::GetIO().DisplaySize * 0.95f);
  ImGui::SetNextWindowSizeConstraints(minSize, ImVec2(max(minSize.x, maxSize.x), max(minSize.y, maxSize.y)));

  use_auto_size_for_the_current_frame = autoSizingRequestedForFrames > 0;
  if (autoSizingRequestedForFrames > 0)
    --autoSizingRequestedForFrames;

  if (moveRequested && autoSizingRequestedForFrames == 0)
  {
    moveRequested = false;
    ImGui::SetNextWindowPos(moveRequestPosition, ImGuiCond_Always, moveRequestPivot);
  }

  if (sizingRequested && autoSizingRequestedForFrames == 0)
  {
    sizingRequested = false;
    ImGui::SetNextWindowSize(sizingRequestSize);
  }

  if (dockingRequested)
  {
    dockingRequested = false;
    ImGui::SetNextWindowDockID(dockingRequestNodeId);
  }

  if (scrollingRequestedPositionY >= 0)
  {
    ImGui::SetNextWindowScroll(ImVec2(-1.0f, scrollingRequestedPositionY));

    --scrollingRequestedForFrames;
    if (scrollingRequestedForFrames <= 0)
      scrollingRequestedPositionY = -1;
  }
}

void DialogWindow::updateImguiDialog()
{
  // NOTE: ImGui porting: BeginChild did not fare well with auto sizing, so using manual bottom alignment for the buttons.
  // Good test dialogs: Viewport grid settings vs. Settings/Camera settings vs Settings/Project settings.

  // This needs to saved because otherwise ImGui::GetFrameHeightWithSpacing() could return an incorrect value when
  // queried from a child control. (For example ContainerPropertyControl::updateImgui changes the
  // vertical spacing.)
  const float separatorHeight = 2.0f;
  const float separatorHeightWithSpacing = separatorHeight + (ImGui::GetStyle().FramePadding.y * 2.0f);
  buttonPanelHeight = buttonsVisible ? (ImGui::GetFrameHeightWithSpacing() + separatorHeightWithSpacing) : 0.0f;

  if (propertiesPanel)
  {
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    const float regionAvailYStart = ImGui::GetContentRegionAvail().y;
    const float propertiesPanelHeight = max(regionAvailYStart - buttonPanelHeight - ImGui::GetStyle().ItemSpacing.y, 0.0f);

    // "c" stands for child. It could be anything.
    if (ImGui::BeginChild("c", ImVec2(0.0f, propertiesPanelHeight), ImGuiChildFlags_NavFlattened, windowFlags))
    {
      const ImGuiWindow *childWindow = ImGui::GetCurrentWindowRead();

      propertiesPanel->updateImgui();
      ImGui::EndChild();

      if (autoSizingRequestedForFrames > 0)
      {
        // The child window's content size is the size needed for child controls. Use a Dummy to increase the size of
        // dialog if required to that size.

        // Unfortunately ImGuiWindow::ContentSize will be only available at the next frame, so we have to calculate it.
        // See CalcWindowContentSize in imgui.cpp.
        const ImVec2 childContentSize(
          ceilf(max(childWindow->DC.CursorMaxPos.x, childWindow->DC.IdealMaxPos.x) - childWindow->DC.CursorStartPos.x),
          ceilf(max(childWindow->DC.CursorMaxPos.y, childWindow->DC.IdealMaxPos.y) - childWindow->DC.CursorStartPos.y));

        // For width use the total width because the Dummy's X position is at the left side of the dialog. For height
        // use the height difference because the Dummy's Y position is at the bottom of the dialog.
        const ImVec2 childSize = ImGui::GetItemRectSize();
        ImGui::Dummy(ImVec2(max(childContentSize.x, childSize.x), max(childContentSize.y - childSize.y, 0.0f)));
      }
    }
    else
    {
      ImGui::EndChild();
    }
  }

  if (buttonsVisible)
  {
    const float availableHeight = floorf(ImGui::GetContentRegionAvail().y - buttonPanelHeight);
    if (availableHeight > 0.0f)
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availableHeight);

    if (propertiesPanel)
    {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
      ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, separatorHeight);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
    }

    buttonsPanel->updateImgui();
  }

  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
  {
    if (preventNavigationWithTheTabKey)
      ImGui::SetKeyOwner(ImGuiKey_Tab, ImGui::GetCurrentWindowRead()->ID);

    // TODO: ImGui porting: allow closing dialogs with Enter when the focus is in an edit box. Also a single Escape press should be
    // enough to close them.
    if (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter))
    {
      clickDialogButton(DIALOG_ID_OK);
    }
    else if (ImGui::Shortcut(ImGuiKey_Escape))
    {
      // By default this will result in DIALOG_ID_CANCEL but closeReturn() can be overridden.
      clickDialogButton(DIALOG_ID_CLOSE);
    }
  }
}

} // namespace PropPanel