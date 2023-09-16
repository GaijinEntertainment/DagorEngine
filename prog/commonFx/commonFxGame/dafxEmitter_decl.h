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


class DafxEmitterParams
{
public:
  int type;
  int count;
  real life;
  int cycles;
  real period;
  real delay;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    type = readType<int>(ptr, len);
    count = readType<int>(ptr, len);
    life = readType<real>(ptr, len);
    cycles = readType<int>(ptr, len);
    period = readType<real>(ptr, len);
    delay = readType<real>(ptr, len);
  }
};
