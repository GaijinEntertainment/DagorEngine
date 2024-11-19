// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "picAsyncLoad.h"
#include <daRg/dag_picture.h>
#include <EASTL/vector_set.h>


namespace darg
{

static eastl::vector_set<AsyncLoadRequest *> requests;


AsyncLoadRequest *PicAsyncLoad::make_request(Picture *pic)
{
  auto req = new AsyncLoadRequest();
  req->pic = pic;
  req->scene = pic->getScene();
  return req;
}


void PicAsyncLoad::insert_pending(AsyncLoadRequest *req) { G_VERIFY(requests.insert(req).second); }


void PicAsyncLoad::cancel_waiting(AsyncLoadRequest *req)
{
  auto it = requests.find(req);
  G_ASSERT_RETURN(it != requests.end(), );
  G_ASSERT((*it)->pic);
  (*it)->pic = nullptr;
}


void PicAsyncLoad::on_scene_shutdown(IGuiScene *gui_scene)
{
  for (AsyncLoadRequest *req : requests)
  {
    if (req->scene == gui_scene && req->pic)
    {
      req->pic->onAsyncLoadStopped(req);
      req->pic = nullptr;
    }
  }
}


void PicAsyncLoad::pic_mgr_async_load_cb(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb,
  const Point2 *picture_sz, void *arg)
{
  AsyncLoadRequest *req = (AsyncLoadRequest *)arg;
  G_ASSERT(req->isActive() == (req->pic != nullptr));
  if (req->isActive())
    req->pic->onLoaded(pid, tid, smp, *tcLt, *tcRb, *picture_sz);
  else if (tid != BAD_TEXTUREID) // when load aborted we receive proper pid and bad tid
    darg::free_texture(tid, pid);
  requests.erase(req);
  delete req;
}


} // namespace darg
