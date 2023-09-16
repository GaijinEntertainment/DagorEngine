#if DAGOR_DBGLEVEL > 0

#include <perfMon/dag_fpsLog.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_files.h>
#include <3d/dag_render.h>
#include <workCycle/wcPrivHooks.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

namespace fpslog
{
#define MAX_LOG 32

struct FrameLog
{
  int time;
  Point3 pos;
  float fps;
};

static FrameLog badLog[MAX_LOG];
static int badLogCount = 0;

static float minFpsThreshold = 15;
static char locName[100];
static int startTime = 0;
static float minFps = 0, maxFps = 0, minGoodFps = 0;
static int numFrames = 0, numLowFps = 0;
static Point3 minFpsPos(0, 0, 0);

static file_ptr_t logFile = NULL;


static void resetFps()
{
  minFps = 1000000;
  minGoodFps = 1000000;
  maxFps = 0;
  numFrames = 0;
  numLowFps = 0;

  badLogCount = 0;

  startTime = get_time_msec();
}


static void on_before_frame(int frame_time_usec)
{
  if (frame_time_usec <= 0)
    return;

  float fps = 1.0e6 / frame_time_usec;

  if (fps < minFps)
  {
    minFps = fps;
    minFpsPos = ::grs_cur_view.pos;
  }

  if (fps > maxFps)
    maxFps = fps;

  if (fps < minFpsThreshold)
  {
    ++numLowFps;

    if (badLogCount < MAX_LOG)
    {
      FrameLog &log = badLog[badLogCount];
      log.time = get_time_msec() - startTime;
      log.pos = ::grs_cur_view.pos;
      log.fps = fps;
      ++badLogCount;
    }
  }
  else if (fps < minGoodFps)
    minGoodFps = fps;

  ++numFrames;
  if (frame_time_usec > 60000)
    logerr("long frame %u: %d usec", dagor_frame_no(), frame_time_usec);
}


void start_log(const char *app_name)
{
  memset(locName, 0, sizeof(locName));
  resetFps();

  char fileName[256];
  const char *fname = "_fpslog.csv";
  strncpy(fileName, app_name, sizeof(fileName) - strlen(fname) - 1);
  fileName[sizeof(fileName) - 1] = 0;
  strcat(fileName, fname);
  logFile = df_open(fileName, DF_WRITE | DF_CREATE | DF_APPEND);

  if (!logFile)
    return;

  df_printf(logFile, "\n<location>;<bad count/time>;<min FPS pos>;"
                     "<min>;<min except bad>;<max>;<average>\n");

  dwc_hook_fps_log = &on_before_frame;
}


void stop_log()
{
  dwc_hook_fps_log = NULL;

  flush_log();

  if (logFile)
  {
    df_close(logFile);
    logFile = NULL;
  }
}


void set_min_fps_threshold(float fps) { minFpsThreshold = fps; }


void set_location_name(const char *name)
{
  if (!logFile)
    return;

  flush_log();
  strncpy(locName, name, sizeof(locName) - 1);
}


void flush_log()
{
  if (!logFile)
    return;

  if (numFrames > 0)
  {
    int totalTime = get_time_msec() - startTime;
    if (totalTime <= 0)
      totalTime = 1;

    df_printf(logFile, "%s;%d;%f %f %f;%f;%f;%f;%f\n", locName, numLowFps, minFpsPos.x, minFpsPos.y, minFpsPos.z, minFps, minGoodFps,
      maxFps, numFrames * 1000.0 / totalTime);

    for (int i = 0; i < badLogCount; ++i)
    {
      FrameLog &log = badLog[i];
      df_printf(logFile, ";%d.%03d;%f %f %f;%f\n", log.time / 1000, log.time % 1000, log.pos.x, log.pos.y, log.pos.z, log.fps);
    }

    df_flush(logFile);
  }

  resetFps();
}

} // namespace fpslog
#endif
