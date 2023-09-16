#pragma once

#include <debug/dag_log.h>
#include <math/dag_Point2.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaderVar.h>
#include <stdio.h>


namespace webbrowser
{

struct TextureBuffer
{
  Texture *data = nullptr;
  TEXTUREID id = BAD_TEXTUREID;
}; // struct TextureBuffer


class DoubleBuffer
{
public:
  bool swap()
  {
    if (!dirty)
      return false;

    this->curBufNo = getCurBackBufNo();
    this->dirty = false;
    return true;
  }

  bool update(const uint8_t *src, uint16_t w, uint16_t h)
  {
    if (!this->resize(w, h))
      return false;

    TextureBuffer &b = this->buffers[this->getCurBackBufNo()];
    uint8_t *dst = nullptr;
    int stride = 0;
    if (!b.data->lockimgEx(&dst, stride, 0, TEXLOCK_WRITE))
      return false;

    if (stride == this->width * 4)
      memcpy(dst, src, w * h * 4);
    else
      for (int i = 0; i < h; ++i)
        memcpy(dst + i * stride, src + i * w * 4, w * 4);

    b.data->unlockimg();
    this->dirty = true;
    this->isEmpty = false;
    return true;
  }


  bool resize(uint16_t w, uint16_t h)
  {
    if (this->width == w && this->height == h)
      return true;

    debug("[BRWS] resizing tex %hux%hu -> %hux%hu", this->width, this->height, w, h);

    if (this->width || this->height)
      destroyBuffers();
    return (w && h) ? createBuffers(w, h) : true;
  }


  void move(uint16_t x_, uint16_t y_)
  {
    this->x = x_;
    this->y = y_;
  }

  TEXTUREID getActiveTextureId() { return this->buffers[curBufNo].id; }
  bool empty() { return this->isEmpty; }
  Point2 size() { return Point2(this->width, this->height); }
  Point2 pos() { return Point2(this->x, this->y); }


private:
  uint8_t getCurBackBufNo() { return this->curBufNo ? 0 : 1; }


  bool createBuffers(uint16_t w, uint16_t h)
  {
    const uint32_t flags = TEXFMT_A8R8G8B8;
    for (int i = 0; i < countof(buffers); ++i)
    {
      const char *texName = getNextTexName();
      TextureBuffer &b = this->buffers[i];
      b.data = d3d::create_tex(nullptr, (int)w, (int)h, flags, 1, texName);
      if (!b.data)
        return false;

      TextureInfo ti;
      d3d_err(b.data->getinfo(ti));
      if (ti.w != w || ti.h != h)
      {
        destroyBuffers();
        return false;
      }

      b.id = register_managed_tex(texName, b.data);
      b.data->texaddr(TEXADDR_CLAMP);
    }

    this->width = w;
    this->height = h;
    return true;
  }


  void destroyBuffers()
  {
    for (int i = 0; i < countof(buffers); ++i)
    {
      TextureBuffer &b = this->buffers[i];
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.id, b.data);
      b = {};
    }

    this->width = this->height = 0;
    this->isEmpty = true;
  }


  const char *getNextTexName()
  {
    static char n[20];
    static int i = 0;
    _snprintf(n, sizeof(n), "b%04d_cef", i++);
    return n;
  }

private:
  TextureBuffer buffers[2];
  uint16_t width = 0, height = 0;
  uint16_t x = 0, y = 0;
  uint8_t curBufNo = 0;
  bool dirty = false;
  bool isEmpty = true;

}; // class DoubleBuffer

} // namespace webbrowser
