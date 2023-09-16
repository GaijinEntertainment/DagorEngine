//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_drv3d.h>

class BaseTexture;
typedef BaseTexture Texture;
class PostFxRenderer;


class DistortionRenderer
{
public:
  DistortionRenderer(int screenWidth, int screenHeight, bool _msaaPs3);
  ~DistortionRenderer();

  void startRenderDistortion(Texture *distortionOffsetTex);
  void endRenderDistortion();
  void applyDistortion(TEXTUREID sourceTexId, Texture *destTex, TEXTUREID distortionOffsetTexId);

private:
  Texture *distortionTargetTex;
  TEXTUREID distortionTargetTexId;
  int sourceTexVarId;
  int distortionOffsetTexVarId;
  PostFxRenderer *distortionFxRenderer;

  int mistEnableVarId;

  bool msaaPs3;

  Driver3dRenderTarget prevRt;
  bool started;
};

extern DistortionRenderer *distortionRenderer;
