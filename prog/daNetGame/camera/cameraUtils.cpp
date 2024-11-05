// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "camera/cameraUtils.h"
#include <math/dag_mathBase.h>

float cam::deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }
