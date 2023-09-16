#include "de3_splash.h"
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <gui/dag_stdGuiRender.h>
#include <debug/dag_debug.h>

static TEXTUREID splash_tex = BAD_TEXTUREID;

void load_splash(const char *filename)
{
  splash_tex = add_managed_texture(filename);
  acquire_managed_tex(splash_tex);
  debug_ctx("loaded splash texture");
}

void show_splash(bool own_render)
{
  check_and_restore_3d_device();

  if (own_render)
    StdGuiRender::start_render();
  Point2 lt = StdGuiRender::get_viewport().leftTop;
  Point2 rb = StdGuiRender::get_viewport().rightBottom;

  StdGuiRender::set_color(0xFFFFFFFF);
  StdGuiRender::set_texture(splash_tex);
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
    debug_ctx("unloaded splash texture");
    release_managed_tex(splash_tex);
    splash_tex = BAD_TEXTUREID;
  }
}
