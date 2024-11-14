// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack_null.h"
#include "call_stack_return_address.h"
#include "call_stack_full_stack.h"
#include "call_stack_selectable.h"
#include <driver.h>


namespace drv3d_dx12::debug::call_stack
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
} // namespace drv3d_dx12::debug::call_stack