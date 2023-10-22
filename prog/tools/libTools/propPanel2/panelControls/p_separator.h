#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CSeparator : public PropertyControlBase
{
public:
  CSeparator(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return 0; }
  unsigned getTypeMaskForGet() const { return 0; }

  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

private:
  WSeparator mSeparator;
};
