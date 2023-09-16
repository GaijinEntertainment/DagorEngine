#pragma once

#include <propPanel2/c_panel_control.h>

// This is a dummy control that allocates space for a real, external control in the panel.
class CPlaceholder : public PropertyControlBase
{
public:
  CPlaceholder(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h);

  virtual unsigned getTypeMaskForSet() const override { return 0; }
  virtual unsigned getTypeMaskForGet() const override { return 0; }
};
