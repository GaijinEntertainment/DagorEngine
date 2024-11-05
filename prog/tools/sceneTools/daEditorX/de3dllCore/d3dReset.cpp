// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

bool dagor_d3d_force_driver_reset = false;
void d3derr_in_device_reset(const char *msg) { DAG_FATAL("%s:\n%s (device reset)", msg, d3d::get_last_error()); }
