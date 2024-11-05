// Copyright (C) Gaijin Games KFT.  All rights reserved.

/// @module gpuBenchmark
#include <render/gpuBenchmarkEntity.h>
#include <EASTL/string_view.h>
#include <sqModules/sqModules.h>
#include <sqrat.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <EASTL/string.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>

namespace gpubenchmark
{

static void initGraphicsAutodetect()
{
  // TODO Skyquake gets the history of results here
  make_graphics_autodetect_entity(Selfdestruct::NO);
}

static bool isGpuBenchmarkRunning()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  return graphicsAutodetect && graphicsAutodetect->isGpuBenchmarkRunning();
}

static void startGpuBenchmark()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  G_ASSERT_RETURN(graphicsAutodetect, );
  if (!graphicsAutodetect->isGpuBenchmarkRunning())
    graphicsAutodetect->startGpuBenchmark();
}

static void closeGraphicsAutodetect()
{
  // TODO Skyquake updates the history here
  destroy_graphics_autodetect_entity();
}

static float getGpuBenchmarkDuration()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  G_ASSERT_RETURN(graphicsAutodetect, 0);
  return graphicsAutodetect->getGpuBenchmarkDuration();
}

static eastl::string getPresetFor60Fps()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  G_ASSERT_RETURN(graphicsAutodetect, "medium");
  return graphicsAutodetect->getPresetFor60Fps().str();
}

static eastl::string getPresetForMaxQuality()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  G_ASSERT_RETURN(graphicsAutodetect, "medium");
  return graphicsAutodetect->getPresetForMaxQuality().str();
}

static eastl::string getPresetForMaxFPS()
{
  GraphicsAutodetect *graphicsAutodetect = get_graphics_autodetect();
  G_ASSERT_RETURN(graphicsAutodetect, "medium");
  return graphicsAutodetect->getPresetForMaxFPS().str();
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_gpu_benchmark, "gpuBenchmark", sq::VM_INTERNAL_UI)
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("initGraphicsAutodetect", initGraphicsAutodetect)
    .Func("isGpuBenchmarkRunning", isGpuBenchmarkRunning)
    .Func("startGpuBenchmark", startGpuBenchmark)
    .Func("closeGraphicsAutodetect", closeGraphicsAutodetect)
    .Func("getGpuBenchmarkDuration", getGpuBenchmarkDuration)
    .Func("getPresetFor60Fps", getPresetFor60Fps)
    .Func("getPresetForMaxQuality", getPresetForMaxQuality)
    .Func("getPresetForMaxFPS", getPresetForMaxFPS)
    /**/;
  return tbl;
}

} // namespace gpubenchmark

static bool gpu_benchmark_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("benchmark", "run_benchmak", 1, 1)
  {
    GraphicsAutodetect *graphicsAutodetect = gpubenchmark::get_graphics_autodetect();
    if (graphicsAutodetect)
    {
      console::print("Benchmark already running.");
    }
    else
    {
      gpubenchmark::make_graphics_autodetect_entity(gpubenchmark::Selfdestruct::YES);
      console::print_d("Running benchmark for %ds...", (int)gpubenchmark::getGpuBenchmarkDuration());
      gpubenchmark::startGpuBenchmark();
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(gpu_benchmark_console_handler);
extern const size_t ecs_pull_gpu_benchmark_ui = 0;
