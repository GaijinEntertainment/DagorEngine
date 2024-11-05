// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "controlType.h"
#include <propPanel/control/container.h>
#include <util/dag_string.h>

namespace PropPanel
{

class TabPagePropertyControl : public ContainerPropertyControl
{
public:
  TabPagePropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h), controlCaption(caption)
  {}

  virtual int getImguiControlType() const { return (int)ControlType::TabPage; }

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return 0; }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }
  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }
  virtual bool isRealContainer() override { return false; }

  // There is getCaption but that returns with a new SimpleString, and there is getCaptionValue that copies to a buffer,
  // so here is a third function for the TabPanelPropertyControl to use.
  const String &getStringCaption() const { return controlCaption; }

  bool getEnabled() const { return controlEnabled; }

private:
  String controlCaption;
  bool controlEnabled = true;
};

} // namespace PropPanel