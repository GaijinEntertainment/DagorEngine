// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <quirrel/frp/dag_frp.h>


namespace darg
{

class GuiScene;

class JoystickAxisObservable : public sqfrp::BaseObservable
{
public:
  JoystickAxisObservable(GuiScene *gui_scene);

  virtual Sqrat::Object getValueForNotify() const override;
  virtual void fillInfo(Sqrat::Table &t) const override;
  virtual void fillInfo(String &t) const override;

  float getValue() { return value; }
  void update(float val);

public:
  int axis = -1;
  float value = 0;
  float lastNotifiedVal = 1e6f;
  float resolution = 0.01f;
  float deadzone = 0.1f;
};

} // namespace darg
