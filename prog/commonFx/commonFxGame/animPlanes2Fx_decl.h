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

class AnimPlane2CurveTime
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

class AnimPlane2Scalar
{
public:
  real scale;
  CubicCurveSampler func;
  AnimPlane2CurveTime time;


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

class AnimPlane2NormScalar
{
public:
  CubicCurveSampler func;
  AnimPlane2CurveTime time;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    func.load(ptr, len);
    time.load(ptr, len, load_cb);
  }
};

class AnimPlane2Rgb
{
public:
  CubicCurveSampler rFunc;
  CubicCurveSampler gFunc;
  CubicCurveSampler bFunc;
  AnimPlane2CurveTime time;


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

class AnimPlane2Color
{
public:
  E3DCOLOR color;
  real fakeHdrBrightness;
  AnimPlane2NormScalar alpha;
  AnimPlane2NormScalar brightness;
  AnimPlane2Rgb rgb;


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

class AnimPlane2Tc
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

class AnimPlane2Life
{
public:
  real life;
  real lifeVar;
  real delayBefore;
  real delayBeforeVar;
  bool repeated;
  real repeatDelay;
  real repeatDelayVar;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    life = readType<real>(ptr, len);
    lifeVar = readType<real>(ptr, len);
    delayBefore = readType<real>(ptr, len);
    delayBeforeVar = readType<real>(ptr, len);
    repeated = readType<int>(ptr, len);
    repeatDelay = readType<real>(ptr, len);
    repeatDelayVar = readType<real>(ptr, len);
  }
};

class AnimPlane2Params
{
public:
  AnimPlane2Life life;
  AnimPlane2Color color;
  AnimPlane2Scalar size;
  AnimPlane2Scalar scaleX;
  AnimPlane2Scalar scaleY;
  AnimPlane2Scalar rotationX;
  AnimPlane2Scalar rotationY;
  AnimPlane2Scalar roll;
  AnimPlane2Scalar posX;
  AnimPlane2Scalar posY;
  AnimPlane2Scalar posZ;
  AnimPlane2Tc tc;
  bool posFromEmitter;
  bool dirFromEmitter;
  bool moveWithEmitter;
  real rollOffset;
  real rollOffsetVar;
  bool randomRollSign;
  Point3 posOffset;
  int facingType;
  real viewZOffset;
  int framesX;
  int framesY;
  real animSpeed;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

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
    moveWithEmitter = readType<int>(ptr, len);
    rollOffset = readType<real>(ptr, len);
    rollOffsetVar = readType<real>(ptr, len);
    randomRollSign = readType<int>(ptr, len);
    posOffset = readType<Point3>(ptr, len);
    facingType = readType<int>(ptr, len);
    viewZOffset = readType<real>(ptr, len);
    framesX = readType<int>(ptr, len);
    framesY = readType<int>(ptr, len);
    animSpeed = readType<real>(ptr, len);
  }
};

class AnimPlanes2FxParams
{
public:
  int seed;
  StdFxShaderParams shader;
  real ltPower;
  real sunLtPower;
  real ambLtPower;
  SmallTab<AnimPlane2Params> array;


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
