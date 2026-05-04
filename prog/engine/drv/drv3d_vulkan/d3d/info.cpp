// Copyright (C) Gaijin Games KFT.  All rights reserved.

// informational getters implementation
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include "globals.h"
#include "physical_device_set.h"
#include "device_memory.h"

using namespace drv3d_vulkan;

const char *d3d::get_driver_name() { return "Vulkan"; }
#if _TARGET_PC
DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::vulkan); }
#endif
const char *d3d::get_device_name() { return Globals::VK::phy.properties.deviceName; }
const char *d3d::get_device_driver_version() { return "1.0"; }
const DriverDesc &d3d::get_driver_desc() { return Globals::desc; }
void *d3d::get_device() { return drv3d_vulkan::Globals::VK::dev.get(); }
unsigned d3d::get_dedicated_gpu_memory_system_internal_overhead_kb() { return Globals::Mem::pool.getInternalDeviceMemoryUsageKb(); }
