// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "captureGbuffer.h"

#include <drv/3d/dag_lock.h>
#include <3d/dag_textureIDHolder.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <EASTL/array.h>
#include <math/dag_TMatrix.h>
#include "camera/sceneCam.h"
#include <render/debugGbuffer.h>
#include "screencap.h"
#include "renderer.h"

static bool is_scheduled = false;
static bool is_in_progress = false;

void capture_gbuffer::make_gbuffer_capture()
{
  G_ASSERT_RETURN(!is_in_progress, );

  debug("Rendering Gbuffer capture...");
  is_in_progress = true;

  DagorDateTime time;
  ::get_local_time(&time);
  String dir(256, "shotGbuffer_%04d.%02d.%02d_%02d.%02d.%02d", time.year, time.month, time.day, time.hour, time.minute, time.second);

  for (size_t i = 0; i < gbuffer_debug_options.size(); i++)
  {
    debug("Rendering Gbuffer capture with: %s", gbuffer_debug_options[i].data());
    console::command(String(0, "render.show_gbuffer %s", gbuffer_debug_options[i].data()));

    {
      d3d::GpuAutoLock gpuLock;
      before_draw_scene(0, 0, 0, ::get_cur_cam_entity());
      draw_scene(0);
    }

    String fileNameOverride(0, "%s/%d_%s", dir, i, gbuffer_debug_options[i].data());
    screencap::make_screenshot(get_world_renderer()->getFinalTargetTex(), fileNameOverride);
  }

  console::command("render.show_gbuffer");

  is_scheduled = false;
  is_in_progress = false;

  const char *screenshotsDir = ::dgs_get_settings()->getBlockByNameEx("screenshots")->getStr("dir", "Screenshots");
  console::print_d("Gbuffer capture saved: %s/%s", screenshotsDir, dir);
}

bool capture_gbuffer::is_gbuffer_capturing_in_progress() { return is_in_progress; }

void capture_gbuffer::schedule_gbuffer_capture() { is_scheduled = true; }

bool capture_gbuffer::is_gbuffer_capture_scheduled() { return is_scheduled; }
