// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fxRenderer.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_decl.h>


void FxRenderer::init()
{
  isInited = true;

  const char *shaderName = "debug_fx_final_render";
  finalRender.init(shaderName);
  if (!finalRender.getMat())
    DAEDITOR3.conError("Shader \"%s\" cannot be found. FX rendering won't work!", shaderName);
}

void FxRenderer::initResolution(int width, int height)
{
  screenRes = IPoint2(width, height);

  colorRt.close();
  colorRt = dag::create_tex(NULL, width, height, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, "fx_debug_color_rt");
  colorRt.getTex2D()->texaddr(TEXADDR_CLAMP);
}

void FxRenderer::render(IGenViewportWnd &wnd, eastl::function<void()> render_fx_cb)
{
  int viewportWidth, viewportHeight;
  wnd.getViewportSize(viewportWidth, viewportHeight);

  if (!isInited)
    init();

  if (screenRes.x != viewportWidth || screenRes.y != viewportHeight)
    initResolution(viewportWidth, viewportHeight);

  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target();
    d3d::set_render_target(colorRt.getTex2D(), 0);

    d3d::clearview(CLEAR_TARGET, E3DCOLOR_MAKE(0, 0, 0, 255), 0, 0);

    render_fx_cb();
  }
  finalRender.render();
}
