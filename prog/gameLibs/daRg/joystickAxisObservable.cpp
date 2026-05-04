// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joystickAxisObservable.h"
#include "guiScene.h"

#include <math/dag_mathUtils.h>


namespace darg
{

JoystickAxisObservable::JoystickAxisObservable(GuiScene *gui_scene)
{
  graph = gui_scene->frpGraph.get();
  HSQOBJECT initial;
  sq_resetobject(&initial);
  initial._type = OT_FLOAT;
  initial._unVal.fFloat = 0.0f;
  id = graph->createWatched(initial);
}


void JoystickAxisObservable::update(float val)
{
  float scaledRes = floorf(cvt(fabs(val), deadzone, 1.0f, 0.0f, 1.0f) / resolution + 0.5f) * resolution;

  value = val >= 0 ? scaledRes : -scaledRes;

  if (fabsf(value - lastNotifiedVal) > resolution * 0.5f)
  {
    lastNotifiedVal = value;
    graph->setValue(id, Sqrat::Object(value, graph->vm));
  }
}

} // namespace darg
