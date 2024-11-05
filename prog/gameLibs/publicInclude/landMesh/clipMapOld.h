//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_color.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_texMgr.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <drv/3d/dag_driver.h>

#include <list>

class BaseTexture;
typedef BaseTexture Texture;
class PostFxRenderer;

enum
{
  CLIPMAP_RENDER_STAGE_0 = 0x00000001,
  CLIPMAP_RENDER_STAGE_1 = 0x00000002,
  CLIPMAP_RENDER_STAGE_2 = 0x00000004,
  CLIPMAP_RENDER_ALL_STAGES = 0x00000007
};


#define USE_VIRTUAL_TEXTURE 0

class ClipmapRenderer
{
public:
  virtual void render(const BBox2 &region, const Point2 &center, unsigned int stages = CLIPMAP_RENDER_ALL_STAGES) = 0;
  virtual void renderFeedback() { G_ASSERT(0); }
};

class DxtCompressJob;

class ShaderMaterial;
class ShaderElement;
class DynShaderQuadBuf;
class RingDynamicVB;

class Clipmap
{
public:
  enum NormalType
  {
    NO_NORMAL = 0,
    NORMAL = 1,
    PARALLAX = 2
  };
  static constexpr int MAX_CLIPMAP = 9;

  Clipmap(unsigned tex_format = 0, NormalType normal_type = NO_NORMAL) // same as TEXFMT_A8R8G8B8
  {
    clipCount = 0;
    normalType = normal_type;
    clipmapTex = 0;
    clipmapTexId = BAD_TEXTUREID;
    clipmapNormalTex = 0;
    clipmapNormalTexId = BAD_TEXTUREID;
    clipmap_normal_texVarId = -1;
    clipmapTexLocal = clipmapTexMain = 0;
    clipmapPyramid = 0;
    constantFillerMat = NULL;
    constantFillerElement = NULL;
    constantFillerBuf = NULL;
    currentFormat = nonDxtFormat = tex_format;
    mem_set_0(totex_ofs);
    mem_set_0(world_to_clip_mul);
    mem_set_0(world_to_clip_ofs);
    constantFillerVBuf = NULL;
    usePyramidTexture = true;
    memset(updatedAfrs, 0, sizeof(updatedAfrs));
  }
  ~Clipmap() { close(); }
  // sz - texture size of each clip, clip_count - number of clipmap stack. multiplayer - size of next clip
  // size in meters of latest clip
  void init(int sz, int clip_count, float st_texel_size, float multiplayer = 2.0, float size = 0);
  void close();

  // returns bits of rendered clip, or 0 if no clip was rendered
  void setMaxTexelSize(float max_texel_size) { maxTexelScale = max_texel_size / (clipmap[0].size.x / clipSize); }

  float getStartTexelSize() const { return texelSize; }
  void setStartTexelSize(float st_texel_size) // same as in init
  {
    texelSize = st_texel_size;
    float multiplayer = getMultiplayer();
    for (int i = 0; i < clipCount; ++i)
    {
      if (i == 0 && clampMaxSize)
        continue;
      else
      {
        Point2 size = Point2(clipSize, clipSize) * texelSize;
        size = size * powf(multiplayer, clipCount - 1 - i);
        clipmap[i].initialSize = size;
      }
    }
    invalidate(false);
  }
  float getMultiplayer() const
  {
    if (clipCount < 2)
      return 1.0f;
    return clipmap[clipCount - 2].initialSize.x / clipmap[clipCount - 1].initialSize.x;
  }
  void changeMultiplayer(float multiplayer) // same as in init
  {
    for (int i = 0; i < clipCount - 1; ++i)
    {
      if (i == 0 && clampMaxSize)
        continue;
      else
      {
        float clipScale = texelSize * powf(multiplayer, clipCount - 1 - i);
        clipmap[i].initialSize = Point2(clipSize, clipSize) * clipScale;
      }
    }
    invalidate(false);
  }

  float getLastClipStartDistance();

  int prepareRender(ClipmapRenderer &render, const Point2 &center, TMatrix4 *globtm, bool force_prepare, float height,
    float maxCenterMoveDist = 0.f,   // allow to move center of clipmap alongside view, for better clipmap fitting
    bool leave_gpu_running = false); // PS3: don't block GPU while compressing DXT.
                                     // DXT compression overlaps with subsequent jobs
                                     // sync later using push_gpu_barrier(false)

  void invalidate(bool force_redraw = true);
  void setRenderStates(bool for_lmesh = false);
  void unsetRenderStates(bool for_lmesh = false, bool for_dyn_lmesh = false);

  void warm();
  bool getBBox(BBox2 &ret) const;
  NormalType getNormalType() const { return normalType; }
  void setNormalType(NormalType tp);


private:
  struct ClipPyramid
  {
    IPoint2 texOfs, texSz, targetTexOfs, targetTexSz;
    Point2 center, size, initialSize;
    enum
    {
      EMPTY,
      RENDERED,
      COPIED,
      BETTER_REDRAW
    } state;
  };
  carray<ClipPyramid, MAX_CLIPMAP> clipmap;
  int clipCount;
  float texelSize;
  unsigned int clipSize;
  float maxTexelScale; // maximum scale ratio of texel size.
  bool clampMaxSize;
  Texture *rtTexture;
  TEXTUREID rtTextureId;
  Texture *rtNormalTexture;
  TEXTUREID rtNormalTextureId;
  PostFxRenderer *copyRenderer;
  int texVarId;
  int texOffsetVarId;
  uint32_t updatedAfrs[MAX_CLIPMAP];


  int clipmapW, clipmapH;
  Texture *clipmapTexLocal, *clipmapTexMain, *clipmapTex;
  Texture *clipmapPyramid;
  TEXTUREID clipmapPyramidId;
  int clipmapPyramidVarId;
  int clipmapPyramidMulOfsVarId;
  int clipmapColOfsVarId, clipmapColMulVarId;
  ShaderMaterial *constantFillerMat;
  ShaderElement *constantFillerElement;
  DynShaderQuadBuf *constantFillerBuf;
  unsigned nonDxtFormat, currentFormat;
  NormalType normalType;

  Texture *clipmapNormalTex;
  TEXTUREID clipmapNormalTexId;
  int clipmap_normal_texVarId;

  RingDynamicVB *constantFillerVBuf;
  TEXTUREID clipmapTexId;
  int clipmap_texVarId;

  carray<int, MAX_CLIPMAP> land_world_to_clipmap_mul_varId;
  carray<int, MAX_CLIPMAP> land_world_to_clipmap_ofs_varId;
  int clipmap_totex_mul_varId;
  carray<int, MAX_CLIPMAP> clipmap_totex_ofs_varId;
  Color4 totex_mul;
  carray<Color4, MAX_CLIPMAP> totex_ofs;
  carray<Color4, MAX_CLIPMAP> world_to_clip_mul;
  carray<Color4, MAX_CLIPMAP> world_to_clip_ofs;

  void copyClipmap(int i);

  int pixelsInPyramidTop, pyramidTexSize;
  Point2 pyramidCenter;
  float pyramidTexelSize;
  bool usePyramidTexture;
  void closeColorMap();
  void closeNormalMap();
  void createColorMap();
  void createNormalMap();
};
