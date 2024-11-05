// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <daGI/daGI.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include "global_vars.h"
#include <scene/dag_tiledScene.h>
#include <GIWindows/GIWindows.h>
namespace dagi
{

void GI3D::initWindows(eastl::unique_ptr<class scene::TiledScene> &&s)
{
  if (!s && cWindows)
    cWindows.reset();
  if (!s)
    return;
  if (!cWindows)
    cWindows.reset(new GIWindows);
  cWindows->init(eastl::move(s));
}

void GI3D::updateWindowsPos(const Point3 &pos)
{
  if (cWindows)
    cWindows->updatePos(pos);
}

void GI3D::invalidateWindows()
{
  if (cWindows)
    cWindows->invalidate();
}

eastl::unique_ptr<scene::TiledScene> &GI3D::getWindowsScene() const
{
  static eastl::unique_ptr<scene::TiledScene> empty;
  return cWindows ? cWindows->windows : empty;
}

}; // namespace dagi
