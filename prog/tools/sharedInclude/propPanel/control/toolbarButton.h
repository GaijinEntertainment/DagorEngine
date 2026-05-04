//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/iconWithNameAndSize.h>

namespace PropPanel
{

class ToolbarButtonPropertyControl : public PropertyControlBase
{
public:
  ToolbarButtonPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]);

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }
  unsigned getWidth() const override;

  void setCaptionValue(const char value[]) override { controlTooltip = value; }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setButtonPictureValues(const char *fname) override;

  void updateImgui() override;

private:
  bool controlEnabled = true;
  IconWithNameAndSize iconWithNameAndSize;
};

} // namespace PropPanel