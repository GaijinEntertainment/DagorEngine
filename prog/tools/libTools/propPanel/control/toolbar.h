// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "toolbarButton.h"
#include "toolbarSeparator.h"
#include "toolbarToggleButton.h"
#include "toolbarToggleButtonGroup.h"
#include <propPanel/control/container.h>
#include "../c_constants.h"
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

  void updateImgui() override
  {
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    float availableSpace;
    float leftMaxX;

    int alignRightFromStart = mControlArray.size();
    const int alignRightFromChild = getAlignRightFromChild();
    for (int i = 0; i < mControlArray.size(); ++i)
    {
      if (alignRightFromChild == i)
      {
        alignRightFromStart = i;
        availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
        availableSpace -= spacing;
        leftMaxX = max(ImGui::GetItemRectMax().x, ImGui::GetCursorPosX());
        leftMaxX += spacing;
        break;
      }
      updateControl(i, 0.0);
    }

    for (int i = alignRightFromStart; i < mControlArray.size(); ++i)
    {
      availableSpace -= mControlsLastRectSize[i].x;
      if (availableSpace < leftMaxX)
        break;
      updateControl(i, availableSpace);
      availableSpace -= spacing;
    }
  }

protected:
  void addControl(PropertyControlBase *pcontrol, bool new_line) override
  {
    const int oldCount = mControlArray.size();
    ContainerPropertyControl::addControl(pcontrol, new_line);
    G_ASSERT(mControlArray.size() == (oldCount + 1));

    PropertyControlBase *control = mControlArray.back();
    const hdpi::Px width = hdpi::_pxScaled(DEFAULT_TOOLBAR_CONTROL_WIDTH);
    control->setWidth(width);
  }

  void updateControl(int index, float offset)
  {
    PropertyControlBase *control = mControlArray[index];

    ImGui::SameLine(offset);

    ImGui::PushID(control);
    control->updateImgui();
    ImGui::PopID();

    mControlsLastRectSize[index] = ImGui::GetItemRectSize();
  };

  String controlCaption;
};

} // namespace PropPanel