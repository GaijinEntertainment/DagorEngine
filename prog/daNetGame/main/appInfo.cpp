// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <time.h>
#include <stdio.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_cpuFreq.h>
#include <memory/dag_framemem.h>
#include <math/dag_TMatrix.h>
#include "render/screencap.h"
#include "render/skies.h"
#include "version.h"
#include "main/gameLoad.h"
#include "main/appProfile.h"
#include "main/level.h"

extern "C"
{
#ifdef TEST_BUILD
  const char *dagor_exe_build_date = __DATE__;
  const char *dagor_exe_build_time = __TIME__;
#else
  extern const char *dagor_exe_build_date;
  extern const char *dagor_exe_build_time;
#endif
}


void set_screen_shot_comments(const TMatrix &view_itm)
{
  DataBlock output_blk(framemem_ptr());
  {
    DataBlock *b = output_blk.addNewBlock("version");
    // b->addStr("game_version", get_game_version_str(verBuf));
    // b->addStr("gui_version", get_gui_vromfs_version().str());
    b->addStr("build_date", dagor_exe_build_date);
    b->addStr("build_time", dagor_exe_build_time);
    b->addInt("build_number", get_build_number());
    b->addInt("dbg_level", DAGOR_DBGLEVEL);
    // b->addStr("platform", get_platform_string_id());
    b->addStr("driver", d3d::get_driver_name());
    b->addStr("device", d3d::get_device_name());
    b->addInt("cur_time", int(time(NULL)));
    // b->addStr("video_vendor_info", get_video_vendor_str());
    char buf[16];
    int t = get_time_msec();
    SNPRINTF(buf, sizeof(buf), "%3d.%02d", t / 1000, (t % 1000) / 10); // the same format as in debug
    b->addStr("t_after_start", buf);
  }
  output_blk.addStr("scene", sceneload::get_current_game().sceneName.c_str());
  if (!app_profile::get().serverSessionId.empty())
    output_blk.addStr("session_id", app_profile::get().serverSessionId.c_str());
  {
    DataBlock *b = output_blk.addNewBlock("camera");
    b->addPoint3("pos", view_itm.getcol(3));
    b->addPoint3("dir", view_itm.getcol(2));
    b->addTm("itm", view_itm);
  }
  {
    const DataBlock *originalConfigBlk = dgs_get_settings()->getBlockByName("originalConfigBlk");
    if (originalConfigBlk)
      output_blk.addNewBlock(originalConfigBlk, "original_config_blk");
    if (dgs_get_settings())
      output_blk.addNewBlock(dgs_get_settings(), "settings");
  }
  DataBlock *b = output_blk.addNewBlock("skies");
  save_daskies(*b);
  save_weather_settings_to_screenshot(*output_blk.addNewBlock("weather"));
  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 64 << 10);
  cwr.seekto(0);
  cwr.resize(0);
  output_blk.saveToTextStream(cwr);
  cwr.write("\0", 1);
  screencap::set_comments((const char *)cwr.data());
}
