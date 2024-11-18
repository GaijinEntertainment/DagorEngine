//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_picMgr.h>
#include <gui/dag_stdGuiRender.h>
#include <squirrel.h>

namespace Sqrat
{
class Table;
}

class SqModules;

namespace darg
{

struct IGuiScene;
class GuiScene;
struct AsyncLoadRequest;


class Picture
{
public:
  Picture(HSQUIRRELVM vm);
  Picture(HSQUIRRELVM vm, const char *name);
  ~Picture();

  void init(const char *name);
  void onLoaded(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 &tcLt, const Point2 &tcRb,
    const Point2 &picture_sz);
  void onAsyncLoadStopped(AsyncLoadRequest *req);

  IGuiScene *getScene() const;
  bool isLoading() const { return loadReq != nullptr; }
  const PictureManager::PicDesc &getPic();
  BlendMode getBlendMode() const { return blendMode; }
  const char *getName() const { return srcName; }

protected:
  bool load(const char *name); //> return true if loaded, false if requested to load asynchronously

protected:
  GuiScene *guiScene = nullptr;
  AsyncLoadRequest *loadReq = nullptr;

  PictureManager::PicDesc pic;
  BlendMode blendMode = PREMULTIPLIED;

  bool lazy = true;
  String nameToLoadOnDemand;
  String srcName;
};


class PictureImmediate : public Picture
{
public:
  PictureImmediate(HSQUIRRELVM vm, const char *name);
};

class LottieAnimation : public Picture
{
public:
  LottieAnimation(HSQUIRRELVM vm, const char *name) : Picture(vm) { init(name); }
  LottieAnimation(const LottieAnimation &) = delete;
  ~LottieAnimation();
};

void bind_lottie_animation(SqModules *module_mgr, Sqrat::Table *exports = nullptr);

} // namespace darg
