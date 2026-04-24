// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/bindQuirrelEx/autoBind.h>
#include <drv/3d/dag_commands.h>

namespace
{
inline int d3d_driver_background_processing_mode(DriverBackgroundProcessingMode mode, DriverMeasurementsAction action)
{
  return d3d::driver_command(Drv3dCommand::DRIVER_BACKGROUND_PROCESSING_MODE, (void *)&mode, (void *)&action, nullptr);
}
} // namespace

SQ_DEF_AUTO_BINDING_MODULE(bind_render, "render")
{
  Sqrat::Table table(vm);

  table.SetValue("D3D_DRIVER_BACKGROUND_PROCESSING_MODE_ALLOWED", eastl::to_underlying(DriverBackgroundProcessingMode::ALLOWED));
  table.SetValue("D3D_DRIVER_BACKGROUND_PROCESSING_MODE_ALLOW_INTRUSIVE_MEASUREMENTS",
    eastl::to_underlying(DriverBackgroundProcessingMode::ALLOW_INTRUSIVE_MEASUREMENTS));
  table.SetValue("D3D_DRIVER_BACKGROUND_PROCESSING_MODE_DISABLE_BACKGROUND_WORK",
    eastl::to_underlying(DriverBackgroundProcessingMode::DISABLE_BACKGROUND_WORK));
  table.SetValue("D3D_DRIVER_BACKGROUND_PROCESSING_MODE_DISABLE_PROFILING_BY_SYSTEM",
    eastl::to_underlying(DriverBackgroundProcessingMode::DISABLE_PROFILING_BY_SYSTEM));
  table.SetValue("D3D_DRIVER_BACKGROUND_PROCESSING_MODE_KEEP_CURRENT",
    eastl::to_underlying(DriverBackgroundProcessingMode::KEEP_CURRENT));

  table.SetValue("D3D_DRIVER_MEASUREMENTS_ACTION_KEEP_ALL", eastl::to_underlying(DriverMeasurementsAction::KEEP_ALL));
  table.SetValue("D3D_DRIVER_MEASUREMENTS_ACTION_COMMIT_RESULTS", eastl::to_underlying(DriverMeasurementsAction::COMMIT_RESULTS));
  table.SetValue("D3D_DRIVER_MEASUREMENTS_ACTION_COMMIT_RESULTS_HIGH_PRIORITY",
    eastl::to_underlying(DriverMeasurementsAction::COMMIT_RESULTS_HIGH_PRIORITY));
  table.SetValue("D3D_DRIVER_MEASUREMENTS_ACTION_DISCARD_PREVIOUS", eastl::to_underlying(DriverMeasurementsAction::DISCARD_PREVIOUS));

  table //
    .Func("d3d_driver_background_processing_mode", d3d_driver_background_processing_mode)
    /**/;

  return table;
}
