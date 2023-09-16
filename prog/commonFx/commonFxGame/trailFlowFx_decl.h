// clang-format off  // generated text, do not modify!
#pragma once
#include "readType.h"

#include <math/dag_curveParams.h>
#include <fx/dag_paramScript.h>


#include <fx/dag_baseFxClasses.h>


namespace ScriptHelpers
{
class TunedElement;
};

#include <stdFxShaderParams_decl.h>
#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>

class TrailFlowFxColor
{
public:
  E3DCOLOR color;
  real scale;
  real fakeHdrBrightness;
  CubicCurveSampler rFunc;
  CubicCurveSampler gFunc;
  CubicCurveSampler bFunc;
  CubicCurveSampler aFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    color = readType<E3DCOLOR>(ptr, len);
    scale = readType<real>(ptr, len);
    fakeHdrBrightness = readType<real>(ptr, len);
    rFunc.load(ptr, len);
    gFunc.load(ptr, len);
    bFunc.load(ptr, len);
    aFunc.load(ptr, len);
  }
};

class TrailFlowFxSize
{
public:
  real radius;
  CubicCurveSampler sizeFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    radius = readType<real>(ptr, len);
    sizeFunc.load(ptr, len);
  }
};

class TrailFlowFxParams
{
public:
  int numTrails;
  int numSegs;
  real timeLength;
  int numFrames;
  real tile;
  real scrollSpeed;
  TrailFlowFxColor color;
  TrailFlowFxSize size;
  real life;
  real randomLife;
  real emitterVelK;
  real turbVel;
  real turbFreq;
  real normalVel;
  Point3 startVel;
  real gravity;
  real viscosity;
  int seed;
  StdFxShaderParams shader;
  bool burstMode;
  real amountScale;
  real ltPower;
  real sunLtPower;
  real ambLtPower;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    numTrails = readType<int>(ptr, len);
    numSegs = readType<int>(ptr, len);
    timeLength = readType<real>(ptr, len);
    numFrames = readType<int>(ptr, len);
    tile = readType<real>(ptr, len);
    scrollSpeed = readType<real>(ptr, len);
    color.load(ptr, len, load_cb);
    size.load(ptr, len, load_cb);
    life = readType<real>(ptr, len);
    randomLife = readType<real>(ptr, len);
    emitterVelK = readType<real>(ptr, len);
    turbVel = readType<real>(ptr, len);
    turbFreq = readType<real>(ptr, len);
    normalVel = readType<real>(ptr, len);
    startVel = readType<Point3>(ptr, len);
    gravity = readType<real>(ptr, len);
    viscosity = readType<real>(ptr, len);
    seed = readType<int>(ptr, len);
    shader.load(ptr, len, load_cb);
    burstMode = readType<int>(ptr, len);
    amountScale = readType<real>(ptr, len);
    ltPower = readType<real>(ptr, len);
    sunLtPower = readType<real>(ptr, len);
    ambLtPower = readType<real>(ptr, len);
  }
};
