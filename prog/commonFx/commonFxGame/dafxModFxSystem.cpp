// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_hlsl_floatx.h>
#include <daFx/dafx.h>
#include <daFx/dafx_def.hlsli>
#include <dafx_globals.hlsli>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texMgr.h>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include "dafxModFx_decl.h"
#include "dafxSystemDesc.h"
#include "dafxQuality.h"
#include "modfx/modfx_decl.hlsl"
#include "modfx/modfx_curve.hlsli"
#include <math/integer/dag_IBBox2.h>
#include <dafxEmitterDebug.h>
#include <daFx/dafx_gravity_zone.hlsli>


namespace dafx
{
extern int acquire_frame_boundary(ContextId cid, TEXTUREID texture_id, IPoint2 frame_dim);
}


static int (*g_modfx_convert_rtag_cb)(int) = nullptr;
void dafx_set_modfx_convert_rtag_cb(int (*convert_rtag_cb)(int)) { g_modfx_convert_rtag_cb = convert_rtag_cb; }


#define ENABLE_DECL(p, a) (p |= 1 << (a))
#define ENABLE_FLAG(p, a) (p |= 1 << (a))
#define ENABLE_MOD(p, a)  (p[(a) / 32] |= 1 << ((a) % 32))

#define ENABLE_ROFS(a, b) rvals[dafx_ex::SystemInfo::a] = (b)
#define ENABLE_SOFS(a, b) svals[dafx_ex::SystemInfo::a] = (b)

const float g_value_threshold = 0.05f;
const float g_ratio_threshold = 0.95f;
const float g_prebake_compression_threshold = 0.05f;

enum
{
  SPAWN_LINEAR = 0,
  SPAWN_BURST = 1,
  SPAWN_DISTANCE_BASED = 2,
  SPAWN_FIXED = 3,
};


enum
{
  RGROUP_LOWRES = 0,
  RGROUP_HIGHRES = 1,
  RGROUP_DISTORTION = 2,
  RGROUP_WATER_PROJ = 3,
  RGROUP_UNDERWATER = 4,
};

enum
{
  RSHADER_DEFAULT = 0,
  RSHADER_BBOARD_ATEST = 1,
  RSHADER_DISTORTION = 2,
  RSHADER_BBOARD_VOLSHAPE = 3,
  RSHADER_BBOARD_RAIN = 4,
  RSHADER_BBOARD_RAIN_DISTORTION = 5,
  RSHADER_BBOARD_VOLFOG_INJECTION = 6,
};

enum
{
  RBLEND_ABLEND = 0,
  RBLEND_PREMULT = 1,
  RBLEND_ADD = 2,
};

enum
{
  POSINIT_SPHERE = 0,
  POSINIT_CYLINDER = 1,
  POSINIT_CONE = 2,
  POSINIT_BOX = 3,
  POSINIT_SPHERE_SECTOR = 4,
};

enum
{
  VELSTART_POINT = 0,
  VELSTART_VEC = 1,
  VELSTART_START_SHAPE = 2,
};

enum
{
  VELADD_POINT = 0,
  VELADD_VEC = 1,
  VELADD_CONE = 2,
};

enum
{
  RSHAPE_TYPE_BBOARD = 0,
  RSHAPE_TYPE_RIBBON = 1,
};

enum
{
  RSHAPE_BBOARD_SCREEN = 0,
  RSHAPE_BBOARD_VIEW_POS = 1,
  RSHAPE_BBOARD_VELOCITY = 2,
  RSHAPE_BBOARD_STATIC_ALIGNED = 3,
  RSHAPE_BBOARD_ADAPTIVE_ALIGNED = 4,
  RSHAPE_BBOARD_VELOCITY_MOTION = 5,
  RSHAPE_BBOARD_START_VELOCITY = 6,
};

enum
{
  RSHAPE_RIBBON_SIDE_ONLY = 0,
  RSHAPE_RIBBON_SIDE_AND_HEAD = 1,
};

enum
{
  RSHAPE_RIBBON_UV_RELATIVE = 0,
  RSHAPE_RIBBON_UV_STATIC = 1,
};

enum
{
  LIGHT_NONE = 0,
  LIGHT_UNIFORM = 1,
  LIGHT_DISC = 2,
  LIGHT_SPHERE = 3,
  LIGHT_NORMALMAP = 4,
};

template <typename T>
void order_swap(T &v1, T &v2, T m)
{
  v1 = max(v1, m);
  v2 = max(v2, m);

  if (v1 > v2)
    eastl::swap(v1, v2);
}

int push_system_data(eastl::vector<unsigned char> &dst, const void *src, int size)
{
  int oldSize = dst.size();
  dst.resize(oldSize + size);
  memcpy(dst.data() + oldSize, src, size);
  return oldSize;
}

template <typename T>
int push_system_data(eastl::vector<unsigned char> &dst, const T &v)
{
  return push_system_data(dst, &v, sizeof(T));
}

void gen_prebake_curve(eastl::vector<unsigned char> &out, CubicCurveSampler &curve, int steps)
{
  out.resize(steps);

  for (int i = 0; i < steps; ++i)
  {
    float k = steps > 1 ? (float)i / (steps - 1) : 0;
    out[i] = saturate(curve.sample(saturate(k))) * 255.f;
  }
}

int push_prebake_curve(eastl::vector<unsigned char> &out, CubicCurveSampler &curve)
{
  eastl::vector<unsigned char> refValues;
  gen_prebake_curve(refValues, curve, MODFX_PREBAKE_CURVE_STEPS_LIMIT);

  bool stop = false;
  eastl::vector<unsigned char> compressedValues = refValues;
  float threshold = g_prebake_compression_threshold * 255.f;

  // compression (min size = 4, as simulation works with ints))
  while (compressedValues.size() > 4 && !stop)
  {
    eastl::vector<unsigned char> testValues;
    gen_prebake_curve(testValues, curve, compressedValues.size() / 2);

    for (int i = 0; i < refValues.size(); ++i)
    {
      uint k0, k1;
      float t = (float)i / (refValues.size() - 1);
      float w = modfx_calc_curve_weight(refValues.size(), t, k0, k1);
      float refV = lerp(refValues[k0], refValues[k1], w);

      w = modfx_calc_curve_weight(testValues.size(), t, k0, k1);
      float newV = lerp(testValues[k0], testValues[k1], w);

      if (fabsf(refV - newV) > threshold)
      {
        stop = true;
        break;
      }
    }

    if (!stop)
      eastl::swap(testValues, compressedValues);
  }

  int ofs = push_system_data(out, (int)compressedValues.size());
  push_system_data(out, compressedValues.data(), compressedValues.size());
  return ofs;
}

void gen_prebake_grad(eastl::vector<E3DCOLOR> &out, const GradientBoxSampler &grad, int steps)
{
  out.resize(steps);

  for (int i = 0; i < steps; ++i)
  {
    float k = steps > 1 ? (float)i / (steps - 1) : 0;
    out[i] = grad.sample(saturate(k));
  }
}

int push_prebake_grad(eastl::vector<unsigned char> &out, const GradientBoxSampler &grad)
{
  eastl::vector<E3DCOLOR> refValues;
  gen_prebake_grad(refValues, grad, MODFX_PREBAKE_GRAD_STEPS_LIMIT);

  bool stop = false;
  eastl::vector<E3DCOLOR> compressedValues = refValues;

  // compression
  while (compressedValues.size() != 1 && !stop)
  {
    eastl::vector<E3DCOLOR> testValues;
    gen_prebake_grad(testValues, grad, compressedValues.size() / 2);

    for (int i = 0; i < refValues.size(); ++i)
    {
      uint k0, k1;
      float t = (float)i / (refValues.size() - 1);
      float w = modfx_calc_curve_weight(refValues.size(), t, k0, k1);
      float4 refV = lerp(unpack_e3dcolor_to_n4f(refValues[k0]), unpack_e3dcolor_to_n4f(refValues[k1]), w);

      w = modfx_calc_curve_weight(testValues.size(), t, k0, k1);
      float4 newV = lerp(unpack_e3dcolor_to_n4f(testValues[k0]), unpack_e3dcolor_to_n4f(testValues[k1]), w);

      if (abs(refV.x - newV.x) > g_prebake_compression_threshold || abs(refV.y - newV.y) > g_prebake_compression_threshold ||
          abs(refV.z - newV.z) > g_prebake_compression_threshold || abs(refV.w - newV.w) > g_prebake_compression_threshold)
      {
        stop = true;
        break;
      }
    }

    if (!stop)
      eastl::swap(testValues, compressedValues);
  }

  int ofs = push_system_data(out, (int)compressedValues.size());
  push_system_data(out, compressedValues.data(), compressedValues.size() * sizeof(E3DCOLOR));
  return ofs;
}

bool dafx_modfx_system_load(const char *ptr, int len, BaseParamScriptLoadCB *load_cb, dafx::ContextId ctx, dafx::SystemDesc &sdesc,
  dafx_ex::SystemInfo &sinfo, dafx_ex::EmitterDebug *&emitter_debug)
{
  CHECK_FX_VERSION(ptr, len, 13);

  FxSpawn parSpawn;
  parSpawn.load(ptr, len, load_cb);

  FxLife parLife;
  parLife.load(ptr, len, load_cb);

  FxPos parPos;
  parPos.load(ptr, len, load_cb);

  FxRadius parRadius;
  parRadius.load(ptr, len, load_cb);

  FxColor parColor;
  parColor.load(ptr, len, load_cb);

  FxRotation parRotation;
  parRotation.load(ptr, len, load_cb);

  FxVelocity parVelocity;
  parVelocity.load(ptr, len, load_cb);

  FxPlacement parPlacement;
  parPlacement.load(ptr, len, load_cb);

  FxTexture parTex;
  parTex.load(ptr, len, load_cb);

  FxEmission parColorEmission;
  parColorEmission.load(ptr, len, load_cb);

  FxThermalEmission parThermalEmission;
  parThermalEmission.load(ptr, len, load_cb);

  FxLighting parLighting;
  parLighting.load(ptr, len, load_cb);

  FxRenderShape parRenderShape;
  parRenderShape.load(ptr, len, load_cb);

  FxBlending parBlending;
  parBlending.load(ptr, len, load_cb);

  FxDepthMask parDepthMask;
  parDepthMask.load(ptr, len, load_cb);

  FxRenderGroup parRenderGroup;
  parRenderGroup.load(ptr, len, load_cb);

  FxRenderShader parRenderShader;
  parRenderShader.load(ptr, len, load_cb);

  FxRenderVolfogInjection parVolfogInjection;
  parVolfogInjection.load(ptr, len, load_cb);

  FxPartTrimming parPartTrimming;
  parPartTrimming.load(ptr, len, load_cb);

  FxGlobalParams parGlobals;
  parGlobals.load(ptr, len, load_cb);

  FxQuality parQuality;
  parQuality.load(ptr, len, load_cb);

  dafx::SystemDesc ddesc;

// data binds
#define GDATA(a) ddesc.reqGlobalValues.push_back(dafx::ValueBindDesc(#a, offsetof(GlobalData, a), sizeof(GlobalData::a)))

  GDATA(dt);
  GDATA(water_level);
  GDATA(globtm);
  GDATA(globtm_sim);
  GDATA(view_dir_x);
  GDATA(view_dir_y);
  GDATA(view_dir_z);
  GDATA(world_view_pos);
  GDATA(gravity_zone_count);
  GDATA(target_size);
  GDATA(target_size_rcp);
  GDATA(depth_size);
  GDATA(depth_size_rcp);
  GDATA(depth_tci_offset);
  GDATA(from_sun_direction);
  GDATA(sun_color);
  GDATA(sky_color);
  GDATA(zn_zfar);
  GDATA(znear_offset);
  GDATA(proj_hk);
  GDATA(wind_dir);
  GDATA(wind_power);
  GDATA(wind_scroll);
  GDATA(depth_size_for_collision);
  GDATA(depth_size_rcp_for_collision);
  GDATA(camera_velocity);

#undef GDATA

  // life prep
  order_swap(parLife.part_life_min, parLife.part_life_max, 0.f);
  if ((parLife.part_life_min == 0.f || parLife.part_life_max == 0.f) && parSpawn.type != SPAWN_FIXED)
  {
    logerr("fx: modfx: life <= 0");
    return false;
  }

  // emitter
  if (parSpawn.type == SPAWN_LINEAR)
  {
    ddesc.emitterData.type = dafx::EmitterType::LINEAR;
    ddesc.emitterData.linearData.countMin = max(parSpawn.linear.count_min, 0);
    ddesc.emitterData.linearData.countMax = max(parSpawn.linear.count_max, max(1, int(ddesc.emitterData.linearData.countMin)));
    ddesc.emitterData.linearData.lifeLimit = parLife.part_life_max;
    ddesc.emitterData.linearData.instant = true;
  }
  else if (parSpawn.type == SPAWN_BURST)
  {
    ddesc.emitterData.type = dafx::EmitterType::BURST;
    ddesc.emitterData.burstData.countMin = max(parSpawn.burst.count_min, 0);
    ddesc.emitterData.burstData.countMax = max(parSpawn.burst.count_max, max(1, int(ddesc.emitterData.burstData.countMin)));
    ddesc.emitterData.burstData.lifeLimit = parLife.part_life_max;
    ddesc.emitterData.burstData.cycles = parSpawn.burst.cycles;
    ddesc.emitterData.burstData.period = parSpawn.burst.period;

    if (parSpawn.burst.cycles == 0 && parSpawn.burst.period == 0)
      ddesc.emitterData.burstData.cycles = 1;
  }
  else if (parSpawn.type == SPAWN_DISTANCE_BASED)
  {
    ddesc.emitterData.type = dafx::EmitterType::DISTANCE_BASED;
    ddesc.emitterData.distanceBasedData.elemLimit = max(parSpawn.distance_based.elem_limit, 1);
    ddesc.emitterData.distanceBasedData.distance = parSpawn.distance_based.distance;
    ddesc.emitterData.distanceBasedData.lifeLimit = parLife.part_life_max;
    ddesc.emitterData.distanceBasedData.idlePeriod = parSpawn.distance_based.idle_period;
    ddesc.specialFlags |= dafx::SystemDesc::FLAG_DISABLE_SIM_LODS; // distance based fx are very sensitive for skipped frames
  }
  else if (parSpawn.type == SPAWN_FIXED)
  {
    ddesc.emitterData.type = dafx::EmitterType::FIXED;
    ddesc.emitterData.fixedData.count = parSpawn.fixed.count;
  }

  ddesc.emitterData.minEmissionFactor = clamp(parGlobals.emission_min, 0.f, 1.f);

  const dafx::Config &cfg = dafx::get_config(ctx);
  unsigned int lim = dafx::get_emitter_limit(ddesc.emitterData, true);
  if (lim == 0 || lim >= cfg.emission_limit)
  {
    logerr("fx: modfx over emitter limit");
    return false;
  }

  ddesc.emitterData.delay = max(parLife.inst_life_delay, 0.f);

  // param validation
  parPos.gpu_placement.enabled &= (parPos.gpu_placement.use_hmap || parPos.gpu_placement.use_depth_above ||
                                   parPos.gpu_placement.use_water || parPos.gpu_placement.use_water_flowmap);

  // validate gpu-only features and quality
  if (parPos.enabled && parPos.gpu_placement.enabled)
  {
    if (parQuality.low_quality)
    {
      logerr("fx: modfx: gpu_placement is not compatible with low quality");
      parPos.gpu_placement.enabled = false;
    }

    dafx_ex::TransformType transformType = static_cast<dafx_ex::TransformType>(parGlobals.transform_type);
    if (transformType != dafx_ex::TRANSFORM_WORLD_SPACE)
    {
      logerr("fx: modfx: gpu_placement requires world_space transform_type");
      parPos.gpu_placement.enabled = false;
    }
  }

  // shaders
  ddesc.emissionData.type = dafx::EmissionType::SHADER;
  ddesc.simulationData.type = dafx::SimulationType::SHADER;

  bool gpuFeatures = parVelocity.collision.enabled && parVelocity.enabled && parPos.enabled;
  gpuFeatures |= parPos.gpu_placement.enabled && parPos.enabled;
  if (gpuFeatures)
  {
    ddesc.emissionData.shader = "dafx_modfx_emission_adv";
    ddesc.simulationData.shader = "dafx_modfx_simulation_adv";
  }
  else
  {
    ddesc.emissionData.shader = "dafx_modfx_emission";
    ddesc.simulationData.shader = "dafx_modfx_simulation";
  }

  // render tags
  int rtag = 0;
  if (parRenderGroup.type == RGROUP_LOWRES)
    rtag = dafx_ex::RTAG_LOWRES;
  else if (parRenderGroup.type == RGROUP_HIGHRES)
    rtag = dafx_ex::RTAG_HIGHRES;
  else if (parRenderGroup.type == RGROUP_DISTORTION)
    rtag = dafx_ex::RTAG_DISTORTION;
  else if (parRenderGroup.type == RGROUP_WATER_PROJ)
    rtag = dafx_ex::RTAG_WATER_PROJ;
  else if (parRenderGroup.type == RGROUP_UNDERWATER)
    rtag = dafx_ex::RTAG_UNDERWATER;

  if (g_modfx_convert_rtag_cb)
    rtag = g_modfx_convert_rtag_cb(rtag);

  auto logerr_if_ribbon = [&](const char *name) {
    if (parRenderShape.type == RSHAPE_TYPE_RIBBON)
    {
      logerr("fx: modfx: Unsupported ribbon shader: %s", name);
      return false;
    }
    return true;
  };

  // TODO The shader only exist with DX12 for now
  bool useBVH = d3d::get_driver_code().is(d3d::dx12) && (rtag == dafx_ex::RTAG_LOWRES || rtag == dafx_ex::RTAG_HIGHRES);

  if (parRenderShader.shader == RSHADER_DEFAULT)
  {
    if (parRenderShape.type == RSHAPE_TYPE_RIBBON)
      if (parRenderShape.ribbon.type == RSHAPE_RIBBON_SIDE_ONLY)
        ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_ribbon_render_side_only"});
      else
        ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_ribbon_render"});
    else
    {
      ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_bboard_render"});
      if (useBVH)
        ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_BVH], "dafx_modfx_bvh"});
    }
  }
  else if (parRenderShader.shader == RSHADER_BBOARD_ATEST && logerr_if_ribbon("modfx_bboard_atest"))
  {
    ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_bboard_render_atest"});
    if (useBVH)
      ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_BVH], "dafx_modfx_bvh"});
  }
  else if (parRenderShader.shader == RSHADER_DISTORTION)
  {
    if (parRenderShape.type == RSHAPE_TYPE_RIBBON)
      if (parRenderShape.ribbon.type == RSHAPE_RIBBON_SIDE_ONLY)
        ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_ribbon_distortion_side_only"});
      else
        ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_ribbon_distortion"});
    else
      ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_bboard_distortion"});
  }
  else if (parRenderShader.shader == RSHADER_BBOARD_RAIN && logerr_if_ribbon("modfx_bboard_rain"))
    ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_bboard_rain"});
  else if (parRenderShader.shader == RSHADER_BBOARD_RAIN_DISTORTION && logerr_if_ribbon("modfx_bboard_rain_distortion"))
    ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_bboard_rain_distortion"});
  else if (parRenderShader.shader == RSHADER_BBOARD_VOLSHAPE && logerr_if_ribbon("modfx_vol_shape"))
  {
    ddesc.renderDescs.push_back({dafx_ex::renderTags[rtag], "dafx_modfx_volshape_render"});
    ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_VOL_WBOIT], "dafx_modfx_volshape_wboit_render"});
    if (useBVH)
      ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_BVH], "dafx_modfx_bvh"});
  }
  else
    logerr("fx: modfx: Could not find proper render shader.");

  if (parRenderShader.shadow_caster && logerr_if_ribbon("FOM shadow"))
  {
    ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_FOM], "dafx_modfx_bboard_render_fom"});
  }

  if (parRenderShader.shader != RSHADER_DISTORTION && parRenderShader.shader != RSHADER_BBOARD_RAIN_DISTORTION &&
      parRenderShape.type != RSHAPE_TYPE_RIBBON && (!parThermalEmission.enabled || parThermalEmission.value >= 0))
  {
    ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_THERMAL],
      parRenderShader.shader == RSHADER_BBOARD_VOLSHAPE ? "dafx_modfx_volshape_thermal" : "dafx_modfx_bboard_thermals"});
  }

  if (parVolfogInjection.enabled && logerr_if_ribbon("dafx_modfx_bboard_volfog_injection"))
    ddesc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_VOLFOG_INJECTION], "dafx_modfx_bboard_volfog_injection"});

#if DAGOR_DBGLEVEL > 0
  for (const auto &unsupportedShader : cfg.unsupportedShaders)
  {
    for (const auto &rdesc : ddesc.renderDescs)
    {
      if (rdesc.shader == unsupportedShader)
      {
        logerr("fx: modfx: loading effect with unsupported shader \"%s\"", unsupportedShader.c_str());
      }
    }
  }
#endif

  // pins and mods
  G_STATIC_ASSERT(MODFX_RDECL_BITS <= 32);
  G_STATIC_ASSERT(MODFX_SDECL_BITS <= 32);

  uint32_t rdecl = 0;
  uint32_t sdecl = 0;

  uint32_t rflags = 0;
  uint32_t sflags = 0;

  uint32_t rmods[MODFX_SMODS_BITS / 32] = {0};
  uint32_t smods[MODFX_SMODS_BITS / 32] = {0};

  eastl::vector<unsigned char> renderData;
  eastl::vector<unsigned char> simulationData;

  eastl::vector<int> renOffsets;
  eastl::vector<int> simOffsets;

  renOffsets.resize(MODFX_RMOD_TOTAL_COUNT, -1);
  simOffsets.resize(MODFX_SMOD_TOTAL_COUNT, -1);

  eastl::vector_map<int, int> rvals;
  eastl::vector_map<int, int> svals;

  ENABLE_ROFS(VAL_FLAGS, MODFX_RDECL_BITS / 8 + MODFX_RMODS_BITS / 8);

  //
  // params prepare start
  //

  if (parPos.enabled)
    ENABLE_DECL(rdecl, MODFX_RDECL_POS);

  // emitter tm
  if (MODFX_RDECL_POS_ENABLED(rdecl))
  {
    TMatrix4 etm = TMatrix4::IDENT;
    int ofs = push_system_data(renderData, etm);
    renOffsets[MODFX_RMOD_INIT_TM] = ofs;
    ENABLE_MOD(rmods, MODFX_RMOD_INIT_TM);
    ENABLE_ROFS(VAL_TM, ofs);
  }

  // life
  if (parSpawn.type != SPAWN_FIXED)
  {
    ENABLE_DECL(sdecl, MODFX_SDECL_LIFE);

    if (parLife.part_life_min != parLife.part_life_max)
    {
      simOffsets[MODFX_SMOD_LIFE_RND] = push_system_data(simulationData, parLife.part_life_max / parLife.part_life_min);
      ENABLE_MOD(smods, MODFX_SMOD_LIFE_RND);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);
    }

    if (parLife.part_life_rnd_offset > g_value_threshold)
    {
      simOffsets[MODFX_SMOD_LIFE_OFFSET] =
        push_system_data(simulationData, min(parLife.part_life_rnd_offset, parLife.part_life_max) / parLife.part_life_max);
      ENABLE_MOD(smods, MODFX_SMOD_LIFE_OFFSET);
    }
  }

  // pos
  if (parPos.enabled)
  {
    if (parPos.attach_last_part_to_emitter)
      ENABLE_FLAG(sflags, MODFX_SFLAG_ATTACH_LAST_PART_TO_EMITTER);

    if (parPos.type == POSINIT_SPHERE)
    {
      ModfxDeclPosInitSphere pp;
      pp.volume = parPos.volume;
      pp.offset = parPos.offset;
      pp.radius = parPos.sphere.radius;
      simOffsets[MODFX_SMOD_POS_INIT_SPHERE] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_SPHERE);
      emitter_debug->type = dafx_ex::SPHERE;
      emitter_debug->sphere = {pp.offset, pp.radius};
    }
    else if (parPos.type == POSINIT_BOX)
    {
      ModfxDeclPosInitBox pp;
      pp.volume = parPos.volume;
      pp.offset = parPos.offset;
      pp.dims = Point3(parPos.box.width, parPos.box.height, parPos.box.depth);
      simOffsets[MODFX_SMOD_POS_INIT_BOX] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_BOX);
      // Under 0 values are used to specify dimensions, along which to use toroidal movement.
      if (pp.dims.x < 0.0f || pp.dims.y < 0.0f || pp.dims.z < 0.0f)
        ENABLE_FLAG(sflags, MODFX_SFLAG_TOROIDAL_MOVEMENT);
      emitter_debug->type = dafx_ex::BOX;
      emitter_debug->box = {pp.offset, pp.dims};
    }
    else if (parPos.type == POSINIT_CYLINDER)
    {
      ModfxDeclPosInitCylinder pp;
      pp.volume = parPos.volume;
      pp.offset = parPos.offset;
      pp.vec = parPos.cylinder.vec;
      normalizeDef(pp.vec, Point3(0, 1, 0));
      pp.radius = parPos.cylinder.radius;
      pp.height = parPos.cylinder.height;
      pp.random_burst = parSpawn.type == SPAWN_BURST ? saturate(parPos.cylinder.random_burst) : 0;
      simOffsets[MODFX_SMOD_POS_INIT_CYLINDER] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_CYLINDER);
      emitter_debug->type = dafx_ex::CYLINDER;
      emitter_debug->cylinder = {pp.offset, pp.vec, pp.radius, pp.height};
    }
    else if (parPos.type == POSINIT_CONE)
    {
      float r1 = max(parPos.cone.width_bottom, 0.01f);
      float r2 = max(parPos.cone.width_top, 0.01f);
      float h = max(parPos.cone.height, 0.01f);
      Point3 yaxis = parPos.cone.vec;
      normalizeDef(yaxis, Point3(0, 1, 0));

      if (fabsf(r1 - r2) < 0.01) // parallel
        r2 = r1 * (r1 > r2 ? 1.1f : 0.9f);

      float r3 = r2 - r1;
      float ro = r3 / h;
      float h2 = r1 / ro;

      if (r1 > r2)
      {
        yaxis = -yaxis;
        h = -h;
        h2 = -h2;
      }

      ModfxDeclPosInitCone pp;
      pp.volume = parPos.volume;
      pp.offset = parPos.offset;
      pp.vec = yaxis;
      pp.h1 = h;
      pp.h2 = h2;
      pp.rad = r1;
      pp.random_burst = parSpawn.type == SPAWN_BURST ? saturate(parPos.cone.random_burst) : 0;
      simOffsets[MODFX_SMOD_POS_INIT_CONE] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_CONE);
      emitter_debug->type = dafx_ex::CONE;
      emitter_debug->cone = {pp.offset, yaxis, h, h2, r1};
    }
    else if (parPos.type == POSINIT_SPHERE_SECTOR)
    {
      ModfxDeclPosInitSphereSector pp;
      pp.vec = parPos.sphere_sector.vec;
      normalizeDef(pp.vec, float3(0, 1, 0));
      pp.radius = parPos.sphere_sector.radius;
      pp.sector = saturate(parPos.sphere_sector.sector);
      pp.volume = parPos.volume;
      pp.random_burst = parSpawn.type == SPAWN_BURST ? saturate(parPos.sphere_sector.random_burst) : 0;
      simOffsets[MODFX_SMOD_POS_INIT_SPHERE_SECTOR] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_SPHERE_SECTOR);
      emitter_debug->type = dafx_ex::SPHERESECTOR;
      emitter_debug->sphereSector = {pp.vec, pp.radius, pp.sector};
    }

    if (parPos.gpu_placement.enabled)
    {
      ModfxDeclPosInitGpuPlacement pp;
      pp.flags = 0;
      pp.flags |= parPos.gpu_placement.use_hmap ? MODFX_GPU_PLACEMENT_HMAP : 0;
      pp.flags |= parPos.gpu_placement.use_water ? MODFX_GPU_PLACEMENT_WATER : 0;
      pp.flags |= parPos.gpu_placement.use_depth_above ? MODFX_GPU_PLACEMENT_DEPTH_ABOVE : 0;
      pp.height_threshold = max(parPos.gpu_placement.placement_threshold, 0.f);
      simOffsets[MODFX_SMOD_POS_INIT_GPU_PLACEMENT] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_POS_INIT_GPU_PLACEMENT);
      if (parPos.gpu_placement.use_water_flowmap)
        ENABLE_FLAG(sflags, MODFX_SFLAG_WATER_FLOWMAP);
    }
  }

  // radius
  order_swap(parRadius.rad_min, parRadius.rad_max, 0.f);

  if (parRadius.enabled && parRadius.rad_max > 0)
  {
    ENABLE_DECL(rdecl, MODFX_RDECL_RADIUS);

    if ((parRadius.rad_min / parRadius.rad_max) >= g_ratio_threshold)
    {
      int ofs = push_system_data(simulationData, parRadius.rad_min);
      simOffsets[MODFX_SMOD_RADIUS_INIT] = ofs;
      ENABLE_SOFS(VAL_RADIUS_MIN, ofs);
      ENABLE_SOFS(VAL_RADIUS_MAX, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_RADIUS_INIT);
    }
    else
    {
      ModfxDeclRadiusInitRnd pp;
      pp.rad_min = parRadius.rad_min;
      pp.rad_max = parRadius.rad_max;

      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_RADIUS_INIT_RND] = ofs;
      ENABLE_SOFS(VAL_RADIUS_MIN, ofs);
      ENABLE_SOFS(VAL_RADIUS_MAX, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_RADIUS_INIT_RND);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);
    }

    if (parRadius.over_part_life.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      simOffsets[MODFX_SMOD_RADIUS_OVER_PART_LIFE] = push_prebake_curve(simulationData, parRadius.over_part_life.curve);
      ENABLE_MOD(smods, MODFX_SMOD_RADIUS_OVER_PART_LIFE);
    }

    if (parRadius.apply_emitter_transform)
      ENABLE_FLAG(sflags, MODFX_SFLAG_RADIUS_APPLY_EMITTER_TRANSFORM);
  }

  // color
  if (parColor.enabled)
  {
    ENABLE_DECL(rdecl, MODFX_RDECL_COLOR);

    if (parColor.col_min == parColor.col_max)
    {
      int ofs = push_system_data(simulationData, (E3DCOLOR)parColor.col_min);
      simOffsets[MODFX_SMOD_COLOR_INIT] = ofs;
      ENABLE_SOFS(VAL_COLOR_MIN, ofs);
      ENABLE_SOFS(VAL_COLOR_MAX, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_INIT);
    }
    else
    {
      ModfxDeclColorInitRnd pp;
      pp.color_min = parColor.col_min;
      pp.color_max = parColor.col_max;

      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_COLOR_INIT_RND] = ofs;
      ENABLE_SOFS(VAL_COLOR_MIN, ofs);
      ENABLE_SOFS(VAL_COLOR_MAX, ofs + sizeof(E3DCOLOR));
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_INIT_RND);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);
    }

    if (parColor.allow_game_override)
    {
      int ofs = push_system_data(simulationData, uint(0xffffffff));
      simOffsets[MODFX_SMOD_COLOR_EMISSION_OVERRIDE] = ofs;
      ENABLE_SOFS(VAL_COLOR_MUL, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_EMISSION_OVERRIDE);
      ENABLE_DECL(sdecl, MODFX_SDECL_EMISSION_COLOR);
    }

    if (parColor.grad_over_part_life.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      simOffsets[MODFX_SMOD_COLOR_OVER_PART_LIFE_GRAD] = push_prebake_grad(simulationData, parColor.grad_over_part_life.grad);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_OVER_PART_LIFE_GRAD);

      if (parColor.grad_over_part_life.use_part_idx_as_key)
        ENABLE_FLAG(sflags, MODFX_SFLAG_COLOR_GRAD_USE_PART_IDX_AS_KEY);
    }

    if (parColor.curve_over_part_life.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      if (parColor.curve_over_part_life.use_threshold)
        ENABLE_FLAG(rflags, MODFX_RFLAG_COLOR_USE_ALPHA_THRESHOLD);

      simOffsets[MODFX_SMOD_COLOR_OVER_PART_LIFE_CURVE] = push_system_data(simulationData, parColor.curve_over_part_life.mask);

      push_prebake_curve(simulationData, parColor.curve_over_part_life.curve);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_OVER_PART_LIFE_CURVE);

      if (parColor.curve_over_part_life.use_part_idx_as_key)
        ENABLE_FLAG(sflags, MODFX_SFLAG_COLOR_CURVE_USE_PART_IDX_AS_KEY);
    }

    if (parColor.curve_over_part_idx.enabled)
    {
      simOffsets[MODFX_SMOD_COLOR_OVER_PART_IDX_CURVE] = push_system_data(simulationData, parColor.curve_over_part_idx.mask);

      push_prebake_curve(simulationData, parColor.curve_over_part_idx.curve);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_OVER_PART_IDX_CURVE);
    }

    if (parColor.alpha_by_velocity.enabled)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ModfxDeclAlphaByVelocity pp;

      float vel_diff = parColor.alpha_by_velocity.vel_max - parColor.alpha_by_velocity.vel_min;
      pp.vel_max = parColor.alpha_by_velocity.vel_max;
      pp.vel_min = parColor.alpha_by_velocity.vel_min;

      if (parColor.alpha_by_velocity.use_emitter_velocity)
        ENABLE_FLAG(sflags, MODFX_SFLAG_ALPHA_BY_EMITTER_VELOCITY);

      pp.inv_vel_diff = 1.f / vel_diff;
      pp.neg_minvel_div_by_diff = -parColor.alpha_by_velocity.vel_min * pp.inv_vel_diff;

      if (vel_diff > FLT_EPSILON)
      {
        simOffsets[MODFX_SMOD_ALPHA_BY_VELOCITY] = push_system_data(simulationData, pp);
        ENABLE_MOD(smods, MODFX_SMOD_ALPHA_BY_VELOCITY);

        if (parColor.alpha_by_velocity.velocity_alpha_curve.enabled)
        {
          simOffsets[MODFX_SMOD_ALPHA_BY_VELOCITY_CURVE] =
            push_prebake_curve(simulationData, parColor.alpha_by_velocity.velocity_alpha_curve.curve);
          ENABLE_MOD(smods, MODFX_SMOD_ALPHA_BY_VELOCITY_CURVE);
        }
      }
    }

    if (parColor.gamma_correction)
      ENABLE_FLAG(rflags, MODFX_RFLAG_GAMMA_CORRECTION);
  }

  // emission
  if (parColorEmission.enabled)
  {
    ModfxColorEmission pp;
    pp.mask = parColorEmission.mask;
    pp.value = parColorEmission.value;

    renOffsets[MODFX_RMOD_COLOR_EMISSION] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_COLOR_EMISSION);

    if (parColorEmission.over_part_life.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_EMISSION_FADE);
      simOffsets[MODFX_SMOD_COLOR_EMISSION_OVER_PART_LIFE] = push_prebake_curve(simulationData, parColorEmission.over_part_life.curve);
      ENABLE_MOD(smods, MODFX_SMOD_COLOR_EMISSION_OVER_PART_LIFE);
    }
  }

  // thermal
  if (parThermalEmission.enabled && parRenderShader.shader != RSHADER_DISTORTION &&
      parRenderShader.shader != RSHADER_BBOARD_RAIN_DISTORTION)
  {
    renOffsets[MODFX_RMOD_THERMAL_EMISSION] = push_system_data(renderData, parThermalEmission.value);
    ENABLE_MOD(rmods, MODFX_RMOD_THERMAL_EMISSION);
    if (parThermalEmission.over_part_life.enabled)
    {
      ENABLE_MOD(rmods, MODFX_RMOD_THERMAL_EMISSION_FADE);
      renOffsets[MODFX_RMOD_THERMAL_EMISSION_FADE] = push_prebake_curve(renderData, parThermalEmission.over_part_life.curve);
      ENABLE_DECL(rdecl, MODFX_RDECL_LIFE_NORM);
    }
  }

  // volfog
  if (parVolfogInjection.enabled)
  {
    ModfxDeclVolfogInjectionParams pp;

    pp.weight_rgb = max(parVolfogInjection.weight_rgb, 0.0f);
    pp.weight_alpha = max(parVolfogInjection.weight_alpha, 0.0f);
    renOffsets[MODFX_RMOD_VOLFOG_INJECTION] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_VOLFOG_INJECTION);
  }

  // rotation
  if (parRotation.enabled)
  {
    ENABLE_DECL(rdecl, MODFX_RDECL_ANGLE);

    if (parVelocity.collision.enabled)
      ENABLE_DECL(sdecl, MODFX_SDECL_COLLISION_TIME); // for rollback life to collision time

    if (parRotation.uv_rotation)
      ENABLE_FLAG(rflags, MODFX_RFLAG_USE_UV_ROTATION);

    parRotation.start_min = -parRotation.start_min * DEG_TO_RAD;
    parRotation.start_max = -parRotation.start_max * DEG_TO_RAD;

    if (fabsf(parRotation.start_min - parRotation.start_max) < g_value_threshold)
    {
      int ofs = push_system_data(simulationData, parRotation.start_min);
      simOffsets[MODFX_SMOD_ROTATION_INIT] = ofs;
      ENABLE_SOFS(VAL_ROTATION_MIN, ofs);
      ENABLE_SOFS(VAL_ROTATION_MAX, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_ROTATION_INIT);
    }
    else
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);

      ModfxDeclRotationInitRnd pp;
      pp.angle_min = parRotation.start_min;
      pp.angle_max = parRotation.start_max;
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_ROTATION_INIT_RND] = ofs;
      ENABLE_SOFS(VAL_ROTATION_MIN, ofs);
      ENABLE_SOFS(VAL_ROTATION_MAX, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_ROTATION_INIT_RND);
    }

    if (parRotation.dynamic && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      ModfxDeclRotationDynamic pp;
      pp.vel_min = -parRotation.vel_min * DEG_TO_RAD * parLife.part_life_max;
      pp.vel_max = -parRotation.vel_max * DEG_TO_RAD * parLife.part_life_max;
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_ROTATION_DYNAMIC] = ofs;
      ENABLE_SOFS(VAL_ROTATION_VEL_MIN, ofs);
      ENABLE_SOFS(VAL_ROTATION_VEL_MAX, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_ROTATION_DYNAMIC);
    }

    if (parRotation.dynamic && parRotation.over_part_life.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      simOffsets[MODFX_SMOD_ROTATION_OVER_PART_LIFE] = push_prebake_curve(simulationData, parRotation.over_part_life.curve);
      ENABLE_MOD(smods, MODFX_SMOD_ROTATION_OVER_PART_LIFE);
    }
  }

  // velocity
  if (parVelocity.enabled && MODFX_RDECL_POS_ENABLED(rdecl))
  {
    if (parVelocity.mass > 0.f)
    {
      int ofs = push_system_data(simulationData, max(parVelocity.mass, 0.f));
      simOffsets[MODFX_SMOD_MASS_INIT] = ofs;
      ENABLE_SOFS(VAL_MASS, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_MASS_INIT);
    }

    if (parVelocity.drag_coeff > 0.f)
    {
      ModfxDeclDragInit pp;
      pp.drag_coeff = max(parVelocity.drag_coeff, 0.f);
      pp.drag_to_rad_k = saturate(parVelocity.drag_to_rad_k);
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_DRAG_INIT] = ofs;
      ENABLE_SOFS(VAL_DRAG_COEFF, ofs);
      ENABLE_SOFS(VAL_DRAG_TO_RAD, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_DRAG_INIT);
    }

    if (parVelocity.mass > 0)
    {
      ENABLE_FLAG(sflags, MODFX_SFLAG_VELOCITY_USE_FORCE_RESOLVER);

      if (parVelocity.drag_coeff > 0 && parVelocity.drag_curve.enabled)
      {
        simOffsets[MODFX_SMOD_DRAG_OVER_PART_LIFE] = push_prebake_curve(simulationData, parVelocity.drag_curve.curve);
        ENABLE_MOD(smods, MODFX_SMOD_DRAG_OVER_PART_LIFE);
      }
    }

    if (parVelocity.apply_gravity)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_FLAG(sflags, MODFX_SFLAG_VELOCITY_APPLY_GRAVITY);
      if (parVelocity.gravity_transform)
      {
        ENABLE_FLAG(sflags, MODFX_SFLAG_VELOCITY_APPLY_GRAVITY_TRANSFORM);
        int ofs = push_system_data(simulationData, Point3(0, 1, 0));
        simOffsets[MODFX_SMOD_VELOCITY_LOCAL_GRAVITY] = ofs;
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_LOCAL_GRAVITY);
        ENABLE_SOFS(VAL_LOCAL_GRAVITY_VEC, ofs);
      }
    }

#if DAFX_USE_GRAVITY_ZONE
    if (parVelocity.gravity_zone == GRAVITY_ZONE_DEFAULT || parVelocity.gravity_zone == GRAVITY_ZONE_PER_EMITTER)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_FLAG(sflags, MODFX_SFLAG_GRAVITY_ZONE_PER_EMITTER);
      int ofs = push_system_data(simulationData, Matrix3::IDENT);
      simOffsets[MODFX_SMOD_GRAVITY_TM] = ofs;
      ENABLE_MOD(smods, MODFX_SMOD_GRAVITY_TM);
      ENABLE_SOFS(VAL_GRAVITY_ZONE_TM, ofs);
    }

    if (parVelocity.gravity_zone == GRAVITY_ZONE_PER_PARTICLE)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_FLAG(sflags, MODFX_SFLAG_GRAVITY_ZONE_PER_PARTICLE);
    }
#endif

    if (parVelocity.apply_parent_velocity)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      Point3 pp(0, 0, 0);
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC] = ofs;
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC);
      ENABLE_SOFS(VAL_VELOCITY_START_ADD, ofs);
    }

    if (parVelocity.start.enabled && (parVelocity.start.vel_min > 0 || parVelocity.start.vel_max > 0))
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);

      ModfxDeclVelocity pp;
      pp.vel_min = parVelocity.start.vel_min;
      pp.vel_max = parVelocity.start.vel_max;
      pp.vec_rnd = parVelocity.start.vec_rnd;
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_VELOCITY_INIT] = ofs;
      ENABLE_SOFS(VAL_VELOCITY_START_MIN, ofs);
      ENABLE_SOFS(VAL_VELOCITY_START_MAX, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_INIT);

      if (parVelocity.start.type == VELSTART_POINT)
      {
        simOffsets[MODFX_SMOD_VELOCITY_INIT_POINT] = push_system_data(simulationData, parVelocity.start.point.offset);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_INIT_POINT);
      }
      else if (parVelocity.start.type == VELSTART_VEC)
      {
        simOffsets[MODFX_SMOD_VELOCITY_INIT_VEC] = push_system_data(simulationData, parVelocity.start.vec.vec);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_INIT_VEC);
      }
      else if (parVelocity.start.type == VELSTART_START_SHAPE)
      {
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_INIT_START_SHAPE);
      }
      else
      {
        G_ASSERT(false);
      }
    }

    if (parVelocity.add.enabled && (parVelocity.add.vel_min > 0 || parVelocity.add.vel_max > 0))
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);

      ModfxDeclVelocity pp;
      pp.vel_min = parVelocity.add.vel_min;
      pp.vel_max = parVelocity.add.vel_max;
      pp.vec_rnd = parVelocity.add.vec_rnd;
      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_VELOCITY_ADD] = ofs;
      ENABLE_SOFS(VAL_VELOCITY_ADD_MIN, ofs);
      ENABLE_SOFS(VAL_VELOCITY_ADD_MAX, ofs + sizeof(float));
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_ADD);

      if (parVelocity.add.apply_emitter_transform)
      {
        G_ASSERT(MOD_ENABLED(rmods, MODFX_RMOD_INIT_TM));
        ENABLE_FLAG(sflags, MODFX_SFLAG_VELOCITY_APPLY_EMITTER_TRANSFORM_TO_ADD);
      }

      if (parVelocity.add.type == VELADD_POINT)
      {
        simOffsets[MODFX_SMOD_VELOCITY_ADD_POINT] = push_system_data(simulationData, parVelocity.add.point.offset);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_ADD_POINT);
      }
      else if (parVelocity.add.type == VELADD_VEC)
      {
        simOffsets[MODFX_SMOD_VELOCITY_ADD_VEC] = push_system_data(simulationData, parVelocity.add.vec.vec);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_ADD_VEC);
      }
      else if (parVelocity.add.type == VELADD_CONE)
      {
        float r1 = max(parVelocity.add.cone.width_bottom, 0.01f);
        float r2 = max(parVelocity.add.cone.width_top, 0.01f);
        float h = max(parVelocity.add.cone.height, 0.01f);
        Point3 yaxis = parVelocity.add.cone.vec;
        normalizeDef(yaxis, Point3(0, 1, 0));

        if (fabsf(r1 - r2) < 0.01)
          r2 = r1 * 1.1f;

        if (r1 > r2)
        {
          yaxis = -yaxis;
          h = -h;
        }

        float r3 = r2 - r1;
        float ro = r3 / h;
        float h2 = r1 / ro;

        float hyp = sqrtf(h * h + r3 * r3);
        float cosv = safediv(h, hyp);

        Point3 origin = -yaxis * h2 + parVelocity.add.cone.offset;

        ModfxDeclVelocityCone pp;
        pp.yaxis = h > 0 ? yaxis : -yaxis;
        pp.origin = origin;
        pp.border_cos = cosv;
        pp.center_power = saturate(parVelocity.add.cone.center_power);
        pp.border_power = saturate(parVelocity.add.cone.border_power);
        simOffsets[MODFX_SMOD_VELOCITY_ADD_CONE] = push_system_data(simulationData, pp);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_ADD_CONE);
      }
      else
      {
        G_ASSERT(false);
      }

      if (parVelocity.decay.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
      {
        simOffsets[MODFX_SMOD_VELOCITY_DECAY] = push_prebake_curve(simulationData, parVelocity.decay.curve);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_DECAY);
      }
    }

    if (parVelocity.force_field.vortex.enabled)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);
      auto &vortex = parVelocity.force_field.vortex;

      ModfxDeclVelocityForceFieldVortex pp;
      pp.axis_direction = vortex.axis_direction;
      normalizeDef(pp.axis_direction, Point3(0, 1, 0));
      pp.direction_rnd = vortex.direction_rnd;
      pp.axis_position = vortex.axis_position;
      pp.position_rnd = vortex.position_rnd / 2.0f;
      pp.rotation_speed_min = vortex.rotation_speed_min;
      pp.rotation_speed_max = vortex.rotation_speed_max;
      pp.pull_speed_min = vortex.pull_speed_min;
      pp.pull_speed_max = vortex.pull_speed_max;

      simOffsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX);

      if (vortex.rotation_speed_over_part_life.enabled)
      {
        simOffsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED] =
          push_prebake_curve(simulationData, vortex.rotation_speed_over_part_life.curve);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED);
      }

      if (parVelocity.force_field.vortex.pull_speed_over_part_life.enabled)
      {
        simOffsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED] =
          push_prebake_curve(simulationData, vortex.pull_speed_over_part_life.curve);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED);
      }
    }

    if (parVelocity.force_field.noise.enabled)
    {
      ModfxForceFieldNoise pp;
      pp.usePosOffset = parVelocity.force_field.noise.type == 1;
      pp.posScale = max(parVelocity.force_field.noise.pos_scale, 0.f);
      pp.powerScale = parVelocity.force_field.noise.power_scale;
      pp.powerRnd = saturate(parVelocity.force_field.noise.power_rnd);
      pp.powerPerPartRnd = saturate(parVelocity.force_field.noise.power_per_part_rnd);
      simOffsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE);

      if (parVelocity.force_field.noise.power_curve.enabled)
      {
        simOffsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE] =
          push_prebake_curve(simulationData, parVelocity.force_field.noise.power_curve.curve);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE);
      }

      if (pp.usePosOffset)
        ENABLE_DECL(rdecl, MODFX_RDECL_POS_OFFSET);
      else
        ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
    }

    if (parVelocity.collision.enabled)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);

      ModfxDeclCollision pp;
      pp.radius_k = max(parVelocity.collision.radius_mod, 0.f);
      pp.reflect_energy = 1.f - saturate(parVelocity.collision.energy_loss);
      pp.reflect_power = saturate(parVelocity.collision.reflect_power);
      pp.emitter_deadzone = max(parVelocity.collision.emitter_deadzone * parVelocity.collision.emitter_deadzone, 0.f);
      pp.collide_flags = MODFX_COLLIDE_WITH_DEPTH * parVelocity.collision.collide_with_depth |
                         MODFX_COLLIDE_WITH_DEPTH_ABOVE * parVelocity.collision.collide_with_depth_above |
                         MODFX_COLLIDE_WITH_HMAP * parVelocity.collision.collide_with_hmap |
                         MODFX_COLLIDE_WITH_WATER * parVelocity.collision.collide_with_water |
                         MODFX_STOP_ROTATION_ON_COLLISION * parVelocity.collision.stop_rotation_on_collision;
      pp.fadeout_radius_min = parVelocity.collision.collision_fadeout_radius_min;
      pp.fadeout_radius_max = parVelocity.collision.collision_fadeout_radius_max;


      simOffsets[MODFX_SMOD_VELOCITY_SCENE_COLLISION] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_SCENE_COLLISION);
      ENABLE_DECL(sdecl, MODFX_SDECL_SIM_FLAGS);

      if (parVelocity.collision.collision_decay >= 0)
      {
        ModfxDeclCollisionDecay decayParams;
        decayParams.collision_decay_scale = 1.f - saturate(parVelocity.collision.collision_decay);

        simOffsets[MODFX_SMOD_COLLISION_DECAY] = push_system_data(simulationData, decayParams);
        ENABLE_MOD(smods, MODFX_SMOD_COLLISION_DECAY);

        ENABLE_DECL(sdecl, MODFX_SDECL_COLLISION_TIME);
        ENABLE_DECL(rdecl, MODFX_RDECL_COLOR); // TODO: only .a is needed, maybe a separate fade should be created
      }
    }

    if (parVelocity.wind.enabled && (parVelocity.wind.directional_force + parVelocity.wind.turbulence_force) > 0.f)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ModfxDeclWind pp;
      pp.directional_force = max(parVelocity.wind.directional_force, 0.f);
      pp.directional_freq = max(parVelocity.wind.directional_freq, 0.f);

      pp.turbulence_force = max(parVelocity.wind.turbulence_force, 0.f);
      pp.turbulence_freq = max(parVelocity.wind.turbulence_freq, 0.f);

      pp.impulse_force = parVelocity.wind.impulse_wind && parVelocity.wind.impulse_wind_force > g_value_threshold
                           ? parVelocity.wind.impulse_wind_force
                           : 0;

      int ofs = push_system_data(simulationData, pp);
      simOffsets[MODFX_SMOD_VELOCITY_WIND] = ofs;
      ENABLE_SOFS(VAL_VELOCITY_WIND_COEFF, ofs);
      ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_WIND);
      if (parVelocity.wind.atten.enabled)
      {
        simOffsets[MODFX_SMOD_VELOCITY_WIND_ATTEN] = push_prebake_curve(simulationData, parVelocity.wind.atten.curve);
        ENABLE_MOD(smods, MODFX_SMOD_VELOCITY_WIND_ATTEN);
      }
    }

    if (parVelocity.camera_velocity.enabled)
    {
      ENABLE_DECL(sdecl, MODFX_SDECL_VELOCITY);
      ModfxDeclCameraVelocity pp;
      pp.velocity_weight = -parVelocity.camera_velocity.velocity_weight;
      simOffsets[MODFX_SMOD_CAMERA_VELOCITY] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_CAMERA_VELOCITY);
    }
  }

  // textures + frame
  if (parTex.enabled && (parTex.frames_x <= 0 || parTex.frames_y <= 0))
  {
#if DAGOR_DBGLEVEL > 0
    logerr("fx: modfx: wrong frame data");
#else
    // This logger makes no sence in release builds.
    // And it's to late to file an error in the production.
    // This error only useful for the developer.
    logwarn("fx: modfx: wrong frame data");
#endif
  }

  ddesc.maxTextureSlotsAllocated = 2; // potential limits - diffuse and normals
  if (parTex.enabled && parTex.frames_x > 0 && parTex.frames_y > 0)
  {
    ddesc.texturesPs.clear();

    int boundaryOffset = DAFX_INVALID_BOUNDARY_OFFSET;
    if (parTex.tex_0)
    {
      Texture *texture = acquire_managed_tex((TEXTUREID)(intptr_t)parTex.tex_0);
      if (texture)
      {
        TEXTUREID tex0 = (TEXTUREID)(uintptr_t)parTex.tex_0;
        boundaryOffset = dafx::acquire_frame_boundary(ctx, tex0, IPoint2(parTex.frames_x, parTex.frames_y));
        texture->texfilter(TEXFILTER_LINEAR);
        if (parTex.enable_aniso)
        {
          texture->setAnisotropy(::dgs_tex_anisotropy);
        }
        ddesc.texturesPs.push_back({tex0, parTex.enable_aniso});
        release_managed_tex(tex0);
      }
    }

    if (parTex.tex_1 && !ddesc.texturesPs.empty())
    {
      Texture *texture = acquire_managed_tex((TEXTUREID)(intptr_t)parTex.tex_1);
      if (texture)
      {
        TEXTUREID tex1 = (TEXTUREID)(uintptr_t)parTex.tex_1;
        texture->texfilter(TEXFILTER_LINEAR);
        ddesc.texturesPs.push_back({tex1, false});
        release_managed_tex(tex1);
      }
    }

    ModfxDeclFrameInfo finfo;
    finfo.frames_x = clamp(parTex.frames_x, 1, DAFX_FLIPBOOK_MAX_KEYFRAME_DIM);
    finfo.frames_y = clamp(parTex.frames_y, 1, DAFX_FLIPBOOK_MAX_KEYFRAME_DIM);
    finfo.boundary_id_offset = boundaryOffset;
    finfo.inv_scale = parTex.frame_scale > 0 ? 1.f / parTex.frame_scale : 1.f;

    renOffsets[MODFX_RMOD_FRAME_INFO] = push_system_data(renderData, finfo);
    ENABLE_MOD(rmods, MODFX_RMOD_FRAME_INFO);

    int fc = finfo.frames_x * finfo.frames_y;
    ModfxDeclFrameInit finit;
    finit.start_frame_min = clamp(parTex.start_frame_min, 0, fc - 1);
    finit.start_frame_max = clamp(parTex.start_frame_max, 0, fc - 1);
    finit.flags = 0;

    if (parTex.flip_x)
      finit.flags |= MODFX_FRAME_FLAGS_FLIP_X;

    if (parTex.flip_y)
      finit.flags |= MODFX_FRAME_FLAGS_FLIP_Y;

    if (parTex.random_flip_x)
      finit.flags |= MODFX_FRAME_FLAGS_RANDOM_FLIP_X;

    if (parTex.random_flip_y)
      finit.flags |= MODFX_FRAME_FLAGS_RANDOM_FLIP_Y;

    if (parTex.disable_loop)
      finit.flags |= MODFX_FRAME_FLAGS_DISABLE_LOOP;

    simOffsets[MODFX_SMOD_FRAME_INIT] = push_system_data(simulationData, finit);
    ENABLE_MOD(smods, MODFX_SMOD_FRAME_INIT);

    ENABLE_DECL(sdecl, MODFX_SDECL_RND_SEED);
    if (finit.start_frame_min > 0 || finit.start_frame_max > 0)
      ENABLE_DECL(rdecl, MODFX_RDECL_FRAME_IDX);

    if (finit.flags)
      ENABLE_DECL(rdecl, MODFX_RDECL_FRAME_FLAGS);

    order_swap(parTex.animation.speed_min, parTex.animation.speed_max, 0.f);
    if (parTex.animation.enabled && (parTex.animation.speed_min > 0 || parTex.animation.speed_max > 0) &&
        MODFX_SDECL_LIFE_ENABLED(sdecl))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_FRAME_IDX);

      ModfxDeclFrameAnimInit pp;
      pp.speed_min = parTex.animation.speed_min;
      pp.speed_max = parTex.animation.speed_max;


      simOffsets[MODFX_SMOD_FRAME_ANIM_INIT] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_FRAME_ANIM_INIT);

      if (parTex.animation.animated_flipbook)
      {
        ENABLE_DECL(rdecl, MODFX_RDECL_FRAME_BLEND);
        ENABLE_FLAG(rflags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK);
      }

      if (parTex.animation.over_part_life.enabled)
      {
        simOffsets[MODFX_SMOD_FRAME_ANIM_OVER_PART_LIFE] = push_prebake_curve(simulationData, parTex.animation.over_part_life.curve);
        ENABLE_MOD(smods, MODFX_SMOD_FRAME_ANIM_OVER_PART_LIFE);
      }
    }

    if (parTex.color_matrix.enabled)
    {
      uint4 pp;
      pp.x = parTex.color_matrix.red;
      pp.y = parTex.color_matrix.green;
      pp.z = parTex.color_matrix.blue;
      pp.w = parTex.color_matrix.alpha;

      renOffsets[MODFX_RMOD_TEX_COLOR_MATRIX] = push_system_data(renderData, pp);
      ENABLE_MOD(rmods, MODFX_RMOD_TEX_COLOR_MATRIX);
    }

    if (parTex.color_remap.enabled)
    {
      renOffsets[MODFX_RMOD_TEX_COLOR_REMAP] = push_prebake_grad(renderData, parTex.color_remap.grad);
      ENABLE_MOD(rmods, MODFX_RMOD_TEX_COLOR_REMAP);

      if (parTex.color_remap.apply_base_color)
        ENABLE_FLAG(rflags, MODFX_RFLAG_TEX_COLOR_REMAP_APPLY_BASE_COLOR);

      if (parTex.color_remap.second_mask_enabled)
      {
        ENABLE_FLAG(rflags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK);
        if (parTex.color_remap.second_mask_apply_base_color)
          ENABLE_FLAG(rflags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK_APPLY_BASE_COLOR);
      }

      if (parTex.color_remap.dynamic.enabled && MODFX_SDECL_LIFE_ENABLED(sdecl))
      {
        renOffsets[MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC] =
          push_system_data(renderData, (float)safeinv(max(parTex.color_remap.dynamic.scale, 1.f)));
        ENABLE_DECL(rdecl, MODFX_RDECL_LIFE_NORM);
        ENABLE_MOD(rmods, MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC);
      }
    }
  }
  else
  {
    if (parTex.tex_0)
      release_managed_tex((TEXTUREID)(intptr_t)parTex.tex_0);
    if (parTex.tex_1)
      release_managed_tex((TEXTUREID)(intptr_t)parTex.tex_1);
  }

  // asepct
  if (fabsf(parRenderShape.aspect - 1.f) > g_value_threshold)
  {
    float aspect = clamp(parRenderShape.aspect, 0.f, 2.f);
    renOffsets[MODFX_RMOD_CUSTOM_ASPECT] =
      push_system_data(renderData, float2(1.f - max(1.f - aspect, 0.f), 1.f - max(aspect - 1.f, 0.f)));
    ENABLE_MOD(rmods, MODFX_RMOD_CUSTOM_ASPECT);
  }

  // camera offsets
  if (fabsf(-parRenderShape.camera_offset) > g_value_threshold)
  {
    renOffsets[MODFX_RMOD_CAMERA_OFFSET] = push_system_data(renderData, -(float)parRenderShape.camera_offset);
    ENABLE_MOD(rmods, MODFX_RMOD_CAMERA_OFFSET);
  }

  // render shapes
  if (parRenderShape.type == RSHAPE_TYPE_BBOARD)
  {
    if (fabsf(parRenderShape.billboard.pivot_offset.x) > g_value_threshold ||
        fabsf(parRenderShape.billboard.pivot_offset.y) > g_value_threshold)
    {
      parRenderShape.billboard.pivot_offset.x = clamp(parRenderShape.billboard.pivot_offset.x, -1.f, 1.f);
      parRenderShape.billboard.pivot_offset.y = clamp(parRenderShape.billboard.pivot_offset.y, -1.f, 1.f);

      unsigned char pp[4] = {(unsigned char)(parRenderShape.billboard.pivot_offset.x * 127 + 127),
        (unsigned char)(parRenderShape.billboard.pivot_offset.y * 127 + 127), 0, 0};
      renOffsets[MODFX_RMOD_PIVOT_OFFSET] = push_system_data(renderData, pp, 4);
      ENABLE_MOD(rmods, MODFX_RMOD_PIVOT_OFFSET);
    }

    if (parRenderShape.billboard.screen_view_clamp.x > FLT_EPSILON || parRenderShape.billboard.screen_view_clamp.y > FLT_EPSILON)
    {
      Point2 screen_view_clamp =
        Point2(max(parRenderShape.billboard.screen_view_clamp.x, 0.0f), max(parRenderShape.billboard.screen_view_clamp.y, 0.0f));
      renOffsets[MODFX_RMOD_SCREEN_CLAMP] = push_system_data(renderData, screen_view_clamp);
      ENABLE_MOD(rmods, MODFX_RMOD_SCREEN_CLAMP);
    }

    if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_STATIC_ALIGNED &&
        (length(parRenderShape.billboard.static_aligned_up_vec) > g_value_threshold ||
          length(parRenderShape.billboard.static_aligned_right_vec) > g_value_threshold))
    {
      ModfxDeclShapeStaticAlignedInit spp;
      spp.up_vec = parRenderShape.billboard.static_aligned_up_vec;
      normalizeDef(spp.up_vec, Point3(0, 0, 0));
      spp.right_vec = parRenderShape.billboard.static_aligned_right_vec;
      normalizeDef(spp.right_vec, Point3(0, 0, 0));

      if (lengthSq(spp.up_vec) > 0)
        ENABLE_DECL(rdecl, MODFX_RDECL_UP_VEC);

      if (lengthSq(spp.right_vec) > 0)
        ENABLE_DECL(rdecl, MODFX_RDECL_RIGHT_VEC);

      simOffsets[MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT] = push_system_data(simulationData, spp);
      ENABLE_MOD(smods, MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_UP_VEC_TO_RDECL_ONCE);

      ModfxDeclShapeStaticAligned rpp;
      rpp.cross_fade_mul = saturate(parRenderShape.billboard.cross_fade_mul);
      rpp.cross_fade_pow = max(parRenderShape.billboard.cross_fade_pow, 1);
      rpp.cross_fade_threshold = clamp(parRenderShape.billboard.cross_fade_threshold, 0.f, 0.99f);
      renOffsets[MODFX_RMOD_SHAPE_STATIC_ALIGNED] = push_system_data(renderData, rpp);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_STATIC_ALIGNED);
    }
    else if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_ADAPTIVE_ALIGNED &&
             length(parRenderShape.billboard.static_aligned_up_vec) > g_value_threshold &&
             MOD_ENABLED(rmods, MODFX_RMOD_CUSTOM_ASPECT))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_UP_VEC);
      simOffsets[MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT] =
        push_system_data(simulationData, normalize(parRenderShape.billboard.static_aligned_up_vec));
      ENABLE_MOD(smods, MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_UP_VEC_TO_RDECL_ONCE);
    }
    else if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_VELOCITY && MODFX_SDECL_VELOCITY_ENABLED(sdecl))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_UP_VEC);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_VELOCITY_TO_RDECL);
    }
    else if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_START_VELOCITY && MODFX_SDECL_VELOCITY_ENABLED(sdecl))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_UP_VEC);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_VELOCITY_TO_RDECL);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_UP_VEC_TO_RDECL_ONCE);
    }
    else if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_VELOCITY_MOTION && MODFX_SDECL_VELOCITY_ENABLED(sdecl) &&
             parRenderShape.billboard.velocity_to_length > g_value_threshold &&
             (parRenderShape.billboard.velocity_to_length_clamp.x > g_value_threshold ||
               parRenderShape.billboard.velocity_to_length_clamp.y > g_value_threshold))
    {
      ENABLE_DECL(rdecl, MODFX_RDECL_UP_VEC);
      ENABLE_DECL(rdecl, MODFX_RDECL_VELOCITY_LENGTH);
      ENABLE_FLAG(sflags, MODFX_SFLAG_PASS_VELOCITY_TO_RDECL);

      ModfxDeclShapeVelocityMotion pp;
      pp.length_mul = parRenderShape.billboard.velocity_to_length;
      pp.velocity_clamp = parRenderShape.billboard.velocity_to_length_clamp;
      order_swap(pp.velocity_clamp.x, pp.velocity_clamp.y, 0.f);
      simOffsets[MODFX_SMOD_SHAPE_VELOCITY_MOTION_INIT] = push_system_data(simulationData, pp);
      ENABLE_MOD(smods, MODFX_SMOD_SHAPE_VELOCITY_MOTION_INIT);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_VELOCITY_MOTION);
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED);
    }
    else if (parRenderShape.billboard.orientation == RSHAPE_BBOARD_VIEW_POS)
    {
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_VIEW_POS);
    }
    else //  parRenderShape.billboard.orientation == RSHAPE_BBOARD_SCREEN
    {
      ENABLE_MOD(rmods, MODFX_RMOD_SHAPE_SCREEN);
    }
  }
  else if (parRenderShape.type == RSHAPE_TYPE_RIBBON)
  {
    ModfxDeclRibbonParams pp;

    pp.uv_tile = max((uint)(round(parRenderShape.ribbon.uv_tile) + 0.5), 1u);
    pp.side_fade_params =
      Point2(1.f / max(parRenderShape.ribbon.side_fade_threshold, 0.0001f), max(parRenderShape.ribbon.side_fade_pow, 0.0001f));

    pp.head_fade_params =
      Point2(1.f / max(parRenderShape.ribbon.head_fade_threshold, 0.0001f), max(parRenderShape.ribbon.head_fade_pow, 0.0001f));

    renOffsets[MODFX_RMOD_RIBBON_PARAMS] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_RIBBON_PARAMS);

    if (parRenderShape.ribbon.uv_mapping == RSHAPE_RIBBON_UV_STATIC)
    {
      ENABLE_FLAG(rflags, MODFX_RFLAG_RIBBON_UV_STATIC);
      ENABLE_DECL(rdecl, MODFX_RDECL_UNIQUE_ID);
    }
  }

  if (parLighting.type != LIGHT_NONE)
  {
    // external light
    {
      ModfxDeclExternalOmnilight pp;
      pp.pos = float3(0, 0, 0);
      pp.color = float3(0, 0, 0);
      pp.radius = 0;
      int ofs = push_system_data(renderData, pp);

      renOffsets[MODFX_RMOD_OMNI_LIGHT_INIT] = ofs;
      ENABLE_MOD(rmods, MODFX_RMOD_OMNI_LIGHT_INIT);

      ENABLE_ROFS(VAL_LIGHT_POS, ofs);
      ENABLE_ROFS(VAL_LIGHT_COLOR, ofs + sizeof(float3));
      ENABLE_ROFS(VAL_LIGHT_RADIUS, ofs + sizeof(float3) + sizeof(float3));
    }

    if (parBlending.type != RBLEND_ADD)
    {
      ModfxDeclLighting pp;
      pp.translucency = saturate(parLighting.translucency) * 255;
      pp.normal_softness = saturate(parLighting.normal_softness) * 255;

      pp.specular_power = clamp<int>(parLighting.specular_power, 0, 255);
      pp.specular_strength = saturate(parLighting.specular_strength) * 255;

      pp.sphere_normal_power = saturate(parLighting.sphere_normal_power) * 255;
      pp.sphere_normal_radius = parLighting.sphere_normal_radius;

      if (parLighting.type == LIGHT_UNIFORM)
        pp.type = MODFX_LIGHTING_TYPE_UNIFORM;
      else if (parLighting.type == LIGHT_DISC)
        pp.type = MODFX_LIGHTING_TYPE_DISC;
      else if (parLighting.type == LIGHT_SPHERE)
        pp.type = MODFX_LIGHTING_TYPE_SPHERE;
      else if (parLighting.type == LIGHT_NORMALMAP)
        pp.type = MODFX_LIGHTING_TYPE_NORMALMAP;

      renOffsets[MODFX_RMOD_LIGHTING_INIT] = push_system_data(renderData, pp);
      ENABLE_MOD(rmods, MODFX_RMOD_LIGHTING_INIT);

      if (parLighting.specular_enabled && pp.specular_power > 0 && pp.specular_strength > 0)
        ENABLE_FLAG(rflags, MODFX_RFLAG_LIGHTING_SPECULART_ENABLED);
      if (parLighting.ambient_enabled)
        ENABLE_FLAG(rflags, MODFX_RFLAG_LIGHTING_AMBIENT_ENABLED);
      if (parLighting.external_lights_enabled)
        ENABLE_FLAG(rflags, MODFX_RFLAG_EXTERNAL_LIGHTS_ENABLED);
    }
  }

  // depth mask
  if (parDepthMask.enabled)
  {
    ModfxDepthMask pp;
    pp.znear_clip_offset = max(parDepthMask.znear_clip, 0.f);
    pp.depth_softness_rcp = 1.f / max(parDepthMask.depth_softness, 0.01f);
    pp.znear_softness_rcp = 1.f / max(parDepthMask.znear_softness, 0.01f);
    renOffsets[MODFX_RMOD_DEPTH_MASK] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_DEPTH_MASK);

    if (parDepthMask.use_part_radius)
      ENABLE_FLAG(rflags, MODFX_RFLAG_DEPTH_MASK_USE_PART_RADIUS);
  }

  // blending
  if (parBlending.type == RBLEND_ADD)
  {
    ENABLE_FLAG(rflags, MODFX_RFLAG_BLEND_ADD);
  }
  else if (parBlending.type == RBLEND_ABLEND)
  {
    ENABLE_FLAG(rflags, MODFX_RFLAG_BLEND_ABLEND);
  }
  else
  {
    ENABLE_FLAG(rflags, MODFX_RFLAG_BLEND_PREMULT);
  }

  // render shaders
  if (parRenderShader.reverse_part_order)
    ENABLE_FLAG(rflags, MODFX_RFLAG_REVERSE_ORDER);

  if (!parRenderShader.allow_screen_proj_discard)
    ddesc.specialFlags |= dafx::SystemDesc::FLAG_SKIP_SCREEN_PROJ_CULL_DISCARD;

  if (parRenderShader.shader == RSHADER_DISTORTION || parRenderShader.shader == RSHADER_BBOARD_RAIN_DISTORTION)
  {
    float strength = parRenderShader.shader == RSHADER_DISTORTION
                       ? (float)parRenderShader.modfx_bboard_distortion.distortion_strength
                       : (float)parRenderShader.modfx_bboard_rain_distortion.distortion_strength;
    renOffsets[MODFX_RMOD_DISTORTION_STRENGTH] = push_system_data(renderData, strength);
    ENABLE_MOD(rmods, MODFX_RMOD_DISTORTION_STRENGTH);
  }

  if (parRenderShader.shader == RSHADER_DISTORTION)
  {
    ModfxDeclDistortionFadeColorColorStrength pp;
    pp.fadeRange = (float)parRenderShader.modfx_bboard_distortion.distortion_fade_range;
    pp.fadePower = (float)parRenderShader.modfx_bboard_distortion.distortion_fade_power;
    pp.color = parRenderShader.modfx_bboard_distortion.distortion_rgb.u;
    pp.colorStrength = (float)parRenderShader.modfx_bboard_distortion.distortion_rgb_strength;

    renOffsets[MODFX_RMOD_DISTORTION_FADE_COLOR_COLORSTRENGTH] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_DISTORTION_FADE_COLOR_COLORSTRENGTH);
  }

  if (parRenderShader.shader == RSHADER_BBOARD_VOLSHAPE)
  {
    ModfxDeclVolShapeParams pp;
    pp.thickness = parRenderShader.modfx_bboard_vol_shape.thickness;
    pp.radius_pow = parRenderShader.modfx_bboard_vol_shape.radius_pow;

    renOffsets[MODFX_RMOD_VOLSHAPE_PARAMS] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_VOLSHAPE_PARAMS);
  }

  parPlacement.enabled &= parPlacement.use_hmap || parPlacement.use_depth_above;

  if (parPlacement.enabled)
  {
    ModfxDeclRenderPlacementParams pp;
    pp.flags = 0;
    pp.flags |= parPlacement.use_hmap ? MODFX_GPU_PLACEMENT_HMAP : 0;
    pp.flags |= parPlacement.use_water ? MODFX_GPU_PLACEMENT_WATER : 0;
    pp.flags |= parPlacement.use_depth_above ? MODFX_GPU_PLACEMENT_DEPTH_ABOVE : 0;
    pp.align_normals_offset = parPlacement.align_normals_offset;
    pp.placement_threshold = parPlacement.placement_threshold;
    renOffsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS] = push_system_data(renderData, pp);
    ENABLE_MOD(rmods, MODFX_RMOD_RENDER_PLACEMENT_PARAMS);
  }

  // part trimming
  if (parPartTrimming.enabled && parPartTrimming.steps > 1)
  {
    ModfxDeclPartTrimming pp;
    pp.steps = parPartTrimming.steps * (parPartTrimming.reversed ? -1 : 1);
    pp.fade_mul = max(parPartTrimming.fade_mul, 0.01f);
    pp.fade_pow = clamp(parPartTrimming.fade_pow, 0.01f, 100.f);
    simOffsets[MODFX_SMOD_PART_TRIMMING] = push_system_data(simulationData, pp);
    ENABLE_MOD(smods, MODFX_SMOD_PART_TRIMMING);
  }

  // transform type
  sinfo.transformType = (dafx_ex::TransformType)parGlobals.transform_type;
  G_ASSERT(sinfo.transformType <= dafx_ex::TRANSFORM_LOCAL_SPACE);

  if (sinfo.transformType == dafx_ex::TRANSFORM_LOCAL_SPACE)
    ENABLE_FLAG(rflags, MODFX_RFLAG_USE_ETM_AS_WTM);

  // service data (shared data)
  if (parSpawn.type == SPAWN_DISTANCE_BASED)
  {
    ModfxDeclServiceTrail par;
    par.last_emitter_pos = Point3(0, 0, 0);
    par.prev_last_emitter_pos = Point3(0, 0, 0);
    par.flags = 0;

    ENABLE_FLAG(sflags, MODFX_SFLAG_TRAIL_ENABLED);
    push_system_data(ddesc.serviceData, par);
  }
  if (MODFX_RDECL_UNIQUE_ID_ENABLED(rdecl))
  {
    ModfxDeclServiceUniqueId par;
    par.particles_emitted = 0;

    push_system_data(ddesc.serviceData, par);
  }

  ddesc.serviceDataSize = ddesc.serviceData.size();

  //
  // params prepare done
  //

  // part elem size
  if (MODFX_RDECL_POS_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_POS_SIZE;

  if (MODFX_RDECL_POS_OFFSET_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_POS_OFFSET_SIZE;

  if (MODFX_RDECL_RADIUS_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_RADIUS_SIZE;

  if (MODFX_RDECL_COLOR_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_COLOR_SIZE;

  if (MODFX_RDECL_ANGLE_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_ANGLE_SIZE;

  if (MODFX_RDECL_UP_VEC_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_UP_VEC_SIZE;

  if (MODFX_RDECL_RIGHT_VEC_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_RIGHT_VEC_SIZE;

  if (MODFX_RDECL_VELOCITY_LENGTH_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_VELOCITY_LENGTH_SIZE;

  if (MODFX_RDECL_UNIQUE_ID_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_UNIQUE_ID_SIZE;

  if (MODFX_RDECL_FRAME_IDX_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_FRAME_IDX_SIZE;

  if (MODFX_RDECL_FRAME_FLAGS_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_FRAME_FLAGS_SIZE;

  if (MODFX_RDECL_FRAME_BLEND_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_FRAME_BLEND_SIZE;

  if (MODFX_RDECL_LIFE_NORM_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_LIFE_NORM_SIZE;

  if (MODFX_RDECL_EMISSION_FADE_ENABLED(rdecl))
    ddesc.renderElemSize += MODFX_RDECL_EMISSION_FADE_SIZE;

  if (ddesc.renderElemSize > 0) // -V547
  {
    ddesc.renderElemSize = (ddesc.renderElemSize - 1) / DAFX_ELEM_STRIDE;
    ddesc.renderElemSize = (ddesc.renderElemSize + 1) * DAFX_ELEM_STRIDE;
  }

  if (MODFX_SDECL_LIFE_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_LIFE_SIZE;

  if (MODFX_SDECL_RND_SEED_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_RND_SEED_SIZE;

  if (MODFX_SDECL_VELOCITY_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_VELOCITY_SIZE;

  if (MODFX_SDECL_EMISSION_COLOR_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_EMISSION_COLOR_SIZE;

  if (MODFX_SDECL_SIM_FLAGS_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_SIM_FLAGS_SIZE;

  if (MODFX_SDECL_COLLISION_TIME_ENABLED(sdecl))
    ddesc.simulationElemSize += MODFX_SDECL_COLLISION_TIME_SIZE;

  if (ddesc.simulationElemSize > 0) // -V547
  {
    ddesc.simulationElemSize = (ddesc.simulationElemSize - 1) / DAFX_ELEM_STRIDE;
    ddesc.simulationElemSize = (ddesc.simulationElemSize + 1) * DAFX_ELEM_STRIDE;
  }

  if (parRenderShape.type == RSHAPE_TYPE_RIBBON && parRenderShape.ribbon.type == RSHAPE_RIBBON_SIDE_AND_HEAD)
    ddesc.renderPrimPerPart = 4;

  // clear-up unitialized offsets
  eastl::vector<int> renOffsetsTmp;
  eastl::vector<int> simOffsetsTmp;
  for (int renOffset : renOffsets)
    if (renOffset >= 0)
      renOffsetsTmp.push_back(renOffset);

  for (int simOffset : simOffsets)
    if (simOffset >= 0)
      simOffsetsTmp.push_back(simOffset);

  eastl::swap(renOffsets, renOffsetsTmp);
  eastl::swap(simOffsets, simOffsetsTmp);

  // prapare parent ren data
  int rheadOffset = MODFX_RDECL_BITS / 8 + MODFX_RMODS_BITS / 8 + sizeof(uint32_t);
  for (int &renOffset : renOffsets)
    renOffset = (renOffset + rheadOffset) / DAFX_ELEM_STRIDE + renOffsets.size();

  sdesc.emissionData.renderRefData.resize(rheadOffset + renOffsets.size() * sizeof(uint32_t) + renderData.size());

  unsigned char *renRefPtr = sdesc.emissionData.renderRefData.data();
  memcpy(renRefPtr, &rdecl, MODFX_RDECL_BITS / 8);
  renRefPtr += MODFX_RDECL_BITS / 8;

  memcpy(renRefPtr, rmods, MODFX_RMODS_BITS / 8);
  renRefPtr += MODFX_RMODS_BITS / 8;

  memcpy(renRefPtr, &rflags, sizeof(uint32_t));
  renRefPtr += sizeof(uint32_t);

  memcpy(renRefPtr, renOffsets.data(), renOffsets.size() * sizeof(int));
  renRefPtr += renOffsets.size() * sizeof(int);

  memcpy(renRefPtr, renderData.data(), renderData.size());
  renRefPtr += renderData.size();

  // prapare parent sim data
  int sheadOffset = MODFX_SDECL_BITS / 8 + MODFX_SMODS_BITS / 8 + sizeof(uint32_t);
  for (int &simOffset : simOffsets)
    simOffset = (simOffset + sheadOffset) / DAFX_ELEM_STRIDE + simOffsets.size();

  sdesc.emissionData.simulationRefData.resize(sheadOffset + simOffsets.size() * sizeof(uint32_t) + simulationData.size());

  unsigned char *simRefPtr = sdesc.emissionData.simulationRefData.data();
  memcpy(simRefPtr, &sdecl, MODFX_SDECL_BITS / 8);
  simRefPtr += MODFX_SDECL_BITS / 8;

  memcpy(simRefPtr, smods, MODFX_SMODS_BITS / 8);
  simRefPtr += MODFX_SMODS_BITS / 8;

  memcpy(simRefPtr, &sflags, sizeof(uint32_t));
  simRefPtr += sizeof(uint32_t);

  memcpy(simRefPtr, simOffsets.data(), simOffsets.size() * sizeof(int));
  simRefPtr += simOffsets.size() * sizeof(int);

  memcpy(simRefPtr, simulationData.data(), simulationData.size());
  simRefPtr += simulationData.size();

  // values offsets
  for (auto &it : rvals)
    sinfo.valueOffsets[it.first] = it.second + rheadOffset + renOffsets.size() * sizeof(int);

  for (auto &it : svals)
    sinfo.valueOffsets[it.first] = it.second + sheadOffset + simOffsets.size() * sizeof(int) + sdesc.emissionData.renderRefData.size();

  sinfo.valueOffsets[dafx_ex::SystemInfo::VAL_FLAGS] = MODFX_RDECL_BITS / 8 + MODFX_RMODS_BITS / 8;

  // parent desc
  sdesc.renderElemSize = sdesc.emissionData.renderRefData.size();
  sdesc.simulationElemSize = sdesc.emissionData.simulationRefData.size();

  sdesc.emitterData.type = dafx::EmitterType::FIXED;
  sdesc.emitterData.fixedData.count = 1;
  sdesc.emissionData.type = dafx::EmissionType::REF_DATA;
  sdesc.simulationData.type = dafx::SimulationType::NONE;

  sdesc.qualityFlags = fx_apply_quality_bits(parQuality, 0xffffffff);

  // done
  sdesc.subsystems.emplace_back(ddesc);

  sinfo.rflags = rflags;
  sinfo.sflags = sflags;
  sinfo.spawnRangeLimit = parGlobals.spawn_range_limit;
  sinfo.maxInstances = parGlobals.max_instances;
  sinfo.playerReserved = parGlobals.player_reserved;
  sinfo.onePointNumber = parGlobals.one_point_number;
  sinfo.onePointRadius = parGlobals.one_point_radius;

  return true;
}
