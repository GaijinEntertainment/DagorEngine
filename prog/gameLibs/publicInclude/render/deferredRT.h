//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_decl.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_texMgr.h>

class BaseTexture;
typedef BaseTexture Texture;
class DeferredRT
{
public:
  enum class StereoMode
  {
    MonoOrMultipass,
    SideBySideHorizontal,
    SideBySideVertical,
  };

  static const int MAX_NUM_MRT = 5;

  void close();
  void setRt();
  void resolve();
  void setVar();
  void debugRender(int show_gbuffer);
  uint32_t recreateDepth(uint32_t fmt); // returns true if 32 bit depth buffer was created
  DeferredRT(const char *name, int w, int h, StereoMode stereo_mode, unsigned msaaFlag, int numRt, const unsigned *texFmt,
    uint32_t depth_fmt);
  ~DeferredRT() { close(); }
  void changeResolution(int w, int h);
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  Texture *getDepth() const { return depth.getTex2D(); }
  TEXTUREID getDepthId() const { return depth.getTexId(); }
  const ManagedTex &getDepthAll() const { return depth; }
  Texture *getRt(uint32_t idx) const { return mrts[idx].getTex2D(); }
  TEXTUREID getRtId(uint32_t idx) const { return mrts[idx].getTexId(); }
  const ManagedTex &getRtAll(uint32_t idx) const { return mrts[idx]; }
  uint32_t getRtNum() const { return numRt; }
  void swapDepth(ResizableTex &ndepth) { eastl::swap(depth, ndepth); }

protected:
  uint32_t recreateDepthInternal(uint32_t fmt);

  IPoint2 calcCreationSize() const;

  StereoMode stereoMode = StereoMode::MonoOrMultipass;
  int numRt = 0, width = 0, height = 0;
  char name[64] = {};

  ResizableTex mrts[MAX_NUM_MRT] = {};
  d3d::SamplerHandle baseSampler = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle blackPixelSampler = d3d::INVALID_SAMPLER_HANDLE;
  ResizableTex depth;
  UniqueTex blackPixelTex;

  bool useResolvedDepth = false;
};
