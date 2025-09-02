//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>

#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_critSec.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <3d/dag_picMgr.h>
#include <3d/dag_texIdSet.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_math3d.h>

class DataBlock;

template <typename TAnimCharIcon>
class RenderAnimCharIconPF final : public PictureManager::PictureRenderFactory
{
public:
  InitOnDemand<TAnimCharIcon, false> ctx;

  virtual void registered() override { ctx.demandInit(); }
  virtual void unregistered() override
  {
    ctx->clearPendReq();
    ctx.demandDestroy();
  }
  virtual void clearPendReq() override { ctx ? ctx->clearPendReq() : (void)0; }
  virtual bool match(const DataBlock &pic_props, int &out_w, int &out_h) override
  {
    return !ctx || ctx->match(pic_props, out_w, out_h);
  }
  virtual bool render(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid) override
  {
    return !ctx || ctx->render(to, x, y, w, h, info, pid);
  }
};

namespace AnimV20
{
class IAnimCharacter2;
}

typedef eastl::unique_ptr<GameResource, GameResDeleter> GameResPtr;

struct IconAnimchar
{
  enum
  {
    FULL,
    SILHOUETTE,
    SAME,
    UNKNOWN
  };

  void init(AnimV20::IAnimCharacter2 *animchar_, const DataBlock *blk_);
  void updateTmWtm(AnimV20::IAnimCharacter2 *parent);

  AnimV20::IAnimCharacter2 *animchar = nullptr;
  const DataBlock *blk = nullptr; // non-null only when setting things up.
  int slotId = -1;
  vec4f scaleV = v_zero();
  int shading = UNKNOWN;
  E3DCOLOR silhouette = 0;
  bool silhouetteHasShadow = false;
  float silhouetteMinShadow = 1.0f;
  E3DCOLOR outline = 0;
  bool calcBBox = true;
  bool swapYZ = true;
  enum struct AttachType
  {
    SLOT,
    SKELETON,
    NODE
  } attachType = AttachType::SLOT;

  eastl::string parentNode = "";
  TMatrix relativeTm = TMatrix::IDENT;
  eastl::vector<SharedTex> managedTextures;
  eastl::vector<eastl::string> hideNodeNames;
};

class DeferredRenderTarget;
using IconAnimcharWithAttachments = eastl::vector<IconAnimchar>;

class EnviPanoramaCache
{
  constexpr static size_t ENVI_PANORAMA_CACHE_SIZE = 3;

public:
  struct Id
  {
    int val = -1;
    operator bool() const { return val >= 0; }
  };

  Id addEnvi(const char *gameres_tex_name)
  {
    if (gameres_tex_name == nullptr || gameres_tex_name[0] == '\0')
      return {};

    for (int i = 0; i < usedSpace; ++i)
    {
      if (!strcmp(gameres_tex_name, texNames[i]))
      {
        if (i != 0)
        {
          eastl::swap(iconEnviPanoramaTextures[0], iconEnviPanoramaTextures[i]);
          eastl::swap(texNames[0], texNames[i]);
        }
        return Id{0};
      }
    }

    debug("Animchar Icon Render: adding new envi panorama `%s` to cache", gameres_tex_name);

    SharedTex tex = dag::get_tex_gameres(gameres_tex_name);
    if (!tex)
    {
      logerr("Animchar Icon Render: enviPanoramaTex '%s' is missing in assets", gameres_tex_name);
      return {};
    }

    usedSpace = min(usedSpace + 1, ENVI_PANORAMA_CACHE_SIZE);
    const size_t cachePos = usedSpace - 1;

    iconEnviPanoramaTextures[cachePos] = eastl::move(tex);
    texNames[cachePos] = String{gameres_tex_name};

    return Id{(int)cachePos};
  }

  bool prefetch(const Id id) const
  {
    G_ASSERT(id);

    const SharedTex &tex = iconEnviPanoramaTextures[id.val];
    G_ASSERT(tex.getTexId() != BAD_TEXTUREID);

    return prefetch_and_check_managed_texture_loaded(tex.getTexId(), true);
  }

  const SharedTex &get(const Id id) const
  {
    G_ASSERT(id);
    return iconEnviPanoramaTextures[id.val];
  }

private:
  eastl::array<SharedTex, ENVI_PANORAMA_CACHE_SIZE> iconEnviPanoramaTextures = {};
  eastl::array<String, ENVI_PANORAMA_CACHE_SIZE> texNames = {};
  size_t usedSpace = 0;
};

class RenderAnimCharIconBase
{
public:
  RenderAnimCharIconBase() = default;
  bool match(const DataBlock &pic_props, int &out_w, int &out_h);
  bool render(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid);
  void clearPendReq();

protected:
  bool renderInternal(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid);
  virtual bool renderIconAnimChars(const IconAnimcharWithAttachments &iconAnimchars, Texture *to, const Driver3dPerspective &persp,
    int x, int y, int w, int h, int dstw, int dsth, const DataBlock &info) = 0;
  virtual bool needsResolveRenderTarget() = 0;
  virtual bool ensureDim(int w, int h) = 0;
  virtual void beforeRender(const DataBlock &) {};
  virtual void afterRender() {};
  virtual void setSunLightParams(const Point4 &lightDir, const Point4 &lightColor) = 0;
  void setEnviParams(const DataBlock &info);
  void setLightParams(const DataBlock &info, bool full_deferred, bool mobile_deferred);
  void clear_to(E3DCOLOR col, Texture *to, int x, int y, int dstw, int dsth) const;
  eastl::unique_ptr<DeferredRenderTarget> target;
  UniqueTexHolder finalTarget;
  PostFxRenderer finalAA;
  EnviPanoramaCache enviPanoramaCache;
  eastl::vector_map<PICTUREID, IconAnimcharWithAttachments> pendReq;
  WinCritSec pendReqCs;
  bool enviInited = false;
};