// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_imgui.h>
#include <propPanel/control/container.h>
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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL; }

  virtual bool getBoolValue() const override { return minimized; }
  virtual void setBoolValue(bool value) override { minimized = value; }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }
  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }
  virtual void setUserDataValue(const void *value) override { userData = const_cast<void *>(value); }
  virtual void *getUserDataValue() const override { return userData; }

  virtual int getCaptionValue(char *buffer, int buflen) const override
  {
    if ((controlCaption.size() + 1) >= buflen)
      return 0;

    memcpy(buffer, controlCaption.c_str(), controlCaption.size());
    buffer[controlCaption.size()] = 0;
    return controlCaption.size();
  }

  virtual void clear() override
  {
    ContainerPropertyControl::clear();

    minimized = false;
  }

  // saving and loading state with datablock

  virtual int saveState(DataBlock &datablk) override
  {
    DataBlock *_block = datablk.addNewBlock(String(64, "group_%d", this->getID()).str());
    _block->addBool("minimize", this->getBoolValue());

    for (PropertyControlBase *control : mControlArray)
      control->saveState(*_block);

    return 0;
  }

  virtual int loadState(DataBlock &datablk) override
  {
    DataBlock *_block = datablk.getBlockByName(String(64, "group_%d", this->getID()).str());
    if (_block)
    {
      this->setBoolValue(_block->getBool("minimize", true));

      for (PropertyControlBase *control : mControlArray)
        control->loadState(*_block);
    }

    return 0;
  }

  virtual void updateImgui() override
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

      // Draw the half frame.
      const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      drawList->AddLine(ImVec2(buttonTopLeft.x, buttonBottomRight.y), ImVec2(buttonTopLeft.x, cursorPos.y),
        Constants::GROUP_BORDER_COLOR);
      drawList->AddLine(ImVec2(buttonTopLeft.x, cursorPos.y), ImVec2(buttonBottomRight.x, cursorPos.y), Constants::GROUP_BORDER_COLOR);

      ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));

      // This is here to prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
      ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
  }

  // There is getCaption but that returns with a new SimpleString, and there is getCaptionValue that copies to a buffer,
  // so here is a third function for the ExtGroupPropertyControl to use.
  const String &getStringCaption() const { return controlCaption; }

  bool isMinimized() const { return minimized; }
  void setMinimized(bool in_minimized) { minimized = in_minimized; }

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