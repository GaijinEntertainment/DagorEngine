// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/deviceResetTelemetry/deviceResetTelemetry.h>

#include <dag/dag_vector.h>

#include <3d/dag_gpuConfig.h>
#include <drv/3d/dag_info.h>

#include <statsd/statsd.h>


void report_device_reset()
{
  // Store tmp objects to have valid const char * for tag values
  const dag::Vector<statsd::MetricTag> tags = {
    {"d3d_driver", d3d::get_driver_name()}, {"gpu_vendor", d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor)}};
  statsd::counter("render.device_reset", 1, tags);
}
