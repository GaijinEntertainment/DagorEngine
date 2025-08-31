// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/levelProfiler.h>
#include "levelProfilerUI.h"

namespace levelprofiler
{

static LevelProfilerUI *g_level_profiler_instance = nullptr;


ILevelProfiler *ILevelProfiler::getInstance()
{
  if (!g_level_profiler_instance)
  {
    g_level_profiler_instance = new LevelProfilerUI();
    g_level_profiler_instance->initialize();
  }
  return g_level_profiler_instance;
}


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
    g_level_profiler_instance->initialize();
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

  auto tab = g_level_profiler_instance->getTab(0);
  if (!tab || !tab->module)
    return;

  // Static cast is safe: Tab 0 is always TextureProfilerUI.
  TextureProfilerUI *textureUI = static_cast<TextureProfilerUI *>(tab->module);
  textureUI->getTextureModule()->selectTexture(texture_name);


  auto table = textureUI->getTextureTable();
  if (table)
    table->setSelectedTexture(texture_name);
}


void register_external_texture(ExternalTextureProvider & /* texture_provider */) // UNUSED
{
  // VOID
}


void unregister_external_texture(ExternalTextureProvider & /* texture_provider */) // UNUSED
{
  // VOID
}

} // namespace levelprofiler


REGISTER_IMGUI_WINDOW("Level Tools", "Level Profiler", levelprofiler::imgui_window);