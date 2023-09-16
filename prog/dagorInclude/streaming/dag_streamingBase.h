//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <streaming/dag_streamingMgr.h>
#include <streaming/dag_streamingCtrl.h>
#include <shaders/dag_renderScene.h>
#include <scene/dag_renderSceneMgr.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_simpleString.h>


class BaseStreamingSceneHolder : public IStreamingSceneStorage
{
protected:
  RenderSceneManager rs;
  RenderScene *envi;
  Point3 enviPos;

  IStreamingSceneManager *ssm;
  StreamingSceneController *ssmCtrl;
  BinaryDump *mainBindump;
  SimpleString baseFolderPath;

public:
  BaseStreamingSceneHolder()
  {
    envi = NULL;
    enviPos = Point3(0, 0, 0);

    ssm = NULL;
    ssmCtrl = NULL;
    mainBindump = NULL;
  }

  ~BaseStreamingSceneHolder()
  {
    clear();
    if (ssm)
    {
      ssm->destroy();
      ssm = NULL;
    }
  }

  // IStreamingSceneStorage interface implementation
  virtual bool bdlGetPhysicalWorldBbox(BBox3 & /*bbox*/) { return false; }

  virtual bool bdlConfirmLoading(unsigned bindump_id, int tag) { return !(tag == _MAKE4C('ENVI') && bindump_id != -1); }

  virtual void bdlTextureMap(unsigned /*bindump_id*/, dag::ConstSpan<TEXTUREID> /*texId*/) {}

  virtual void bdlEnviLoaded(unsigned bindump_id, RenderScene *_envi)
  {
    if (bindump_id == -1) // for master bindump only!
      envi = _envi;
  }
  virtual void bdlSceneLoaded(unsigned bindump_id, RenderScene *scene)
  {
    if (bindump_id != -1)
      ddsx::tex_pack2_perform_delayed_data_loading();
    rs.addScene(scene, bindump_id);
  }
  virtual void bdlBinDumpLoaded(unsigned /*bindump_id*/) {}

  virtual bool bdlCustomLoad(unsigned /*bindump_id*/, int /*tag*/, IGenLoad & /*crd*/, dag::ConstSpan<TEXTUREID> /*texId*/)
  {
    return true;
  }

  virtual const char *bdlGetBasePath() { return NULL; }


  virtual void delBinDump(unsigned bindump_id) { rs.delScene(bindump_id); }
  virtual float getBinDumpOptima(unsigned bindump_id) { return ssmCtrl ? ssmCtrl->getBinDumpOptima(bindump_id) : 0; }

  // virtual methods for derived classes
  virtual void unloadAllBinDumps() { rs.delAllScenes(); }

  // routers for internal data
  void renderEnvi(const TMatrix &view_tm, float roty = 0.0f)
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
  void render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask) { rs.render(vf, render_id, render_flags_mask); }
  void render(int render_id = 0, unsigned render_flags_mask = 0xFFFFFFFFU)
  {
    rs.render(*visibility_finder, render_id, render_flags_mask);
  }
  void renderTrans() { rs.renderTrans(); }

  void act(const Point3 &observer)
  {
    if (ssmCtrl)
      ssmCtrl->setObserverPos(observer);
    if (ssm)
      ssm->act();
  }

  void openStreaming(const DataBlock &blk, const char *folderPath, bool auto_by_distance = true)
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
  void openSingle(const char *level_fn)
  {
    clear();
    baseFolderPath = ".";

    mainBindump = load_binary_dump(level_fn, *this, -1);
    if (!ssm)
    {
      ssm = IStreamingSceneManager::create();
      ssm->setClient(this);
    }
  }

  void clear()
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

  void waitForLoadingDone()
  {
    if (!ssm)
      return;
    while (ssm->isLoading())
      act(::grs_cur_view.itm.getcol(3));
  }

  // access to protected members
  RenderSceneManager &getRsm() { return rs; }
  IStreamingSceneManager *getSsm() { return ssm; }
  StreamingSceneController *getSsmCtrl() { return ssmCtrl; }
  RenderScene *getEnvi() { return envi; }
  Point3 getEnviPos() { return enviPos; }

  void setEnviPos(const Point3 &p) { enviPos = p; }
};
