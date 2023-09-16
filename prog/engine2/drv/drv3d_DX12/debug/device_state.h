#pragma once

#if _TARGET_PC_WIN
#include "device_state_pc.h"
#endif

#include "device_state_null.h"

namespace drv3d_dx12
{
namespace debug
{
#if _TARGET_PC_WIN
#if COMMAND_BUFFER_DEBUG_INFO_DEFINED
using DeviceState = ::drv3d_dx12::debug::pc::DeviceState;
#else
using DeviceState = ::drv3d_dx12::debug::null::DeviceState;
#endif
#else
using DeviceState = ::drv3d_dx12::debug::null::DeviceState;
#endif
} // namespace debug
} // namespace drv3d_dx12