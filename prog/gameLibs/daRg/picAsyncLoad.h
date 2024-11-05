// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_picMgr.h>
#include <math/dag_math2d.h>


namespace darg
{

class Picture;
struct IGuiScene;


struct AsyncLoadRequest
{
  Picture *pic = nullptr; // null if cancelled
  IGuiScene *scene = nullptr;
  Point2 tcLt = Point2(0, 0);
  Point2 tcRb = Point2(0, 0);

  bool isActive() { return pic != nullptr; }
};


class PicAsyncLoad
{
public:
  static AsyncLoadRequest *make_request(Picture *pic);
  static void insert_pending(AsyncLoadRequest *req);
  static void cancel_waiting(AsyncLoadRequest *req);
  static void on_scene_shutdown(IGuiScene *gui_scene);

  static void pic_mgr_async_load_cb(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb,
    const Point2 *picture_sz, void *arg);
};


static inline void free_texture(TEXTUREID tex_id, PICTUREID pic_id)
{
  if (pic_id != BAD_PICTUREID)
    PictureManager::free_picture(pic_id);
  else if (tex_id != BAD_TEXTUREID)
  {
    release_managed_tex(tex_id);
    if (get_managed_texture_refcount(tex_id) == 0)
      evict_managed_tex_id(tex_id);
  }
}

} // namespace darg
