#include <fx/dag_fxInterface.h>
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
#include <dafxSparkModules/dafx_code_common_decl.hlsl>
#include "dafxSparks_decl.hlsl"
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
  HUID_ACES_IS_ACTIVE = 0xD6872FCEu
}; //

enum
{
  RGROUP_HIGHRES = 0,
  RGROUP_LOWRES = 1,
  RGROUP_UNDERWATER = 2,
};

static dafx::ContextId g_dafx_ctx;

struct DafxSparks : BaseParticleEffect
{
  dafx::ContextId ctx;
  dafx::SystemId sid;
  dafx::InstanceId iid;
  dafx::SystemDesc parentDesc;
  dafx_ex::SystemInfo sinfo;

  int sortOrder = 0;
  TMatrix fxTm = TMatrix::IDENT;
  TMatrix emitterTm = TMatrix::IDENT;
  int gameResId = 0;

  ~DafxSparks()
  {
    if (iid && g_dafx_ctx)
      dafx::destroy_instance(g_dafx_ctx, iid);
  }

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    CHECK_FX_VERSION(ptr, len, 4);

    if (!g_dafx_ctx)
    {
      logwarn("fx: sparks: failed to load params data, context was not initialized");
      return;
    }

    parentDesc = dafx::SystemDesc();

    DafxEmitterInfo emInfo;
    if (!emInfo.loadParamsData(g_dafx_ctx, ptr, len, load_cb))
      return;

    DafxSparksSimParams simParams = {};
    DafxSparksRenParams renParams = {};
    DafxSparksGlobalParams parGlobals = {};
    DafxSparksQuality parQuality = {};
    DafxRenderGroup parRenderGroup = {};
    if (len)
    {
      simParams.load(ptr, len, load_cb);
      renParams.load(ptr, len, load_cb);
      parGlobals.load(ptr, len, load_cb);
      parQuality.load(ptr, len, load_cb);
      parRenderGroup.load(ptr, len, load_cb);
    }

    sinfo.maxInstances = parGlobals.max_instances;
    sinfo.playerReserved = parGlobals.player_reserved;
    sinfo.spawnRangeLimit = parGlobals.spawn_range_limit;
    sinfo.onePointNumber = parGlobals.one_point_number;
    sinfo.onePointRadius = parGlobals.one_point_radius;

    G_ASSERT(sizeof(ParentRenData) == sizeof(DafxSparksRenParams) + sizeof(TMatrix4) + sizeof(Point3));
    G_ASSERT(sizeof(ParentSimData) == sizeof(DafxSparksSimParams) + sizeof(TMatrix4) * 2);

    dafx::SystemDesc desc;
    desc.emissionData.type = dafx::EmissionType::SHADER;
    desc.simulationData.type = dafx::SimulationType::SHADER;
    desc.emissionData.shader = "dafx_sparks_ps_emission";
    desc.simulationData.shader = "dafx_sparks_ps_simulation";

    desc.renderElemSize = DAFX_REN_DATA_SIZE;
    desc.simulationElemSize = DAFX_SIM_DATA_SIZE;

    desc.emitterData = emInfo.emitterData;
    desc.emitterData.minEmissionFactor = clamp(parGlobals.emission_min, 0.f, 1.f);
    gameResId = load_cb->getSelfGameResId();
    desc.gameResId = gameResId;
    parentDesc.gameResId = gameResId;

#define GDATA(a) \
  desc.reqGlobalValues.push_back(dafx::ValueBindDesc(#a, offsetof(dafx_ex::GlobalData, a), sizeof(dafx_ex::GlobalData::a)))

    GDATA(dt);
    GDATA(globtm);
    GDATA(globtm_prev);
    GDATA(world_view_pos);
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
    parentDesc.emissionData.type = dafx::EmissionType::REF_DATA;
    parentDesc.simulationData.type = dafx::SimulationType::NONE;

    parentDesc.emissionData.renderRefData.resize(parentDesc.renderElemSize);
    parentDesc.emissionData.simulationRefData.resize(parentDesc.simulationElemSize);

    memcpy(parentDesc.emissionData.renderRefData.data(), &TMatrix4::IDENT, sizeof(TMatrix4));
    memcpy(parentDesc.emissionData.simulationRefData.data(), &TMatrix4::IDENT, sizeof(TMatrix4));
    memcpy(parentDesc.emissionData.simulationRefData.data() + sizeof(TMatrix4), &TMatrix4::IDENT, sizeof(TMatrix4));

    memcpy(parentDesc.emissionData.renderRefData.data() + sizeof(TMatrix4) + sizeof(Point3), &renParams, sizeof(DafxSparksRenParams));
    memcpy(parentDesc.emissionData.simulationRefData.data() + sizeof(TMatrix4) * 2, &simParams, sizeof(DafxSparksSimParams));

    // binding lambda shortcuts
    typedef dafx_ex::SystemInfo::ValueType ValueType;
    auto desc_bind = [this](dafx::ValueBindDesc &&bind_desc) -> void { parentDesc.localValuesDesc.push_back(bind_desc); };
    auto sinfo_desc_bind = [this](dafx::ValueBindDesc &&bind_desc, ValueType sinfo_valueType) -> void {
      sinfo.valueOffsets[sinfo_valueType] = bind_desc.offset;
      parentDesc.localValuesDesc.push_back(bind_desc);
    };

    // rparams
    desc_bind(dafx::ValueBindDesc("fx_tm", 0, 64));
    desc_bind(dafx::ValueBindDesc("fx_velocity", 64, 12));
    // rparams dynamic
    desc_bind(dafx::ValueBindDesc("blending", 76, 4));
    desc_bind(dafx::ValueBindDesc("motionScale", 80, 4));
    desc_bind(dafx::ValueBindDesc("hdrScale", 84, 4));
    desc_bind(dafx::ValueBindDesc("arrowShape", 88, 4));

    // sparams
    desc_bind(dafx::ValueBindDesc("emitter_wtm", DAFX_PARENT_REN_DATA_SIZE, 64));
    desc_bind(dafx::ValueBindDesc("emitter_ntm", DAFX_PARENT_REN_DATA_SIZE + 64, 64));
    // sparams dynamic
    desc_bind(dafx::ValueBindDesc("pos.center", DAFX_PARENT_REN_DATA_SIZE + 128, 12));
    desc_bind(dafx::ValueBindDesc("pos.size", DAFX_PARENT_REN_DATA_SIZE + 140, 12));
    sinfo_desc_bind(dafx::ValueBindDesc("width.start", DAFX_PARENT_REN_DATA_SIZE + 152, 4), ValueType::VAL_RADIUS_MIN);
    sinfo_desc_bind(dafx::ValueBindDesc("width.end", DAFX_PARENT_REN_DATA_SIZE + 156, 4), ValueType::VAL_RADIUS_MAX);
    desc_bind(dafx::ValueBindDesc("life.start", DAFX_PARENT_REN_DATA_SIZE + 160, 4));
    desc_bind(dafx::ValueBindDesc("life.end", DAFX_PARENT_REN_DATA_SIZE + 164, 4));
    desc_bind(dafx::ValueBindDesc("seed", DAFX_PARENT_REN_DATA_SIZE + 168, 4));
    sinfo_desc_bind(dafx::ValueBindDesc("velocity.center", DAFX_PARENT_REN_DATA_SIZE + 172, 12), ValueType::VAL_VELOCITY_ADD_VEC3);
    sinfo_desc_bind(dafx::ValueBindDesc("velocity.scale", DAFX_PARENT_REN_DATA_SIZE + 184, 12), ValueType::VAL_VELOCITY_START_VEC3);
    desc_bind(dafx::ValueBindDesc("velocity.radiusMin", DAFX_PARENT_REN_DATA_SIZE + 196, 4));
    desc_bind(dafx::ValueBindDesc("velocity.radiusMax", DAFX_PARENT_REN_DATA_SIZE + 200, 4));
    desc_bind(dafx::ValueBindDesc("velocity.azimutMax", DAFX_PARENT_REN_DATA_SIZE + 204, 4));
    desc_bind(dafx::ValueBindDesc("velocity.azimutNoise", DAFX_PARENT_REN_DATA_SIZE + 208, 4));
    desc_bind(dafx::ValueBindDesc("velocity.inclinationMin", DAFX_PARENT_REN_DATA_SIZE + 212, 4));
    desc_bind(dafx::ValueBindDesc("velocity.inclinationMax", DAFX_PARENT_REN_DATA_SIZE + 216, 4));
    desc_bind(dafx::ValueBindDesc("velocity.inclinationNoise", DAFX_PARENT_REN_DATA_SIZE + 220, 4));
    desc_bind(dafx::ValueBindDesc("restitution", DAFX_PARENT_REN_DATA_SIZE + 224, 4));
    desc_bind(dafx::ValueBindDesc("friction", DAFX_PARENT_REN_DATA_SIZE + 228, 4));
    sinfo_desc_bind(dafx::ValueBindDesc("color0", DAFX_PARENT_REN_DATA_SIZE + 232, 4), ValueType::VAL_COLOR_MIN);
    sinfo_desc_bind(dafx::ValueBindDesc("color1", DAFX_PARENT_REN_DATA_SIZE + 236, 4), ValueType::VAL_COLOR_MAX);
    desc_bind(dafx::ValueBindDesc("color1Portion", DAFX_PARENT_REN_DATA_SIZE + 240, 4));
    desc_bind(dafx::ValueBindDesc("colorEnd", DAFX_PARENT_REN_DATA_SIZE + 244, 4));
    desc_bind(dafx::ValueBindDesc("velocityBias", DAFX_PARENT_REN_DATA_SIZE + 248, 4));
    sinfo_desc_bind(dafx::ValueBindDesc("dragCoefficient", DAFX_PARENT_REN_DATA_SIZE + 252, 4), ValueType::VAL_DRAG_COEFF);
    desc_bind(dafx::ValueBindDesc("turbulenceForce.start", DAFX_PARENT_REN_DATA_SIZE + 256, 4));
    desc_bind(dafx::ValueBindDesc("turbulenceForce.end", DAFX_PARENT_REN_DATA_SIZE + 260, 4));
    desc_bind(dafx::ValueBindDesc("turbulenceFreq.start", DAFX_PARENT_REN_DATA_SIZE + 264, 4));
    desc_bind(dafx::ValueBindDesc("turbulenceFreq.end", DAFX_PARENT_REN_DATA_SIZE + 268, 4));
    desc_bind(dafx::ValueBindDesc("liftForce", DAFX_PARENT_REN_DATA_SIZE + 272, 12));
    desc_bind(dafx::ValueBindDesc("liftTime", DAFX_PARENT_REN_DATA_SIZE + 284, 4));
    desc_bind(dafx::ValueBindDesc("spawnNoise", DAFX_PARENT_REN_DATA_SIZE + 288, 4));
    desc_bind(dafx::ValueBindDesc("spawnNoisePos.center", DAFX_PARENT_REN_DATA_SIZE + 292, 12));
    desc_bind(dafx::ValueBindDesc("spawnNoisePos.size", DAFX_PARENT_REN_DATA_SIZE + 304, 12));
    desc_bind(dafx::ValueBindDesc("hdrScale1", DAFX_PARENT_REN_DATA_SIZE + 316, 4));

    parentDesc.subsystems.push_back(desc);
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
    else if (id == _MAKE4C('PFXE') || id == _MAKE4C('PFXF'))
      dafx::set_instance_status(g_dafx_ctx, iid, value ? *(bool *)value : false);
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
      dafx::set_instance_visibility(g_dafx_ctx, iid, value ? *(bool *)value : false);
    else if (id == _MAKE4C('PFXG'))
      dafx::warmup_instance(g_dafx_ctx, iid, value ? *(float *)value : 0);
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
      *(dafx::SystemDesc **)value = &parentDesc;
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
      *(float *)value = sinfo.spawnRangeLimit;
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

  void setTm(const TMatrix *value) override
  {
    fxTm = *(TMatrix *)value;
    TMatrix4 fxTm4 = fxTm;
    fxTm4 = fxTm4.transpose();
    dafx::set_instance_value(g_dafx_ctx, iid, "fx_tm", &fxTm4, 64);
  }

  void setEmitterTm(const TMatrix *value) override
  {
    emitterTm = *(TMatrix *)value;
    TMatrix4 fxTm4 = emitterTm;
    dafx::set_instance_value(g_dafx_ctx, iid, "emitter_wtm", &fxTm4, 64);

    TMatrix emitterNormalTm;
    emitterNormalTm.setcol(0, normalize(emitterTm.getcol(0)));
    emitterNormalTm.setcol(1, normalize(emitterTm.getcol(1)));
    emitterNormalTm.setcol(2, normalize(emitterTm.getcol(2)));
    emitterNormalTm.setcol(3, Point3(0, 0, 0));

    fxTm4 = emitterNormalTm;
    dafx::set_instance_value(g_dafx_ctx, iid, "emitter_ntm", &fxTm4, 64);
  }

  void setVelocity(const Point3 *v) override
  {
    if (!iid || !v)
      return;

    dafx::set_instance_value(g_dafx_ctx, iid, "fx_velocity", v, 12);
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
