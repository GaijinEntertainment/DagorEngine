// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>

namespace PropPanel
{

class ColorChangeBuffer
{
public:
  static E3DCOLOR get() { return value; }
  static void set(E3DCOLOR new_value) { value = new_value; }

private:
  static E3DCOLOR value;
};

} // namespace PropPanel