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


class LightfxShadowParams
{
public:
  bool enabled;
  bool is_dynamic_light;
  bool shadows_for_dynamic_objects;
  bool shadows_for_gpu_objects;
  int quality;
  int shrink;
  int priority;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    is_dynamic_light = readType<int>(ptr, len);
    shadows_for_dynamic_objects = readType<int>(ptr, len);
    shadows_for_gpu_objects = readType<int>(ptr, len);
    quality = readType<int>(ptr, len);
    shrink = readType<int>(ptr, len);
    priority = readType<int>(ptr, len);
  }
};

class LightfxParams
{
public:
  int ref_slot;
  bool allow_game_override;
  Point3 offset;
  real mod_scale;
  real mod_radius;
  real life_time;
  real fade_time;
  LightfxShadowParams shadow;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    ref_slot = readType<int>(ptr, len);
    allow_game_override = readType<int>(ptr, len);
    offset = readType<Point3>(ptr, len);
    mod_scale = readType<real>(ptr, len);
    mod_radius = readType<real>(ptr, len);
    life_time = readType<real>(ptr, len);
    fade_time = readType<real>(ptr, len);
    shadow.load(ptr, len, load_cb);
  }
};

class ModFxQuality
{
public:
  bool low_quality;
  bool medium_quality;
  bool high_quality;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    low_quality = readType<int>(ptr, len);
    medium_quality = readType<int>(ptr, len);
    high_quality = readType<int>(ptr, len);
  }
};

class ModfxParams
{
public:
  int ref_slot;
  Point3 offset;
  Point3 scale;
  Point3 rotation;
  real delay;
  real distance_lag;
  real mod_part_count;
  real mod_life;
  real mod_radius;
  real mod_rotation_speed;
  real mod_velocity_start;
  real mod_velocity_add;
  real mod_velocity_drag;
  real mod_velocity_drag_to_rad;
  real mod_velocity_mass;
  E3DCOLOR mod_color;
  real global_life_time_min;
  real global_life_time_max;
  int transform_type;
  int render_group;
  ModFxQuality quality;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 8);

    ref_slot = readType<int>(ptr, len);
    offset = readType<Point3>(ptr, len);
    scale = readType<Point3>(ptr, len);
    rotation = readType<Point3>(ptr, len);
    delay = readType<real>(ptr, len);
    distance_lag = readType<real>(ptr, len);
    mod_part_count = readType<real>(ptr, len);
    mod_life = readType<real>(ptr, len);
    mod_radius = readType<real>(ptr, len);
    mod_rotation_speed = readType<real>(ptr, len);
    mod_velocity_start = readType<real>(ptr, len);
    mod_velocity_add = readType<real>(ptr, len);
    mod_velocity_drag = readType<real>(ptr, len);
    mod_velocity_drag_to_rad = readType<real>(ptr, len);
    mod_velocity_mass = readType<real>(ptr, len);
    mod_color = readType<E3DCOLOR>(ptr, len);
    global_life_time_min = readType<real>(ptr, len);
    global_life_time_max = readType<real>(ptr, len);
    transform_type = readType<int>(ptr, len);
    render_group = readType<int>(ptr, len);
    quality.load(ptr, len, load_cb);
  }
};

class CFxGlobalParams
{
public:
  real spawn_range_limit;
  int max_instances;
  int player_reserved;
  real one_point_number;
  real one_point_radius;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    spawn_range_limit = readType<real>(ptr, len);
    max_instances = readType<int>(ptr, len);
    player_reserved = readType<int>(ptr, len);
    one_point_number = readType<real>(ptr, len);
    one_point_radius = readType<real>(ptr, len);
  }
};

class CompModfx
{
public:
  SmallTab<ModfxParams> array;
  real instance_life_time_min;
  real instance_life_time_max;
  real fx_scale_min;
  real fx_scale_max;
  void *fx1;
  void *fx2;
  void *fx3;
  void *fx4;
  void *fx5;
  void *fx6;
  void *fx7;
  void *fx8;
  void *fx9;
  void *fx10;
  void *fx11;
  void *fx12;
  void *fx13;
  void *fx14;
  void *fx15;
  void *fx16;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    array.resize(readType<int>(ptr, len));
    for (auto &param : array)
      param.load(ptr, len, load_cb);
    instance_life_time_min = readType<real>(ptr, len);
    instance_life_time_max = readType<real>(ptr, len);
    fx_scale_min = readType<real>(ptr, len);
    fx_scale_max = readType<real>(ptr, len);
    int fx1_id = readType<int>(ptr, len);
    fx1 = load_cb->getReference(fx1_id);
    if (fx1 == nullptr && load_cb->getBrokenRefName(fx1_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx1", load_cb->getBrokenRefName(fx1_id));
    int fx2_id = readType<int>(ptr, len);
    fx2 = load_cb->getReference(fx2_id);
    if (fx2 == nullptr && load_cb->getBrokenRefName(fx2_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx2", load_cb->getBrokenRefName(fx2_id));
    int fx3_id = readType<int>(ptr, len);
    fx3 = load_cb->getReference(fx3_id);
    if (fx3 == nullptr && load_cb->getBrokenRefName(fx3_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx3", load_cb->getBrokenRefName(fx3_id));
    int fx4_id = readType<int>(ptr, len);
    fx4 = load_cb->getReference(fx4_id);
    if (fx4 == nullptr && load_cb->getBrokenRefName(fx4_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx4", load_cb->getBrokenRefName(fx4_id));
    int fx5_id = readType<int>(ptr, len);
    fx5 = load_cb->getReference(fx5_id);
    if (fx5 == nullptr && load_cb->getBrokenRefName(fx5_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx5", load_cb->getBrokenRefName(fx5_id));
    int fx6_id = readType<int>(ptr, len);
    fx6 = load_cb->getReference(fx6_id);
    if (fx6 == nullptr && load_cb->getBrokenRefName(fx6_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx6", load_cb->getBrokenRefName(fx6_id));
    int fx7_id = readType<int>(ptr, len);
    fx7 = load_cb->getReference(fx7_id);
    if (fx7 == nullptr && load_cb->getBrokenRefName(fx7_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx7", load_cb->getBrokenRefName(fx7_id));
    int fx8_id = readType<int>(ptr, len);
    fx8 = load_cb->getReference(fx8_id);
    if (fx8 == nullptr && load_cb->getBrokenRefName(fx8_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx8", load_cb->getBrokenRefName(fx8_id));
    int fx9_id = readType<int>(ptr, len);
    fx9 = load_cb->getReference(fx9_id);
    if (fx9 == nullptr && load_cb->getBrokenRefName(fx9_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx9", load_cb->getBrokenRefName(fx9_id));
    int fx10_id = readType<int>(ptr, len);
    fx10 = load_cb->getReference(fx10_id);
    if (fx10 == nullptr && load_cb->getBrokenRefName(fx10_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx10", load_cb->getBrokenRefName(fx10_id));
    int fx11_id = readType<int>(ptr, len);
    fx11 = load_cb->getReference(fx11_id);
    if (fx11 == nullptr && load_cb->getBrokenRefName(fx11_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx11", load_cb->getBrokenRefName(fx11_id));
    int fx12_id = readType<int>(ptr, len);
    fx12 = load_cb->getReference(fx12_id);
    if (fx12 == nullptr && load_cb->getBrokenRefName(fx12_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx12", load_cb->getBrokenRefName(fx12_id));
    int fx13_id = readType<int>(ptr, len);
    fx13 = load_cb->getReference(fx13_id);
    if (fx13 == nullptr && load_cb->getBrokenRefName(fx13_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx13", load_cb->getBrokenRefName(fx13_id));
    int fx14_id = readType<int>(ptr, len);
    fx14 = load_cb->getReference(fx14_id);
    if (fx14 == nullptr && load_cb->getBrokenRefName(fx14_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx14", load_cb->getBrokenRefName(fx14_id));
    int fx15_id = readType<int>(ptr, len);
    fx15 = load_cb->getReference(fx15_id);
    if (fx15 == nullptr && load_cb->getBrokenRefName(fx15_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx15", load_cb->getBrokenRefName(fx15_id));
    int fx16_id = readType<int>(ptr, len);
    fx16 = load_cb->getReference(fx16_id);
    if (fx16 == nullptr && load_cb->getBrokenRefName(fx16_id))
      G_ASSERTF(0, "dafx compound: invalid sub fx reference %s for fx16", load_cb->getBrokenRefName(fx16_id));
  }
};
