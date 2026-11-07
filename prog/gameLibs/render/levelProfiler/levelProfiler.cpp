// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/levelProfiler.h>
#include "levelProfilerUI.h"

namespace levelprofiler
{

static LevelProfilerUI *g_level_profiler_instance = nullptr;


static void imgui_window()
{
  if (g_level_profiler_instance)
    g_level_profiler_instance->drawUI();
}


void init()
{
  if (!g_level_profiler_instance)
  {
    g_level_profiler_instance = new LevelProfilerUI();
    g_level_profiler_instance->init();
  }
}


void teardown()
{
  if (g_level_profiler_instance)
  {
    g_level_profiler_instance->shutdown();
    delete g_level_profiler_instance;
    g_level_profiler_instance = nullptr;
  }
}


void select_texture(const char *texture_name)
{
  if (!g_level_profiler_instance || !texture_name)
    return;

  g_level_profiler_instance->selectTexture(texture_name);
}

} // namespace levelprofiler


REGISTER_IMGUI_WINDOW("Level Tools", "Level Profiler", levelprofiler::imgui_window);