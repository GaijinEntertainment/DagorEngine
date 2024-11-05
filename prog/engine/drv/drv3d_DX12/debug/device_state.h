// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN
#include "device_state_pc.h"
#endif

#include "device_state_null.h"

namespace drv3d_dx12::debug
{
#if _TARGET_PC_WIN
#if COMMAND_BUFFER_DEBUG_INFO_DEFINED
using DeviceState = pc::DeviceState;
#else
using DeviceState = null::DeviceState;
#endif
#else
using DeviceState = null::DeviceState;
#endif
} // namespace drv3d_dx12::debug