// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin_ssv.h"
#include <de3_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_interface.h>
#include "sceneHolder.h"
#include <libTools/util/strUtil.h>
#include <workCycle/dag_delayedAction.h>
#include <debug/dag_logSys.h>

ssviewplugin::Plugin *ssviewplugin::Plugin::self = NULL;

bool ssviewplugin::Plugin::prepareRequiredServices() { return true; }

ssviewplugin::Plugin::Plugin()
{
  self = this;
  streamingScene = NULL;
  set_delayed_action_max_quota(5000);
}


ssviewplugin::Plugin::~Plugin()
{
  self = NULL;
  del_it(streamingScene);
}


void ssviewplugin::Plugin::registered()
{
  if (!streamingScene)
    streamingScene = new (inimem) SceneHolder;
}
void ssviewplugin::Plugin::unregistered() { del_it(streamingScene); }

void ssviewplugin::Plugin::setVisible(bool vis)
{
  if (isVisible != vis)
  {
    if (vis)
    {
      streamingScene->openStreaming(strmBlk, ::make_full_path(DAGORED2->getGameDir(), streamingFolder));
    }
    else
    {
      if (streamingScene->getSsm() && streamingScene->getSsm()->isLoading())
      {
        DAEDITOR3.conNote("Waiting for streaming to shutdown...");

        while (streamingScene->getSsm()->isLoading())
        {
          streamingScene->act(grs_cur_view.tm.getcol(3));
          perform_delayed_actions();
        }

        DAEDITOR3.conNote("Done.");
      }

      streamingScene->clear();
    }
  }
  isVisible = vis;
}

bool ssviewplugin::Plugin::begin(int toolbar_id, unsigned menu_id) { return true; }


bool ssviewplugin::Plugin::end() { return true; }


void ssviewplugin::Plugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  streamingScene->skipEnviData = DAGORED2->getPluginByName("environment") != NULL;

  isDebugVisible = local_data.getBool("isDebugVisible", isDebugVisible);
  streamingFolder = blk.getStr("streamingFolder", streamingFolder);
  streamingBlkPath = blk.getStr("streamingBlk", "");

  strmBlk.load(::make_full_path(DAGORED2->getGameDir(), streamingBlkPath));

  // disable loading of current level using streaming - it will be shown by editor anyway
  String fname(DAGORED2->getProjectFileName());
  remove_trailing_string(fname, ".level.blk");
  fname += ".bin";

  if (stricmp(strmBlk.getStr("maindump", ""), fname) == 0)
    strmBlk.removeParam("maindump");
  else
    for (int i = 0; i < strmBlk.blockCount(); i++)
      if (stricmp(strmBlk.getBlock(i)->getStr("stream", ""), fname) == 0)
      {
        strmBlk.removeBlock(i);
        break;
      }

  if (isVisible && streamingFolder.length())
    loadScene(::make_full_path(DAGORED2->getGameDir(), streamingFolder));
}


void ssviewplugin::Plugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  local_data.setBool("isDebugVisible", isDebugVisible);
  blk.setStr("streamingFolder", streamingFolder);
  blk.setStr("streamingBlk", streamingBlkPath);
}


void ssviewplugin::Plugin::clearObjects()
{
  streamingFolder = "";
  if (streamingScene)
    streamingScene->clear();
}


void ssviewplugin::Plugin::actObjects(float dt)
{
  if (isVisible && streamingScene)
  {
    IGenViewportWnd *viewport = DAGORED2->getCurrentViewport();
    if (!viewport)
      return;

    viewport->getCameraTransform(grs_cur_view.tm);
    streamingScene->act(grs_cur_view.tm.getcol(3));
  }
}


void ssviewplugin::Plugin::beforeRenderObjects(IGenViewportWnd *vp) {}


void ssviewplugin::Plugin::renderObjects()
{
  if (isVisible && isDebugVisible && streamingScene && streamingScene->getSsmCtrl())
    streamingScene->getSsmCtrl()->renderDbg();
}


void ssviewplugin::Plugin::renderTransObjects() {}

void *ssviewplugin::Plugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IRenderingService);
  return NULL;
}

void ssviewplugin::Plugin::renderGeometry(Stage stage)
{
  switch (stage)
  {
    case STG_RENDER_ENVI: streamingScene->renderEnvi(grs_cur_view.tm); break;

    case STG_RENDER_STATIC_OPAQUE:
      streamingScene->render(EDITORCORE->queryEditorInterface<IVisibilityFinderProvider>()->getVisibilityFinder(), 0, 0xFFFFFFFF);
      break;

    case STG_RENDER_STATIC_TRANS: streamingScene->renderTrans(); break;

    case STG_RENDER_SHADOWS:
      streamingScene->render(EDITORCORE->queryEditorInterface<IVisibilityFinderProvider>()->getVisibilityFinder(), 1,
        RenderScene::RenderObject::ROF_CastShadows);
      break;
  }
}

bool ssviewplugin::Plugin::loadScene(const char *streaming_folder)
{
  if (!streamingScene || !streaming_folder || !streaming_folder[0])
    return false;

  streamingScene->openStreaming(strmBlk, streaming_folder);
  return true;
}
