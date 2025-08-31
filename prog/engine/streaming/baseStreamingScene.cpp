// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <streaming/dag_streamingBase.h>
#include <shaders/dag_renderScene.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/3d/dag_matricesAndPerspective.h>

BaseStreamingSceneHolder::BaseStreamingSceneHolder()
{
  envi = NULL;
  enviPos = Point3(0, 0, 0);

  ssm = NULL;
  ssmCtrl = NULL;
  mainBindump = NULL;
}

BaseStreamingSceneHolder::~BaseStreamingSceneHolder()
{
  clear(); // -V1053
  if (ssm)
  {
    ssm->destroy();
    ssm = NULL;
  }
}

void BaseStreamingSceneHolder::bdlSceneLoaded(unsigned bindump_id, RenderScene *scene)
{
  if (bindump_id != -1)
    ddsx::tex_pack2_perform_delayed_data_loading();
  rs.addScene(scene, bindump_id);
}

void BaseStreamingSceneHolder::openStreaming(const DataBlock &blk, const char *folderPath, bool auto_by_distance)
{
  clear();
  if (!folderPath)
    folderPath = blk.getStr("baseFolderPath", "");
  baseFolderPath = folderPath;

  const char *md_fname = blk.getStr("maindump", NULL);
  if (md_fname)
    mainBindump = load_binary_dump(String(0, "%s/%s", folderPath, md_fname), *this, -1);
  if (!ssm)
  {
    ssm = IStreamingSceneManager::create();
    ssm->setClient(this);
  }

  if (auto_by_distance)
    ssmCtrl = new (inimem) StreamingSceneController(*ssm, blk, baseFolderPath);
}

void BaseStreamingSceneHolder::renderEnvi(const TMatrix &view_tm, float roty)
{
  if (envi)
  {
    TMatrix tm;
    if (float_nonzero(roty))
      tm = view_tm * rotyTM(roty);
    else
      tm = view_tm;
    tm.setcol(3, -tm % enviPos);
    d3d::settm(TM_VIEW, tm);

    for (int ei = 0; ei < envi->obj.size(); ei++)
    {
      if (envi->obj[ei].lods.size())
        envi->obj[ei].lods[0].mesh->render();
      // the_scene.enviscene->obj[ei].lods[0].mesh->render_trans();
    }

    d3d::settm(TM_VIEW, view_tm);
  }
}

void BaseStreamingSceneHolder::clear()
{
  if (ssm)
    ssm->unloadAllBinDumps();
  if (ssmCtrl)
    delete ssmCtrl;
  ssmCtrl = NULL;

  unloadAllBinDumps();
  if (envi)
  {
    delete envi;
    envi = NULL;
  }

  if (mainBindump)
    unload_binary_dump(mainBindump, false);
  mainBindump = NULL;
}

bool BaseStreamingSceneHolder::hasClipmap() const
{
  for (auto &scene : rs.getScenes())
    if (scene->hasClipmap())
      return true;
  return false;
}

bool BaseStreamingSceneHolder::getClipmapMinMaxHeight(float &min_height, float &max_height) const
{
  min_height = 100000;
  max_height = -100000;
  for (auto &scene : rs.getScenes())
  {
    float minH, maxH;
    if (scene->getClipmapMinMaxHeight(minH, maxH))
    {
      min_height = min(minH, min_height);
      max_height = max(maxH, max_height);
    }
  }
  return min_height < max_height;
}
