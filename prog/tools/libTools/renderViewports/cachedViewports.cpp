// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/renderViewports/cachedViewports.h>
#include <libTools/renderViewports/renderViewport.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>

#include <debug/dag_debug.h>
#include <util/dag_string.h>

CachedRenderViewports::CachedRenderViewports() : viewports(inimem_ptr()), texScale(1.0, 1.0)
{
  curRenderVp = -1;
  rtWidth = 0;
  rtHeight = 0;
  prevX = 0;
  prevY = 0;
  prevW = 0;
  prevH = 0;
  minz = 0;
  maxz = 1;
  lastRt = new (inimem) Driver3dRenderTarget;
}
CachedRenderViewports::~CachedRenderViewports() { del_it(lastRt); }

int CachedRenderViewports::addViewport(RenderViewport *vp, void *userdata, bool enabled)
{
  int i;
  for (i = 0; i < viewports.size(); i++)
    if (!viewports[i].vp)
      break;

  if (i >= viewports.size())
    i = append_items(viewports, 1);
  viewports[i].vp = vp;
  viewports[i].userData = userdata;
  viewports[i].needDraw = enabled;
  viewports[i].useCache = false;
  viewports[i].forceCache = false;

  return i;
}

void CachedRenderViewports::removeViewport(int vp_idx)
{
  if (vp_idx < 0 || vp_idx >= viewports.size())
    return;

  // mark as unused
  viewports[vp_idx].vp = NULL;
  viewports[vp_idx].userData = NULL;
  disableViewport(vp_idx);

  // erase tailing empry entries
  for (int i = viewports.size() - 1; i >= 0; i--)
    if (!viewports[i].vp)
      erase_items(viewports, i, 1);
    else
      break;
}

bool CachedRenderViewports::startRender(int vp_idx)
{
  // useCache = true; needDraw = true;

  if (!viewports[vp_idx].vp || !viewports[vp_idx].needDraw)
    return false;

  curRenderVp = vp_idx;

  // Matrices.
  viewports[vp_idx].vp->setViewProjTms();

  viewports[vp_idx].vp->setView();
  return true;
}
void CachedRenderViewports::endRender()
{
  if (curRenderVp == -1)
    return;

  curRenderVp = -1;
}
