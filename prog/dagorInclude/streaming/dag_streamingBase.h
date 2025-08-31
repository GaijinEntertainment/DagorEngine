//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <streaming/dag_streamingMgr.h>
#include <streaming/dag_streamingCtrl.h>
#include <scene/dag_renderSceneMgr.h>
#include <util/dag_simpleString.h>

class RenderScene;
class DataBlock;
class IGenLoad;

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
  BaseStreamingSceneHolder();
  BaseStreamingSceneHolder(const BaseStreamingSceneHolder &) = delete;
  ~BaseStreamingSceneHolder();

  // IStreamingSceneStorage interface implementation
  virtual bool bdlGetPhysicalWorldBbox(BBox3 & /*bbox*/) { return false; }

  virtual bool bdlConfirmLoading(unsigned bindump_id, int tag) { return !(tag == _MAKE4C('ENVI') && bindump_id != -1); }

  virtual void bdlTextureMap(unsigned /*bindump_id*/, dag::ConstSpan<TEXTUREID> /*texId*/) {}

  virtual void bdlEnviLoaded(unsigned bindump_id, RenderScene *_envi)
  {
    if (bindump_id == -1) // for master bindump only!
      envi = _envi;
  }
  virtual void bdlSceneLoaded(unsigned bindump_id, RenderScene *scene);
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
  void renderEnvi(const TMatrix &view_tm, float roty = 0.0f);
  void render(const VisibilityFinder &vf, int render_id = 0, unsigned render_flags_mask = 0xFFFFFFFFU)
  {
    rs.render(vf, render_id, render_flags_mask);
  }

  void renderTrans() { rs.renderTrans(); }

  void act(const Point3 &observer)
  {
    if (ssmCtrl)
      ssmCtrl->setObserverPos(observer);
    if (ssm)
      ssm->act();
  }

  void openStreaming(const DataBlock &blk, const char *folderPath, bool auto_by_distance = true);

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

  void clear();

  void waitForLoadingDone(const Point3 &view_pos)
  {
    if (!ssm)
      return;
    while (ssm->isLoading())
      act(view_pos);
  }

  // access to protected members
  RenderSceneManager &getRsm() { return rs; }
  IStreamingSceneManager *getSsm() { return ssm; }
  StreamingSceneController *getSsmCtrl() { return ssmCtrl; }
  RenderScene *getEnvi() { return envi; }
  Point3 getEnviPos() { return enviPos; }

  void setEnviPos(const Point3 &p) { enviPos = p; }

  bool hasClipmap() const;

  bool getClipmapMinMaxHeight(float &min_height, float &max_height) const;
};
