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

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>

class FlowPsColor2
{
public:
  E3DCOLOR color;
  E3DCOLOR color2;
  real scale;
  real fakeHdrBrightness;
  CubicCurveSampler rFunc;
  CubicCurveSampler gFunc;
  CubicCurveSampler bFunc;
  CubicCurveSampler aFunc;
  CubicCurveSampler specFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    color = readType<E3DCOLOR>(ptr, len);
    color2 = readType<E3DCOLOR>(ptr, len);
    scale = readType<real>(ptr, len);
    fakeHdrBrightness = readType<real>(ptr, len);
    rFunc.load(ptr, len);
    gFunc.load(ptr, len);
    bFunc.load(ptr, len);
    aFunc.load(ptr, len);
    specFunc.load(ptr, len);
  }
};

class FlowPsSize2
{
public:
  real radius;
  real lengthDt;
  CubicCurveSampler sizeFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    radius = readType<real>(ptr, len);
    lengthDt = readType<real>(ptr, len);
    sizeFunc.load(ptr, len);
  }
};

class FlowPsParams2
{
public:
  int framesX;
  int framesY;
  FlowPsColor2 color;
  FlowPsSize2 size;
  real life;
  real randomLife;
  real randomVel;
  real normalVel;
  Point3 startVel;
  real gravity;
  real viscosity;
  real randomRot;
  real rotSpeed;
  real rotViscosity;
  real randomPhase;
  real animSpeed;
  int numParts;
  int seed;
  void *texture;
  void *normal;
  int shader;
  int shaderType;
  bool sorted;
  bool useColorMult;
  int colorMultMode;
  bool burstMode;
  real amountScale;
  real turbulenceScale;
  real turbulenceTimeScale;
  real turbulenceMultiplier;
  real windScale;
  real ltPower;
  real sunLtPower;
  real ambLtPower;
  bool distortion;
  real softnessDistance;
  bool waterProjection;
  bool waterProjectionOnly;
  bool airFriction;
  bool simulateInLocalSpace;
  bool animatedFlipbook;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 14);

    framesX = readType<int>(ptr, len);
    framesY = readType<int>(ptr, len);
    color.load(ptr, len, load_cb);
    size.load(ptr, len, load_cb);
    life = readType<real>(ptr, len);
    randomLife = readType<real>(ptr, len);
    randomVel = readType<real>(ptr, len);
    normalVel = readType<real>(ptr, len);
    startVel = readType<Point3>(ptr, len);
    gravity = readType<real>(ptr, len);
    viscosity = readType<real>(ptr, len);
    randomRot = readType<real>(ptr, len);
    rotSpeed = readType<real>(ptr, len);
    rotViscosity = readType<real>(ptr, len);
    randomPhase = readType<real>(ptr, len);
    animSpeed = readType<real>(ptr, len);
    numParts = readType<int>(ptr, len);
    seed = readType<int>(ptr, len);
    texture = load_cb->getReference(readType<int>(ptr, len));
    normal = load_cb->getReference(readType<int>(ptr, len));
    shader = readType<int>(ptr, len);
    shaderType = readType<int>(ptr, len);
    sorted = readType<int>(ptr, len);
    useColorMult = readType<int>(ptr, len);
    colorMultMode = readType<int>(ptr, len);
    burstMode = readType<int>(ptr, len);
    amountScale = readType<real>(ptr, len);
    turbulenceScale = readType<real>(ptr, len);
    turbulenceTimeScale = readType<real>(ptr, len);
    turbulenceMultiplier = readType<real>(ptr, len);
    windScale = readType<real>(ptr, len);
    ltPower = readType<real>(ptr, len);
    sunLtPower = readType<real>(ptr, len);
    ambLtPower = readType<real>(ptr, len);
    distortion = readType<int>(ptr, len);
    softnessDistance = readType<real>(ptr, len);
    waterProjection = readType<int>(ptr, len);
    waterProjectionOnly = readType<int>(ptr, len);
    airFriction = readType<int>(ptr, len);
    simulateInLocalSpace = readType<int>(ptr, len);
    animatedFlipbook = readType<int>(ptr, len);
  }
};
