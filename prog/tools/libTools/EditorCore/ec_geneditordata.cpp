// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_cachedRender.h"

#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_cm.h>

#include <sepGui/wndPublic.h>

#include <ioSys/dag_dataBlock.h>

#include <util/dag_globDef.h>
#include <util/dag_string.h>


GeneralEditorData::GeneralEditorData() : currentViewportId(-1), vpw(midmem), curEH(NULL)
{
  vcmode = IEditorCoreEngine::VCM_CacheAll;
  tbManager = new (uimem) ToolBarManager();
  G_ASSERT(tbManager);
  viewportLayout = CM_VIEWPORT_LAYOUT_4;
}


GeneralEditorData::~GeneralEditorData()
{
  clear_all_ptr_items(vpw);
  del_it(tbManager);
}


void GeneralEditorData::addViewport(void *parent, IGenEventHandler *eh, IWndManager *manager, ViewportWindow *v)
{
  v->init(eh);

  hdpi::Px menu_w, menu_h;
  v->getMenuAreaSize(menu_w, menu_h);
  manager->setMenuArea(parent, menu_w, menu_h);

  vpw.push_back(v);
  setViewportCacheMode(IEditorCoreEngine::VCM_NoCacheAll);
}


ViewportWindow *GeneralEditorData::createViewport(void *parent, int x, int y, int w, int h, IGenEventHandler *eh, IWndManager *manager)
{
  ViewportWindow *v = new ViewportWindow(parent, x, y, w, h);
  addViewport(parent, eh, manager, v);
  return v;
}


void GeneralEditorData::deleteViewport(int n)
{
  if (n >= getViewportCount() || n < 0)
    return;

  if (n == currentViewportId)
    currentViewportId = -1;

  erase_ptr_items(vpw, n, 1);
}


void GeneralEditorData::load(const DataBlock &blk)
{
  viewportLayout = blk.getInt("viewportLayoutId", CM_VIEWPORT_LAYOUT_1);
  for (int i = 0; i < getViewportCount(); i++)
    vpw[i]->load(*blk.getBlockByNameEx(String(32, "viewport%d", i)));

  load_camera_objects(blk);
}


void GeneralEditorData::save(DataBlock &blk) const
{
  blk.setInt("viewportLayoutId", viewportLayout);
  for (int i = 0; i < getViewportCount(); i++)
    vpw[i]->save(*blk.addBlock(String(32, "viewport%d", i)));

  save_camera_objects(blk);
}


ViewportWindow *GeneralEditorData::getActiveViewport() const
{
  for (int i = 0; i < getViewportCount(); ++i)
    if (vpw[i]->isActive())
      return vpw[i];
  return NULL;
}


ViewportWindow *GeneralEditorData::getRenderViewport() const
{
  if (!ec_cached_viewports)
    return NULL;
  int id = ec_cached_viewports->getCurRenderVp();
  if (id == -1)
    return NULL;
  return (ViewportWindow *)ec_cached_viewports->getViewportUserData(id);
}


void GeneralEditorData::setEH(IGenEventHandler *eh)
{
  for (int i = 0; i < getViewportCount(); ++i)
    vpw[i]->setEventHandler(eh);
}


void GeneralEditorData::setViewportCacheMode(IEditorCoreEngine::ViewportCacheMode mode)
{
  vcmode = mode;
  for (int i = 0; i < getViewportCount(); ++i)
  {
    if (!vpw[i]->isVisible())
      continue;

    if (vcmode == IEditorCoreEngine::VCM_NoCacheAll)
      vpw[i]->enableCache(false);
    else if (vcmode == IEditorCoreEngine::VCM_CacheAll || (vcmode == IEditorCoreEngine::VCM_CacheAllButActive && !vpw[i]->isActive()))
      vpw[i]->enableCache(true);
    else
      vpw[i]->enableCache(false);
  }
}


void GeneralEditorData::invalidateCache()
{
  for (int i = 0; i < getViewportCount(); ++i)
    if (vpw[i]->isVisible())
      vpw[i]->invalidateCache();
}


int GeneralEditorData::findViewportIndex(IGenViewportWnd *w) const
{
  for (int i = 0; i < getViewportCount(); ++i)
    if (vpw[i] == w)
      return i;

  // G_ASSERT(getViewportCount() && "TEMP!!! findViewportIndex(): viewports count == 0!");
  return getViewportCount() - 1;
}


void GeneralEditorData::act(real dt)
{
  for (int i = 0; i < getViewportCount(); ++i)
    vpw[i]->act(dt);

  tbManager->act();

  int viewportId;
  for (viewportId = getViewportCount(); --viewportId >= 0;)
    if (vpw[viewportId]->isActive())
      break;
  if (viewportId >= 0)
    currentViewportId = viewportId;
}


void GeneralEditorData::redrawClientRect()
{
  for (int i = 0; i < getViewportCount(); ++i)
    vpw[i]->redrawClientRect();
}


void GeneralEditorData::setZnearZfar(real zn, real zf)
{
  for (int i = 0; i < getViewportCount(); ++i)
  {
    if (!vpw[i]->isVisible())
      continue;
    vpw[i]->setZnearZfar(zn, zf, true);
  }
}


IGenViewportWnd *GeneralEditorData::screenToViewport(int &x, int &y) const
{
  for (int i = 0; i < getViewportCount(); ++i)
  {
    if (!vpw[i]->isVisible())
      continue;

    int mx = x;
    int my = y;
    vpw[i]->screenToClient(mx, my);
    if (mx >= 0 && my >= 0 && mx < vpw[i]->getW() && my < vpw[i]->getH())
    {
      x = mx;
      y = my;
      return vpw[i];
    }
  }
  return NULL;
}
