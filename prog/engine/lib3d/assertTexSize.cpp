#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <debug/dag_fatal.h>

void assert_tex_size(Texture *tex, int w, int h)
{
  d3d_err(tex);
  TextureInfo ti;
  d3d_err(tex->getinfo(ti));
  if (ti.w != w || ti.h != h)
    DAG_FATAL("can't create %dx%d render target (%dx%d created)", w, h, ti.w, ti.h);
}
