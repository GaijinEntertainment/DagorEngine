// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiJoystick.h>

namespace HumanInput
{
class IAndroidJoystick : public IGenJoystick
{
public:
  virtual ~IAndroidJoystick(){};
  virtual bool updateState() = 0;
};
} // namespace HumanInput