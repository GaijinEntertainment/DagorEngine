// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <movie/daGuiMovie.h>
#include <daGUI/dag_IRenderElem.h>
#include <daGUI/dag_IBehaviour.h>
#include <daGUI/dag_guiGlobals.h>
#include <daGUI/dag_object.h>
#include <videoPlayer/dag_videoPlayer.h>
#include <util/dag_simpleString.h>
#include "yuvFrame.h"
#include "webcache.h"
#include <datacache/datacache.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>

namespace dagui
{
class MovieBhv;
}

static void on_movie_loaded(const char *, datacache::ErrorCode, datacache::Entry *entry, void *arg);

static const int MAX_LOAD_IDS_COUNT = 6;

class dagui::MovieBhv : public dagui::IBehaviour
{
public:
  MovieBhv()
  {
    objsAttached = 0;
    player = NULL;
    timeQuantUsec = 8000;

    curLoadId = autoStartId = cmdId = stateId = loopId = timeQuantUsecId = -1;
  }

  virtual bool init()
  {
    if (!setBehaviourId("Movie"))
      return false;

    events = EV_AFTER_RECALC | EV_RENDER | EV_TIMER;

    for (int i = 0; i < MAX_LOAD_IDS_COUNT; ++i)
    {
      char propName[16] = "movie-load";
      if (i > 0)
        snprintf(propName, sizeof(propName), "movie-load%d", i);
      loadIds[i] = dagui::add_prop_name_id(propName);
    }
    autoStartId = dagui::add_prop_name_id("movie-autoStart");
    loopId = dagui::add_prop_name_id("movie-loop");
    cmdId = dagui::add_prop_name_id("movie-cmd");
    stateId = dagui::add_prop_name_id("movie-state");
    bufFramesId = dagui::add_prop_name_id("movie-bufFrames");
    timeQuantUsecId = dagui::add_prop_name_id("movie-timeQuantUsec");
    prebufMsecId = dagui::add_prop_name_id("movie-prebufMsec");

    return true;
  }

  virtual int onAttach(Object *obj)
  {
    obj->getScene().errorMessage(ObjScene::MSG_NOTE, "attached Movie to %p", obj);
    curLoadId = loadIds[0];
    obj->pushProps();
    setState(obj, NULL, -1);

    obj->markChanged();
    obj->markSceneChanged();

    objsAttached++;
    if (objsAttached > 1)
      obj->getScene().errorMessage(ObjScene::MSG_ERROR, "only one Movie object allowed, attached upto now: %d", objsAttached);
    return RETCODE_OBJ_CHANGED;
  }

  virtual void onDetach(Object *obj)
  {
    closeMovie();
    obj->popProps();
    objsAttached--;
  }

  virtual void afterRecalc(Object *obj, int) { checkProperty(obj, &MovieBhv::openMovieWebcache); }

  virtual bool render(Object *obj, int, Tab<dagui::Object *> &)
  {
    if (!player)
      return true;

    TEXTUREID texIdY, texIdU, texIdV;
    d3d::SamplerHandle smp;
    if (player->getFrame(texIdY, texIdU, texIdV, smp))
    {
      yuvframerender::l = obj->re->pos.x + obj->re->padding.left;
      yuvframerender::t = obj->re->pos.y + obj->re->padding.top;
      yuvframerender::r = obj->re->pos.x + obj->re->padding.right + obj->re->size.x;
      yuvframerender::b = obj->re->pos.y + obj->re->padding.bottom + obj->re->size.y;

      yuvframerender::startRender();
      yuvframerender::render(texIdY, texIdU, texIdV, smp);
      StdGuiRender::flush_data();
      yuvframerender::endRender();
      player->onCurrentFrameDispatched();
    }
    else if (player->isFinished())
    {
      if (obj->stProps.getProp(stateId, -1) != 0)
        setState(obj, "stopped", 0);
    }
    else
      logwarn("getFrame()=false, ids=%d,%d,%d", texIdY, texIdU, texIdV);


    return true;
  }

  virtual bool onCmd(Object *, int, int, void *) { return false; }
  virtual int receiveEvent(Object *, int, Object *, void *, InitializerType) { return RETCODE_NOTHING; }

  virtual void timerEvent(Object *obj, float)
  {
    if (!player)
      return;

    if (player->isFinished())
    {
      int nextLoadId = getNextLoadId(obj);
      if (nextLoadId != curLoadId && (nextLoadId != loadIds[0] || (obj->finalProps.cmpiProp(loopId, "yes"))))
      {
        curLoadId = nextLoadId;
        checkProperty(obj, &MovieBhv::openMovieWebcache);
      }
      return;
    }

    player->advance(timeQuantUsec);
  }

  // stub unused handlers
  virtual void clearHotKeys(Object *) {}
  virtual void regHotKeys(Object *) {}
  virtual bool onInsert(Object *, Object *, int) { return false; }
  virtual bool onRemove(Object *, int) { return false; }
  virtual int onFocus(Object *, Object *, int) { return RETCODE_NOTHING; }

  bool openMovieWebcache(Object *obj, const char *fname)
  {
    datacache::Backend *webcache = dagui::get_webcache_internal();
    const char *realFname = fname;
    if (webcache)
    {
      datacache::ErrorCode err = datacache::ERR_OK;
      ProxyObject *proxy = obj->proxy();
      datacache::EntryHolder entry(webcache->get(fname, &err, on_movie_loaded, proxy));
      if (err != datacache::ERR_PENDING)
        ProxyObject::onRelease(proxy, 0);
      if (entry)
        realFname = entry->getPath();
      else if (err == datacache::ERR_PENDING)
        return true;
    }
    return openMovie(obj, realFname);
  }

  bool openMovie(Object *obj, const char *fname)
  {
    timeQuantUsec = obj->finalProps.getPropInt(timeQuantUsecId, 8000);
    int buf_num = obj->finalProps.getPropInt(bufFramesId, 4);
    const char *fext = dd_get_fname_ext(fname);

    String fname_with_ext;
    if (!fext)
    {
      fname_with_ext.setStrCat(fname, ".ivf");
      if (dd_file_exist(fname_with_ext))
        fname = fname_with_ext, fext = ".ivf";
      else
      {
        fname_with_ext.setStrCat(fname, ".ogv");
        if (dd_file_exist(fname_with_ext))
          fname = fname_with_ext, fext = ".ogv";
      }
    }

    if (!player)
    {
      if (fext && dd_stricmp(fext, ".ivf") == 0)
      {
        player = IGenVideoPlayer::create_av1_video_player(fname, buf_num);
      }
      else
      {
        player = IGenVideoPlayer::create_ogg_video_player(fname, buf_num);
      }
    }
    else
    {
      yuvframerender::term();
      player = player->reopenFile(fname);
    }

    if (!player || !yuvframerender::init(player->getFrameSize(), false))
    {
      obj->getScene().errorMessage(ObjScene::MSG_ERROR, player ? "YUV shader not found!" : "cannot create movie player");
      destroy_it(player);
      return false;
    }

    return true;
  }

  void closeMovie()
  {
    if (!player)
      return;

    yuvframerender::term();
    destroy_it(player);
    loadedVideo.clear();
  }

  void checkProperty(Object *obj, bool (MovieBhv::*openMovieFunc)(Object *, const char *))
  {
    const char *videoFileOrUrl = obj->finalProps.getProp(curLoadId);
    if (videoFileOrUrl && !obj->finalProps.cmpProp(curLoadId, loadedVideo))
      loadedVideo = applyMovie(obj, videoFileOrUrl, openMovieFunc) ? videoFileOrUrl : "";
    doCmd(obj);
  }

  bool applyMovie(Object *obj, const char *video_fn, bool (MovieBhv::*openMovieFunc)(Object *, const char *))
  {
    if ((this->*openMovieFunc)(obj, video_fn))
    {
      setState(obj, "stopped", 0);
      if (obj->finalProps.cmpiProp(autoStartId, "yes"))
        obj->setDynProps(cmdId, "start");
      if (player)
      {
        IPoint2 sz = player->getFrameSize();
        if (obj->stProps.getProp(PROPID_WIDTH, 0) != sz.x || obj->stProps.getProp(PROPID_HEIGHT, 0) != sz.y)
        {
          obj->stProps.setProp(PROPID_WIDTH, sz.x);
          obj->stProps.setProp(PROPID_HEIGHT, sz.y);
          obj->markChanged();
          obj->markSceneChanged();
        }
      }
      return true;
    }
    else
    {
      setState(obj, NULL, -1);
      return false;
    }
  }

  int getNextLoadId(dagui::Object *obj)
  {
    for (int i = 0; i < MAX_LOAD_IDS_COUNT; ++i)
    {
      if (curLoadId != loadIds[i])
        continue;

      if ((i + 1) >= MAX_LOAD_IDS_COUNT)
        break;

      if (!obj->finalProps.getProp(loadIds[i + 1]))
        break;

      return loadIds[i + 1];
    }

    return loadIds[0];
  }

  void doCmd(dagui::Object *obj)
  {
    const char *cmd = obj->finalProps.getProp(cmdId);
    if (!cmd)
      return;
    if (!player)
    {
      if (loadedVideo.empty())
        obj->getScene().errorMessage(ObjScene::MSG_ERROR, "cannot start: movie not loaded");
    }
    else if (strcmp(cmd, "start") == 0)
    {
      player->stop(true);
      player->rewind();
      player->setAutoRewind(getNextLoadId(obj) == curLoadId && obj->finalProps.cmpiProp(loopId, "yes"));
      player->start(obj->finalProps.getPropInt(prebufMsecId, 10));
      setState(obj, "playing", 1);
    }
    else if (strcmp(cmd, "stop") == 0)
    {
      player->stop(true);
      player->rewind();
      setState(obj, "stopped", 0);
    }
    else if (strcmp(cmd, "pause") == 0)
    {
      player->stop(false);
      setState(obj, "paused", 2);
    }
    else if (strcmp(cmd, "resume") == 0)
    {
      player->setAutoRewind(getNextLoadId(obj) == curLoadId && obj->finalProps.cmpiProp(loopId, "yes"));
      player->start(0);
      setState(obj, "playing", 1);
    }
    else
      obj->getScene().errorMessage(ObjScene::MSG_ERROR, "unknown movie cmd=<%s>", cmd);

    obj->props.delProp(cmdId);
    obj->finalProps.delProp(cmdId);
  }

  inline void setState(Object *obj, const char *s_name, int s_id)
  {
    if (s_name)
      obj->setDynProps(stateId, s_name);
    else
    {
      obj->props.delProp(stateId);
      obj->finalProps.delProp(stateId);
    }
    obj->stProps.setProp(stateId, s_id);
    obj->getScene().errorMessage(ObjScene::MSG_NOTE, "movie: set state <%s>, %d", s_name, s_id);
  }

  int curLoadId, autoStartId, cmdId, stateId, loopId, timeQuantUsecId;
  int bufFramesId, prebufMsecId;
  SimpleString loadedVideo;
  carray<int, MAX_LOAD_IDS_COUNT> loadIds;

  // single instance of player, since only one movie object allowed here
  int objsAttached;
  IGenVideoPlayer *player;
  int timeQuantUsec;
};

static dagui::MovieBhv bhv;

static void on_movie_loaded(const char *key, datacache::ErrorCode, datacache::Entry *entry, void *arg)
{
  dagui::ProxyObject *proxy = (dagui::ProxyObject *)arg;
  dagui::Object *obj = proxy->ptr;
  if (obj && bhv.loadedVideo == key)
  {
    if (entry)
    {
      if (!bhv.applyMovie(obj, entry->getPath(), &dagui::MovieBhv::openMovie))
        bhv.loadedVideo.clear();
      bhv.doCmd(obj);
    }
    else if (!bhv.player)
    {
      bhv.loadedVideo.clear();
      bhv.doCmd(obj);
    }
  }
  dagui::ProxyObject::onRelease(proxy, 0);
  if (entry)
    entry->free();
}

void register_dagui_movie_behaviour() { dagui::register_gui_behaviour(&bhv); }
void unregister_dagui_movie_behaviour() { dagui::unregister_gui_behaviour(&bhv); }
