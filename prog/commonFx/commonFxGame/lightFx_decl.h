// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// clang-format off  // generated text, do not modify!
#include "readType.h"

#include <math/dag_Point3.h>
#include <math/dag_curveParams.h>
#include <fx/dag_paramScript.h>


#include <fx/dag_baseFxClasses.h>


namespace ScriptHelpers
{
class TunedElement;
};

#include <lightfxShadow_decl.h>

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

  bool load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION_OPT(ptr, len, 3);

    allow_game_override = readType<int>(ptr, len);
    color = readType<E3DCOLOR>(ptr, len);
    scale = readType<real>(ptr, len);
    if (!rFunc.load(ptr, len))
      return false;
    if (!gFunc.load(ptr, len))
      return false;
    if (!bFunc.load(ptr, len))
      return false;
    if (!aFunc.load(ptr, len))
      return false;
    return true;
  }
};

class LightFxSize
{
public:
  real radius;
  CubicCurveSampler sizeFunc;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  bool load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION_OPT(ptr, len, 2);

    radius = readType<real>(ptr, len);
    if (!sizeFunc.load(ptr, len))
      return false;
    return true;
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
  LightfxShadowParams shadow;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  bool load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION_OPT(ptr, len, 3);

    phaseTime = readType<real>(ptr, len);
    burstMode = readType<int>(ptr, len);
    cloudLight = readType<int>(ptr, len);
    if (!color.load(ptr, len, load_cb))
      return false;
    if (!size.load(ptr, len, load_cb))
      return false;
    if (!shadow.load(ptr, len, load_cb))
      return false;
    return true;
  }
};
