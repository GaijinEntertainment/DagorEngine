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


class StdFxShaderParams
{
public:
  void *texture;
  int shader;
  int shaderType;
  bool sorted;
  bool useColorMult;
  bool distortion;
  real softnessDistance;
  bool waterProjection;
  bool waterProjectionOnly;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 6);

    texture = load_cb->getReference(readType<int>(ptr, len));
    shader = readType<int>(ptr, len);
    shaderType = readType<int>(ptr, len);
    sorted = readType<int>(ptr, len);
    useColorMult = readType<int>(ptr, len);
    distortion = readType<int>(ptr, len);
    softnessDistance = readType<real>(ptr, len);
    waterProjection = readType<int>(ptr, len);
    waterProjectionOnly = readType<int>(ptr, len);
  }
};
