// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <movie/fullScreenMovie.h>
#include <videoPlayer/dag_videoPlayer.h>
#include <debug/dag_debug.h>
#include <util/dag_localization.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <3d/dag_picMgr.h>
#include <stdlib.h>
#include <workCycle/dag_workCycle.h>
#include <daGUI/dag_guiApi.h>
#include <daGUI/dag_fixedPropIds.h>
#include "playerCreators.h"
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_cpuJobs.h>

#define VIDEO_SETTINGS_BLOCK "_video_settings"

class DaGuiPlayer : public IGenVideoPlayer
{
public:
  DaGuiPlayer(const char *fname) : cur(0), scn(NULL), startTime(0)
  {
    blk.load(fname);
    const DataBlock *settings = blk.getBlockByNameEx(VIDEO_SETTINGS_BLOCK);
    {
      length = settings->getInt("length", 0);
      fadeInTime = settings->getInt("fadeInTime", -1);
      fadeOutTime = settings->getInt("fadeOutTime", -1);
      G_ASSERT(fadeOutTime + fadeInTime <= length);
    }
    blk.removeBlock(VIDEO_SETTINGS_BLOCK);
  }
  virtual ~DaGuiPlayer() { stop(); }
  virtual void destroy() { delete this; }
  virtual IGenVideoPlayer *reopenFile(const char *fname)
  {
    destroy();
    return new DaGuiPlayer(fname);
  }
  virtual IGenVideoPlayer *reopenMem(dag::ConstSpan<char> data)
  {
    (void)data;
    DEBUG_CTX("not implemented");
    destroy();
    return NULL;
  }

  virtual void start(int prebuf_delay_ms = 100)
  {
    (void)prebuf_delay_ms;
    if (!scn)
    {
      scn = new dagui::ObjScene;
      scn->loadScene(blk, NULL, "");

      // Triplehead.
      real w = StdGuiRender::screen_width();
      real h = StdGuiRender::screen_height();
      if (w / h >= 3)
      {
        scn->getRoot()->setDynPropsInt(dagui::PROPID_WIDTH, int(w / 3 + 0.5f));
        scn->getRoot()->setDynPropsInt2(dagui::PROPID_POS, int(w / 3 + 0.5f), 0);
        scn->markChanged(scn->getRoot());
      }

      scn->applyPendingChanges(true);
    }
    cur = 0;
    startTime = get_time_msec();
  }

  virtual void stop(bool reset = false)
  {
    del_it(scn);
    if (reset)
      cur = 0;
  }

  virtual void rewind() { cur = 0; }

  virtual void setAutoRewind(bool auto_rewind) { (void)auto_rewind; }

  //! returns frame size of loaded video
  virtual IPoint2 getFrameSize() { return IPoint2(0, 0); }

  //! returns movie duration in ms or -1 when container doesn't provide duration
  virtual int getDuration() { return length; }

  //! returns current playback position in ms
  virtual int getPosition() { return cur; }

  //! returns true when movie is fully played back and stopped
  virtual bool isFinished() { return cur >= length; }

  //! reads/decodes ahead frames; tries to spend not more that max_usec microseconds
  virtual void advance(int max_usec)
  {
    int oldCur = cur;
    cur = get_time_msec() - startTime;

    if (scn && (oldCur != cur))
    {
      scn->act((cur - oldCur) * 0.001f);
    }

    (void)max_usec;
    if (cpujobs::is_inited())
    {
      cpujobs::release_done_jobs();
      PictureManager::discard_unused_picture();
    }
  }

  //! returns current frame in YUV format (supported by: Ogg Theora player, PAM player)
  virtual bool getFrame(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV, d3d::SamplerHandle &smp)
  {
    (void)idY, (void)idU, (void)idV, (void)smp;
    return false;
  }

  //! render current frame to screen rectangle (supported by: WMW player)
  virtual bool renderFrame(const IPoint2 *lt = NULL, const IPoint2 *rb = NULL)
  {
    (void)lt, (void)rb;

    StdGuiRender::start_render();
    if (scn)
    {
      scn->beforeRender();
      scn->render();
    }
    float fade = 0.f;
    real w = StdGuiRender::screen_width();
    real h = StdGuiRender::screen_height();

    if (cur < fadeInTime)
      fade = (1.0 - float(cur) / float(fadeInTime));
    else if ((fadeOutTime > 0) && (cur > (length - fadeOutTime)))
    {
      fade = 1.0 - float(length - cur) / float(fadeOutTime);
    }

    StdGuiRender::reset_textures();
    StdGuiRender::set_color(0, 0, 0, (int)(255.f * fade));
    StdGuiRender::render_quad(Point2(0, 0), Point2(0, h), Point2(w, h), Point2(w, 0), Point2(0, 0), Point2(0, 1), Point2(1, 1),
      Point2(1, 0));
    StdGuiRender::reset_textures();

    StdGuiRender::end_render();

    return true;
  }
  virtual void onCurrentFrameDispatched() {}
  virtual void beforeResetDevice() {}
  virtual void afterResetDevice() {}


protected:
  int length, cur;
  int fadeInTime, fadeOutTime;
  DataBlock blk;
  dagui::ObjScene *scn;
  int startTime;
};

static IGenVideoPlayer *create_dagui(const char *fname) { return new DaGuiPlayer(fname); }

void init_player_for_dagui() { create_dagui_movie_player = &create_dagui; }
