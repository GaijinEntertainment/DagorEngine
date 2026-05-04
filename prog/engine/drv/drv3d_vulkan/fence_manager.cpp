// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fence_manager.h"
#include "backend.h"
#include "backend_interop.h"

using namespace drv3d_vulkan;

bool ThreadedFence::isInDeviceLost() { return Backend::interop.deviceLost.load(); }
