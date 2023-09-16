#include "d3d_initialSettings.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>


D3dInitialSettings::D3dInitialSettings(int screen_w, int screen_h) :
  nonaaZbufSize(screen_w, screen_h),
  aaZbufSize(screen_w, screen_h),
  resolution(screen_w, screen_h),
  maxThreads(4),
  vsync(false),
  allowRetinaRes(false),
  useMpGLEngine(false),
  max_genmip_tex_sz(4096)
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");

  int min_target_size = blk_video.getInt("min_target_size", 0);
  IPoint2 defZDim = IPoint2(max(screen_w, min_target_size), max(screen_h, min_target_size));

  nonaaZbufSize = blk_video.getIPoint2("nonaa_zbuf_max", defZDim);
  aaZbufSize = blk_video.getIPoint2("aa_zbuf_max", defZDim);

  useMpGLEngine = blk_video.getBool("mpGLEngine", useMpGLEngine);
  vsync = blk_video.getBool("vsync", vsync);
  maxThreads = blk_video.getInt("maxThreads", maxThreads);
  max_genmip_tex_sz = blk_video.getInt("max_genmip_tex_sz", max_genmip_tex_sz);
}
