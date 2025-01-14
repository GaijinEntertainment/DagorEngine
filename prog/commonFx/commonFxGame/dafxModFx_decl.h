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

#include <math/dag_gradientBoxParams.h>

class FxValueGradOpt
{
public:
  bool enabled;
  bool use_part_idx_as_key;
  GradientBoxSampler grad;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    enabled = readType<int>(ptr, len);
    use_part_idx_as_key = readType<int>(ptr, len);
    grad.load(ptr, len);
  }
};

class FxValueCurveOpt
{
public:
  bool enabled;
  CubicCurveSampler curve;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    curve.load(ptr, len);
  }
};

class FxSpawnFixed
{
public:
  int count;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    count = readType<int>(ptr, len);
  }
};

class FxSpawnLinear
{
public:
  int count_min;
  int count_max;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    count_min = readType<int>(ptr, len);
    count_max = readType<int>(ptr, len);
  }
};

class FxSpawnBurst
{
public:
  int count_min;
  int count_max;
  int cycles;
  real period;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    count_min = readType<int>(ptr, len);
    count_max = readType<int>(ptr, len);
    cycles = readType<int>(ptr, len);
    period = readType<real>(ptr, len);
  }
};

class FxDistanceBased
{
public:
  int elem_limit;
  real distance;
  real idle_period;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    elem_limit = readType<int>(ptr, len);
    distance = readType<real>(ptr, len);
    idle_period = readType<real>(ptr, len);
  }
};

class FxSpawn
{
public:
  int type;
  FxSpawnLinear linear;
  FxSpawnBurst burst;
  FxDistanceBased distance_based;
  FxSpawnFixed fixed;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    type = readType<int>(ptr, len);
    linear.load(ptr, len, load_cb);
    burst.load(ptr, len, load_cb);
    distance_based.load(ptr, len, load_cb);
    fixed.load(ptr, len, load_cb);
  }
};

class FxLife
{
public:
  real part_life_min;
  real part_life_max;
  real part_life_rnd_offset;
  real inst_life_delay;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    part_life_min = readType<real>(ptr, len);
    part_life_max = readType<real>(ptr, len);
    part_life_rnd_offset = readType<real>(ptr, len);
    inst_life_delay = readType<real>(ptr, len);
  }
};

class FxInitPosSphere
{
public:
  real radius;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    radius = readType<real>(ptr, len);
  }
};

class FxInitPosSphereSector
{
public:
  Point3 vec;
  real radius;
  real sector;
  real random_burst;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    vec = readType<Point3>(ptr, len);
    radius = readType<real>(ptr, len);
    sector = readType<real>(ptr, len);
    random_burst = readType<real>(ptr, len);
  }
};

class FxInitPosCylinder
{
public:
  Point3 vec;
  real radius;
  real height;
  real random_burst;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    vec = readType<Point3>(ptr, len);
    radius = readType<real>(ptr, len);
    height = readType<real>(ptr, len);
    random_burst = readType<real>(ptr, len);
  }
};

class FxInitPosBox
{
public:
  real width;
  real height;
  real depth;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    width = readType<real>(ptr, len);
    height = readType<real>(ptr, len);
    depth = readType<real>(ptr, len);
  }
};

class FxInitPosCone
{
public:
  Point3 vec;
  real width_top;
  real width_bottom;
  real height;
  real random_burst;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    vec = readType<Point3>(ptr, len);
    width_top = readType<real>(ptr, len);
    width_bottom = readType<real>(ptr, len);
    height = readType<real>(ptr, len);
    random_burst = readType<real>(ptr, len);
  }
};

class FxInitGpuPlacement
{
public:
  bool enabled;
  real placement_threshold;
  bool use_hmap;
  bool use_depth_above;
  bool use_water;
  bool use_water_flowmap;
  bool use_normal;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    placement_threshold = readType<real>(ptr, len);
    use_hmap = readType<int>(ptr, len);
    use_depth_above = readType<int>(ptr, len);
    use_water = readType<int>(ptr, len);
    use_water_flowmap = readType<int>(ptr, len);
    use_normal = readType<int>(ptr, len);
  }
};

class FxPos
{
public:
  bool enabled;
  bool attach_last_part_to_emitter;
  int type;
  real volume;
  Point3 offset;
  FxInitPosSphere sphere;
  FxInitPosCylinder cylinder;
  FxInitPosCone cone;
  FxInitPosBox box;
  FxInitPosSphereSector sphere_sector;
  FxInitGpuPlacement gpu_placement;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 6);

    enabled = readType<int>(ptr, len);
    attach_last_part_to_emitter = readType<int>(ptr, len);
    type = readType<int>(ptr, len);
    volume = readType<real>(ptr, len);
    offset = readType<Point3>(ptr, len);
    sphere.load(ptr, len, load_cb);
    cylinder.load(ptr, len, load_cb);
    cone.load(ptr, len, load_cb);
    box.load(ptr, len, load_cb);
    sphere_sector.load(ptr, len, load_cb);
    gpu_placement.load(ptr, len, load_cb);
  }
};

class FxRadius
{
public:
  bool enabled;
  real rad_min;
  real rad_max;
  bool apply_emitter_transform;
  FxValueCurveOpt over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    rad_min = readType<real>(ptr, len);
    rad_max = readType<real>(ptr, len);
    apply_emitter_transform = readType<int>(ptr, len);
    over_part_life.load(ptr, len, load_cb);
  }
};

class FxColorCurveOpt
{
public:
  bool enabled;
  E3DCOLOR mask;
  bool use_part_idx_as_key;
  bool use_threshold;
  CubicCurveSampler curve;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    enabled = readType<int>(ptr, len);
    mask = readType<E3DCOLOR>(ptr, len);
    use_part_idx_as_key = readType<int>(ptr, len);
    use_threshold = readType<int>(ptr, len);
    curve.load(ptr, len);
  }
};

class FxMaskedCurveOpt
{
public:
  bool enabled;
  E3DCOLOR mask;
  CubicCurveSampler curve;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    mask = readType<E3DCOLOR>(ptr, len);
    curve.load(ptr, len);
  }
};

class FxAlphaByVelocity
{
public:
  bool enabled;
  real vel_min;
  real vel_max;
  bool use_emitter_velocity;
  FxValueCurveOpt velocity_alpha_curve;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    vel_min = readType<real>(ptr, len);
    vel_max = readType<real>(ptr, len);
    use_emitter_velocity = readType<int>(ptr, len);
    velocity_alpha_curve.load(ptr, len, load_cb);
  }
};

class FxColor
{
public:
  bool enabled;
  bool allow_game_override;
  bool gamma_correction;
  E3DCOLOR col_min;
  E3DCOLOR col_max;
  FxValueGradOpt grad_over_part_life;
  FxColorCurveOpt curve_over_part_life;
  FxMaskedCurveOpt curve_over_part_idx;
  FxAlphaByVelocity alpha_by_velocity;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 5);

    enabled = readType<int>(ptr, len);
    allow_game_override = readType<int>(ptr, len);
    gamma_correction = readType<int>(ptr, len);
    col_min = readType<E3DCOLOR>(ptr, len);
    col_max = readType<E3DCOLOR>(ptr, len);
    grad_over_part_life.load(ptr, len, load_cb);
    curve_over_part_life.load(ptr, len, load_cb);
    curve_over_part_idx.load(ptr, len, load_cb);
    alpha_by_velocity.load(ptr, len, load_cb);
  }
};

class FxRotation
{
public:
  bool enabled;
  bool uv_rotation;
  real start_min;
  real start_max;
  bool dynamic;
  real vel_min;
  real vel_max;
  FxValueCurveOpt over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    uv_rotation = readType<int>(ptr, len);
    start_min = readType<real>(ptr, len);
    start_max = readType<real>(ptr, len);
    dynamic = readType<int>(ptr, len);
    vel_min = readType<real>(ptr, len);
    vel_max = readType<real>(ptr, len);
    over_part_life.load(ptr, len, load_cb);
  }
};

class FxInitVelocityVec
{
public:
  Point3 vec;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    vec = readType<Point3>(ptr, len);
  }
};

class FxInitVelocityPoint
{
public:
  Point3 offset;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    offset = readType<Point3>(ptr, len);
  }
};

class FxInitVelocityCone
{
public:
  Point3 vec;
  Point3 offset;
  real width_top;
  real width_bottom;
  real height;
  real center_power;
  real border_power;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    vec = readType<Point3>(ptr, len);
    offset = readType<Point3>(ptr, len);
    width_top = readType<real>(ptr, len);
    width_bottom = readType<real>(ptr, len);
    height = readType<real>(ptr, len);
    center_power = readType<real>(ptr, len);
    border_power = readType<real>(ptr, len);
  }
};

class FxVelocityStart
{
public:
  bool enabled;
  real vel_min;
  real vel_max;
  real vec_rnd;
  int type;
  FxInitVelocityPoint point;
  FxInitVelocityVec vec;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    vel_min = readType<real>(ptr, len);
    vel_max = readType<real>(ptr, len);
    vec_rnd = readType<real>(ptr, len);
    type = readType<int>(ptr, len);
    point.load(ptr, len, load_cb);
    vec.load(ptr, len, load_cb);
  }
};

class FxVelocityAdd
{
public:
  bool enabled;
  bool apply_emitter_transform;
  real vel_min;
  real vel_max;
  real vec_rnd;
  int type;
  FxInitVelocityPoint point;
  FxInitVelocityVec vec;
  FxInitVelocityCone cone;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    apply_emitter_transform = readType<int>(ptr, len);
    vel_min = readType<real>(ptr, len);
    vel_max = readType<real>(ptr, len);
    vec_rnd = readType<real>(ptr, len);
    type = readType<int>(ptr, len);
    point.load(ptr, len, load_cb);
    vec.load(ptr, len, load_cb);
    cone.load(ptr, len, load_cb);
  }
};

class FxCollision
{
public:
  bool enabled;
  real radius_mod;
  real reflect_power;
  real energy_loss;
  real emitter_deadzone;
  real collision_decay;
  bool collide_with_depth;
  bool collide_with_depth_above;
  bool collide_with_hmap;
  bool collide_with_water;
  real collision_fadeout_radius_min;
  real collision_fadeout_radius_max;
  bool stop_rotation_on_collision;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 5);

    enabled = readType<int>(ptr, len);
    radius_mod = readType<real>(ptr, len);
    reflect_power = readType<real>(ptr, len);
    energy_loss = readType<real>(ptr, len);
    emitter_deadzone = readType<real>(ptr, len);
    collision_decay = readType<real>(ptr, len);
    collide_with_depth = readType<int>(ptr, len);
    collide_with_depth_above = readType<int>(ptr, len);
    collide_with_hmap = readType<int>(ptr, len);
    collide_with_water = readType<int>(ptr, len);
    collision_fadeout_radius_min = readType<real>(ptr, len);
    collision_fadeout_radius_max = readType<real>(ptr, len);
    stop_rotation_on_collision = readType<int>(ptr, len);
  }
};

class FxWind
{
public:
  bool enabled;
  real directional_force;
  real directional_freq;
  real turbulence_force;
  real turbulence_freq;
  FxValueCurveOpt atten;
  bool impulse_wind;
  real impulse_wind_force;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    directional_force = readType<real>(ptr, len);
    directional_freq = readType<real>(ptr, len);
    turbulence_force = readType<real>(ptr, len);
    turbulence_freq = readType<real>(ptr, len);
    atten.load(ptr, len, load_cb);
    impulse_wind = readType<int>(ptr, len);
    impulse_wind_force = readType<real>(ptr, len);
  }
};

class FxForceFieldVortex
{
public:
  bool enabled;
  Point3 axis_direction;
  real direction_rnd;
  Point3 axis_position;
  Point3 position_rnd;
  real rotation_speed_min;
  real rotation_speed_max;
  real pull_speed_min;
  real pull_speed_max;
  FxValueCurveOpt rotation_speed_over_part_life;
  FxValueCurveOpt pull_speed_over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    axis_direction = readType<Point3>(ptr, len);
    direction_rnd = readType<real>(ptr, len);
    axis_position = readType<Point3>(ptr, len);
    position_rnd = readType<Point3>(ptr, len);
    rotation_speed_min = readType<real>(ptr, len);
    rotation_speed_max = readType<real>(ptr, len);
    pull_speed_min = readType<real>(ptr, len);
    pull_speed_max = readType<real>(ptr, len);
    rotation_speed_over_part_life.load(ptr, len, load_cb);
    pull_speed_over_part_life.load(ptr, len, load_cb);
  }
};

class FxForceFieldNoise
{
public:
  bool enabled;
  int type;
  real pos_scale;
  real power_scale;
  real power_rnd;
  real power_per_part_rnd;
  FxValueCurveOpt power_curve;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    type = readType<int>(ptr, len);
    pos_scale = readType<real>(ptr, len);
    power_scale = readType<real>(ptr, len);
    power_rnd = readType<real>(ptr, len);
    power_per_part_rnd = readType<real>(ptr, len);
    power_curve.load(ptr, len, load_cb);
  }
};

class FxForceField
{
public:
  FxForceFieldVortex vortex;
  FxForceFieldNoise noise;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    vortex.load(ptr, len, load_cb);
    noise.load(ptr, len, load_cb);
  }
};

class FxVelocityGpuPlacement
{
public:
  bool enabled;
  real height_min;
  real height_max;
  real climb_power;
  real reserved;
  bool use_hmap;
  bool use_depth_above;
  bool use_water;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    height_min = readType<real>(ptr, len);
    height_max = readType<real>(ptr, len);
    climb_power = readType<real>(ptr, len);
    reserved = readType<real>(ptr, len);
    use_hmap = readType<int>(ptr, len);
    use_depth_above = readType<int>(ptr, len);
    use_water = readType<int>(ptr, len);
  }
};

class FxCameraVelocity
{
public:
  bool enabled;
  real velocity_weight;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    velocity_weight = readType<real>(ptr, len);
  }
};

class FxVelocity
{
public:
  bool enabled;
  real mass;
  real drag_coeff;
  real drag_to_rad_k;
  bool apply_gravity;
  bool gravity_transform;
  bool apply_parent_velocity;
  FxVelocityStart start;
  FxVelocityAdd add;
  FxForceField force_field;
  FxValueCurveOpt decay;
  FxValueCurveOpt drag_curve;
  FxCollision collision;
  FxVelocityGpuPlacement gpu_hmap_limiter;
  FxWind wind;
  FxCameraVelocity camera_velocity;
  int gravity_zone;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 14);

    enabled = readType<int>(ptr, len);
    mass = readType<real>(ptr, len);
    drag_coeff = readType<real>(ptr, len);
    drag_to_rad_k = readType<real>(ptr, len);
    apply_gravity = readType<int>(ptr, len);
    gravity_transform = readType<int>(ptr, len);
    apply_parent_velocity = readType<int>(ptr, len);
    start.load(ptr, len, load_cb);
    add.load(ptr, len, load_cb);
    force_field.load(ptr, len, load_cb);
    decay.load(ptr, len, load_cb);
    drag_curve.load(ptr, len, load_cb);
    collision.load(ptr, len, load_cb);
    gpu_hmap_limiter.load(ptr, len, load_cb);
    wind.load(ptr, len, load_cb);
    camera_velocity.load(ptr, len, load_cb);
    gravity_zone = readType<int>(ptr, len);
  }
};

class FxPlacement
{
public:
  bool enabled;
  real placement_threshold;
  bool use_hmap;
  bool use_depth_above;
  bool use_water;
  real align_normals_offset;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    enabled = readType<int>(ptr, len);
    placement_threshold = readType<real>(ptr, len);
    use_hmap = readType<int>(ptr, len);
    use_depth_above = readType<int>(ptr, len);
    use_water = readType<int>(ptr, len);
    align_normals_offset = readType<real>(ptr, len);
  }
};

class FxTextureAnim
{
public:
  bool enabled;
  bool animated_flipbook;
  real speed_min;
  real speed_max;
  FxValueCurveOpt over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    animated_flipbook = readType<int>(ptr, len);
    speed_min = readType<real>(ptr, len);
    speed_max = readType<real>(ptr, len);
    over_part_life.load(ptr, len, load_cb);
  }
};

class FxColorMatrix
{
public:
  bool enabled;
  E3DCOLOR red;
  E3DCOLOR green;
  E3DCOLOR blue;
  E3DCOLOR alpha;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    red = readType<E3DCOLOR>(ptr, len);
    green = readType<E3DCOLOR>(ptr, len);
    blue = readType<E3DCOLOR>(ptr, len);
    alpha = readType<E3DCOLOR>(ptr, len);
  }
};

class FxColorRemapDyanmic
{
public:
  bool enabled;
  real scale;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    scale = readType<real>(ptr, len);
  }
};

class FxColorRemap
{
public:
  bool enabled;
  bool apply_base_color;
  GradientBoxSampler grad;
  FxColorRemapDyanmic dynamic;
  bool second_mask_enabled;
  bool second_mask_apply_base_color;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    enabled = readType<int>(ptr, len);
    apply_base_color = readType<int>(ptr, len);
    grad.load(ptr, len);
    dynamic.load(ptr, len, load_cb);
    second_mask_enabled = readType<int>(ptr, len);
    second_mask_apply_base_color = readType<int>(ptr, len);
  }
};

class FxTexture
{
public:
  bool enabled;
  bool enable_aniso;
  void *tex_0;
  void *tex_1;
  int frames_x;
  int frames_y;
  real frame_scale;
  int start_frame_min;
  int start_frame_max;
  bool flip_x;
  bool flip_y;
  bool random_flip_x;
  bool random_flip_y;
  bool disable_loop;
  FxColorMatrix color_matrix;
  FxColorRemap color_remap;
  FxTextureAnim animation;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    enabled = readType<int>(ptr, len);
    enable_aniso = readType<int>(ptr, len);
    tex_0 = load_cb->getReference(readType<int>(ptr, len));
    tex_1 = load_cb->getReference(readType<int>(ptr, len));
    frames_x = readType<int>(ptr, len);
    frames_y = readType<int>(ptr, len);
    frame_scale = readType<real>(ptr, len);
    start_frame_min = readType<int>(ptr, len);
    start_frame_max = readType<int>(ptr, len);
    flip_x = readType<int>(ptr, len);
    flip_y = readType<int>(ptr, len);
    random_flip_x = readType<int>(ptr, len);
    random_flip_y = readType<int>(ptr, len);
    disable_loop = readType<int>(ptr, len);
    color_matrix.load(ptr, len, load_cb);
    color_remap.load(ptr, len, load_cb);
    animation.load(ptr, len, load_cb);
  }
};

class FxEmission
{
public:
  bool enabled;
  real value;
  E3DCOLOR mask;
  FxValueCurveOpt over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    value = readType<real>(ptr, len);
    mask = readType<E3DCOLOR>(ptr, len);
    over_part_life.load(ptr, len, load_cb);
  }
};

class FxThermalEmission
{
public:
  bool enabled;
  real value;
  FxValueCurveOpt over_part_life;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    value = readType<real>(ptr, len);
    over_part_life.load(ptr, len, load_cb);
  }
};

class FxLighting
{
public:
  int type;
  real translucency;
  real sphere_normal_radius;
  real sphere_normal_power;
  real normal_softness;
  bool specular_enabled;
  real specular_power;
  real specular_strength;
  bool ambient_enabled;
  bool external_lights_enabled;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 5);

    type = readType<int>(ptr, len);
    translucency = readType<real>(ptr, len);
    sphere_normal_radius = readType<real>(ptr, len);
    sphere_normal_power = readType<real>(ptr, len);
    normal_softness = readType<real>(ptr, len);
    specular_enabled = readType<int>(ptr, len);
    specular_power = readType<real>(ptr, len);
    specular_strength = readType<real>(ptr, len);
    ambient_enabled = readType<int>(ptr, len);
    external_lights_enabled = readType<int>(ptr, len);
  }
};

class FxRenderShapeBillboard
{
public:
  int orientation;
  Point3 static_aligned_up_vec;
  Point3 static_aligned_right_vec;
  real cross_fade_mul;
  int cross_fade_pow;
  real cross_fade_threshold;
  Point2 pivot_offset;
  real velocity_to_length;
  Point2 velocity_to_length_clamp;
  Point2 screen_view_clamp;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 9);

    orientation = readType<int>(ptr, len);
    static_aligned_up_vec = readType<Point3>(ptr, len);
    static_aligned_right_vec = readType<Point3>(ptr, len);
    cross_fade_mul = readType<real>(ptr, len);
    cross_fade_pow = readType<int>(ptr, len);
    cross_fade_threshold = readType<real>(ptr, len);
    pivot_offset = readType<Point2>(ptr, len);
    velocity_to_length = readType<real>(ptr, len);
    velocity_to_length_clamp = readType<Point2>(ptr, len);
    screen_view_clamp = readType<Point2>(ptr, len);
  }
};

class FxRenderShapeRibbon
{
public:
  int type;
  int uv_mapping;
  real uv_tile;
  real side_fade_threshold;
  real side_fade_pow;
  real head_fade_threshold;
  real head_fade_pow;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    type = readType<int>(ptr, len);
    uv_mapping = readType<int>(ptr, len);
    uv_tile = readType<real>(ptr, len);
    side_fade_threshold = readType<real>(ptr, len);
    side_fade_pow = readType<real>(ptr, len);
    head_fade_threshold = readType<real>(ptr, len);
    head_fade_pow = readType<real>(ptr, len);
  }
};

class FxRenderShape
{
public:
  real aspect;
  real camera_offset;
  int type;
  FxRenderShapeBillboard billboard;
  FxRenderShapeRibbon ribbon;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    aspect = readType<real>(ptr, len);
    camera_offset = readType<real>(ptr, len);
    type = readType<int>(ptr, len);
    billboard.load(ptr, len, load_cb);
    ribbon.load(ptr, len, load_cb);
  }
};

class FxBlending
{
public:
  int type;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    type = readType<int>(ptr, len);
  }
};

class FxRenderGroup
{
public:
  int type;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    type = readType<int>(ptr, len);
  }
};

class FxRenderShaderDummy
{
public:
  real placeholder;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    placeholder = readType<real>(ptr, len);
  }
};

class FxRenderShaderDistortion
{
public:
  real distortion_strength;
  real distortion_fade_range;
  real distortion_fade_power;
  E3DCOLOR distortion_rgb;
  real distortion_rgb_strength;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 5);

    distortion_strength = readType<real>(ptr, len);
    distortion_fade_range = readType<real>(ptr, len);
    distortion_fade_power = readType<real>(ptr, len);
    distortion_rgb = readType<E3DCOLOR>(ptr, len);
    distortion_rgb_strength = readType<real>(ptr, len);
  }
};

class FxRenderShaderVolShape
{
public:
  real thickness;
  real radius_pow;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    thickness = readType<real>(ptr, len);
    radius_pow = readType<real>(ptr, len);
  }
};

class FxRenderVolfogInjection
{
public:
  bool enabled;
  real weight_rgb;
  real weight_alpha;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    enabled = readType<int>(ptr, len);
    weight_rgb = readType<real>(ptr, len);
    weight_alpha = readType<real>(ptr, len);
  }
};

class FxRenderShader
{
public:
  bool reverse_part_order;
  bool shadow_caster;
  bool allow_screen_proj_discard;
  int shader;
  FxRenderShaderDummy modfx_bboard_render;
  FxRenderShaderDummy modfx_bboard_render_atest;
  FxRenderShaderDistortion modfx_bboard_distortion;
  FxRenderShaderVolShape modfx_bboard_vol_shape;
  FxRenderShaderDummy modfx_bboard_rain;
  FxRenderShaderDistortion modfx_bboard_rain_distortion;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 11);

    reverse_part_order = readType<int>(ptr, len);
    shadow_caster = readType<int>(ptr, len);
    allow_screen_proj_discard = readType<int>(ptr, len);
    shader = readType<int>(ptr, len);
    modfx_bboard_render.load(ptr, len, load_cb);
    modfx_bboard_render_atest.load(ptr, len, load_cb);
    modfx_bboard_distortion.load(ptr, len, load_cb);
    modfx_bboard_vol_shape.load(ptr, len, load_cb);
    modfx_bboard_rain.load(ptr, len, load_cb);
    modfx_bboard_rain_distortion.load(ptr, len, load_cb);
  }
};

class FxDepthMask
{
public:
  bool enabled;
  bool use_part_radius;
  real depth_softness;
  real znear_softness;
  real znear_clip;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    enabled = readType<int>(ptr, len);
    use_part_radius = readType<int>(ptr, len);
    depth_softness = readType<real>(ptr, len);
    znear_softness = readType<real>(ptr, len);
    znear_clip = readType<real>(ptr, len);
  }
};

class FxPartTrimming
{
public:
  bool enabled;
  bool reversed;
  int steps;
  real fade_mul;
  real fade_pow;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 2);

    enabled = readType<int>(ptr, len);
    reversed = readType<int>(ptr, len);
    steps = readType<int>(ptr, len);
    fade_mul = readType<real>(ptr, len);
    fade_pow = readType<real>(ptr, len);
  }
};

class FxGlobalParams
{
public:
  real spawn_range_limit;
  int max_instances;
  int player_reserved;
  int transform_type;
  real emission_min;
  real one_point_number;
  real one_point_radius;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 4);

    spawn_range_limit = readType<real>(ptr, len);
    max_instances = readType<int>(ptr, len);
    player_reserved = readType<int>(ptr, len);
    transform_type = readType<int>(ptr, len);
    emission_min = readType<real>(ptr, len);
    one_point_number = readType<real>(ptr, len);
    one_point_radius = readType<real>(ptr, len);
  }
};

class FxQuality
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
