//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_drv3dConsts.h>


class PostFxRenderer;
class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;


class IRenderDynamicCubeFace
{
public:
  virtual void renderDynamicCubeFace(int tex_var_id, TEXTUREID tex_id, CubeTexture *texture, unsigned int face_no) = 0;
};


class DynamicCube
{
public:
  int dynamicCubeTex1VarId;
  int dynamicCubeTex2VarId;
  int dynamicCubeTexVarId;


  DynamicCube(unsigned int num_mips, unsigned int size, float blur, unsigned texFormat = 0);

  ~DynamicCube();

  bool refresh();
  void beforeRender(float blend_to_next, IRenderDynamicCubeFace *render);
  void reset(IRenderDynamicCubeFace *render);

protected:
  unsigned int numDynamicCubeTexMips;
  unsigned int dynamicCubeSize;
  float dynamicCubeBlur;

  CubeTexture *dynamicCubeTex1;
  TEXTUREID dynamicCubeTex1Id;

  CubeTexture *dynamicCubeTex2;
  TEXTUREID dynamicCubeTex2Id;

  CubeTexture *dynamicCubeTex;
  TEXTUREID dynamicCubeTexId;

  Texture *dynamicCubeDepthTex;

  int blendCubesParamsVarId;
  int dynamicCubeTexBlendVarId;

  PostFxRenderer *blendCubesRenderer;
  PostFxRenderer *blurCubesRenderer;

  int dynamicCubeFaceNo;
  int blendCubesStage;
};
