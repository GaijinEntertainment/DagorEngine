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

class LightFxColor
{
public:
  bool allow_game_override;
  E3DCOLOR color;
  real scale;
  CubicCurveSampler rFunc;
  CubicCurveSampler gFunc;
  CubicCurveSampler bFunc;
  CubicCurveSampler aFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    allow_game_override = readType<int>(ptr, len);
    color = readType<E3DCOLOR>(ptr, len);
    scale = readType<real>(ptr, len);
    rFunc.load(ptr, len);
    gFunc.load(ptr, len);
    bFunc.load(ptr, len);
    aFunc.load(ptr, len);
  }
};

class LightFxSize
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

class LightFxParams
{
public:
  real phaseTime;
  bool burstMode;
  bool cloudLight;
  LightFxColor color;
  LightFxSize size;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    phaseTime = readType<real>(ptr, len);
    burstMode = readType<int>(ptr, len);
    cloudLight = readType<int>(ptr, len);
    color.load(ptr, len, load_cb);
    size.load(ptr, len, load_cb);
  }
};
