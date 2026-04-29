// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace drv3d_vulkan
{

using UnsafeFunc = void (*)(void *user_data);

// Executes func(user_data) handling hw exception if possible and returning false if such exception happened
//
bool execute_unsafe(UnsafeFunc func, void *user_data) noexcept;

} // namespace drv3d_vulkan
