//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>
#include <math/dag_Point3.h>
#include <3d/dag_texMgr.h>
#include <math/dag_TMatrix4.h>


#define DEBUG_CALC_ON_CPU 1 // 0 - disabled, 1 - scalar, 2 - vector, PC supported too

class RainX7
{
public:
  RainX7(const DataBlock &blk);
  ~RainX7();

  void update(float dt, const Point3 &camera_speed);
  void render(uint32_t view_id = 0);

  void setOccluder(float x1, float z1, float x2, float z2, float y);

  enum
  {
    MAX_VIEWS = 2,
  };

  union
  {
    struct
    {
      float parAlphaFadeSpeedBegin;
      float parAlphaFadeSpeedEnd;
      float parLength;
      float parSpeed;
      float parWidth;
      float parDensity;
      float parWind;
      float parAlpha;
    };
    float floatParameters[8];
  };
  Color3 parLighting;
  Color3 initialParLighting;
  float perPassMul;
  float translucencyFactor = 0;

  // protected:

  struct Pass
  {
    float speedScale;
    Point3 wind;
    Point3 offset;
    Point3 random;
  };

  unsigned int numParticles;
  float particleBox;
  unsigned int numPassesMax;
  TEXTUREID diffuseTexId;
  float cameraToGravity;

  ShaderMaterial *material;
  dynrender::RElem rendElem;
  Vbuffer *vb;
  Ibuffer *ib;
  int prevGlobTmLine0VarId;
  int prevGlobTmLine1VarId;
  int prevGlobTmLine2VarId;
  int prevGlobTmLine3VarId;
  int sizeScaleVarId;
  int forwardVarId;
  int posOffsetVarId;
  int velocityVarId;
  int particleBoxVarId;
  int lightingVarId;
  int diffuseTexVarId;
  int occluderPosVarId;
  int occluderHeightVarId;

  unsigned int numPassesToRender;
  float timePassed;
  float alphaFade;
  Tab<Pass> passes;
  int noiseX;
  int noiseY;
  int noiseZ;

  Point3 currentCameraPos[MAX_VIEWS];
  Point3 prevCameraPos[MAX_VIEWS];
  TMatrix4 currentGlobTm[MAX_VIEWS];
  TMatrix4 prevGlobTm[MAX_VIEWS];

  void setConstants(uint32_t view_id);
  void setPassConstants(unsigned int pass_no, float alpha);
};
