// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <propPanel/commonWindow/dialogWindow.h>
#include "dialogManagerInternal.h"
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/control/container.h>
#include <propPanel/constants.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <gui/dag_imgui.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <workCycle/dag_workCycle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// autoSize() never lets the dialog grow past this fraction of the display size.
static constexpr const float AUTO_SIZE_MAX_DISPLAY_FRACTION = 0.8f;

static constexpr const float SEPARATOR_HEIGHT = 2.0f;

static hdpi::Px initialExtWidth = hdpi::Px::ZERO;
static hdpi::Px initialExtHeight = hdpi::Px::ZERO;

// The ItemSpacing/FramePadding used for the button panel.
static void get_button_panel_style_vars(bool is_modal, ImVec2 &item_spacing, ImVec2 &frame_padding)
{
  const int modalSpacing = hdpi::_pxS(PropPanel::Constants::MODAL_WINDOW_ITEM_SPACING);
  const ImGuiStyle &style = ImGui::GetStyle();
  item_spacing = is_modal ? ImVec2(modalSpacing, modalSpacing) : style.ItemSpacing;
  frame_padding = is_modal ? ImVec2(hdpi::_pxS(6), hdpi::_pxS(6)) : style.FramePadding;
}

// Same layout as the title bar in ImGui::RenderWindowTitleBarContents(). Use it with the title bar's frame padding style set.
static float calculate_title_bar_width(const char *caption, bool is_modal, bool has_close_button)
{
  const ImGuiStyle &style = ImGui::GetStyle();

  // DialogManager::renderModalDialog() draws the title of a modal dialog with the bold font.
  if (is_modal)
    ImGui::PushFont(imgui_get_bold_font(), 0.0f);
  const float buttonSize = ImGui::GetFontSize();
  const float textWidth = ImGui::CalcTextSize(caption, nullptr, true).x;
  if (is_modal)
    ImGui::PopFont();

  float padLeft = style.FramePadding.x;
  float padRight = style.FramePadding.x;
  if (has_close_button)
    padRight += buttonSize + style.ItemInnerSpacing.x;

  // BeginPopupModal() sets ImGuiWindowFlags_NoCollapse, so only modeless dialogs have a collapse button.
  if (!is_modal && style.WindowMenuButtonPosition == ImGuiDir_Right)
    padRight += buttonSize + style.ItemInnerSpacing.x;
  else if (!is_modal && style.WindowMenuButtonPosition == ImGuiDir_Left)
    padLeft += buttonSize + style.ItemInnerSpacing.x;

  if (padLeft > style.FramePadding.x)
    padLeft += style.ItemInnerSpacing.x;
  if (padRight > style.FramePadding.x)
    padRight += style.ItemInnerSpacing.x;

  // Centered text keeps the same distance from both sides.
  if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
    padLeft = padRight = max(padLeft, padRight);

  return padLeft + textWidth + padRight;
}

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
  // In the first frame the viewport is 0x0-sized. Let ImGui position the window.
  // This could happen in a very early wingw::message_box() call.
  if (ImGui::GetFrameCount() == 0)
    return;

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

void DialogWindow::autoSize(bool auto_center, bool use_preferred_size)
{
  autoSizeUsePreferredSize = use_preferred_size;
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

DialogWindow::DialogFrameSizing DialogWindow::beforeUpdateImguiDialog(const Point2 &content_frame_padding)
{
  const ImGuiStyle &style = ImGui::GetStyle();

  DialogFrameSizing sizing;
  sizing.autoSize = autoSizingRequestedForFrames > 0;

  // Measured out here because ImGui::GetFrameHeightWithSpacing() could return an incorrect value when queried from a
  // child control. (For example ContainerPropertyControl::updateImgui changes the vertical spacing.)
  ImVec2 buttonItemSpacing, buttonFramePadding;
  get_button_panel_style_vars(isModal(), buttonItemSpacing, buttonFramePadding);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, buttonItemSpacing);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, buttonFramePadding);
  buttonPanelHeight = calculateButtonPanelHeight();
  ImGui::PopStyleVar(2);

  ImVec2 minSize(initialWidth, initialHeight);
  ImVec2 maxSize(FLT_MAX, FLT_MAX);

  if (sizing.autoSize)
  {
    // The window does not exist yet this frame, so look up its state from last frame to find which viewport it is on.
    const ImGuiWindow *existingWindow = ImGui::FindWindowByName(dialogCaption);
    const ImGuiViewport *viewport = (existingWindow && existingWindow->Viewport) ? existingWindow->Viewport : ImGui::GetMainViewport();

    // Limit the window here, not just its content. A dialog already taller than this from a previous show()
    // would otherwise keep that size, since nothing can shrink a window that Begin() itself never limited.
    maxSize.x = viewport->WorkSize.x * AUTO_SIZE_MAX_DISPLAY_FRACTION;
    maxSize.y = viewport->WorkSize.y * AUTO_SIZE_MAX_DISPLAY_FRACTION;

    // The ItemSpacing.y gaps are between EndChild() and Dummy(), and between Dummy() and the button panel if it is shown.
    const float titleBarHeight = existingWindow ? existingWindow->TitleBarHeight : ImGui::GetFrameHeight();
    const float chromeWidth = style.WindowPadding.x * 2.0f;
    const float gapsHeight = style.ItemSpacing.y * (buttonsVisible ? 2.0f : 1.0f);
    const float chromeHeight = buttonPanelHeight + gapsHeight + (style.WindowPadding.y * 2.0f) + titleBarHeight;

    sizing.maxContentWidth = max(maxSize.x - chromeWidth, 0.0f);
    sizing.maxContentHeight = max(maxSize.y - chromeHeight, 0.0f);

    if (autoSizeUsePreferredSize && propertiesPanel)
    {
      // Predict the panel's size so the dialog starts at its final size, instead of visibly growing into it over the
      // auto-sizing frames. getPreferredSize() takes a content-only budget, so add the chrome back to turn its result
      // into an outer-window minSize.
      // Use the correct frame padding, a modal dialog's larger frame padding is still in effect here.
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(content_frame_padding.x, content_frame_padding.y));
      const Point2 preferredPanelSize = propertiesPanel->getPreferredSize(sizing.maxContentWidth);
      ImGui::PopStyleVar();

      if (preferredPanelSize.x > 0.0f)
      {
        // The extra pixel is needed because ImGui rounds a window's measured ContentSize up, and the panel's items
        // can start at a fractional X.
        // A control's preferred size is a best-effort estimate and may exceed maxContentWidth.
        // A panel taller than its limit gets a vertical scrollbar, which takes from its width.
        const float scrollbarWidth = preferredPanelSize.y > sizing.maxContentHeight ? style.ScrollbarSize : 0.0f;
        minSize.x = clamp(preferredPanelSize.x + 1.0f + chromeWidth + scrollbarWidth, minSize.x, maxSize.x);
      }

      if (preferredPanelSize.y > 0.0f)
        minSize.y = clamp(preferredPanelSize.y + chromeHeight, minSize.y, maxSize.y);

      // ImGui's auto sizing does not include the title bar, so a long caption would be clipped.
      const float titleBarWidth = calculate_title_bar_width(dialogCaption, isModal(), !isModal() || closeButtonVisible);
      minSize.x = clamp(titleBarWidth, minSize.x, maxSize.x);
    }
  }

  ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

  if (sizing.autoSize)
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

  return sizing;
}

float DialogWindow::calculateButtonPanelHeight() const
{
  const float separatorHeightWithSpacing = SEPARATOR_HEIGHT + ImGui::GetStyle().FramePadding.y;
  return buttonsVisible ? (ImGui::GetFrameHeightWithSpacing() + separatorHeightWithSpacing) : 0.0f;
}

void DialogWindow::updateImguiDialog(const DialogFrameSizing &sizing)
{
  // NOTE: ImGui porting: BeginChild did not fare well with auto sizing, so using manual bottom alignment for the buttons.
  // Good test dialogs: Viewport grid settings vs. Settings/Camera settings vs Settings/Project settings.
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec2 buttonItemSpacing, buttonFramePadding;
  get_button_panel_style_vars(isModal(), buttonItemSpacing, buttonFramePadding);

  if (propertiesPanel)
  {
    const ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_HorizontalScrollbar;
    const float regionAvailYStart = ImGui::GetContentRegionAvail().y;
    // Without the button panel nothing fills the gap below the panel, and an auto resized dialog would shrink by it in every frame.
    const float spaceBelowPanel = buttonsVisible ? (buttonPanelHeight + style.ItemSpacing.y) : 0.0f;
    const float propertiesPanelHeight = max(regionAvailYStart - spaceBelowPanel, 0.0f);
    ImVec2 childWindowSize(0.0f, propertiesPanelHeight);

    if (sizing.autoSize)
    {
      // Clamp here, before the child opens. The size only grows within a frame, so Dummy() below cannot shrink it back.
      // The "- style.ItemSpacing.y" is because of the Dummy().
      childWindowSize.x = min(ImGui::GetContentRegionAvail().x, sizing.maxContentWidth);
      childWindowSize.y = min(max(propertiesPanelHeight - style.ItemSpacing.y, 0.0f), sizing.maxContentHeight);
    }

    // "c" stands for child. It could be anything.
    if (ImGui::BeginChild("c", childWindowSize, ImGuiChildFlags_NavFlattened, windowFlags))
    {
      const ImGuiWindow *childWindow = ImGui::GetCurrentWindowRead();

      propertiesPanel->updateImgui();
      ImguiHelper::hookWindowScrollbarsForTestRuntime();
      ImGui::EndChild();

      if (sizing.autoSize)
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

        const float targetWidth = min(childContentSize.x, sizing.maxContentWidth);
        const float targetHeight = min(childContentSize.y, sizing.maxContentHeight);

        ImGui::Dummy(ImVec2(max(targetWidth, childSize.x), max(targetHeight - childSize.y, 0.0f)));
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

    // If the navigation cursor is visible then pressing Enter is the same as pressing Space, ImGui handles it as a button click. So we
    // only handle Enter when navigation cursor is not visible to provide a Windows-like behavior: closing the dialog with Enter.
    // TODO: ImGui porting: allow closing dialogs with Enter when the focus is in an edit box. Also a single Escape press should be
    // enough to close them.
    if (!ImGui::GetCurrentContext()->NavCursorVisible && (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter)))
    {
      if (buttonsPanel->getChildCount() > 0)
        clickDialogButton(buttonsPanel->getByIndex(0)->getID());
    }
    else if (ImGui::Shortcut(ImGuiKey_Escape))
    {
      // By default this will result in DIALOG_ID_CANCEL but closeReturn() can be overridden.
      clickDialogButton(DIALOG_ID_CLOSE);
    }
  }
}

} // namespace PropPanel