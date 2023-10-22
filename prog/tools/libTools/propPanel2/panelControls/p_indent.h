#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CIndent : public PropertyControlBase
{
public:
  CIndent(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return 0; }
  unsigned getTypeMaskForGet() const { return 0; }
};
