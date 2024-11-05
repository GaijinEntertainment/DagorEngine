// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiDecl.h>

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