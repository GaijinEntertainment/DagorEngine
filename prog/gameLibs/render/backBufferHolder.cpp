// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/backBufferHolder.h>
#include <render/hdrRender.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <ioSys/dag_dataBlock.h>

eastl::unique_ptr<BackBufferHolder> BackBufferHolder::holder;

void BackBufferHolder::init()
{
  Texture *backBuffer = d3d::get_backbuffer_tex();
  if (backBuffer)
  {
    readable = true;
  }
  else
  {
    readable = false;
    int backBufferWidth = 0;
    int backBufferHeight = 0;
    d3d::get_screen_size(backBufferWidth, backBufferHeight);
    backBuffer = d3d::create_tex(NULL, backBufferWidth, backBufferHeight, TEXCF_RTARGET, 1, "srgb_frame");
    TEXTUREID texId = register_managed_tex("srgb_frame", backBuffer);
    srgbFrame = TextureIDPair(backBuffer, texId);
    srgbFrame.getTex2D()->texaddr(TEXADDR_CLAMP);
  }
}

void BackBufferHolder::close() { srgbFrame.releaseAndEvictTexId(); }

BackBufferHolder::BackBufferHolder() { init(); }

void BackBufferHolder::create()
{
  bool readable =
    ::dgs_get_settings() && ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("backbufferHolderReadable", true);
  if (!readable)
  {
    holder.reset();
    return;
  }
  if (holder)
    return;
  holder.reset(new BackBufferHolder());
}

void BackBufferHolder::update()
{
  if (!holder)
    return;
  holder->close();
  holder->init();
}

void BackBufferHolder::destroy() { holder.reset(); }

const TextureIDPair BackBufferHolder::getTex()
{
  if (!holder)
    return TextureIDPair();

  if (hdrrender::is_hdr_enabled())
    return TextureIDPair(hdrrender::get_render_target_tex(), hdrrender::get_render_target_tex_id());

  if (!holder->readable)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target();
    d3d::copy_from_current_render_target(holder->srgbFrame.getTex2D());
  }
  else
  {
    G_ASSERT(holder->srgbFrame.getId() == BAD_TEXTUREID && !holder->srgbFrame.getTex2D());
    Texture *backBuffer = d3d::get_backbuffer_tex();
    G_ASSERT(backBuffer);
    backBuffer->texaddr(TEXADDR_CLAMP);
    TEXTUREID texId = register_managed_tex("\x7f\x7f\x7f_tmp_backbuffer", backBuffer);
    holder->srgbFrame = TextureIDPair(backBuffer, texId);
  }

  return holder->srgbFrame;
}

void BackBufferHolder::releaseTex()
{
  if (holder && !hdrrender::is_hdr_enabled() && holder->readable)
    holder->srgbFrame.releaseAndEvictTexId();
}

BackBufferHolder::~BackBufferHolder() { close(); }

void BackBufferHolder::d3dReset(bool)
{
  if (holder && holder->readable)
  {
    holder->close(); // get_backbuffer_tex has changed.
    holder->init();
  }
}

REGISTER_D3D_AFTER_RESET_FUNC(BackBufferHolder::d3dReset);
