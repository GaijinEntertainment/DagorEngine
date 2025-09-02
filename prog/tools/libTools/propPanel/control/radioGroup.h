// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include <propPanel/imguiHelper.h>

namespace PropPanel
{

class RadioGroupPropertyControl : public ContainerPropertyControl
{
public:
  RadioGroupPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h), controlCaption(caption)
  {
    // Use the default item spacing between radio buttons.
    const int itemSpacingY = (int)floorf(ImGui::GetStyle().ItemSpacing.y);
    setVerticalSpaceBetweenControls(hdpi::_pxActual(itemSpacingY));
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setIntValue(int value) override
  {
    if (value == selectedValue)
      return;

    selectedValue = RADIO_SELECT_NONE;

    for (PropertyControlBase *control : mControlArray)
    {
      if (control->getID() != getID())
        continue;

      if (control->getIntValue() == value)
      {
        control->setBoolValue(true);
        selectedValue = value;
      }
      else
      {
        control->setBoolValue(false);
      }
    }
  }

  int getIntValue() const override { return selectedValue; }

  void setBoolValue(bool value) override { G_ASSERT(false && "Old radio button use, contact developers"); }

  bool getBoolValue() const override
  {
    G_ASSERT(false && "Old radio button use, contact developers");
    return false;
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void reset() override
  {
    setIntValue(RADIO_SELECT_NONE);

    ContainerPropertyControl::reset();
  }

  void clear() override
  {
    selectedValue = RADIO_SELECT_NONE;

    ContainerPropertyControl::clear();
  }

  bool isRealContainer() override { return false; }

  void updateImgui() override
  {
    if (!controlCaption.empty())
      ImguiHelper::labelOnly(controlCaption);

    ContainerPropertyControl::updateImgui();
  }

private:
  void onWcChange(WindowBase *source) override
  {
    for (PropertyControlBase *control : mControlArray)
    {
      if (control->getID() != getID())
        continue;

      if (control->getBoolValue())
      {
        selectedValue = control->getIntValue();
        break;
      }
    }

    ContainerPropertyControl::onWcChange(source);
  }

  String controlCaption;
  bool controlEnabled = true;
  int selectedValue = RADIO_SELECT_NONE;
};

} // namespace PropPanel