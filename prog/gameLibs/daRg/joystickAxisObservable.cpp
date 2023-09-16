#include "joystickAxisObservable.h"
#include "guiScene.h"

#include <math/dag_mathUtils.h>


namespace darg
{

JoystickAxisObservable::JoystickAxisObservable(GuiScene *gui_scene) : BaseObservable(gui_scene->frpGraph.get()) { generation = -1; }


void JoystickAxisObservable::update(float val)
{
  float scaledRes = floorf(cvt(fabs(val), deadzone, 1.0f, 0.0f, 1.0f) / resolution + 0.5f) * resolution;

  value = val >= 0 ? scaledRes : -scaledRes;

  if (fabsf(value - lastNotifiedVal) > resolution * 0.5f)
  {
    lastNotifiedVal = value;
    String errMsg;
    triggerRoot(errMsg);
  }
}


Sqrat::Object JoystickAxisObservable::getValueForNotify() const { return Sqrat::Object(value, graph->vm); }

void JoystickAxisObservable::fillInfo(Sqrat::Table &t) const { t.SetValue("value", value); }

void JoystickAxisObservable::fillInfo(String &s) const { s.printf(0, "value = %.3f", value); }

} // namespace darg
