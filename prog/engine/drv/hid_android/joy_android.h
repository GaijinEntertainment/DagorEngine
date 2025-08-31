// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>

namespace HumanInput
{
class IAndroidJoystick : public IGenJoystick
{
public:
  virtual ~IAndroidJoystick() {}
  virtual bool updateState() = 0;
};
} // namespace HumanInput