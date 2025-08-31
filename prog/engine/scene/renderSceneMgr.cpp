// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scene/dag_renderSceneMgr.h>
#include <shaders/dag_renderScene.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>


RenderSceneManager::RenderSceneManager(int max_dump_reserve)
{
  rs.reserve(max_dump_reserve);
  rsInfo.reserve(max_dump_reserve);
}

RenderSceneManager::~RenderSceneManager()
{
  delAllScenes();
  clear_and_shrink(rs);
  clear_and_shrink(rsInfo);
}

void RenderSceneManager::set_checked_frustum(bool checked)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->set_checked_frustum(checked);
}

void RenderSceneManager::doPrepareVisCtx(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask /*=0xFFFFFFFFU*/)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->doPrepareVisCtx(vf, render_id, render_flags_mask);
}

void RenderSceneManager::allocPrepareVisCtx(int render_id)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->allocPrepareVisCtx(render_id);
}

void RenderSceneManager::setPrepareVisCtxToRender(int render_id)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->setPrepareVisCtxToRender(render_id);
}

void RenderSceneManager::pushPreparedVisCtx()
{
  for (int i = 0; i < rs.size(); i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->pushPreparedVisCtx();
}

void RenderSceneManager::popPreparedVisCtx()
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->popPreparedVisCtx();
}

void RenderSceneManager::savePreparedVisCtx(int render_id)
{
  for (int i = 0; i < rs.size(); i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->savePreparedVisCtx(render_id);
}

void RenderSceneManager::restorePreparedVisCtx(int render_id)
{
  for (int i = 0; i < rs.size(); i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->restorePreparedVisCtx(render_id);
}

void RenderSceneManager::render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->render(vf, render_id, render_flags_mask);
}

void RenderSceneManager::renderTrans()
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].renderable)
      rs[i]->render_trans();
}

bool RenderSceneManager::setSceneRenderable(unsigned rs_id, bool renderable)
{
  for (int i = 0, e = rs.size(); i < e; i++)
  {
    if (rsInfo[i].rsId == rs_id) // do not skip loading rs[i]
    {
      rsInfo[i].renderable = renderable;
      return true;
    }
  }

  return false;
}
bool RenderSceneManager::isSceneRenderable(unsigned rs_id)
{
  for (int i = 0, e = rs.size(); i < e; i++)
  {
    if (rs[i] && rsInfo[i].rsId == rs_id)
      return rsInfo[i].renderable;
  }

  return false; // unloaded
}

int RenderSceneManager::loadScene(const char *lmdir, unsigned rs_id)
{
  RenderScene *sc = new RenderScene;
  sc->load(lmdir, true);
  return addScene(sc, rs_id);
}
int RenderSceneManager::addScene(RenderScene *_rs, unsigned rs_id)
{
  int id = -1;

  for (int i = 0, e = rs.size(); i < e; i++)
    if (!rs[i])
    {
      id = i;
      break;
    }

  if (id == -1)
  {
    rs.push_back(NULL);
    id = append_items(rsInfo, 1);
  }
  rsInfo[id].rsId = rs_id;
  rsInfo[id].renderable = true;
  rs[id] = _rs;

  return id;
}
void RenderSceneManager::delScene(unsigned rs_id)
{
  for (int i = 0, e = rs.size(); i < e; i++)
    if (rs[i] && rsInfo[i].rsId == rs_id)
    {
      RenderScene *sc = rs[i];
      rsInfo[i].rsId = -1;
      rs[i] = NULL;

      delete sc;

      // remove empty trailing slots
      for (i = rs.size() - 1; i >= 0; i--)
        if (!rs[i])
        {
          erase_items(rs, i, 1);
          erase_items(rsInfo, i, 1);
        }
        else
          break;

      return;
    }
}

void RenderSceneManager::delAllScenes()
{
  for (int i = rs.size() - 1; i >= 0; i--)
    if (rs[i])
      delete rs[i];

  rs.clear();
  rsInfo.clear();
}
