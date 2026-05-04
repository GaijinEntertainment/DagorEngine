// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_consts.h>


void d3d_get_render_target(Driver3dRenderTarget &rt) { d3d::get_render_target(rt); }
void d3d_set_render_target(Driver3dRenderTarget &rt) { d3d::set_render_target(rt); }

void d3d_get_view_proj(ViewProjMatrixContainer &vp)
{
  d3d::gettm(TM_VIEW, vp.savedView);
  vp.p_ok = d3d::getpersp(vp.p);
  // Get proj tm also even though persp is ok. Overally there is no good having
  // uninitialized proj whereas everyone can use it
  d3d::gettm(TM_PROJ, &vp.savedProj);
}

void d3d_set_view_proj(const ViewProjMatrixContainer &vp)
{
  d3d::settm(TM_VIEW, vp.savedView);
  if (!vp.p_ok)
    d3d::settm(TM_PROJ, &vp.savedProj);
  else
    d3d::setpersp(vp.p);
}

void d3d_get_view(int &viewX, int &viewY, int &viewW, int &viewH, float &viewN, float &viewF)
{
  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);
}
void d3d_set_view(int viewX, int viewY, int viewW, int viewH, float viewN, float viewF)
{
  d3d::setview(viewX, viewY, viewW, viewH, viewN, viewF);
  d3d::setscissor(max(0, viewX), max(0, viewY), viewW, viewH);
}
