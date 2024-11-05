// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "videorecorder.h"
#if _TARGET_PC_WIN
#include "renderer.h"
#include <videoEncoder/videoEncoder.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <windows.h>
#include <osApiWrappers/dag_direct.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/sharedComponent.h>
#include "cinematicMode.h"

wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len);

namespace videorecorder
{
void toggle_record()
{
  CinematicMode *cinematicMode = get_cinematic_mode();
  if (!cinematicMode)
    return;
  if (cinematicMode->getVideoEncoder().isRecording())
  {
    cinematicMode->getVideoEncoder().stop();
    cinematicMode->getVideoEncoder().shutdown();
    toggle_hide_gui();
  }
  else
  {
    VideoSettings settings;
    const char *dir = "records/";
    dd_mkdir(dir);
    DagorDateTime time;
    ::get_local_time(&time);
    String fname;
    fname.printf(256, "%srecording_%04d.%02d.%02d_%02d.%02d.%02d.%s", dir, time.year, time.month, time.day, time.hour, time.minute,
      time.second, "mp4");
    settings.fname = fname.c_str();
    IRenderWorld *wr = get_world_renderer();
    TextureInfo info;
    wr->getFinalTargetTex()->getinfo(info);
    settings.width = info.w;
    settings.height = info.h;
    settings.fps = 30;
    cinematicMode->setFps(30);
    toggle_hide_gui();
    cinematicMode->getVideoEncoder().init(d3d::get_device());
    cinematicMode->getVideoEncoder().start(settings);
  }
}
} // namespace videorecorder
#else
namespace videorecorder
{
void toggle_record() {}
} // namespace videorecorder
#endif
