#pragma once

#include "call_stack_null.h"
#include "call_stack_return_address.h"
#include "call_stack_full_stack.h"
#include "call_stack_selectable.h"

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
#if DX12_SELECTABLE_CALL_STACK_CAPTURE
using namespace selectable;
#elif DX12_FULL_CALL_STACK_CAPTURE
using namespace full_stack;
#elif COMMANDS_STORE_RETURN_ADDRESS
using namespace return_address;
#else
using namespace null;
#endif
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12