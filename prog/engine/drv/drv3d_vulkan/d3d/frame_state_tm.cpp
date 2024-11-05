// Copyright (C) Gaijin Games KFT.  All rights reserved.

// driver shared frame state global matrices implementation
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_decl.h>
#include "frameStateTM.inc.h"
#include "globals.h"
#include "global_lock.h"

using namespace drv3d_vulkan;
namespace drv3d_vulkan
{
FrameStateTM g_frameState;
}

#define CHECK_MAIN_THREAD() VERIFY_GLOBAL_LOCK_ACQUIRED()
#include "frameStateTM.inc.cpp"
#undef CHECK_MAIN_THREAD
