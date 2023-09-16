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
#include <stdFxShaderParams_decl.h>

class AnimPlaneCurveTime
{
public:
  real timeScale;
  real minTimeOffset;
  real maxTimeOffset;
  int outType;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    timeScale = readType<real>(ptr, len);
    minTimeOffset = readType<real>(ptr, len);
    maxTimeOffset = readType<real>(ptr, len);
    outType = readType<int>(ptr, len);
  }
};

class AnimPlaneScalar
{
public:
  real scale;
  CubicCurveSampler func;
  AnimPlaneCurveTime time;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    scale = readType<real>(ptr, len);
    func.load(ptr, len);
    time.load(ptr, len, load_cb);
  }
};

class AnimPlaneNormScalar
{
public:
  CubicCurveSampler func;
  AnimPlaneCurveTime time;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    func.load(ptr, len);
    time.load(ptr, len, load_cb);
  }
};

class AnimPlaneRgb
{
public:
  CubicCurveSampler rFunc;
  CubicCurveSampler gFunc;
  CubicCurveSampler bFunc;
  AnimPlaneCurveTime time;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    rFunc.load(ptr, len);
    gFunc.load(ptr, len);
    bFunc.load(ptr, len);
    time.load(ptr, len, load_cb);
  }
};

class AnimPlaneColor
{
public:
  E3DCOLOR color;
  real fakeHdrBrightness;
  AnimPlaneNormScalar alpha;
  AnimPlaneNormScalar brightness;
  AnimPlaneRgb rgb;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    color = readType<E3DCOLOR>(ptr, len);
    fakeHdrBrightness = readType<real>(ptr, len);
    alpha.load(ptr, len, load_cb);
    brightness.load(ptr, len, load_cb);
    rgb.load(ptr, len, load_cb);
  }
};

class AnimPlaneTc
{
public:
  real uOffset;
  real vOffset;
  real uSize;
  real vSize;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    uOffset = readType<real>(ptr, len);
    vOffset = readType<real>(ptr, len);
    uSize = readType<real>(ptr, len);
    vSize = readType<real>(ptr, len);
  }
};

class AnimPlaneLife
{
public:
  real life;
  real lifeVar;
  real delayBefore;
  real delayBeforeVar;
  bool repeated;
  bool canRepeatWithoutEmitter;
  real repeatDelay;
  real repeatDelayVar;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    life = readType<real>(ptr, len);
    lifeVar = readType<real>(ptr, len);
    delayBefore = readType<real>(ptr, len);
    delayBeforeVar = readType<real>(ptr, len);
    repeated = readType<int>(ptr, len);
    canRepeatWithoutEmitter = readType<int>(ptr, len);
    repeatDelay = readType<real>(ptr, len);
    repeatDelayVar = readType<real>(ptr, len);
  }
};

class AnimPlaneParams
{
public:
  AnimPlaneLife life;
  AnimPlaneColor color;
  AnimPlaneScalar size;
  AnimPlaneScalar scaleX;
  AnimPlaneScalar scaleY;
  AnimPlaneScalar rotationX;
  AnimPlaneScalar rotationY;
  AnimPlaneScalar roll;
  AnimPlaneScalar posX;
  AnimPlaneScalar posY;
  AnimPlaneScalar posZ;
  AnimPlaneTc tc;
  bool posFromEmitter;
  bool dirFromEmitter;
  bool dirFromEmitterRandom;
  bool moveWithEmitter;
  real rollOffset;
  real rollOffsetVar;
  bool randomRollSign;
  Point3 posOffset;
  int facingType;
  real viewZOffset;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 5);

    life.load(ptr, len, load_cb);
    color.load(ptr, len, load_cb);
    size.load(ptr, len, load_cb);
    scaleX.load(ptr, len, load_cb);
    scaleY.load(ptr, len, load_cb);
    rotationX.load(ptr, len, load_cb);
    rotationY.load(ptr, len, load_cb);
    roll.load(ptr, len, load_cb);
    posX.load(ptr, len, load_cb);
    posY.load(ptr, len, load_cb);
    posZ.load(ptr, len, load_cb);
    tc.load(ptr, len, load_cb);
    posFromEmitter = readType<int>(ptr, len);
    dirFromEmitter = readType<int>(ptr, len);
    dirFromEmitterRandom = readType<int>(ptr, len);
    moveWithEmitter = readType<int>(ptr, len);
    rollOffset = readType<real>(ptr, len);
    rollOffsetVar = readType<real>(ptr, len);
    randomRollSign = readType<int>(ptr, len);
    posOffset = readType<Point3>(ptr, len);
    facingType = readType<int>(ptr, len);
    viewZOffset = readType<real>(ptr, len);
  }
};

class AnimPlanesFxParams
{
public:
  int seed;
  StdFxShaderParams shader;
  real ltPower;
  real sunLtPower;
  real ambLtPower;
  SmallTab<AnimPlaneParams> array;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    seed = readType<int>(ptr, len);
    shader.load(ptr, len, load_cb);
    ltPower = readType<real>(ptr, len);
    sunLtPower = readType<real>(ptr, len);
    ambLtPower = readType<real>(ptr, len);
    array.resize(readType<int>(ptr, len));
    for (auto &param : array)
      param.load(ptr, len, load_cb);
  }
};
