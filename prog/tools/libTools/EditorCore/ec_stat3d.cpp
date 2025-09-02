// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_stat3d.h"
#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_cm.h>
#include <perfMon/dag_graphStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_dataBlock.h>

#include <debug/dag_debug.h>
#include <libTools/renderViewports/renderViewport.h>
#include <libTools/renderViewports/cachedViewports.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_query.h>
#include <3d/dag_render.h>
#include <3d/dag_texMgr.h>

#include <gui/dag_stdGuiRenderEx.h>
#include <util/dag_string.h>
#include <render/dynmodelRenderer.h>
#include <perfMon/gpuProfiler.h>

#define COLOR_HISTOGRAM_GRAPH_NAME      "Color histogram"
#define BRIGHTNESS_HISTOGRAM_GRAPH_NAME "Brightness histogram"
#define HISTOGRAM_UPDATE_TIME           1000
#define HISTOGRAM_WIDTH                 256
#define HISTOGRAM_HEIGHT                128


extern CachedRenderViewports *ec_cached_viewports;


static const char *ds_blk_text = "draw_prim {\nid:i=1\nname:t=Draw prim\nconsole:t=dprim\n"
                                 "color:c=255,0,0,255\nup:i=5000\n}\n"
                                 "triangle {\nid:i=2\nname:t=Draw tri\nconsole:t=tri\n"
                                 "color:c=0,255,0,255\nup:i=100000\n}\n"
                                 "time {\nid:i=4\nname:t=FPS\nconsole:t=fps\n"
                                 "color:c=0,0,255,255\nup:i=50\n}\n"
                                 "memory {\nid:i=8\nname:t=Free mem\nconsole:t=mem\n"
                                 "color:c=255,255,0,255\nup:i=200\n}\n"
                                 "lockvi {\nid:i=16\nname:t=Lock v/i buf\nconsole:t=lockvi\n"
                                 "color:c=255,0,255,255\nup:i=5000\n}\n"
                                 "locktex {\nid:i=32\nname:t=Lock tex\nconsole:t=locktex\n"
                                 "color:c=0,255,255,255\nup:i=1\n}\n"
                                 "render_target {\nid:i=64\nname:t=RT set\nconsole:t=rt\n"
                                 "color:c=255,50,50,255\nup:i=20\n}\n"
                                 "pixel_shader {\nid:i=128\nname:t=PS set\nconsole:t=ps\n"
                                 "color:c=50,255,50,255\nup:i=50\n}\n"
                                 "vertex_shader {\nid:i=256\nname:t=VS set\nconsole:t=vs\n"
                                 "color:c=50,255,255,255\nup:i=100\n}\n"
                                 "pixel_shader_const {\nid:i=512\nname:t=PS const\nconsole:t=psc\n"
                                 "color:c=255,255,50,255\nup:i=300\n}\n"
                                 "vertex_shader_const {\nid:i=1024\nname:t=VS const\nconsole:t=vsc\n"
                                 "color:c=50,50,255,255\nup:i=5000\n}\n"
                                 "refresh_usec:i=0\n";
static int fps_min, fps_max, fps_cnt, fps_last_time = 0;
static double ifps_total, fps_avg;

static int64_t cpu_render_reft = 0;
static int cpu_render_time = 0, total_cpu_render_time = 0;
static int cpu_usage_min = 0, cpu_usage_max = 0;
static float cpu_usage_avg = 0, cpu_usage_total = 0;

static DrawStatSingle frame_stat;
static dynrend::Statistics unit_stat;

static const int stat_count = 11;
static const char *stat_names[stat_count] = {
  "fps", "cpu frame", "triangles", "dip", "lock_vbib", "rt", "shaders", "instances", "rpasses", "dip/unit", "tri/unit"};
static bool displayed_stats[stat_count] = {true, true, true, true, true, true, true, true, true, true, true};

void ec_init_stat3d()
{
  static bool inited = false;
  if (inited)
    return;
  inited = true;
  fps_min = 0;
  fps_max = 0;
  fps_avg = 0;
  fps_cnt = 0;
  ifps_total = 0;
  fps_last_time = 0;
#if !_TARGET_STATIC_LIB
  install_gpu_profile_func_table();
#endif
}


void ec_stat3d_save_enabled_graphs(DataBlock &blk)
{
  blk.clearData();

  for (int i = 0; i < stat_count; ++i)
    if (displayed_stats[i])
      blk.addStr("stat", stat_names[i]);
}

static int getStatByName(const char *name)
{
  for (int i = 0; i < stat_count; ++i)
    if (strcmp(stat_names[i], name) == 0)
      return i;

  return -1;
}

void ec_stat3d_load_enabled_graphs(int vp_idx, const DataBlock *blk)
{
  if (blk)
  {
    for (int i = 0; i < stat_count; ++i)
      displayed_stats[i] = false;

    const int nid = blk->getNameId("stat");
    for (int i = 0; i < blk->paramCount(); ++i)
      if (blk->getParamType(i) == DataBlock::TYPE_STRING && blk->getParamNameId(i) == nid)
      {
        const char *statName = blk->getStr(i);
        const int statIndex = getStatByName(statName);
        if (statIndex >= 0)
          displayed_stats[statIndex] = true;
      }
  }

  if (!vp_idx && ::ec_cached_viewports)
    ::ec_cached_viewports->forceViewportCache(vp_idx, false);
}

void ec_stat3d_wait_frame_end(bool frame_start)
{
  if (!frame_start)
  {
    cpu_render_time = get_time_usec_qpc(cpu_render_reft);
    Stat3D::stopStatSingle(frame_stat);
  }

  /*
  d3d::EventQuery *q = d3d::create_event_query();
  if (d3d::issue_event_query(q))
  {
    while (!d3d::get_event_query_status(q, true))
      ;
  }
  total_cpu_render_time = get_time_usec_qpc(cpu_render_reft);
  d3d::release_event_query(q);
  */

  if (frame_start)
  {
    total_cpu_render_time = get_time_usec_qpc(cpu_render_reft);
    Stat3D::startStatSingle(frame_stat);
    cpu_render_reft = ref_time_ticks_qpc();
  }
}

void ec_stat3d_on_unit_begin() { dynrend::reset_statistics(); }

void ec_stat3d_on_unit_end() { unit_stat = dynrend::get_statistics(); }

void ViewportWindow::drawStat3d()
{
  using hdpi::_pxScaled;

  nextStat3dLineY = _pxScaled(40);

  if (!needStat3d())
    return;

  double cfps = 1.e6 / total_cpu_render_time;
  if (cfps > 0)
  {
    fps_cnt++;
    ifps_total += cfps < 1e5 ? 1.0 / cfps : 1e-5;
    float cpu_usage = ((cpu_render_time / 1e6) * cfps) * 100;
    cpu_usage_total += cpu_usage;
    int ct = ::get_time_msec();

    if (ct > fps_last_time + 1000)
    {
      fps_avg = fps_cnt / ifps_total;
      fps_min = cfps;
      fps_max = cfps;
      cpu_usage_min = cpu_usage;
      cpu_usage_max = cpu_usage;
      cpu_usage_avg = cpu_usage_total / fps_cnt;
      fps_cnt = 0;
      ifps_total = 0;
      cpu_usage_total = 0;
      fps_last_time = ct;
    }
    else
    {
      if (fps_max < cfps)
        fps_max = cfps;
      if (fps_min > cfps)
        fps_min = cfps;
      if (cpu_usage_max < cpu_usage)
        cpu_usage_max = cpu_usage;
      if (cpu_usage_min > cpu_usage)
        cpu_usage_min = cpu_usage;
    }
  }

  bool do_start = !StdGuiRender::is_render_started();
  if (do_start)
    StdGuiRender::continue_render();

  StdGuiRender::set_font(0);
  StdGuiRender::set_color(COLOR_WHITE);

  G_STATIC_ASSERT(stat_count == (2 + DRAWSTAT_NUM + 2));
  hdpi::Px lineX = _pxScaled(8);
  for (int i = -2; i < DRAWSTAT_NUM + 2; i++)
  {
    const int zeroBasedIndex = i + 2;
    if (!displayed_stats[zeroBasedIndex])
      continue;

    const char *name = stat_names[zeroBasedIndex];

    if (i >= 0)
    {
      int val = i == DRAWSTAT_NUM       ? unit_stat.dips
                : i == DRAWSTAT_NUM + 1 ? unit_stat.triangles
                : i > 0                 ? frame_stat.val[i]
                                        : (int)frame_stat.tri;

      String curStat(128, "%s: %d", name, val);
      drawText(lineX, nextStat3dLineY, curStat);
    }
    else if (i == -1)
    {
      float cpuFrameAvg = fps_avg > 0 ? ((cpu_usage_avg / 100) * 1000 / fps_avg) : 0.0;
      String curStat(128, "CPU frame avg: %5.1fms, current: %5.1fms, with wait %5.1fms ", cpuFrameAvg, cpu_render_time / 1000.,
        total_cpu_render_time / 1000.);
      drawText(lineX, nextStat3dLineY, curStat);
    }
    else
    {
      String curStat(256, "FPS avg: %5.1f, min: %3d, max: %3d", fps_avg, fps_min, fps_max);
      drawText(lineX, nextStat3dLineY, curStat);
    }

    nextStat3dLineY += _pxScaled(20);
  }

  StdGuiRender::flush_data();
  if (do_start)
    StdGuiRender::end_render();
}


void ViewportWindow::fillStat3dStatSettings(ViewportWindowStatSettingsDialog &dialog)
{
  G_STATIC_ASSERT((CM_STATS_SETTINGS_STAT3D_ITEM_LAST + 1 - CM_STATS_SETTINGS_STAT3D_ITEM0) >= stat_count);

  PropPanel::TLeafHandle statGroup = dialog.addGroup(CM_STATS_SETTINGS_STAT3D_GROUP, "Stat 3d", calcStat3d);

  for (int i = 0; i < stat_count; ++i)
    dialog.addOption(statGroup, CM_STATS_SETTINGS_STAT3D_ITEM0 + i, stat_names[i], displayed_stats[i]);
}

void ViewportWindow::handleStat3dStatSettingsDialogChange(int pcb_id, bool value)
{
  if (pcb_id >= CM_STATS_SETTINGS_STAT3D_ITEM0 && pcb_id < (CM_STATS_SETTINGS_STAT3D_ITEM0 + stat_count))
  {
    const int statIndex = pcb_id - CM_STATS_SETTINGS_STAT3D_ITEM0;
    displayed_stats[statIndex] = value;
  }
}
