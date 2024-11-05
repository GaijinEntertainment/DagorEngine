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


class DafxEmitterDistanceBased
{
public:
  int elem_limit;
  real distance;
  real idle_period;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    elem_limit = readType<int>(ptr, len);
    distance = readType<real>(ptr, len);
    idle_period = readType<real>(ptr, len);
  }
};

class DafxEmitterParams
{
public:
  int type;
  int count;
  real life;
  int cycles;
  real period;
  real delay;
  DafxEmitterDistanceBased distance_based;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    type = readType<int>(ptr, len);
    count = readType<int>(ptr, len);
    life = readType<real>(ptr, len);
    cycles = readType<int>(ptr, len);
    period = readType<real>(ptr, len);
    delay = readType<real>(ptr, len);
    distance_based.load(ptr, len, load_cb);
  }
};
