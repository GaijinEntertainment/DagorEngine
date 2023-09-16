#pragma once

#include "device_context_state_null.h"
#if _TARGET_PC_WIN
#include "device_context_state_pc.h"
#else
#include "device_context_state_xbox.h"
#endif

namespace drv3d_dx12
{
namespace debug
{
#if _TARGET_PC_WIN
#if COMMAND_BUFFER_DEBUG_INFO_DEFINED
using DeviceContextState = pc::DeviceContextState;
#else
using DeviceContextState = null::DeviceContextState;
#endif
#else
using DeviceContextState = xbox::DeviceContextState;
#endif
} // namespace debug
} // namespace drv3d_dx12