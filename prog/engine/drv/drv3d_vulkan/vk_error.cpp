// Copyright (C) Gaijin Games KFT.  All rights reserved.

// TLS like wrapper for "last error" style error reporting
#include "vk_error.h"

namespace
{
thread_local VkResult tlsLastErrorCode = VK_SUCCESS;
} // anonymous namespace

void drv3d_vulkan::set_last_error(VkResult error) { tlsLastErrorCode = error; }
VkResult drv3d_vulkan::get_last_error() { return tlsLastErrorCode; }