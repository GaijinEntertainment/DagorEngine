// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <libTools/renderViewports/renderViewport.h>


RenderViewport::RenderViewport()
{
  lt.x = lt.y = 0;
  rb.x = rb.y = 100;
  viewMatrix.identity();
  projectionMatrix.identity();
  wireframe = false;
}


void RenderViewport::setView(real minz, real maxz)
{
  int x1 = lt.x;
  int y1 = lt.y;
  int x2 = rb.x;
  int y2 = rb.y;

  d3d::setview(x1, y1, x2 - x1, y2 - y1, minz, maxz);
  d3d::setwire(wireframe);
  ::grs_draw_wire = wireframe;
}


void RenderViewport::setViewProjTms()
{
  ::grs_cur_view.tm = viewMatrix;
  ::grs_cur_view.itm = inverse(viewMatrix);
  ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

  d3d::settm(TM_VIEW, viewMatrix);
  if (projectionMatrix[0][1] == 0 && projectionMatrix[0][2] == 0 && projectionMatrix[0][3] == 0 && projectionMatrix[1][0] == 0 &&
      projectionMatrix[1][2] == 0 && projectionMatrix[1][3] == 0 && projectionMatrix[2][0] == 0 && projectionMatrix[2][1] == 0 &&
      projectionMatrix[2][3] == 1 && projectionMatrix[3][0] == 0 && projectionMatrix[3][1] == 0 && projectionMatrix[3][3] == 0)
  {
    d3d::setpersp(Driver3dPerspective(projectionMatrix[0][0], projectionMatrix[1][1], -projectionMatrix[3][2] / projectionMatrix[2][2],
      projectionMatrix[3][2] / (1 - projectionMatrix[2][2]), 0, 0));
  }
  else
    d3d::settm(TM_PROJ, &projectionMatrix);
}


void RenderViewport::resetView()
{
  int w, h;
  d3d::get_target_size(w, h);
  d3d::setview(0, 0, w, h, 0, 1);
  d3d::setwire(false);
}


void RenderViewport::setPersp(real wk, real hk, real zn, real zf)
{
  float q = zf / (zf - zn);
  projectionMatrix = Matrix44(wk, 0, 0, 0, 0, hk, 0, 0, 0, 0, q, 1, 0, 0, -q * zn, 0);
}


void RenderViewport::setPerspHFov(real fov, real zn, real zf)
{
  real wk = 1 / tanf(fov * 0.5f);
  real hk = wk * (rb.x - lt.x) / (rb.y - lt.y);
  setPersp(wk, hk, zn, zf);
}

void RenderViewport::setFullScreenViewRect()
{
  lt.x = lt.y = 0;

  Driver3dRenderTarget rt;
  d3d::get_render_target(rt);
  d3d::set_render_target();

  int x, y;
  d3d::get_target_size(x, y);

  rb.x = x;
  rb.y = y;

  d3d::set_render_target(rt);
}
