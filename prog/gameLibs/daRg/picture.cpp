// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_picture.h>
#include "guiScene.h"
#include "picAsyncLoad.h"

// To consider: move this to some public engine header
extern void lottie_discard_animation(PICTUREID pid);

namespace darg
{

Picture::Picture(HSQUIRRELVM vm)
{
  guiScene = GuiScene::get_from_sqvm(vm);
  G_ASSERT(guiScene);
}


Picture::Picture(HSQUIRRELVM vm, const char *name)
{
  guiScene = GuiScene::get_from_sqvm(vm);
  G_ASSERT(guiScene);

  init(name);
}


PictureImmediate::PictureImmediate(HSQUIRRELVM vm, const char *name) : Picture(vm)
{
  lazy = false;
  init(name);
}


Picture::~Picture()
{
  if (loadReq)
    PicAsyncLoad::cancel_waiting(loadReq);
  guiScene->needToDiscardPictures = true;
}


IGuiScene *Picture::getScene() const { return guiScene; }


const PictureManager::PicDesc &Picture::getPic()
{
  if (!nameToLoadOnDemand.empty())
  {
    load(nameToLoadOnDemand);
    clear_and_shrink(nameToLoadOnDemand);
  }

  return pic;
}


void Picture::init(const char *name)
{
  blendMode = PREMULTIPLIED;
  const char *s = name;

  if (*s)
  {
    if (*s == '!')
    {
      blendMode = NONPREMULTIPLIED;
      ++s;
    }
    else if (*s == '+')
    {
      blendMode = ADDITIVE;
      ++s;
    }

    srcName = name;

    if (lazy)
      nameToLoadOnDemand = s;
    else
      load(s);
  }
}


bool Picture::load(const char *name)
{
  if (loadReq)
  {
    PicAsyncLoad::cancel_waiting(loadReq);
    loadReq = nullptr;
  }

  PICTUREID prevPicId = pic.pic;
  TEXTUREID prevTexId = pic.tex;
  PICTUREID picId = BAD_PICTUREID;
  TEXTUREID texId = BAD_TEXTUREID;
  d3d::SamplerHandle smpId = d3d::INVALID_SAMPLER_HANDLE;

  AsyncLoadRequest *req = PicAsyncLoad::make_request(this);

  bool sync = PictureManager::get_picture_ex(name, picId, texId, smpId, &req->tcLt, &req->tcRb, /*picSize*/ nullptr,
    PicAsyncLoad::pic_mgr_async_load_cb, req);

  pic.pic = picId;
  pic.tex = texId;
  pic.smp = smpId;
  darg::free_texture(prevTexId, prevPicId);

  if (sync)
  {
    pic.tcLt = req->tcLt;
    pic.tcRb = req->tcRb;
    delete req;
    return true;
  }
  else
  {
    PicAsyncLoad::insert_pending(req);
    loadReq = req;
    return false;
  }
}


void Picture::onAsyncLoadStopped(AsyncLoadRequest *req)
{
  G_ASSERT_RETURN(loadReq, );
  G_ASSERT_RETURN(loadReq == req, );
  loadReq = nullptr;
}


void Picture::onLoaded(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 &tcLt, const Point2 &tcRb,
  const Point2 &picture_sz)
{
  G_UNUSED(picture_sz);

  PICTUREID prevPicId = pic.pic;
  TEXTUREID prevTexId = pic.tex;

  if (pid != BAD_PICTUREID && tid != BAD_TEXTUREID)
  {
    G_ASSERTF(pid != pic.pic, "pid: %X | %X", pid, pic.pic);
    pic.pic = pid;
    pic.tex = tid;
    pic.smp = smp;
    pic.tcLt = tcLt;
    pic.tcRb = tcRb;
  }
  else
  {
    pic.pic = BAD_PICTUREID;
    pic.tex = BAD_TEXTUREID;
    pic.smp = d3d::INVALID_SAMPLER_HANDLE;
    pic.tcLt.zero();
    pic.tcRb.zero();
  }

  loadReq = nullptr;

  darg::free_texture(prevTexId, prevPicId);

  guiScene->onPictureLoaded(this);
  if (pid != BAD_PICTUREID && tid == BAD_TEXTUREID) // async load was aborted, schedule new attempt
  {
    lazy = true;
    init(srcName);
  }
}

} // namespace darg
