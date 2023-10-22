#include <3d/dag_splashScreen.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <math/dag_Point2.h>


void show_splash_screen(Texture *tex, bool independent_render)
{
  if (!check_and_restore_3d_device())
    return;

  d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 0, 0);

  d3d::stretch_rect(tex, NULL, NULL, NULL);

  if (independent_render)
  {
    d3d::update_screen();
  }
}

void release_splash_screen_resources() {}
