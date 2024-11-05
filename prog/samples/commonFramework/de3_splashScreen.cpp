// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_splashScreen.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <gui/dag_stdGuiRender.h>
#include <debug/dag_debug.h>

static TEXTUREID splash_tex = BAD_TEXTUREID;
static d3d::SamplerHandle splash_sampler = d3d::INVALID_SAMPLER_HANDLE;

void load_splash(const char *filename)
{
  splash_tex = add_managed_texture(filename);
  acquire_managed_tex(splash_tex);
  splash_sampler = get_texture_separate_sampler(splash_tex);
  if (splash_sampler == d3d::INVALID_SAMPLER_HANDLE)
    splash_sampler = d3d::request_sampler({});
  DEBUG_CTX("loaded splash texture");
}

void show_splash(bool own_render)
{
  check_and_restore_3d_device();

  if (own_render)
    StdGuiRender::start_render();
  Point2 lt = StdGuiRender::get_viewport().leftTop;
  Point2 rb = StdGuiRender::get_viewport().rightBottom;

  StdGuiRender::set_color(0xFFFFFFFF);
  StdGuiRender::set_texture(splash_tex, splash_sampler);
  StdGuiRender::render_rect(lt, rb);
  if (own_render)
  {
    StdGuiRender::end_render();

    d3d::update_screen();
  }
}

void unload_splash()
{
  if (splash_tex != BAD_TEXTUREID)
  {
    DEBUG_CTX("unloaded splash texture");
    release_managed_tex(splash_tex);
    splash_tex = BAD_TEXTUREID;
  }
}
