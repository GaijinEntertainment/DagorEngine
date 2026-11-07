// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/cameraIndoor/cameraIndoor_api.h>
#include <math/dag_mathBase.h>
#include <scene/dag_tiledScene.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <vecmath/dag_vecMath.h>

static ShaderVariableInfo camera_indoorVarId("camera_indoor", true);

static constexpr float INDOOR_RAMP_METERS = 0.25f;
static constexpr float INDOOR_TIME_SECONDS = 0.5f;
static constexpr float INDOOR_SNAP = 1e-3f;
static constexpr float CAMERA_CUT_METERS = 2.f;
static constexpr float CAMERA_MAX_SPEED = 100.f;

static float camera_indoor_target(const scene::TiledScene &indoors, vec3f pos)
{
  const vec4f ramp = v_splats(INDOOR_RAMP_METERS);
  vec4f deepest = v_neg(ramp);
  bool done = false;
  indoors.boxCull<false, true>(bbox3f{v_sub(pos, ramp), v_add(pos, ramp)}, 0, 0, [&](scene::node_index, mat44f_cref m) {
    mat33f axes; // orthogonal box axes only
    v_mat44_transpose_to_mat33(axes, m);
    const vec3f halfLenSq = v_mul(V_C_HALF, v_madd(axes.col0, axes.col0, v_madd(axes.col1, axes.col1, v_mul(axes.col2, axes.col2))));
    const vec3f proj = v_abs(v_mat33_mul_vec3(axes, v_sub(pos, m.col3)));
    done = v_check_xyz_all_true(v_cmp_le(proj, halfLenSq));
    if (!done)
      deepest = v_max(deepest, v_hmin3(v_mul(v_sub(halfLenSq, proj), v_rsqrt(v_add(halfLenSq, halfLenSq)))));
    return done;
  });
  return done ? 1.f : 1.f + v_extract_x(deepest) / INDOOR_RAMP_METERS;
}

void update_camera_indoor(const scene::TiledScene &indoors, const Point3 &cam_pos, float dt, Point3 &last_cam_pos, float &target,
  float &value)
{
  if (!VariableMap::isGlobVariablePresent(camera_indoorVarId))
    return;
  const bool moved = last_cam_pos != cam_pos;
  if (!moved && value == target)
    return;
  dt = max(dt, 0.f);
  bool cut = false;
  if (moved)
  {
    cut = lengthSq(cam_pos - last_cam_pos) > sqr(CAMERA_CUT_METERS + CAMERA_MAX_SPEED * dt);
    last_cam_pos = cam_pos;
    target = camera_indoor_target(indoors, v_ldu_p3_safe(&cam_pos.x));
  }
  float newValue = cut ? target : approach(value, target, dt, INDOOR_TIME_SECONDS);
  if (fabsf(target - newValue) < INDOOR_SNAP)
    newValue = target;
  if (newValue == value)
    return;
  value = newValue;
  camera_indoorVarId.set_float(newValue);
}

void reset_camera_indoor() { camera_indoorVarId.set_float(0.f); }
