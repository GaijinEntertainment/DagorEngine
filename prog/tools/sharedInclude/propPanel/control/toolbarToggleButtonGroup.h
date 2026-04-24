//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/iconWithNameAndSize.h>

namespace PropPanel
{

class ToolbarToggleButtonGroupPropertyControl : public PropertyControlBase
{
public:
  ToolbarToggleButtonGroupPropertyControl(int id, ControlEventHandler *event_handler, PropPanel::ContainerPropertyControl *parent,
    int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]);

  int getImguiControlType() const override;

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }
  unsigned getWidth() const override;

  void setCaptionValue(const char value[]) override { controlTooltip = value; }
  void setBoolValue(bool value) override { controlValue = value; }
  bool getBoolValue() const override { return controlValue; }

  void reset() override;

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setButtonPictureValues(const char *fname) override;

  void updateImgui() override;

private:
  // Both the start_index and end_index are inclusive.
  void getGroupRange(PropPanel::ContainerPropertyControl &parent, int &start_index, int &end_index) const;

  void handleButtonClick();

  bool controlValue = false;
  bool controlEnabled = true;
  IconWithNameAndSize iconWithNameAndSize;
};

} // namespace PropPanel