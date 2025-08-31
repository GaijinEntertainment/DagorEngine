// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_imgui.h>
#include <propPanel/control/container.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

namespace PropPanel
{

class GroupPropertyControl : public ContainerPropertyControl
{
public:
  GroupPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h), controlCaption(caption)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL; }

  bool getBoolValue() const override { return minimized; }
  void setBoolValue(bool value) override { minimized = value; }
  void setCaptionValue(const char value[]) override { controlCaption = value; }
  void setEnabled(bool enabled) override { controlEnabled = enabled; }
  void setUserDataValue(const void *value) override { userData = const_cast<void *>(value); }
  void *getUserDataValue() const override { return userData; }

  int getCaptionValue(char *buffer, int buflen) const override
  {
    if ((controlCaption.size() + 1) >= buflen)
      return 0;

    memcpy(buffer, controlCaption.c_str(), controlCaption.size());
    buffer[controlCaption.size()] = 0;
    return controlCaption.size();
  }

  void clear() override
  {
    ContainerPropertyControl::clear();

    minimized = false;
  }

  // saving and loading state with datablock

  int saveState(DataBlock &datablk, bool by_name) override
  {
    String groupId;
    if (by_name)
      groupId.printf(64, "group_%s", getStringCaption().c_str());
    else
      groupId.printf(64, "group_%d", getID());

    DataBlock *_block = datablk.addNewBlock(groupId.c_str());
    _block->addBool("minimize", this->getBoolValue());

    for (PropertyControlBase *control : mControlArray)
      control->saveState(*_block, by_name);

    return 0;
  }

  int loadState(DataBlock &datablk, bool by_name) override
  {
    String groupId;
    if (by_name)
      groupId.printf(64, "group_%s", getStringCaption().c_str());
    else
      groupId.printf(64, "group_%d", getID());

    DataBlock *_block = datablk.getBlockByName(groupId.c_str());
    if (_block)
    {
      this->setBoolValue(_block->getBool("minimize", true));

      for (PropertyControlBase *control : mControlArray)
        control->loadState(*_block, by_name);
    }

    return 0;
  }

  void updateImgui() override
  {
    // NOTE: if you modify this then you might have to modify the code in ExtGroupPropertyControl too!

    // SetNextItemOpen does nothing if SkipItems is set (and CollapsingHeader also always returns false if SkipItems
    // is set), so to avoid unintended openness change we do nothing too if SkipItems is set.
    if (ImGui::GetCurrentWindow()->SkipItems)
      return;

    const bool wasMinimized = minimized;
    if (wasMinimized)
      ImGui::PushFont(imgui_get_bold_font());

    ImGui::SetNextItemOpen(!minimized);
    minimized = !ImGui::CollapsingHeader(controlCaption);
    setFocusToPreviousImGuiControlAndScrollToItsTopIfRequested();
    showJumpToGroupContextMenuOnRightClick();

    if (wasMinimized)
      ImGui::PopFont();

    if (!minimized)
    {
      const ImVec2 buttonTopLeft = ImGui::GetItemRectMin();
      const ImVec2 buttonBottomRight = ImGui::GetItemRectMax();

      addVerticalSpaceAfterControl();

      // The indentation set in the style is too big but I did not want to hardcode another value here.
      const float indent = floorf(ImGui::GetStyle().IndentSpacing / 2.0f);
      ImGui::Indent(indent);
      ContainerPropertyControl::updateImgui();
      ImGui::Unindent(indent);

      addVerticalSpaceAfterControl();

      const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      drawHalfFrame(buttonTopLeft.x, buttonBottomRight.y, buttonBottomRight.x, cursorPos.y);
      ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));
      ImGui::Dummy(ImVec2(0.0f, 0.0f)); // Prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
    }
  }

  // There is getCaption but that returns with a new SimpleString, and there is getCaptionValue that copies to a buffer,
  // so here is a third function for the ExtGroupPropertyControl to use.
  const String &getStringCaption() const { return controlCaption; }

  bool isMinimized() const { return minimized; }
  void setMinimized(bool in_minimized) { minimized = in_minimized; }

  // Draw the left bottom of the frame with an optionally rounded corner.
  static void drawHalfFrame(float line_left_x, float line_top_y, float line_right_x, float line_bottom_y)
  {
    const ImU32 lineColor = getOverriddenColorU32(ColorOverride::GROUP_BORDER);
    if ((lineColor & IM_COL32_A_MASK) == 0)
      return;

    const float rounding = ImGui::GetStyle().FrameRounding;
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->PathLineTo(ImVec2(line_left_x + 0.5f, line_top_y - rounding + 0.5f));
    drawList->PathArcToFast(ImVec2(line_left_x + rounding + 0.5f, line_bottom_y - rounding + 0.5f), rounding, 6, 3);
    drawList->PathLineTo(ImVec2(line_right_x + 0.5f, line_bottom_y + 0.5f));
    drawList->PathStroke(lineColor);
  }

protected:
  void setFocusToPreviousImGuiControlAndScrollToItsTopIfRequested()
  {
    if (focus_helper.getRequestedControlToFocus() != this)
      return;

    ImGui::SetFocusID(ImGui::GetItemID(), ImGui::GetCurrentWindow());
    ImGui::FocusWindow(ImGui::GetCurrentWindow());
    ImGui::SetScrollHereY(0.0f); // Scroll to the last item's top.

    focus_helper.clearRequest();
  }

  void showJumpToGroupContextMenuOnRightClick()
  {
    // Imitate Windows where the context menu comes up on mouse button release.
    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Right) || !ImGui::IsItemHovered())
      return;

    // Make right click reach PanelWindowPropertyControl to be able to show its context menu.
    ContainerPropertyControl *parent = getParent();
    if (parent)
    {
      // getRootParent stops if the event handlers differ, so we have to find the real root manually.
      while (ContainerPropertyControl *grandParent = parent->getParent())
        parent = grandParent;

      parent->onWcRightClick(nullptr);
    }
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  bool minimized = false;
  void *userData = nullptr;
};

} // namespace PropPanel