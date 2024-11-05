// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_consts_base.h>


#include "frameStateTM.inc.h"

namespace drv3d_metal
{
    FrameStateTM g_frameState;
}

using namespace drv3d_metal;

#define CHECK_MAIN_THREAD()

#include "frameStateTM.inc.cpp"
