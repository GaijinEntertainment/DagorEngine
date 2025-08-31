// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point2.h>
#include <daFx/dafx.h>
#include <daFx/dafx_def.hlsli>
#include <daFx/dafx_hlsl_funcs.hlsli>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include <dafxEmitter.h>
#include <dafxSparks_decl.h>
#include <dafxEmitter_decl.h>
#include <dafxSparkModules/dafx_code_common_decl.hlsli>
#include "dafxSparks_decl.hlsli"
#include "dafxSystemDesc.h"
#include "dafxQuality.h"

namespace dafx_ex
{
#include <dafx_globals.hlsli>
}

enum
{
  HUID_ACES_RESET = 0xA781BF24u
}; //

enum
{
  RGROUP_HIGHRES = 0,
  RGROUP_LOWRES = 1,
  RGROUP_UNDERWATER = 2,
};

enum
{
  COLLISION_DEFAULT = 0,
  COLLISION_DISABLED = 1,
  COLLISION_RELAXED = 2,
};

static dafx::ContextId g_dafx_ctx;
extern void dafx_report_broken_res(int game_res_id, const char *fx_type);

struct DafxSparks : BaseParticleEffect
{
  dafx::ContextId ctx;
  dafx::SystemId sid;
  dafx::InstanceId iid;
  dafx::SystemDesc parentDesc;
  dafx_ex::SystemInfo sinfo;

  int sortOrder = 0;
  int gameResId = 0;
  bool resLoaded = false;

  ~DafxSparks()
  {
    if (iid && g_dafx_ctx)
      dafx::destroy_instance(g_dafx_ctx, iid);
  }
  DafxSparks &operator=(const DafxSparks &) = default; // should be used with caution only in clone()

  DAFX_INLINE
  float modfx_calc_curve_weight(uint steps, float life_k, uint_ref k0, uint_ref k1)
  {
    uint s = steps - 1;
    float k = s * life_k;

    k0 = (uint)k;
    k1 = min(k0 + 1, s);
    return k - (float)k0;
  }

  DAFX_INLINE
  void gen_prebake_curve(eastl::vector<unsigned char> &out, CubicCurveSampler &curve, int steps)
  {
    G_UNUSED(curve);
    out.resize(steps);

    for (int i = 0; i < steps; ++i)
    {
      float k = steps > 1 ? (float)i / (steps - 1) : 0;
      out[i] = saturate(curve.sample(saturate(k))) * 255.f;
    }
  }

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    gameResId = load_cb->getSelfGameResId();
    resLoaded = false;
    if (loadParamsDataInternal(ptr, len, load_cb))
      resLoaded = true;
    else
      dafx_report_broken_res(gameResId, "dafx_sparks");
  }

  bool loadParamsDataInternal(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION_OPT(ptr, len, 5);

    if (!g_dafx_ctx)
    {
      logwarn("fx: sparks: failed to load params data, context was not initialized");
      return false;
    }

    parentDesc = dafx::SystemDesc();

    DafxEmitterInfo emInfo;
    if (!emInfo.loadParamsData(g_dafx_ctx, ptr, len, load_cb))
      return false;

    DafxSparksSimParams simParams = {};
    DafxSparksRenParams renParams = {};
    DafxSparksGlobalParams parGlobals = {};
    DafxSparksOptionalModifiers parOptionalModifiers = {};
    DafxSparksQuality parQuality = {};
    DafxRenderGroup parRenderGroup = {};
    if (len)
    {
#define V(b)                      \
  if (!b.load(ptr, len, load_cb)) \
    return false;

      V(simParams);
      V(renParams);
      V(parGlobals);
      V(parOptionalModifiers);
      V(parQuality);
      V(parRenderGroup);

#undef V
    }

    sinfo.maxInstances = parGlobals.max_instances;
    sinfo.playerReserved = parGlobals.player_reserved;
    sinfo.onePointNumber = parGlobals.one_point_number;
    sinfo.onePointRadius = parGlobals.one_point_radius;
    sinfo.transformType = static_cast<dafx_ex::TransformType>(parGlobals.transform_type);
    sinfo.rflags = 0;

    bool localSpace = sinfo.transformType == dafx_ex::TransformType::TRANSFORM_LOCAL_SPACE;
    sinfo.rflags |= localSpace << DAFX_SPARK_DECL_LOCAL_SPACE;

    bool collisionDisabled = parOptionalModifiers.collision == COLLISION_DISABLED;
    sinfo.rflags |= collisionDisabled << DAFX_SPARK_DECL_COLLISION_DISABLED_FLAG;

    bool collisionRelaxed = parOptionalModifiers.collision == COLLISION_RELAXED;
    sinfo.rflags |= collisionRelaxed << DAFX_SPARK_DECL_COLLISION_RELAXED_FLAG;

    G_STATIC_ASSERT(sizeof(ParentRenData) == sizeof(DafxSparksRenParams) + sizeof(ParentRenData::tm) + sizeof(ParentRenData::flags));
    G_STATIC_ASSERT(sizeof(ParentSimData) == sizeof(DafxSparksSimParams) + sizeof(ParentSimData::fxVelocity) +
                                               sizeof(ParentSimData::widthOverLifeCurve) + sizeof(ParentSimData::gravityTm));

    dafx::SystemDesc desc;
    desc.emissionData.type = dafx::EmissionType::SHADER;
    desc.simulationData.type = dafx::SimulationType::SHADER;
    desc.emissionData.shader = "dafx_sparks_ps_emission";
    desc.simulationData.shader = "dafx_sparks_ps_simulation";

    desc.renderElemSize = DAFX_REN_DATA_SIZE;
    desc.simulationElemSize = DAFX_SIM_DATA_SIZE;

    desc.emitterData = emInfo.emitterData;
    desc.emitterData.minEmissionFactor = clamp(parGlobals.emission_min, 0.f, 1.f);
    desc.gameResId = gameResId;
    parentDesc.gameResId = gameResId;

    if (!parOptionalModifiers.allowScreenProjDiscard)
      desc.specialFlags |= dafx::SystemDesc::FLAG_SKIP_SCREEN_PROJ_CULL_DISCARD;

#define GDATA(a) \
  desc.reqGlobalValues.push_back(dafx::ValueBindDesc(#a, offsetof(dafx_ex::GlobalData, a), sizeof(dafx_ex::GlobalData::a)))

    GDATA(dt);
    GDATA(globtm);
    GDATA(globtm_sim);
    GDATA(world_view_pos);
    GDATA(gravity_zone_count);
    GDATA(proj_hk);
    GDATA(target_size);
    GDATA(target_size_rcp);
    GDATA(depth_size);
    GDATA(depth_size_rcp);
    GDATA(depth_size_for_collision);
    GDATA(depth_size_rcp_for_collision);
    GDATA(depth_tci_offset);
    GDATA(zn_zfar);
    GDATA(wind_dir);
    GDATA(wind_power);

#undef GDATA

    int rtag = 0;
    if (parRenderGroup.type == RGROUP_LOWRES)
      rtag = dafx_ex::RTAG_LOWRES;
    else if (parRenderGroup.type == RGROUP_HIGHRES)
      rtag = dafx_ex::RTAG_HIGHRES;
    else if (parRenderGroup.type == RGROUP_UNDERWATER)
      rtag = dafx_ex::RTAG_UNDERWATER;

    desc.renderDescs.push_back({dafx_ex::renderTags[rtag], "sparks_ps"});
    desc.renderDescs.push_back({dafx_ex::renderTags[dafx_ex::RTAG_THERMAL], "sparks_thermal"});

    parentDesc.qualityFlags = fx_apply_quality_bits(parQuality, 0xffffffff);

    parentDesc.renderElemSize = DAFX_PARENT_REN_DATA_SIZE;
    parentDesc.simulationElemSize = DAFX_PARENT_SIM_DATA_SIZE;

    parentDesc.emitterData.type = dafx::EmitterType::FIXED;
    parentDesc.emitterData.fixedData.count = 1;
    parentDesc.emitterData.spawnRangeLimit = parGlobals.spawn_range_limit;
    parentDesc.emissionData.type = dafx::EmissionType::REF_DATA;
    parentDesc.simulationData.type = dafx::SimulationType::NONE;

    parentDesc.emissionData.renderRefData.resize(parentDesc.renderElemSize);
    parentDesc.emissionData.simulationRefData.resize(parentDesc.simulationElemSize);

    memcpy(parentDesc.emissionData.renderRefData.data(), &TMatrix4::IDENT, sizeof(TMatrix4));
    memcpy(parentDesc.emissionData.renderRefData.data() + sizeof(TMatrix4), &sinfo.rflags, sizeof(uint));
    memset(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_FX_VELOCITY * 4, 0, sizeof(float3));
    memcpy(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_GRAVITY_TM * 4, &Matrix3::IDENT,
      sizeof(Matrix3));

    if (parOptionalModifiers.widthOverLife.enabled)
    {
      memset(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_HEADER * 4, 1,
        sizeof(uint));
      static constexpr int stepCnt = 32;
      eastl::vector<unsigned char> refValues;
      gen_prebake_curve(refValues, parOptionalModifiers.widthOverLife.curve, stepCnt);
      memcpy(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_BODY * 4,
        refValues.data(), stepCnt * sizeof(unsigned char));
    }
    else
    {
      memset(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_HEADER * 4, 0,
        sizeof(uint));
    }

    memcpy(parentDesc.emissionData.renderRefData.data() + sizeof(TMatrix4) + sizeof(uint), &renParams, sizeof(DafxSparksRenParams));
    memcpy(parentDesc.emissionData.simulationRefData.data() + DAFX_PARENT_SIM_DATA_EXTRA_END * 4, &simParams,
      sizeof(DafxSparksSimParams));

    using ValueType = dafx_ex::SystemInfo::ValueType;
    auto desc_bind = [this](int &ofs, const char *name, int size) -> void {
      parentDesc.localValuesDesc.push_back(dafx::ValueBindDesc(name, ofs, size));
      ofs += size;
    };
    auto sinfo_desc_bind = [this, &desc_bind](int &ofs, const char *name, int size, ValueType sinfo_valueType) -> void {
      sinfo.valueOffsets[sinfo_valueType] = ofs;
      desc_bind(ofs, name, size);
    };
    int ofs = 0;

#define DESC_BIND(type, name)                   desc_bind(ofs, name, sizeof(type))
#define SINFO_DESC_BIND(type, name, value_type) sinfo_desc_bind(ofs, name, sizeof(type), value_type)

    // rparams
    DESC_BIND(TMatrix4, "tm");
    DESC_BIND(int, "flags");
    // rparams dynamic
    DESC_BIND(float, "blending");
    DESC_BIND(float, "motionScale");
    DESC_BIND(float, "motionScaleMax");
    DESC_BIND(float, "hdrScale");
    DESC_BIND(float, "arrowShape");

    ofs = DAFX_PARENT_REN_DATA_SIZE;

    // sparams
    DESC_BIND(Point3, "fx_velocity");
    DESC_BIND(CurveData, "width_over_life_curve");
    SINFO_DESC_BIND(Matrix3, "gravity_tm", ValueType::VAL_GRAVITY_ZONE_TM);

    // sparams dynamic
    DESC_BIND(Point3, "pos.center");
    DESC_BIND(Point3, "pos.size");
    SINFO_DESC_BIND(float, "width.start", ValueType::VAL_RADIUS_MIN);
    SINFO_DESC_BIND(float, "width.end", ValueType::VAL_RADIUS_MAX);
    DESC_BIND(float, "life.start");
    DESC_BIND(float, "life.end");
    DESC_BIND(int, "seed");
    SINFO_DESC_BIND(Point3, "velocity.center", ValueType::VAL_VELOCITY_ADD_VEC3);
    SINFO_DESC_BIND(Point3, "velocity.scale", ValueType::VAL_VELOCITY_START_VEC3);
    DESC_BIND(float, "velocity.radiusMin");
    DESC_BIND(float, "velocity.radiusMax");
    DESC_BIND(float, "velocity.azimutMax");
    DESC_BIND(float, "velocity.azimutNoise");
    DESC_BIND(float, "velocity.inclinationMin");
    DESC_BIND(float, "velocity.inclinationMax");
    DESC_BIND(float, "velocity.inclinationNoise");
    DESC_BIND(float, "restitution");
    DESC_BIND(float, "friction");
    SINFO_DESC_BIND(E3DCOLOR, "color0", ValueType::VAL_COLOR_MIN);
    SINFO_DESC_BIND(E3DCOLOR, "color1", ValueType::VAL_COLOR_MAX);
    DESC_BIND(float, "color1Portion");
    DESC_BIND(float, "hdrBias");
    DESC_BIND(E3DCOLOR, "colorEnd");
    DESC_BIND(float, "velocityBias");
    SINFO_DESC_BIND(float, "dragCoefficient", ValueType::VAL_DRAG_COEFF);
    DESC_BIND(float, "turbulenceForce.start");
    DESC_BIND(float, "turbulenceForce.end");
    DESC_BIND(float, "turbulenceFreq.start");
    DESC_BIND(float, "turbulenceFreq.end");
    DESC_BIND(float, "liftForce");
    DESC_BIND(float, "liftTime");
    DESC_BIND(float, "spawnNoise");
    DESC_BIND(Point3, "spawnNoisePos.center");
    DESC_BIND(Point3, "spawnNoisePos.size");
    DESC_BIND(float, "hdrScale1");
    SINFO_DESC_BIND(float, "windForce", ValueType::VAL_VELOCITY_WIND_COEFF);
    DESC_BIND(float, "gravity_zone");

    parentDesc.subsystems.push_back(desc);

#undef DESC_BIND
#undef SINFO_DESC_BIND

    return true;
  }

  void setParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXR'))
    {
      // context was recreated and all systems was released
      if (ctx != g_dafx_ctx)
      {
        G_ASSERT(!sid && !iid);
        sid = dafx::SystemId();
        iid = dafx::InstanceId();
        ctx = g_dafx_ctx;
      }

      if (iid)
        dafx::destroy_instance(g_dafx_ctx, iid);

      eastl::string name;
      name.append_sprintf("sparks_ps_%d", gameResId);
      sid = dafx::get_system_by_name(g_dafx_ctx, name);
      bool forceRecreate = value && *(bool *)value;
      if (sid && forceRecreate)
      {
        dafx::release_system(g_dafx_ctx, sid);
        sid = dafx::SystemId();
      }

      if (!sid)
      {
        sid = dafx::register_system(g_dafx_ctx, parentDesc, name);
        if (!sid)
          logerr("fx: modfx: failed to register system");
      }

      if (sid && forceRecreate)
      {
        iid = dafx::create_instance(g_dafx_ctx, sid);
        G_ASSERT_RETURN(iid, );
        dafx::set_instance_status(g_dafx_ctx, iid, false);
      }
      return;
    }

    if (sid && !iid && id == _MAKE4C('PFXE')) // for tools
      iid = dafx::create_instance(g_dafx_ctx, sid);

    if (!iid)
      return;

    if (id == HUID_TM)
      setTm((TMatrix *)value);
    else if (id == HUID_EMITTER_TM)
      setEmitterTm((TMatrix *)value);
    else if (id == HUID_SPAWN_RATE)
      setSpawnRate((float *)value);
    else if (id == HUID_ACES_RESET)
      reset();
    else if (id == HUID_VELOCITY)
      setVelocity((Point3 *)value);
    else if (id == _MAKE4C('PFXE'))
    {
      G_ASSERT(value);
      BaseEffectEnabled *dafxe = static_cast<BaseEffectEnabled *>(value);
      dafx::set_instance_status(g_dafx_ctx, iid, dafxe->enabled, dafxe->distanceToCam);
    }
    else if (id == _MAKE4C('PFXO'))
      sortOrder = *(int *)value;
    else if (id == _MAKE4C('PFXP'))
    {
      Point4 pos = *(Point4 *)value;
      pos.w -= sortOrder * 0.1f;
      dafx::set_instance_pos(g_dafx_ctx, iid, pos);
    }
    else if (id == _MAKE4C('PFXI'))
    {
      ((eastl::vector<dafx::InstanceId> *)value)->push_back(iid);
    }
    else if (id == _MAKE4C('PFXV'))
      dafx::set_instance_visibility(g_dafx_ctx, iid, value ? *(uint32_t *)value : 0);
    else if (id == _MAKE4C('PFXG'))
      dafx::warmup_instance(g_dafx_ctx, iid, value ? *(float *)value : 0);
    else if (id == _MAKE4C('GZTM'))
      dafx::set_instance_value(g_dafx_ctx, iid, "gravity_tm", value, sizeof(Matrix3));
  }

  void *getParam(unsigned id, void *value) override
  {
    if (id == HUID_ACES_IS_ACTIVE)
    {
      *(bool *)value = dafx::is_instance_renderable_active(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('PFXQ'))
    {
      *(dafx::SystemDesc **)value = resLoaded ? &parentDesc : nullptr;
      return value;
    }
    else if (id == _MAKE4C('PFVR'))
    {
      *(dafx_ex::SystemInfo **)value = &sinfo;
      return value;
    }
    else if (id == _MAKE4C('FXLM'))
    {
      *(int *)value = sinfo.maxInstances;
    }
    else if (id == _MAKE4C('FXLP'))
    {
      *(int *)value = sinfo.playerReserved;
    }
    else if (id == _MAKE4C('FXLR'))
    {
      *(float *)value = parentDesc.emitterData.spawnRangeLimit;
    }
    else if (id == _MAKE4C('FXLX'))
    {
      *(Point2 *)value = Point2(sinfo.onePointNumber, sinfo.onePointRadius);
      return value;
    }
    else if (id == _MAKE4C('CACH'))
    {
      return (void *)1; //-V566
    }
    else if (id == _MAKE4C('CIID'))
    {
      return (void *)&iid; //-V566
    }
    else if (id == _MAKE4C('PFXI'))
    {
      *(dafx::InstanceId *)value = iid;
      return value;
    }
    else if (id == _MAKE4C('PFXC') && value)
      *(bool *)value = dafx::is_instance_renderable_visible(g_dafx_ctx, iid);
    else if (id == _MAKE4C('FXLD'))
    {
      if (value)
        ((eastl::vector<dafx::SystemDesc *> *)value)->push_back(&parentDesc);
      return value;
    }
    else if (id == _MAKE4C('FXIC') && value)
    {
      *(int *)value = dafx::get_instance_elem_count(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('FXID') && value)
    {
      *(dafx::InstanceId *)value = iid;
      return value;
    }

    return nullptr;
  }

  BaseParamScript *clone() override
  {
    DafxSparks *v = new DafxSparks();
    *v = *this;
    v->iid = dafx::InstanceId();

    if (sid)
    {
      G_ASSERT(ctx == g_dafx_ctx);
      v->iid = dafx::create_instance(g_dafx_ctx, sid);
      G_ASSERT(v->iid);
      dafx::set_instance_status(g_dafx_ctx, v->iid, false);
    }

    return v;
  }

  void reset()
  {
    if (!iid)
      return;

    dafx::reset_instance(g_dafx_ctx, iid);

    setTm(&TMatrix::IDENT);
    setEmitterTm(&TMatrix::IDENT);
    Point3 vel(0, 0, 0);
    setVelocity(&vel);

    float spawnRate = 1.f;
    setSpawnRate(&spawnRate);
  }

  void setTransform(const TMatrix &value, dafx_ex::TransformType tr_type)
  {
    int type = sinfo.transformType;
    if (type == dafx_ex::TRANSFORM_DEFAULT)
      type = tr_type;

    TMatrix4 fxTm4 = value;
    bool localSpace = type == dafx_ex::TRANSFORM_LOCAL_SPACE;

    sinfo.rflags &= ~(1 << DAFX_SPARK_DECL_LOCAL_SPACE);
    sinfo.rflags |= localSpace << DAFX_SPARK_DECL_LOCAL_SPACE;

    dafx::set_instance_value(g_dafx_ctx, iid, "tm", &fxTm4, sizeof(fxTm4));
    dafx::set_instance_value(g_dafx_ctx, iid, "flags", &sinfo.rflags, sizeof(sinfo.rflags));
  }

  void setTm(const TMatrix *value) override
  {
    if (iid && value)
      setTransform(*value, dafx_ex::TRANSFORM_LOCAL_SPACE);
  }

  void setEmitterTm(const TMatrix *value) override
  {
    if (iid && value)
      setTransform(*value, dafx_ex::TRANSFORM_WORLD_SPACE);
  }

  void setVelocity(const Point3 *v) override
  {
    if (!iid || !v)
      return;

    dafx::set_instance_value(g_dafx_ctx, iid, "fx_velocity", v, sizeof(Point3));
  }

  void setSpawnRate(const real *v) override
  {
    if (!iid)
      return;

    dafx::set_instance_emission_rate(g_dafx_ctx, iid, v ? *v : 0);
  }

  void update(float) override {}

  void render(unsigned, const TMatrix &) override {}

  void spawnParticles(BaseParticleFxEmitter *, real) override {}
};

class DafxSparksFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new DafxSparks(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "DafxSparks"; }
};

static DafxSparksFactory factory;

void register_dafx_sparks_factory() { register_effect_factory(&factory); }

void dafx_sparksfx_set_context(dafx::ContextId ctx) { g_dafx_ctx = ctx; }
