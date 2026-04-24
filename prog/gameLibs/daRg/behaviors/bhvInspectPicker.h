// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvInspectPicker : public darg::Behavior
{
public:
  BhvInspectPicker();

  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/,
    int /*button_id*/, Point2 pos, int /*accum_res*/) override;
};


extern BhvInspectPicker bhv_inspect_picker;


} // namespace darg
