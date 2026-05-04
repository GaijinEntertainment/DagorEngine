// Copyright (C) Gaijin Games KFT.  All rights reserved.

///@module autoGraphics

#include <generic/dag_enumerate.h>
#include <render/autoGraphics.h>

#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <quirrel/bindQuirrelEx/autoBind.h>


namespace auto_graphics
{

static SQInteger getPresetForEachAvailableTargetFps(HSQUIRRELVM vm)
{
  const auto autoPresets = auto_quality_preset::get_preset_for_each_available_target_fps();

  Sqrat::Array targetFps(vm, autoPresets.size());
  Sqrat::Array preset(vm, autoPresets.size());
  Sqrat::Array resolution(vm, autoPresets.size());

  for (auto [i, autoPreset] : enumerate(autoPresets, 0))
  {
    targetFps.SetValue(i, autoPreset.targetFps);
    preset.SetValue(i, autoPreset.preset.c_str());
    resolution.SetValue(i, autoPreset.resolution);
  }

  Sqrat::Array result(vm, 3);
  result.SetValue(0, targetFps).SetValue(1, preset).SetValue(2, resolution);
  Sqrat::PushVar(vm, result);
  return 1;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_auto_graphics, "autoGraphics", sq::VM_INTERNAL_UI)
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("is_auto_preset_available", auto_quality_preset::is_valid)
    .SquirrelFunc("get_preset_for_each_available_target_fps", getPresetForEachAvailableTargetFps, 1)
    /**/;
  return tbl;
}

} // namespace auto_graphics

extern const size_t ecs_pull_auto_graphics_ui = 0;
