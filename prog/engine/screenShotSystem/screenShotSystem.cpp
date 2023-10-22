#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSceneRenderer.h>
#include <image/dag_tga.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <screenShotSystem/dag_screenShotSystem.h>
#include <math/dag_TMatrix4.h>
#include <supp/_platform.h>
#include <osApiWrappers/dag_clipboard.h>

static bool save_tga32_shot(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  return ::save_tga32(fn, im, wd, ht, stride);
}

String ScreenShotSystem::last_error(tmpmem_ptr());
String ScreenShotSystem::file_ext("tga");
ScreenShotSystem::write_image_cb ScreenShotSystem::writeImageCb = save_tga32_shot;
String ScreenShotSystem::prefix("shot");

char *ScreenShotSystem::lockScreen(int &w, int &h, int &stride, int &format, bool &fast)
{
  char *p;
  fast = false;
  format = -1;
  p = (char *)d3d::fast_capture_screen(w, h, stride, format);
  if (p)
  {
    if (format != d3d::CAPFMT_X8R8G8B8)
      d3d::end_fast_capture_screen();
    else
      fast = true;
  }
  if (!fast)
  {
    ::check_and_restore_3d_device();
    p = (char *)d3d::capture_screen(w, h, stride);
  }
  return p;
}

void ScreenShotSystem::unlockScreen(bool fast)
{
  if (fast)
  {
    d3d::end_fast_capture_screen();
  }
  else
    d3d::release_capture_buffer();
}

bool ScreenShotSystem::setFrustrum(float left, float right, float bottom, float top, float znear, float zfar)
{
  float width = right - left;
  float height = top - bottom;
  float depth = zfar - znear;
  float M001 = 2.0f * znear / width;
  float M002 = -2.0f * znear / height;
  float M003 = (right + left) / width;
  float M004 = (top + bottom) / height;
  float M005 = (zfar + znear) / depth;
  float M006 = 1.0f;
  float M007 = -(1.0f * zfar * znear) / depth;
  Matrix44 proj;
  proj = Matrix44(M001, 0, M003, 0, 0, M002, M004, 0, 0, 0, M005, M007, 0, 0, M006, 0);
  proj = proj.transpose();
  //--Apply the matrix to the projection matrix
  if (!d3d::settm(TM_PROJ, &proj))
    return false;
  return true;
}

bool ScreenShotSystem::makeScreenShot(ScreenShot &info, IHDRDecoder *decoder)
{
  d3d::set_render_target();
  int format;
  if (decoder && decoder->IsActive())
  {
    decoder->Decode();
    info.picture = (void *)decoder->LockResult(info.stride, info.w, info.h);
  }
  else
    info.picture = (void *)lockScreen(info.w, info.h, info.stride, format, info.fastLocked);
  if (!info.picture)
  {
    message(String(128, "Can't make ScreenShot: %s", d3d::get_last_error()));
    return false;
  }
  return true;
}


bool ScreenShotSystem::saveScreenShotTo(ScreenShot &info, char *fileName)
{
  if (!info.picture)
    return false;
  makeWaterMark(info);

  if (writeImageCb(fileName, (TexPixel32 *)info.picture, info.w, info.h, info.stride))
  {
    message(String(128, "%sScreen shot \"%s\"saved", info.fastLocked ? " fast " : "", fileName));
    return true;
  }
  else
  {
    message(String(128, "screen shot \"%s\" wasn't saved (IO error)", fileName));
    return false;
  }
}

bool ScreenShotSystem::saveScreenShotToClipboard(ScreenShot &info)
{
  if (!info.picture)
    return false;

  makeWaterMark(info);
  return clipboard::set_clipboard_bmp_image((TexPixel32 *)info.picture, info.w, info.h, info.stride);
}


bool ScreenShotSystem::makeTexScreenShot(ScreenShot &info, Texture *tex)
{
  info = ScreenShot();
  if (!tex)
    return false;
  TextureInfo ti;
  int stride;
  char *ptr;
  tex->getinfo(ti, 0);
  if (!(ti.cflg & (TEXCF_RTARGET | TEXCF_SYSMEM | TEXCF_UNORDERED | TEXCF_READABLE)))
    logwarn("can't make screenshot of unlockable texture");
  else if (ti.cflg & TEXFMT_MASK)
  {
    logwarn("can't make screenshot of format 0x%x", ti.cflg & TEXFMT_MASK);
  }
  else if (!tex->lockimg((void **)&ptr, stride, 0, TEXLOCK_READ))
  {
    logwarn("can't lock texture for screenshot");
  }
  else
  {
    info.w = ti.w;
    info.h = ti.h;
    info.stride = info.w * 4;
    info.picture = memalloc(info.stride * info.h, tmpmem_ptr());
    if (!info.picture)
    {
      message(String(128, "Not enough memory: %d needed", info.stride * info.h));
      return false;
    }
    info.allocated = true;

    char *op = (char *)info.picture;
    for (int y = info.h; y > 0; y--, ptr = (char *)ptr + stride, op += info.stride)
      memcpy(op, ptr, info.stride);
    tex->unlockimg();
    return true;
  }
  return false;
}

bool ScreenShotSystem::makeDepthTexScreenShot(ScreenShot &info, Texture *tex)
{
  info = ScreenShot();
  if (!tex)
    return false;
  TextureInfo ti;
  int stride;
  char *ptr;
  tex->getinfo(ti, 0);
  if (!(ti.cflg & (TEXCF_RTARGET | TEXCF_SYSMEM | TEXCF_UNORDERED | TEXCF_READABLE)))
    logwarn("can't make screenshot of unlockable texture");
  else if (!tex->lockimg((void **)&ptr, stride, 0, TEXLOCK_READ))
  {
    logwarn("can't lock texture for screenshot");
  }
  else
  {
    info.w = ti.w;
    info.h = ti.h;
    info.stride = info.w * 4;
    info.picture = memalloc(info.stride * info.h, tmpmem_ptr());
    if (!info.picture)
    {
      message(String(128, "Not enough memory: %d needed", info.stride * info.h));
      tex->unlockimg();
      return false;
    }
    info.allocated = true;

    unsigned char *op = reinterpret_cast<unsigned char *>(info.picture);
    float minDepth = 1;
    float maxDepth = 0;
    for (int y = 0; y < info.h; ++y, ptr = reinterpret_cast<char *>(ptr) + stride)
    {
      memcpy(op, ptr, info.stride);
      for (int x = 0; x < info.stride; x += 4, op += 4)
      {
        float depth;
        if ((ti.cflg & TEXFMT_MASK) == TEXFMT_DEPTH24)
        {
          const unsigned DEPTH_MASK = 0xFFFFFF00;
          const unsigned depth24bit = *reinterpret_cast<unsigned *>(op) & DEPTH_MASK;
          depth = static_cast<float>(depth24bit) / DEPTH_MASK;
        }
        else if ((ti.cflg & TEXFMT_MASK) == TEXFMT_DEPTH32)
        {
          depth = *reinterpret_cast<float *>(op);
        }
        else
        {
          message(String(128, "%d is unsupported depth format for screen-shots ", ti.cflg & TEXFMT_MASK));
          tex->unlockimg();
          return false;
        }
        minDepth = min(minDepth, depth);
        maxDepth = max(maxDepth, depth);
        *reinterpret_cast<float *>(op) = depth;
      }
    }
    op = reinterpret_cast<unsigned char *>(info.picture);
    float rcpDepth = safediv(0xFF, maxDepth - minDepth);
    for (int y = 0; y < info.h; ++y, op += info.stride)
    {
      for (int x = 0; x < info.stride; x += 4)
      {
        float depth = (reinterpret_cast<float *>(op)[x / 4] - minDepth) * rcpDepth;
        op[x] = op[x + 1] = op[x + 2] = static_cast<unsigned char>(depth);
        op[x + 3] = 0xFF;
      }
    }
    tex->unlockimg();
    return true;
  }
  return false;
}

bool ScreenShotSystem::makeHugeScreenShot(float fov, float aspect, float znear, float zfar, int quadrants, ScreenShot &info)
{
  info = ScreenShot();

  d3d::set_render_target();
  float top = znear / fov;
  float bottom = -top;
  float right = top * aspect;
  float left = -right;

  if (quadrants <= 1)
    quadrants = 1;
  int w, h, str, format;
  if (!d3d::get_render_target_size(w, h, nullptr))
  {
    message(String(128, "can't get target size: %s", d3d::get_last_error()));
    return false;
  }
  info.w = w * quadrants;
  info.h = h * quadrants;
  info.stride = info.w * 4;
  info.picture = memalloc(info.stride * info.h, tmpmem_ptr());
  if (!info.picture)
  {
    message(String(128, "Not enough memory: %d needed", info.stride * info.h));
    return false;
  }
  info.allocated = true;

  double width = right - left;
  double height = bottom - top;
  for (int ySub = 0; ySub < quadrants; ++ySub)
  {
    for (int xSub = 0; xSub < quadrants; ++xSub)
    {
      // d3d::set_render_target();
      setFrustrum(left + (double)(xSub)*width / quadrants, left + (double)(xSub + 1) * width / quadrants,
        top + (double)(ySub)*height / quadrants, top + (double)(ySub + 1) * height / quadrants, znear, zfar);
      if (dagor_get_current_game_scene())
        dagor_get_current_game_scene_renderer()->render(*dagor_get_current_game_scene());
      d3d::update_screen();
      d3d::update_screen();
      char *p = lockScreen(w, h, str, format, info.fastLocked);
      if (!p)
      {
        message(String(128, "Can't lock %s", d3d::get_last_error()));
        unlockScreen(info.fastLocked);
        return false;
      }
      char *to = ((char *)info.picture) + ((quadrants - 1 - xSub) * w * 4 + (info.stride * h * ySub));
      char *from = ((char *)p);
      for (int y = 0; y < h; ++y, to += info.stride, from += str)
        memcpy(to, from, w * 4);
    }
  }
  unlockScreen(info.fastLocked);
  return true;
}

bool ScreenShotSystem::makeHugeScreenShot(const Driver3dPerspective &p, int multiply, ScreenShot &info)
{
  int w, h;
  d3d::get_render_target_size(w, h, nullptr);
  return makeHugeScreenShot(p.wk / float(h) * float(w), float(w) / float(h), p.zn, p.zf, multiply, info);
}

void ScreenShotSystem::makeWaterMark(ScreenShot &info)
{
  if (info.hasWaterMark)
    return;
  // todo: waterMarks
  info.hasWaterMark = true;
}

bool ScreenShotSystem::saveScreenShot(ScreenShot &info)
{
  DagorDateTime time;
  ::get_local_time(&time);

  String fname(256, "%s %04d.%02d.%02d %02d.%02d.%02d", prefix.str(), time.year, time.month, time.day, time.hour, time.minute,
    time.second);
  String fn(256, "%s.%s", fname.str(), file_ext.str());
  if (::dd_file_exist(fn))
  {
    int cur_scrshot = 1;
    for (; ::dd_file_exist(String(256, "%s.%d.%s", (const char *)fname, cur_scrshot, (char *)file_ext)); ++cur_scrshot)
      ;
    fn = String(256, "%s.%d.%s", (const char *)fname, cur_scrshot, (char *)file_ext);
  }

  return saveScreenShotTo(info, fn);
}

const char *ScreenShotSystem::lastMessage() { return last_error; }

void ScreenShotSystem::setWriteImageCB(write_image_cb cb, const char *ext)
{
  if (!cb || !file_ext)
  {
    writeImageCb = save_tga32_shot;
    file_ext = "tga";
  }
  else
  {
    writeImageCb = cb;
    file_ext = ext;
  }
}

void ScreenShotSystem::message(const String &s) { last_error = s; }

void ScreenShotSystem::ScreenShot::clear()
{
  if (allocated && picture)
    memfree(picture, tmpmem);
  else if (picture)
    ScreenShotSystem::unlockScreen(fastLocked);
  picture = NULL;
  allocated = false;
}

ScreenShotSystem::ScreenShot::ScreenShot()
{
  picture = NULL;
  allocated = fastLocked = hasWaterMark = false;
  w = h = stride = 0;
}
