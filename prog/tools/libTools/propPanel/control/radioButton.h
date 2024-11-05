// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class RadioButtonPropertyControl : public PropertyControlBase
{
public:
  RadioButtonPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int index) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), selectionIndex(index)
  {}

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }

  virtual int getIntValue() const override { return selectionIndex; }
  virtual bool getBoolValue() const override { return checked; }
  virtual void setBoolValue(bool value) override { checked = value; }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    const bool clicked = ImGui::RadioButton(controlCaption, checked);
    const bool doubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    setPreviousImguiControlTooltip();

    if (clicked)
      onWcChange(nullptr);
    else if (doubleClicked)
      onWcDoubleClick(nullptr);
  }

private:
  virtual void onWcChange(WindowBase *source) override
  {
    // NOTE: ImGui porting: this is implemented a bit differently than in the original propPanel2
    mParent->setIntValue(selectionIndex);

    mParent->onWcChange(source);
  }

  String controlCaption;
  int selectionIndex;
  bool controlEnabled = true;
  bool checked = false;
};

} // namespace PropPanel