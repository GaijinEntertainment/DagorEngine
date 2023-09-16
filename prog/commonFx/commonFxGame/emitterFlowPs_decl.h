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

class EmitterFlowPsParams
{
public:
  void *fx1;
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
  int numParts;
  int seed;
  real amountScale;
  real turbulenceScale;
  real turbulenceTimeScale;
  real turbulenceMultiplier;
  real windScale;
  bool burstMode;
  real ltPower;
  real sunLtPower;
  real ambLtPower;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    int fx1_id = readType<int>(ptr, len);
    fx1 = load_cb->getReference(fx1_id);
    if (fx1 == nullptr && load_cb->getBrokenRefName(fx1_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx1", load_cb->getBrokenRefName(fx1_id));
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
    numParts = readType<int>(ptr, len);
    seed = readType<int>(ptr, len);
    amountScale = readType<real>(ptr, len);
    turbulenceScale = readType<real>(ptr, len);
    turbulenceTimeScale = readType<real>(ptr, len);
    turbulenceMultiplier = readType<real>(ptr, len);
    windScale = readType<real>(ptr, len);
    burstMode = readType<int>(ptr, len);
    ltPower = readType<real>(ptr, len);
    sunLtPower = readType<real>(ptr, len);
    ambLtPower = readType<real>(ptr, len);
  }
};
