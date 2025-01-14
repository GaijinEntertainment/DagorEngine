// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <videoPlayer/dag_videoPlayer.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_tex3d.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <math/integer/dag_IPoint2.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_miscApi.h>
#include <stdio.h>
#if _TARGET_PC
#include "data_pc.h"
#else
#include "data_min.h"
#include <drv/3d/dag_platform.h>
#endif

#ifdef SUPPORT_AV1

#include <dav1d/dav1d.h>
#include <common/attributes.h>
#include <input/input.h>
#include <util/dag_console.h>


class Av1VideoPlayer : public IGenVideoPlayer, public VideoPlaybackData
{
  String name;
  bool autoRewind;
  bool isPlayed;
  int frameW, frameH;
  int lastRdPos;
  int lastFrameTime, frameTime;
  int64_t referenceTime;
  int basePosMsec;
  int movieDurMsec;
  int prebufDelay;

  Dav1dSettings libSettings;
  Dav1dContext *libContext;
  Dav1dPicture picture;
  Dav1dData data;
  DemuxerContext *inCtx;
  uint32_t total, timebase[2], fps[2];
  static int threadsNum;

public:
  virtual void destroy() { delete this; }

  virtual IGenVideoPlayer *reopenFile(const char *fname)
  {
    destroy();
    return create_av1_video_player(fname);
  }


  virtual IGenVideoPlayer *reopenMem(dag::ConstSpan<char>) { return NULL; }

  bool openFile(const char *fname)
  {
    lastRdPos = -1;
    isPlayed = false;
    movieDurMsec = -1;
    autoRewind = false;
    basePosMsec = 0;
    name = String(::df_get_real_name(fname));
    if (name.empty())
    {
      logerr("File %s not found", fname);
      return false;
    }
    unsigned dims[2];
    int res = input_open(&inCtx, "ivf", name.c_str(), fps, &total, timebase, dims);
    if (res)
    {
      logerr("AV1 open file error: %d, filename: %s", res, name.c_str());
      return false;
    }
    frameW = dims[0];
    frameH = dims[1];

    dav1d_default_settings(&libSettings);
    libSettings.n_threads = threadsNum;
    libSettings.max_frame_delay = threadsNum;
    libSettings.apply_grain = 0;
    res = dav1d_open(&libContext, &libSettings);
    if (res)
    {
      logerr("Dav1d open error: %d, filename: %s", res, name.c_str());
      return false;
    }
    frameTime = timebase[1] * 1000000 / timebase[0];
    movieDurMsec = total * frameTime;
    return input_read(inCtx, &data) >= 0 && initVideoBuffers(frameW, frameH);
  }

  virtual void start(int prebuf_delay_ms = 100)
  {
    if (isPlayed)
    {
      return;
    }
    isPlayed = true;
    referenceTime = ref_time_ticks_qpc();
    lastFrameTime = prebuf_delay_ms * 1000;
    if (vBuf.used)
    {
      return;
    }
    while (vBuf.used < 1 && !isFinished())
      advance(5000);
    prebufDelay = prebuf_delay_ms * 1000;
    lastFrameTime = prebufDelay;
    for (int i = 0; i < vBuf.used; i++)
    {
      vBuf.getRd(i).time = lastFrameTime;
      lastFrameTime += frameTime;
    }
    referenceTime = ref_time_ticks_qpc();
  }

  virtual void stop(bool reset = false)
  {
    isPlayed = false;
    if (!reset)
    {
      return;
    }
    vBuf.wrPos = vBuf.rdPos = (vBuf.rdPos + 1) % vBuf.depth;
    vBuf.used = 0;
    lastRdPos = -1;
    basePosMsec = 0;
  }

  virtual void rewind()
  {
    cleanupDav1d();
    int res;
    unsigned dims[2];
    res = input_open(&inCtx, "ivf", name.c_str(), fps, &total, timebase, dims);
    if (res)
    {
      logerr("AV1 open file error: %d, filename: %s", res, name.c_str());
      return;
    }
    frameW = dims[0];
    frameH = dims[1];
    dav1d_default_settings(&libSettings);
    libSettings.n_threads = threadsNum;
    libSettings.max_frame_delay = threadsNum;
    libSettings.apply_grain = 0;
    res = dav1d_open(&libContext, &libSettings);
    if (res)
    {
      logerr("Dav1d open error: %d, filename: %s", res, name.c_str());
      return;
    }
    frameTime = timebase[1] * 1000000 / timebase[0];
    movieDurMsec = total * frameTime;
    res = input_read(inCtx, &data);
    if (res)
    {
      logerr("Read data error: %d, filename: %s", res, name.c_str());
      return;
    }
    int lastFrameTimeFix = get_time_usec_qpc(referenceTime) + prebufDelay;
    if (lastFrameTimeFix > lastFrameTime)
    {
      basePosMsec -= (lastFrameTimeFix - lastFrameTime) / 1000;
      lastFrameTime = lastFrameTimeFix;
    }
  }

  virtual void setAutoRewind(bool auto_rewind) { autoRewind = auto_rewind; }

  virtual IPoint2 getFrameSize() { return IPoint2(frameW, frameH); }

  virtual int getDuration() { return movieDurMsec; }

  virtual int getPosition()
  {
    int pos = basePosMsec + int(get_time_usec_qpc(referenceTime) / 1000);
    return movieDurMsec > 0 ? pos % movieDurMsec : pos;
  }

  virtual bool isFinished() { return !isPlayed; }

  void enqueue_frame(int t)
  {
    if (vBuf.bufLeft() <= 1)
    {
      return;
    }
    if (lastRdPos == vBuf.wrPos)
      lastRdPos = -1;
    vBuf.getWr().time = t;
    Texture *tex = vBuf.getWr().texY;
    unsigned char *ptr;
    int stride;
    int fh2 = frameH / 2, fw2 = frameW / 2;
    // y
    if (!tex->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      dav1d_picture_unref(&picture);
      tex->unlockimg();
      return;
    }
    uint8_t *yPointer = (uint8_t *)picture.data[0];
    if (stride == picture.stride[0] && stride == frameW)
      memcpy(ptr, yPointer, frameW * frameH);
    else
      for (int y = 0; y < frameH; y++, ptr += stride, yPointer += picture.stride[0])
        memcpy(ptr, yPointer, frameW);
    tex->unlockimg();
    // u
    if (!vBuf.getWr().texU->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      dav1d_picture_unref(&picture);
      tex->unlockimg();
      return;
    }
    uint8_t *uPointer = (uint8_t *)picture.data[1];
    for (int y = 0; y < fh2; y++, ptr += stride, uPointer += picture.stride[1])
      memcpy(ptr, uPointer, fw2);
    vBuf.getWr().texU->unlockimg();
    // v
    if (!vBuf.getWr().texV->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      dav1d_picture_unref(&picture);
      tex->unlockimg();
      return;
    }
    uint8_t *vPointer = (uint8_t *)picture.data[2];
    for (int y = 0; y < fh2; y++, ptr += stride, vPointer += picture.stride[1])
      memcpy(ptr, vPointer, fw2);
    vBuf.getWr().texV->unlockimg();

    if (vBuf.bufLeft() > 1)
      vBuf.incWr();

    dav1d_picture_unref(&picture);
  }

  int rebaseTimer(int t_usec)
  {
    if (vBuf.used && vBuf.getRd().time < t_usec)
      t_usec = vBuf.getRd().time;
    if (lastFrameTime < t_usec)
      t_usec = lastFrameTime;

    if (t_usec < 1000000 + frameTime * vBuf.depth)
      return t_usec;

    t_usec = ((t_usec - frameTime * vBuf.depth) / 1000000) * 1000000;

    for (int i = 0; i < vBuf.used; i++)
      vBuf.getRd(i).time -= t_usec;
    basePosMsec += t_usec / 1000;
    lastFrameTime -= t_usec;

    referenceTime = rel_ref_time_ticks(referenceTime, t_usec);
    return get_time_usec_qpc(referenceTime);
  }

  virtual void advance(int max_usec)
  {
    if (!isPlayed)
      return;
    int endTime = get_time_usec_qpc(referenceTime);
    if (endTime > 120000000)
      endTime = rebaseTimer(endTime);
    endTime += max_usec;
    int targetTime = endTime + frameTime / 2;
    while (targetTime > vBuf.getRd().time && vBuf.used > 1)
      vBuf.incRd();
    int res;
    do
    {
      if (!picture.p.w)
      {
        if (data.sz > 0 || !input_read(inCtx, &data))
        {
          res = dav1d_send_data(libContext, &data);
          if (res < 0 && res != DAV1D_ERR(EAGAIN))
          {
            dav1d_data_unref(&data);
            isPlayed = false;
            return;
          }
          res = dav1d_get_picture(libContext, &picture);
          if (res < 0)
          {
            if (res != DAV1D_ERR(EAGAIN))
            {
              isPlayed = false;
              return;
            }
            continue;
          }
        }
        else
        {
          res = dav1d_get_picture(libContext, &picture);
          if (res < 0)
          {
            if (res != DAV1D_ERR(EAGAIN) || !autoRewind)
            {
              isPlayed = false;
            }
            else
            {
              rewind();
            }
            return;
          }
        }
      }

      if (vBuf.bufLeft() < 2)
      {
        return;
      }
      if (!d3d::get_event_query_status(vBuf.buf[vBuf.wrPos].ev, false))
      {
        return;
      }

      int t = lastFrameTime + frameTime;
      enqueue_frame(t);
      lastFrameTime = t;

    } while (get_time_usec_qpc(referenceTime) < endTime);
  }

  virtual bool getFrame(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV, d3d::SamplerHandle &smp)
  {
    smp = sampler;
    if (vBuf.used)
    {
      int targetTime = get_time_usec_qpc(referenceTime) + frameTime / 2;
      while (targetTime > vBuf.getRd().time && vBuf.used > 1)
        vBuf.incRd();
    }

    if (!vBuf.used)
    {
      if (lastRdPos >= 0)
      {
#if _TARGET_PC
        if (vBuf.buf[0].texIdY == BAD_TEXTUREID)
        {
          idY = useVideo[currentVideoFrame].texIdY;
          idU = useVideo[currentVideoFrame].texIdU;
          idV = useVideo[currentVideoFrame].texIdV;
        }
        else
#endif
        {
          idY = vBuf.buf[lastRdPos].texIdY;
          idU = vBuf.buf[lastRdPos].texIdU;
          idV = vBuf.buf[lastRdPos].texIdV;
        }
        return true;
      }
      return false;
    }

    lastRdPos = vBuf.rdPos;
    getCurrentTex(idY, idU, idV);
    return true;
  }

  virtual void onCurrentFrameDispatched() override
  {
    if (vBuf.buf[0].texIdY != BAD_TEXTUREID && lastRdPos >= 0)
    {
      auto &ev = vBuf.buf[lastRdPos].ev;
      if (!ev)
        ev = d3d::create_event_query(); // Init on demand to ensure that it's called from render (main) thread
#if _TARGET_C1







#endif
      d3d::issue_event_query(ev);
    }
  }

  virtual bool renderFrame(const IPoint2 *, const IPoint2 *) { return false; }

  virtual void beforeResetDevice()
  {
    for (int bufNo = 0; bufNo < vBuf.getDepth(); bufNo++)
    {
      if (vBuf.buf[bufNo].ev)
      {
        d3d::release_event_query(vBuf.buf[bufNo].ev);
        vBuf.buf[bufNo].ev = NULL;
      }
    }
  }

  virtual void afterResetDevice()
  {
    for (int bufNo = 0; bufNo < vBuf.getDepth(); bufNo++)
    {
      if (!vBuf.buf[bufNo].ev)
        vBuf.buf[bufNo].ev = d3d::create_event_query();
    }
  }

  Av1VideoPlayer(int q_depth)
  {
    picture = {0};
    data = {0};
    name = "";
    autoRewind = false;
    isPlayed = false;
    frameW = 0;
    frameH = 0;
    lastRdPos = -1;
    lastFrameTime = 0;
    frameTime = 0;
    referenceTime = 0;
    basePosMsec = 0;
    movieDurMsec = 0;
    libContext = NULL;
    inCtx = NULL;
    total = 0;
    timebase[0] = timebase[1] = 0;
    fps[0] = fps[1] = 0;
    prebufDelay = 100000;
    dav1d_default_settings(&libSettings);
    libSettings.n_threads = threadsNum;
    libSettings.max_frame_delay = threadsNum;
    libSettings.apply_grain = 0;
    initBuffers(q_depth);
  }
  ~Av1VideoPlayer()
  {
    cleanupDav1d();
    termVideoBuffers();
    termBuffers();
  }
  void cleanupDav1d()
  {
    if (inCtx)
      input_close(inCtx);
    if (libContext)
    {
      if (picture.p.w)
        dav1d_picture_unref(&picture);
      for (;;)
      {
        int res = dav1d_get_picture(libContext, &picture);
        if (res == DAV1D_ERR(EAGAIN))
          break;
        if (res >= 0)
          dav1d_picture_unref(&picture);
      }
      dav1d_close(&libContext);
    }
    if (data.sz > 0)
      dav1d_data_unref(&data);
  }

  static void setThreadsCounts(int threads_num) { threadsNum = threads_num; }
};
#endif

IGenVideoPlayer *IGenVideoPlayer::create_av1_video_player(const char *fname, int buf_frames)
{
#ifdef SUPPORT_AV1
  Av1VideoPlayer *vp = new (midmem) Av1VideoPlayer(buf_frames);
  if (vp->openFile(fname))
    return vp;
  vp->destroy();
#else
  G_UNUSED(fname);
  G_UNUSED(buf_frames);
#endif
  return NULL;
}

#ifdef SUPPORT_AV1

#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
int Av1VideoPlayer::threadsNum = 4;
#else
int Av1VideoPlayer::threadsNum = 2;
#endif

using namespace console;
static bool av1_tile_settings(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("av1_player", "set_threads", 1, 2) { Av1VideoPlayer::setThreadsCounts(argv[1][0] - '0'); }
  return found;
}

REGISTER_CONSOLE_HANDLER(av1_tile_settings);

#endif