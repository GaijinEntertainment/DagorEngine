// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include "input/inputControls.h"

MAKE_TYPE_FACTORY(SensScale, controls::SensScale);

namespace bind_dascript
{
inline const controls::SensScale &get_sens_scale() { return controls::sens_scale; }
} // namespace bind_dascript
