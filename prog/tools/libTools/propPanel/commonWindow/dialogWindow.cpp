// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <propPanel/commonWindow/dialogWindow.h>
#include "dialogManagerInternal.h"
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/control/container.h>
#include <propPanel/constants.h>
#include <propPanel/focusHelper.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <workCycle/dag_workCycle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

static constexpr const float SEPARATOR_HEIGHT = 2.0f;

static hdpi::Px initialExtWidth = hdpi::Px::ZERO;
static hdpi::Px initialExtHeight = hdpi::Px::ZERO;

namespace PropPanel
{

class DialogButtonsHandler : public PropPanel::ControlEventHandler
{
public:
  explicit DialogButtonsHandler(DialogWindow &in_dialog) : dialog(in_dialog) {}

  void onClick(int pcb_id, ContainerPropertyControl *panel) override { dialog.onButtonPanelClick(pcb_id); }

private:
  DialogWindow &dialog;
};

/*static*/
void DialogWindow::setInitialSizeExtension(hdpi::Px w, hdpi::Px h)
{
  initialExtWidth = w;
  initialExtHeight = h;
}

/*static*/
IPoint2 DialogWindow::getInitialSizeExtension() { return IPoint2(hdpi::_px(initialExtWidth), hdpi::_px(initialExtHeight)); }

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

  if (initialWidth != 0)
    initialWidth += hdpi::_px(initialExtWidth);
  if (initialHeight != 0)
    initialHeight += hdpi::_px(initialExtHeight);

  G_ASSERT(!propertiesPanel);
  if (!hide_panel)
    propertiesPanel = new ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO);

  buttonEventHandler = new DialogButtonsHandler(*this);
  buttonsPanel = new ContainerPropertyControl(0, buttonEventHandler, nullptr, 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO);
  buttonsPanel->createButton(DIALOG_ID_OK, "Ok");
  buttonsPanel->createButton(DIALOG_ID_CANCEL, "Cancel", true, false);
}

void DialogWindow::applyInitialFocus()
{
  if (initialFocusId == DIALOG_ID_NONE)
    return;

  PropertyControlBase *control = buttonsPanel->getById(initialFocusId);
  if (!control && propertiesPanel)
    control = propertiesPanel->getById(initialFocusId);

  if (!control)
    return;

  if (modal)
  {
    const void *oldControlToFocus = focus_helper.getRequestedControlToFocus();
    control->setFocus();

    // Re-request the focus but with the focus rectangle displayed. This allows pressing the button with the Space key.
    const void *newControlToFocus = focus_helper.getRequestedControlToFocus();
    if (newControlToFocus != oldControlToFocus)
      focus_helper.requestFocus(newControlToFocus, /*show_focus_rectangle = */ true);
  }
  else
  {
    control->setFocus();
  }
}

void DialogWindow::show()
{
  modal = false;

  applyInitialFocus();

  // Modals (BeginPopupModal) are centered by ImGui, so this is only needed here.
  if (!moveRequested && (!dockingRequested || dockingRequestNodeId == 0) && !hasEverBeenShown())
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

  if (modal_dialog_event_handler)
    modal_dialog_event_handler->beforeModalDialogShown();

  dialogResult = DIALOG_ID_NONE;
  modal = true;
  visible = true;

  applyInitialFocus();
  dialog_manager.showDialog(*this);

  while (visible)
    dagor_work_cycle();

  if (modal_dialog_event_handler)
    modal_dialog_event_handler->afterModalDialogShown();

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

void DialogWindow::positionBesideWindow(const char *window_name, bool prefer_left_side, bool use_same_height)
{
  ImGuiWindow *window = ImGui::FindWindowByName(window_name);
  if (!window || !window->Viewport)
    return;

  float requiredWidth = sizingRequested ? sizingRequestSize.x : 0.0f;
  if (requiredWidth <= 0.0f)
  {
    requiredWidth = getWindowSize().x;
    if (requiredWidth <= 0.0f)
      requiredWidth = initialWidth;
  }

  const bool fitsToLeft = (window->Pos.x - requiredWidth) >= window->Viewport->WorkPos.x;
  const bool fitsToRight =
    (window->Pos.x + window->Size.x + requiredWidth) <= (window->Viewport->WorkPos.x + window->Viewport->WorkSize.x);
  if ((prefer_left_side && fitsToLeft) || (fitsToLeft && !fitsToRight))
  {
    moveRequestPosition = window->Pos;
    moveRequestPivot = Point2(1.0f, 0.0f);
  }
  else if (fitsToRight)
  {
    moveRequestPosition = Point2(window->Pos.x + window->Size.x, window->Pos.y);
    moveRequestPivot = Point2(0.0f, 0.0f);
  }
  else
  {
    moveRequestPosition = window->Viewport->WorkPos + (window->Viewport->WorkSize / 2.0f);
    moveRequestPivot = Point2(0.5f, 0.5f);
  }

  moveRequested = true;

  if (use_same_height)
  {
    sizingRequestSize = Point2(sizingRequested ? sizingRequestSize.x : 0.0f, window->Size.y);
    sizingRequested = true;
  }
}

void DialogWindow::dockTo(unsigned dock_node_id)
{
  dockingRequested = true;
  dockingRequestNodeId = dock_node_id;
}

bool DialogWindow::hasEverBeenShown() const
{
  const ImGuiID windowId = ImHashStr(dialogCaption.c_str());
  const ImGuiWindow *window = ImGui::FindWindowByID(windowId);
  if (window && ((window->SetWindowPosAllowFlags & ImGuiCond_FirstUseEver) == 0 ||
                  (window->SetWindowSizeAllowFlags & ImGuiCond_FirstUseEver) == 0))
    return true;

  return ImGui::FindWindowSettingsByID(windowId);
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

void DialogWindow::createDialogButton(int id, const char *text)
{
  if (buttonsPanel)
    buttonsPanel->createButton(id, text, true, false);
}

bool DialogWindow::removeDialogButton(int id)
{
  if (buttonsPanel)
    return buttonsPanel->removeById(id);
  return false;
}

void DialogWindow::setDialogButtonEnabled(int id, bool enabled)
{
  if (buttonsPanel)
  {
    buttonsPanel->setEnabledById(id, enabled);
  }
}

void DialogWindow::setDialogButtonTooltip(int id, const char *text)
{
  if (buttonsPanel)
  {
    buttonsPanel->setTooltipId(id, text);
  }
}

void DialogWindow::onButtonPanelClick(int id)
{
  if (id == DIALOG_ID_OK && onOk())
    hide(id);
  else if (id == DIALOG_ID_CANCEL && onCancel())
    hide(id);
  else if (id == DIALOG_ID_CLOSE && onClose())
    hide(closeReturn());
}

void DialogWindow::beforeUpdateImguiDialog(bool &use_auto_size_for_the_current_frame)
{
  const ImVec2 minSize(initialWidth, initialHeight);
  const ImVec2 maxSize(FLT_MAX, FLT_MAX);
  ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

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
  ImGuiStyle &style = ImGui::GetStyle();
  const int modalSpacing = hdpi::_pxS(PropPanel::Constants::MODAL_WINDOW_ITEM_SPACING);
  const ImVec2 buttonItemSpacing = isModal() ? ImVec2(modalSpacing, modalSpacing) : style.ItemSpacing;
  const ImVec2 buttonFramePadding = isModal() ? ImVec2(hdpi::_pxS(6), hdpi::_pxS(6)) : style.FramePadding;

  // This needs to saved because otherwise ImGui::GetFrameHeightWithSpacing() could return an incorrect value when
  // queried from a child control. (For example ContainerPropertyControl::updateImgui changes the
  // vertical spacing.)
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, buttonItemSpacing);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, buttonFramePadding);
  const float separatorHeightWithSpacing = SEPARATOR_HEIGHT + style.FramePadding.y;
  buttonPanelHeight = buttonsVisible ? (ImGui::GetFrameHeightWithSpacing() + separatorHeightWithSpacing) : 0.0f;
  ImGui::PopStyleVar(2);

  if (propertiesPanel)
  {
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    const float regionAvailYStart = ImGui::GetContentRegionAvail().y;
    const float propertiesPanelHeight = max(regionAvailYStart - buttonPanelHeight - style.ItemSpacing.y, 0.0f);

    // "c" stands for child. It could be anything.
    const ImVec2 childWindowSize(0.0f, autoSizingRequestedForFrames > 0 ? 0.0f : propertiesPanelHeight);
    if (ImGui::BeginChild("c", childWindowSize, ImGuiChildFlags_NavFlattened, windowFlags))
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
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, buttonItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, buttonFramePadding);

    const float availableHeight = floorf(ImGui::GetContentRegionAvail().y - buttonPanelHeight);
    if (availableHeight > 0.0f)
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availableHeight);

    if (propertiesPanel)
    {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
      ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, SEPARATOR_HEIGHT);
    }

    buttonsPanel->updateImgui();

    ImGui::PopStyleVar(2);
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