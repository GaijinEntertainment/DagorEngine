//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class PostFxRenderer;
class BaseTexture;
class DeferredRenderTarget;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;

namespace light_probe
{
class Cube;
};


class IRenderLightProbeFace
{
public:
  enum Element
  {
    Scene = 1,
    Cockpit = 2,
    Resolve = 4,
    Water = 8,
    Envi = 16,
    Trans = 32,
    Copy = 64,
    Blend = 128,
    All = Scene | Cockpit | Resolve | Water | Envi | Trans | Copy
  };

  virtual void renderLightProbeFace(CubeTexture *cubeTex, int rt_no, unsigned int face_no, unsigned element,
    const TMatrix4 &gbufferTm) = 0;
};


class DynamicLightProbe
{
public:
  enum class Mode
  {
    Separated = 0,
    Combined = 1
  };

  DynamicLightProbe();
  void init(unsigned int size, const char *name, unsigned fmt, int cube_index, Mode _mode = Mode::Combined);
  void close();

  ~DynamicLightProbe();

  bool refresh(float total_blend_time); // return true, if refresh started
  void update(float dt, IRenderLightProbeFace *cb, bool cockpit = false, const TMatrix4 &cockpitTm = TMatrix4::IDENT,
    bool force_flush = false);
  Mode getMode() const { return mode; }
  const ManagedTex *getCurrentProbe() const { return currentProbe; }
  // void beforeRender(float blend_to_next, IRenderDynamicCubeFace *render);
  void invalidate();

protected:
  int cubeIndex;
  Mode mode;
  light_probe::Cube *probe[3] = {};
  UniqueTex result;
  const ManagedTex *currentProbe;
  int currentProbeId;
  PostFxRenderer *blendCubesRenderer;
  TMatrix4 cockpitTm_inv[2];
  int state;
  float currentBlendTime, totalBlendTime;
  unsigned valid;
};
