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


class LightfxShadowParams
{
public:
  bool enabled;
  bool is_dynamic_light;
  bool shadows_for_dynamic_objects;
  bool shadows_for_gpu_objects;
  bool sdf_shadows;
  int quality;
  int shrink;
  int priority;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  bool load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION_OPT(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    is_dynamic_light = readType<int>(ptr, len);
    shadows_for_dynamic_objects = readType<int>(ptr, len);
    shadows_for_gpu_objects = readType<int>(ptr, len);
    sdf_shadows = readType<int>(ptr, len);
    quality = readType<int>(ptr, len);
    shrink = readType<int>(ptr, len);
    priority = readType<int>(ptr, len);
    return true;
  }
};
