//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>

class DataBlock;
class String;

// Picture id type
typedef int PICTUREID;

// Invalid picture id
#define BAD_PICTUREID -1


namespace PictureManager
{
struct PicDesc;

typedef void (*async_load_done_cb_t)(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb,
  const Point2 *picture_sz, void *arg);

// Picture can be obtained by real texture name or by picture name in BLK file:
//   "tex/mytex.tga"        - real texture name
//   "tex/mytextures#mytex" - picture "mytex" in tex/mytextures.blk

// startup/release manager
void init(const DataBlock *params = NULL);
void prepare_to_release(bool final_term = true);
void release();

// perform actions per-frame (to be called at the beginning of each frame render to perform some pending actions)
void per_frame_update();

// perform actions before/after d3d reset (e.g., invalidate dynamic atlases)
void before_d3d_reset();
void after_d3d_reset();

// add ref, return picture ID, texture ID and left-top, right-bottom tex coords and picture size
// if load_done_cb is not NULL - request considered asynchronous (i.e callback will be called on
// load completion from cpujobs::release_done_jobs())
// return true if request was synchronously done (or picture already loaded)
// and false otherwise (i.e. it was successfully sheduled)
bool get_picture_ex(const char *file_name, PICTUREID &out_pic_id, TEXTUREID &out_tex_id, Point2 *out_tc_lefttop = NULL,
  Point2 *out_tc_rightbottom = NULL, Point2 *picture_size = NULL, async_load_done_cb_t load_done_cb = NULL, void *cb_arg = NULL);

// add ref, fill picture description
void get_picture(const char *file_name, PicDesc &out_pic);

// add picture manually (if get_it == true, than function setup refcount as if you called get_picture*() after)
PICTUREID add_picture(BaseTexture *tex, const char *as_name, bool get_it = false);

// return TEXTUREID and pointers to lt/rb tex coord data for specified PICTUREID
// returns BAD_TEXTUREID on error
TEXTUREID get_picture_data(PICTUREID pid, Point2 &out_lt, Point2 &out_rb);

// return picture size in pixels or (0,0) on error
Point2 get_picture_pix_size(PICTUREID pid);

// return total texture size in pixels & picture size in 0..1 (in texture coordinates).
// return false, if texture not exists
bool get_picture_size(PICTUREID pid, Point2 &out_total_texsz, Point2 &out_picsz);

// add ref.
void add_ref_picture(PICTUREID pid);

// del ref. if refcount is zero, unload texture
void free_picture(PICTUREID pic_id);

// remove all unused pictures from memory
bool discard_unused_picture(bool force_lock = true); // return false if discard was skipped

// loads texture atlas internal data
bool load_tex_atlas_data(const char *ta_fn, DataBlock &out_ta_blk, String &out_ta_tex_fn);

// returns true when atlas referred by 'file_name' picture is dynamic; optionally returns alpha format
bool is_picture_in_dynamic_atlas(const char *file_name, bool *out_premul_alpha = NULL);


struct PictureRenderFactory
{
  PictureRenderFactory *next = NULL; // slist
  static PictureRenderFactory *tail;
  PictureRenderFactory()
  {
    next = tail;
    tail = this;
  }

  virtual void registered() = 0;
  virtual void unregistered() = 0;
  virtual void clearPendReq() = 0;
  virtual bool match(const DataBlock &pic_props, int &out_w, int &out_h) = 0;
  virtual bool render(Texture *to, int x, int y, int w, int h, const DataBlock &pic_props, PICTUREID pid) = 0;
  virtual void updatePerFrame() {}
};
void register_pic_render_factory(PictureRenderFactory *f);
void unregister_pic_render_factory(PictureRenderFactory *f);
} // namespace PictureManager

struct PictureManager::PicDesc
{
  // Picture and texture IDs
  PICTUREID pic;
  TEXTUREID tex;
  // left-top and right-bottom texture coords
  Point2 tcLt, tcRb;

  PicDesc() : pic(BAD_PICTUREID), tex(BAD_TEXTUREID), tcLt(0, 0), tcRb(0, 0) {}
  PicDesc(const PicDesc &picture) : pic(picture.pic), tex(picture.tex), tcLt(picture.tcLt), tcRb(picture.tcRb)
  {
    if (pic > BAD_PICTUREID)
      PictureManager::add_ref_picture(pic);
  }

  ~PicDesc() { release(); }

  void operator=(const PicDesc &picture)
  {
    release();
    pic = picture.pic;
    tex = picture.tex;
    tcLt = picture.tcLt;
    tcRb = picture.tcRb;
    if (pic > BAD_PICTUREID)
      PictureManager::add_ref_picture(pic);
  }

  void init(const char *file_name)
  {
    PICTUREID old_pic = pic;
    PictureManager::get_picture(file_name, *this);
    if (old_pic > BAD_PICTUREID)
      PictureManager::free_picture(old_pic);
  }
  void release()
  {
    if (pic > BAD_PICTUREID)
      PictureManager::free_picture(pic);
    pic = BAD_PICTUREID;
    tex = BAD_TEXTUREID;
  }

  inline Point2 getSize() const { return get_picture_pix_size(pic); }

  void updateTc() const // actualize TC for picture in dynamic atlas
  {
    if ((pic & 0xC0000000) == 0x40000000)
      get_picture_data(pic, const_cast<Point2 &>(tcLt), const_cast<Point2 &>(tcRb));
  }
};
