//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_picMgr.h>
#include <gui/dag_stdGuiRender.h>
#include <squirrel.h>
#include <util/dag_string.h>

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
  // isLoading covers only the async load request; a dynamic atlas pic may still lack content
  // after it (pending factory render, restore after eviction). Loading placeholders use
  // isContentReady; fallbackImage selection and render code keep isLoading, since a pic with
  // a pending render is present (valid id, declared size), just not drawn into the atlas yet.
  bool isLoading() const { return loadReq != nullptr; }
  bool isContentReady() const;
  const PictureManager::PicDesc &getPic();
  bool prefetch(); //> starts pending load, returns true if it is in flight
  BlendMode getBlendMode() const { return blendMode; }
  const char *getName() const { return srcName; }
  Point2 getLoadedPicSize();

protected:
  bool load(const char *name); //> return true if loaded, false if requested to load asynchronously
  void finishPrefetch();

public:
  static constexpr TexFormat def_tex_format = TexFormat::SRGB_IN_UNORM;
  TexFormat texFormat = def_tex_format;

protected:
  GuiScene *guiScene = nullptr;
  AsyncLoadRequest *loadReq = nullptr;

  PictureManager::PicDesc pic;
  BlendMode blendMode = PREMULTIPLIED;

  bool lazy = true;
  bool prefetchPending = false;
  String nameToLoadOnDemand;
  String srcName;
};


class PictureImmediate : public Picture
{
public:
  PictureImmediate(HSQUIRRELVM vm, const char *name);
};

} // namespace darg
