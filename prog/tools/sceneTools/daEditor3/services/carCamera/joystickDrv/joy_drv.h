#pragma once

#include <humanInput/dag_hiDecl.h>

namespace HumanInput
{
class IGenJoystick;
}

class IJoyCallback
{
public:
  virtual void updateJoyState() = 0;
};


HumanInput::IGenJoystick *get_joystick();
void startup_joystick(IJoyCallback *);
void update_joysticks();
void shutdown_joystick();