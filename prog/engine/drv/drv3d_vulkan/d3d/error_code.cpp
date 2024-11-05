// Copyright (C) Gaijin Games KFT.  All rights reserved.

// tls error code getters implementation
#include <drv/3d/dag_info.h>
#include "vk_error.h"

const char *d3d::get_last_error() { return drv3d_vulkan::vulkan_error_string(drv3d_vulkan::get_last_error()); }
uint32_t d3d::get_last_error_code() { return drv3d_vulkan::get_last_error(); }
