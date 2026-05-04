// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "controlType.h"
#include "spinEditFloat.h"
#include "spinEditInt.h"
#include "toolbarSeparator.h"
#include <propPanel/control/container.h>
#include <propPanel/control/toolbarButton.h>
#include <propPanel/control/toolbarToggleButton.h>
#include <propPanel/control/toolbarToggleButtonGroup.h>
#include <propPanel/constants.h>
#include <util/dag_string.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class ToolbarPropertyControl : public ContainerPropertyControl
{
public:
  ToolbarPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, const char *caption) :
    ContainerPropertyControl(id, event_handler, parent), controlCaption(caption)
  {}

  int getImguiControlType() const override { return (int)ControlType::Toolbar; }

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void createEditInt(int id, const char caption[], int value, bool enabled, bool new_line) override
  {
    SpinEditIntPropertyControl *newControl = new SpinEditIntPropertyControl(mEventHandler, this, id, caption,
      /*width_includes_label = */ false);

    newControl->setIntValue(value);
    newControl->setEnabled(enabled);
    addControl(newControl, new_line);
  }

  void createEditFloat(int id, const char caption[], float value, int prec, bool enabled, bool new_line) override
  {
    SpinEditFloatPropertyControl *newControl = new SpinEditFloatPropertyControl(mEventHandler, this, id, caption, prec,
      /*width_includes_label = */ false);

    newControl->setFloatValue(value);
    newControl->setEnabled(enabled);
    addControl(newControl, new_line);
  }

  void createCheckBox(int id, const char caption[], bool value, bool enabled = true, bool new_line = true) override
  {
    G_UNUSED(new_line);

    ToolbarToggleButtonPropertyControl *newControl =
      new ToolbarToggleButtonPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
    newControl->setEnabled(enabled);
    addControl(newControl, new_line);
  }

  void createButton(int id, const char caption[], bool enabled = true, bool new_line = true) override
  {
    G_UNUSED(new_line);

    ToolbarButtonPropertyControl *newControl =
      new ToolbarButtonPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
    newControl->setEnabled(enabled);
    addControl(newControl, new_line);
  }

  void createSeparator(int id, bool new_line = true) override
  {
    G_UNUSED(new_line);

    ToolbarSeparatorPropertyControl *newControl =
      new ToolbarSeparatorPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0));
    addControl(newControl, new_line);
  }

  void createRadio(int id, const char caption[], bool enabled = true, bool new_line = true) override
  {
    G_UNUSED(new_line);

    ToolbarToggleButtonGroupPropertyControl *newControl =
      new ToolbarToggleButtonGroupPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
    addControl(newControl, new_line);
  }

  void clear() override { ContainerPropertyControl::clear(); }

  void setToolbarControlWidth(int width)
  {
    toolbarControlWidth = width;

    for (PropertyControlBase *control : mControlArray)
      setControlWidth(*control);
  }

  void updateImgui() override
  {
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const int alignRightFromChild = getAlignRightFromChild();

    for (int i = 0; i < mControlArray.size(); ++i)
    {
      PropertyControlBase *control = mControlArray[i];

      ImGui::SameLine();

      // Force starting a new row if we are after a separator control, and there is not enough room left in this row
      // till the next separator. So try to keep controls belonging together in the same row.
      bool forceNewRow = false;
      if (i != 0 && mControlArray[i - 1]->getImguiControlType() == (int)ControlType::ToolbarSeparator)
      {
        const int nextIndex = getNextSeparatorIndex(i);
        const float widthTillNextSeparator = getTotalWidth(i, nextIndex >= 0 ? nextIndex : mControlArray.size());
        forceNewRow = widthTillNextSeparator > ImGui::GetContentRegionAvail().x;
      }

      if (forceNewRow || (control->getWidth() + spacing) > ImGui::GetContentRegionAvail().x)
      {
        ImGui::NewLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing); // Add some spacing before the control in the new line.
      }

      if (alignRightFromChild == i)
      {
        const float widthOfRightAlignedControls = getTotalWidth(i, mControlArray.size());
        const float spaceBeforeRightAlignedControls = ImGui::GetContentRegionAvail().x - widthOfRightAlignedControls;
        if (spaceBeforeRightAlignedControls > 0.0f)
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBeforeRightAlignedControls);
      }

      ImGui::PushID(control);
      control->updateImgui();
      ImGui::PopID();

      mControlsLastRectSize[i] = ImGui::GetItemRectSize();
    }
  }

protected:
  float getTotalWidth(int start_index, int end_index) const
  {
    G_ASSERT(start_index >= 0);
    G_ASSERT(end_index <= mControlArray.size());

    float width = 0.0f;
    for (int i = start_index; i < end_index; ++i)
      width += mControlArray[i]->getWidth() + ImGui::GetStyle().ItemSpacing.x;
    return width;
  }

  int getNextSeparatorIndex(int start_index) const
  {
    G_ASSERT(start_index >= 0);

    for (int i = start_index; i < mControlArray.size(); ++i)
      if (mControlArray[i]->getImguiControlType() == (int)ControlType::ToolbarSeparator)
        return i;
    return -1;
  }

  void setControlWidth(PropertyControlBase &control) { control.setWidth(hdpi::_pxScaled(toolbarControlWidth)); }

  void addControl(PropertyControlBase *pcontrol, bool new_line) override
  {
    const int oldCount = mControlArray.size();
    ContainerPropertyControl::addControl(pcontrol, new_line);
    G_ASSERT(mControlArray.size() == (oldCount + 1));

    setControlWidth(*mControlArray.back());
  }

  String controlCaption;
  int toolbarControlWidth = Constants::TOOLBAR_DEFAULT_CONTROL_WIDTH;
};

} // namespace PropPanel