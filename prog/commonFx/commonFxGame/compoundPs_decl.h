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

class CompoundPsParams
{
public:
  void *fx1;
  Point3 fx1_offset;
  Point3 fx1_scale;
  Point3 fx1_rot;
  Point3 fx1_rot_speed;
  real fx1_delay;
  real fx1_emitter_lifetime;
  void *fx2;
  Point3 fx2_offset;
  Point3 fx2_scale;
  Point3 fx2_rot;
  Point3 fx2_rot_speed;
  real fx2_delay;
  real fx2_emitter_lifetime;
  void *fx3;
  Point3 fx3_offset;
  Point3 fx3_scale;
  Point3 fx3_rot;
  Point3 fx3_rot_speed;
  real fx3_delay;
  real fx3_emitter_lifetime;
  void *fx4;
  Point3 fx4_offset;
  Point3 fx4_scale;
  Point3 fx4_rot;
  Point3 fx4_rot_speed;
  real fx4_delay;
  real fx4_emitter_lifetime;
  void *fx5;
  Point3 fx5_offset;
  Point3 fx5_scale;
  Point3 fx5_rot;
  Point3 fx5_rot_speed;
  real fx5_delay;
  real fx5_emitter_lifetime;
  void *fx6;
  Point3 fx6_offset;
  Point3 fx6_scale;
  Point3 fx6_rot;
  Point3 fx6_rot_speed;
  real fx6_delay;
  real fx6_emitter_lifetime;
  void *fx7;
  Point3 fx7_offset;
  Point3 fx7_scale;
  Point3 fx7_rot;
  Point3 fx7_rot_speed;
  real fx7_delay;
  real fx7_emitter_lifetime;
  void *fx8;
  Point3 fx8_offset;
  Point3 fx8_scale;
  Point3 fx8_rot;
  Point3 fx8_rot_speed;
  real fx8_delay;
  real fx8_emitter_lifetime;
  bool useCommonEmitter;
  real ltPower;
  real sunLtPower;
  real ambLtPower;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 8);

    int fx1_id = readType<int>(ptr, len);
    fx1 = load_cb->getReference(fx1_id);
    if (fx1 == nullptr && load_cb->getBrokenRefName(fx1_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx1", load_cb->getBrokenRefName(fx1_id));
    fx1_offset = readType<Point3>(ptr, len);
    fx1_scale = readType<Point3>(ptr, len);
    fx1_rot = readType<Point3>(ptr, len);
    fx1_rot_speed = readType<Point3>(ptr, len);
    fx1_delay = readType<real>(ptr, len);
    fx1_emitter_lifetime = readType<real>(ptr, len);
    int fx2_id = readType<int>(ptr, len);
    fx2 = load_cb->getReference(fx2_id);
    if (fx2 == nullptr && load_cb->getBrokenRefName(fx2_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx2", load_cb->getBrokenRefName(fx2_id));
    fx2_offset = readType<Point3>(ptr, len);
    fx2_scale = readType<Point3>(ptr, len);
    fx2_rot = readType<Point3>(ptr, len);
    fx2_rot_speed = readType<Point3>(ptr, len);
    fx2_delay = readType<real>(ptr, len);
    fx2_emitter_lifetime = readType<real>(ptr, len);
    int fx3_id = readType<int>(ptr, len);
    fx3 = load_cb->getReference(fx3_id);
    if (fx3 == nullptr && load_cb->getBrokenRefName(fx3_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx3", load_cb->getBrokenRefName(fx3_id));
    fx3_offset = readType<Point3>(ptr, len);
    fx3_scale = readType<Point3>(ptr, len);
    fx3_rot = readType<Point3>(ptr, len);
    fx3_rot_speed = readType<Point3>(ptr, len);
    fx3_delay = readType<real>(ptr, len);
    fx3_emitter_lifetime = readType<real>(ptr, len);
    int fx4_id = readType<int>(ptr, len);
    fx4 = load_cb->getReference(fx4_id);
    if (fx4 == nullptr && load_cb->getBrokenRefName(fx4_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx4", load_cb->getBrokenRefName(fx4_id));
    fx4_offset = readType<Point3>(ptr, len);
    fx4_scale = readType<Point3>(ptr, len);
    fx4_rot = readType<Point3>(ptr, len);
    fx4_rot_speed = readType<Point3>(ptr, len);
    fx4_delay = readType<real>(ptr, len);
    fx4_emitter_lifetime = readType<real>(ptr, len);
    int fx5_id = readType<int>(ptr, len);
    fx5 = load_cb->getReference(fx5_id);
    if (fx5 == nullptr && load_cb->getBrokenRefName(fx5_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx5", load_cb->getBrokenRefName(fx5_id));
    fx5_offset = readType<Point3>(ptr, len);
    fx5_scale = readType<Point3>(ptr, len);
    fx5_rot = readType<Point3>(ptr, len);
    fx5_rot_speed = readType<Point3>(ptr, len);
    fx5_delay = readType<real>(ptr, len);
    fx5_emitter_lifetime = readType<real>(ptr, len);
    int fx6_id = readType<int>(ptr, len);
    fx6 = load_cb->getReference(fx6_id);
    if (fx6 == nullptr && load_cb->getBrokenRefName(fx6_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx6", load_cb->getBrokenRefName(fx6_id));
    fx6_offset = readType<Point3>(ptr, len);
    fx6_scale = readType<Point3>(ptr, len);
    fx6_rot = readType<Point3>(ptr, len);
    fx6_rot_speed = readType<Point3>(ptr, len);
    fx6_delay = readType<real>(ptr, len);
    fx6_emitter_lifetime = readType<real>(ptr, len);
    int fx7_id = readType<int>(ptr, len);
    fx7 = load_cb->getReference(fx7_id);
    if (fx7 == nullptr && load_cb->getBrokenRefName(fx7_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx7", load_cb->getBrokenRefName(fx7_id));
    fx7_offset = readType<Point3>(ptr, len);
    fx7_scale = readType<Point3>(ptr, len);
    fx7_rot = readType<Point3>(ptr, len);
    fx7_rot_speed = readType<Point3>(ptr, len);
    fx7_delay = readType<real>(ptr, len);
    fx7_emitter_lifetime = readType<real>(ptr, len);
    int fx8_id = readType<int>(ptr, len);
    fx8 = load_cb->getReference(fx8_id);
    if (fx8 == nullptr && load_cb->getBrokenRefName(fx8_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx8", load_cb->getBrokenRefName(fx8_id));
    fx8_offset = readType<Point3>(ptr, len);
    fx8_scale = readType<Point3>(ptr, len);
    fx8_rot = readType<Point3>(ptr, len);
    fx8_rot_speed = readType<Point3>(ptr, len);
    fx8_delay = readType<real>(ptr, len);
    fx8_emitter_lifetime = readType<real>(ptr, len);
    useCommonEmitter = readType<int>(ptr, len);
    ltPower = readType<real>(ptr, len);
    sunLtPower = readType<real>(ptr, len);
    ambLtPower = readType<real>(ptr, len);
  }
};
