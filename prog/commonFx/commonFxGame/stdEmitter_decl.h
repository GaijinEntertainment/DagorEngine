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


class StdEmitterParams
{
public:
  real radius;
  real height;
  real width;
  real depth;
  bool isVolumetric;
  bool isTrail;
  int geometry;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    radius = readType<real>(ptr, len);
    height = readType<real>(ptr, len);
    width = readType<real>(ptr, len);
    depth = readType<real>(ptr, len);
    isVolumetric = readType<int>(ptr, len);
    isTrail = readType<int>(ptr, len);
    geometry = readType<int>(ptr, len);
  }
};
