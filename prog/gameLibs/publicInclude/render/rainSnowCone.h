//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>
#include <3d/dag_texMgr.h>

class DataBlock;

class RainSnowCone
{
public:
  static constexpr int NUM_LAYERS = 4;

  RainSnowCone(const DataBlock *blk, const Point3 &camera_pos);
  ~RainSnowCone();

  void update(float dt, const Point3 &camera_pos);
  void render();

  const Color4 &getColorMult() const { return rainColorMult; }
  void setColorMult(const Color4 &col) { rainColorMult = col; }

protected:
  ShaderMaterial *material;
  dynrender::RElem rendElem;
  Vbuffer *vb;
  Ibuffer *ib;
  TEXTUREID coneTexId[NUM_LAYERS];
  int coneTexVarId[NUM_LAYERS];
  int texcoordVarId[NUM_LAYERS];

  float length;
  float radius;
  float layerScale[NUM_LAYERS];
  float cameraSmooth;
  float stretch;
  float stretchCenter;
  float scroll;
  float scrollGravitation;
  float maxVelCamera;
  float velGravitation;
  Color4 rainColorMult;

  Point3 xzMoveDir;
  Point3 prevCameraPos;
  Point3 coneDir;
  Color4 color;
  float offset[NUM_LAYERS];
};
