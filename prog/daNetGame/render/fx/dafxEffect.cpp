// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texMgr.h>
#include <drv/3d/dag_tex3d.h>
#include <daFx/dafx.h>
#include <fx/dag_fxInterface.h>
#include <math/dag_curveParams.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/random/dag_random.h>
#include <ioSys/dag_dataBlock.h>
#include <flowPs2_decl.h>
#include <staticVisSphere.h>
#include "fx.h"
#include "stdEmitter.h"
#include "fxRenderTags.h"
#include <shaders/dag_shaders.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>
#include <daFx/dafx_def.hlsli>
#include <daFx/dafx_hlsl_funcs.hlsli>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include "shaders/dafx_flowps2_decl.hlsli"

namespace dafx_ex
{
#include <dafx_globals.hlsli>
}

static dafx::ContextId g_dafx_ctx;

static const char *g_shader_names[FX__NUM_STD_SHADERS] = {
  "ablend_particles_aces",
  "additive_trails_aces",
  "addsmooth_particles_aces",
  "atest_particles_aces",
  "premultalpha_particles_aces",
  "gbuffer_atest_particles_aces",
  "atest_particles_refraction_aces",
  "gbuffer_patch_aces",
};

static const char *g_shader_haze = "haze_particles_aces";
static const char *g_shader_fom = "fom_particles_aces";
static const char *g_shader_fom_additive = "fom_additive_particles_aces";
static const char *g_shader_volmedia = "volmedia_particles_aces";
static const char *g_shader_bvh = "dafx_modfx_bvh";

#define MAX_FADE_SCALE (4.f)

void StdCubicCurveLoad(StdCubicCurve &dst, const CubicCurveSampler &src)
{
  G_ASSERT(src.type <= 5);
  dst.type = src.type;
  dst.coef.x = src.coef[0];
  dst.coef.y = src.coef[1];
  dst.coef.z = src.coef[2];
  dst.coef.w = src.coef[3];

  memset(dst.spline_coef, 0, 4 * 5 * sizeof(float));

  if (src.splineCoef)
    memcpy(dst.spline_coef, src.splineCoef, min(src.type, 5) * 4 * sizeof(float));
};

void StdEmitterLoad(StdEmitter &dst, const StdParticleEmitter &src)
{
  dst.worldTm = src.emitterTm;
  dst.normalTm = TMatrix4::IDENT;

  for (int y = 0; y < 3; ++y)
    for (int x = 0; x < 3; ++x)
      dst.normalTm[x][y] = src.normTm.m[x][y];

  dst.emitterVel = Point3(0, 0, 0);
  dst.prevEmitterPos = Point3(0, 0, 0);
  dst.size = Point3(src.par.width, src.par.height, src.par.depth);
  dst.radius = src.par.radius;
  dst.isVolumetric = src.par.isVolumetric;
  dst.isTrail = src.par.isTrail;
  dst.geometryType = src.par.geometry;
};

void Flowps2SizeLoad(Flowps2Size &dst, const FlowPsSize2 &src)
{
  dst.radius = src.radius;
  dst.lengthDt = src.lengthDt;
  StdCubicCurveLoad(dst.sizeFunc, src.sizeFunc);
};

void Flowps2ColorLoad(Flowps2Color &dst, const FlowPsColor2 &src)
{
  Color4 c1 = color4(src.color);
  Color4 c2 = color4(src.color2);
  dst.color = Point4(c1.r, c1.g, c1.b, c1.a);
  dst.color2 = Point4(c2.r, c2.g, c2.b, c2.a);
  dst.fakeHdrBrightness = src.fakeHdrBrightness;

  StdCubicCurveLoad(dst.rFunc, src.rFunc);
  StdCubicCurveLoad(dst.gFunc, src.gFunc);
  StdCubicCurveLoad(dst.bFunc, src.bFunc);
  StdCubicCurveLoad(dst.aFunc, src.aFunc);
  StdCubicCurveLoad(dst.specFunc, src.specFunc);
}

#define RPARAM_FXTM       (offsetof(ParentRenData, fxTm))
#define RPARAM_LIGHTPOS   (offsetof(ParentRenData, lightPos))
#define RPARAM_LIGHTPARAM (offsetof(ParentRenData, lightParam))
#define RPARAM_FADESCALE  (offsetof(ParentRenData, fadeScale))
#define RPARAM_SOFTNESS   (offsetof(ParentRenData, softnessDepthRcp))

#define RPOFS              DAFX_PARENT_REN_DATA_SIZE
#define SPARAM_EMITTER_WTM (RPOFS + offsetof(ParentSimData, emitter) + offsetof(StdEmitter, worldTm))
#define SPARAM_EMITTER_NTM (RPOFS + offsetof(ParentSimData, emitter) + offsetof(StdEmitter, normalTm))
#define SPARAM_EMITTER_VEL (RPOFS + offsetof(ParentSimData, emitter) + offsetof(StdEmitter, emitterVel))
#define SPARAM_FXSCALE     (RPOFS + offsetof(ParentSimData, fxScale))
#define SPARAM_SEED        (RPOFS + offsetof(ParentSimData, seed))
#define SPARAM_COLORMULT   (RPOFS + offsetof(ParentSimData, colorMult))
#define SPARAM_ISUSERSCALE (RPOFS + offsetof(ParentSimData, isUserScale))
#define SPARAM_ALPHAHEIGHT (RPOFS + offsetof(ParentSimData, alphaHeightRange))
#define SPARAM_LOCALWIND   (RPOFS + offsetof(ParentSimData, localWindSpeed))
#define SPARAM_ONMOVING    (RPOFS + offsetof(ParentSimData, onMoving))
#define SPARAM_LIFE        (RPOFS + offsetof(ParentSimData, life))

struct DafxFlowPs2Instance : BaseParticleEffect
{
  dafx::SystemId sid;
  dafx::InstanceId iid;
  TMatrix fxTm = TMatrix::IDENT;
  TMatrix emitterTm = TMatrix::IDENT;
  TMatrix emitterNormalTm = TMatrix::IDENT;
  float fxScale = 1.f;
  float fadeScale = 1.f;
  float softnessDepth = 1.f;
  float spawnRate = 1.f;
  float life = 0.f;
  float lifeRef = 0.f;
  bool distortion = false;

  DafxFlowPs2Instance() {}

  ~DafxFlowPs2Instance()
  {
    if (iid)
      dafx::destroy_instance(g_dafx_ctx, iid);
  }

  void reset()
  {
    if (!iid)
      return;

    dafx::reset_instance(g_dafx_ctx, iid);

    setTm(&TMatrix::IDENT);
    setEmitterTm(&TMatrix::IDENT);
    Color4 colorMult(1, 1, 1, 1);
    setColor4Mult(&colorMult);

    Point3 vel0(0, 0, 0);
    setWind(&vel0);
    setVelocity(&vel0);

    int seed = grnd();
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_SEED, &seed, 4);

    Point4 lpos(0, 0, 0, 0);
    dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_LIGHTPOS, &lpos, 16);
    dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_LIGHTPARAM, &lpos, 16);

    spawnRate = 1.f;
    setSpawnRate(&spawnRate);

    life = lifeRef;
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_LIFE, &life, 4);

    int i0 = 0;
    float f1 = 1;

    setParam(_MAKE4C('ONMV'), &i0);
    setParam(_MAKE4C('SCAL'), &f1);
    setParam(_MAKE4C('FADE'), &f1);
  }

  void loadParamsData(const char *, int, BaseParamScriptLoadCB *) override { G_ASSERT(false); }

  BaseParamScript *clone() override
  {
    G_ASSERT(false);
    return nullptr;
  }

  void update(float) override {}

  void render(unsigned, const TMatrix &) override { G_ASSERT(false); }

  void setParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXE'))
    {
      if (value && *(bool *)value)
      {
        if (!iid)
        {
          iid = dafx::create_instance(g_dafx_ctx, sid);
          reset();
        }
        dafx::set_instance_status(g_dafx_ctx, iid, true);
      }
      else
      {
        if (iid)
          dafx::destroy_instance(g_dafx_ctx, iid);
        iid = dafx::InstanceId();
      }
      return;
    }

    if (!iid)
      return;

    if (id == HUID_STARTPHASE)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_RANDOMPHASE)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_BURST)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_SET_NUMPARTS)
    {
      G_ASSERT(false);
    }
    // else if (id == HUID_ACES_DISTRIBUTE_MODE)
    //   G_ASSERT(false);
    else if (id == HUID_EMITTER)
      ; // no-op
    else if (id == _MAKE4C('DENS'))
      G_ASSERT(false);
    else if (id == _MAKE4C('REST'))
      G_ASSERT(false);
    else if (id == _MAKE4C('CLBL'))
      ; // no-op
    else if (id == _MAKE4C('GRND'))
      ; // no-op
    else if (id == _MAKE4C('DIST'))
      ; // no-op
    else if (id == HUID_TM)
      setTm((TMatrix *)value);
    else if (id == HUID_EMITTER_TM)
      setEmitterTm((TMatrix *)value);
    else if (id == HUID_VELOCITY)
      setVelocity((Point3 *)value);
    else if (id == HUID_COLOR_MULT)
      setColorMult((Color3 *)value);
    else if (id == HUID_COLOR4_MULT)
      setColor4Mult((Color4 *)value);
    else if (id == HUID_SEED)
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_SEED, value, 4);
    else if (id == HUID_SPAWN_RATE)
      setSpawnRate((float *)value);
    else if (id == _MAKE4C('WIND'))
      setWind((Point3 *)value);
    else if (id == _MAKE4C('ONMV'))
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_ONMOVING, value, 1);
    else if (id == HUID_ACES_ALPHA_HEIGHT_RANGE)
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_ALPHAHEIGHT, value, 8);
    else if (id == HUID_ACES_RESET)
      reset();
    else if (id == _MAKE4C('PFXF'))
      dafx::set_instance_status(g_dafx_ctx, iid, value && *(bool *)value);
    else if (id == _MAKE4C('PFXD'))
    {
      setSoftnessDepth(*(float *)value);
    }
    else if (id == HUID_LIFE)
    {
      life = *(float *)value;
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_LIFE, &life, 4);
      setSpawnRate(&spawnRate);
    }
    else if (id == _MAKE4C('FADE'))
    {
      fadeScale = 1.f / *(float *)value;
      G_ASSERT(fadeScale > 0.f && fadeScale <= MAX_FADE_SCALE);
      dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_FADESCALE, &fadeScale, 4);
    }
    else if (id == _MAKE4C('SCAL'))
    {
      int v = 1;
      fxScale = max(*(float *)value, 0.f);
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_FXSCALE, value, 4);
      dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_ISUSERSCALE, &v, 1);
      setSoftnessDepth(softnessDepth);
    }
    else if (id == _MAKE4C('PFXL'))
    {
      Point4 *v = (Point4 *)value;
      dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_LIGHTPOS, v + 0, 16);
      dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_LIGHTPARAM, v + 1, 16);
    }
    else if (id == _MAKE4C('PFXP'))
    {
      Point4 pos = *(Point4 *)value;
      dafx::set_instance_pos(g_dafx_ctx, iid, pos);
    }
    else if (id == _MAKE4C('PFXI'))
    {
      ((eastl::vector<dafx::InstanceId> *)value)->push_back(iid);
    }
  }

  void *getParam(unsigned id, void *value) override
  {
    if (id == HUID_EMITTER)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_COLOR_MULT)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_TM)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_EMITTER_TM)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_VELOCITY)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_BURST)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_BURST_DONE)
    {
      G_ASSERT(false);
    }
    else if (id == HUID_SPAWN_RATE)
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('VISM'))
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('WBOX'))
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('STAT'))
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('PFXS'))
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('PFXD'))
    {
      G_ASSERT(false);
    }
    else if (id == _MAKE4C('DSTR'))
    {
      if (value)
        *(bool *)value = distortion;
      return value;
    }
    else if (id == HUID_ACES_IS_ACTIVE)
    {
      *(bool *)value = iid ? dafx::is_instance_renderable_active(g_dafx_ctx, iid) : false;
      return value;
    }
    else if (id == _MAKE4C('SCAL'))
    {
      *(float *)value = fxScale;
      return value;
    }
    else if (id == _MAKE4C('PFXI'))
    {
      *(dafx::InstanceId *)value = iid;
      return value;
    }

    return nullptr;
  }

  void spawnParticles(BaseParticleFxEmitter *, real) override { G_ASSERT(false); }

  void setTm(const TMatrix *value) override
  {
    fxTm = *(TMatrix *)value;

    int v = 0;
    fxScale = length(fxTm.getcol(0));
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_FXSCALE, &fxScale, 4);
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_ISUSERSCALE, &v, 1);

    TMatrix4 fxTm4 = fxTm;
    fxTm4 = fxTm4.transpose();
    dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_FXTM, &fxTm4, 64);

    setSoftnessDepth(softnessDepth);
  }

  void setEmitterTm(const TMatrix *value) override
  {
    emitterTm = *(TMatrix *)value;

    TMatrix4 fxTm4 = emitterTm;
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_EMITTER_WTM, &fxTm4, 64);

    emitterNormalTm.setcol(0, normalize(emitterTm.getcol(0)));
    emitterNormalTm.setcol(1, normalize(emitterTm.getcol(1)));
    emitterNormalTm.setcol(2, normalize(emitterTm.getcol(2)));

    fxTm4 = emitterNormalTm;
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_EMITTER_NTM, &fxTm4, 64);
  }

  void setVelocity(const Point3 *value) override { dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_EMITTER_VEL, value, 12); }

  void setColorMult(const Color3 *value) override
  {
    Color4 v;
    v.r = value->r;
    v.g = value->g;
    v.b = value->b;
    v.a = 1;
    setColor4Mult(&v);
  }

  void setColor4Mult(const Color4 *value) override { dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_COLORMULT, value, 16); }

  void setWind(const Point3 *value) override
  {
    Point3 localWindSpeed = inverse(fxTm) % *value;
    dafx::set_instance_value(g_dafx_ctx, iid, SPARAM_LOCALWIND, &localWindSpeed, 12);
  }

  void setSpawnRate(const real *v) override
  {
    if (!iid)
      return;

    spawnRate = *v;
    float k = spawnRate * safeinv(life / lifeRef);
    dafx::set_instance_emission_rate(g_dafx_ctx, iid, max(k, 0.f));
  }

  void setSoftnessDepth(float v)
  {
    softnessDepth = v;
    float rcp = safeinv(fxScale * softnessDepth);
    dafx::set_instance_value(g_dafx_ctx, iid, RPARAM_SOFTNESS, &rcp, 4);
  }
};

struct DafxFlowPs2System : BaseParticleEffect
{
  FlowPsParams2 par;
  dafx::SystemId sid;
  ParentRenData rdata;
  ParentSimData sdata;
  StdParticleEmitter emitter;
  bool canCreateInstance = false;

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    CHECK_FX_VERSION(ptr, len, 2);

    par.load(ptr, len, load_cb);

    StaticVisSphere staticVisibilitySphere;
    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter.loadParamsData(ptr, len, load_cb);

    par.rotSpeed *= TWOPI;
    if (par.framesX < 1)
      par.framesX = 1;
    if (par.framesY < 1)
      par.framesY = 1;

    G_ASSERT(par.life > 0);
    G_ASSERT((par.framesX & (par.framesX - 1)) == 0);
    G_ASSERT((par.framesY & (par.framesY - 1)) == 0);

    rdata.fxTm = TMatrix4::IDENT;
    rdata.lightPos = Point4(0, 0, 0, 0);
    rdata.lightParam = Point4(0, 0, 0, 0);
    rdata.colorScale = par.color.scale;
    rdata.fadeScale = 1;
    rdata.softnessDepthRcp = 1;
    rdata.shader = par.shader;
    rdata.framesX = par.framesX;
    rdata.framesY = par.framesY;

    sdata.seed = par.seed;
    sdata.fxScale = 1.f;
    sdata.colorMult = Point4(1, 1, 1, 1);
    sdata.isUserScale = 0;
    sdata.alphaHeightRange = Point2(-100000.0f, -100000.0f);
    sdata.localWindSpeed = Point3(0, 0, 0);
    sdata.onMoving = 0;
    sdata.life = par.life;
    StdEmitterLoad(sdata.emitter, emitter);

    Flowps2SizeLoad(sdata.size, par.size);
    Flowps2ColorLoad(sdata.color, par.color);
    sdata.startPhase = 0;
    sdata.randomPhase = par.randomPhase;
    sdata.randomLife = par.randomLife;
    sdata.randomVel = par.randomVel;
    sdata.normalVel = par.normalVel;
    sdata.startVel = par.startVel;
    sdata.gravity = par.gravity;
    sdata.viscosity = par.viscosity;
    sdata.randomRot = par.randomRot;
    sdata.rotSpeed = par.rotSpeed;
    sdata.rotViscosity = par.rotViscosity;
    sdata.animSpeed = par.animSpeed;
    sdata.windScale = par.windScale;
    sdata.animatedFlipbook = par.animatedFlipbook;

    // dafx is hangling tex management, so no need to keep an acquired copy
    Texture *texture = acquire_managed_tex((TEXTUREID)(intptr_t)par.texture);
    if (texture)
    {
      release_managed_tex((TEXTUREID)(intptr_t)par.texture);
      release_managed_tex((TEXTUREID)(intptr_t)par.texture);
    }

    if (par.normal)
    {
      Texture *normalMapTex = acquire_managed_tex((TEXTUREID)(intptr_t)par.normal);
      if (normalMapTex)
      {
        release_managed_tex((TEXTUREID)(intptr_t)par.normal);
        release_managed_tex((TEXTUREID)(intptr_t)par.normal);
      }
    }
  }

  BaseParamScript *clone() override
  {
    if (!sid || !canCreateInstance) // compoundPs trying to grab a copy
    {
      DafxFlowPs2System *v = new DafxFlowPs2System();
      *v = *this;
      return v;
    }
    else
    {
      DafxFlowPs2Instance *v = new DafxFlowPs2Instance();
      v->sid = sid;
      v->iid = dafx::create_instance(g_dafx_ctx, sid);
      G_ASSERT(v->iid);
      v->life = par.life;
      v->lifeRef = par.life;
      v->distortion = par.distortion;
      v->reset();
      dafx::set_instance_status(g_dafx_ctx, v->iid, false); // fx manager creates instances in advance
      return v;
    }
  }

  void update(float) override { G_ASSERT(false); }

  void render(unsigned, const TMatrix &) override { G_ASSERT(false); }

  void setParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXA'))
      canCreateInstance = value ? *(bool *)value : false;
  }

  void *getParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXS'))
    {
      *(int *)value = 1;
      return value;
    }
    else if (id == _MAKE4C('PFXD'))
    {
      *(int *)value = 0;
      return value;
    }
    else if (id == _MAKE4C('DSTR'))
    {
      *(bool *)value = par.distortion;
      return value;
    }
    else if (id == _MAKE4C('PFXT'))
    {
      *(dafx::SystemId *)value = sid;
      return value;
    }

    return nullptr;
  }

  void spawnParticles(BaseParticleFxEmitter *, real) override { G_ASSERT(false); }
};

bool register_dafx_system(
  BaseEffectObject *src, uint32_t tags, FxRenderGroup group, const DataBlock &blk, int sort_depth, const eastl::string &name)
{
  G_ASSERT(src);

  int isDafx = 0;
  if (!src->getParam(_MAKE4C('PFXS'), &isDafx) || !isDafx)
  {
    G_ASSERT(false);
    return false;
  }

  isDafx = 0;
  if (src->getParam(_MAKE4C('PFXD'), &isDafx) && isDafx)
  {
    G_ASSERT(false);
    return false;
  }

  DafxFlowPs2System *srcSys = static_cast<DafxFlowPs2System *>(src);

  if (srcSys->sid) // already registered
    return true;

  const FlowPsParams2 &par = srcSys->par;

  if (par.life == 0)
  {
    logerr("fx: %s have life=0", name);
    return false;
  }

  if (par.shader < 0 || par.shader >= FX__NUM_STD_SHADERS)
  {
    logerr("fx: %s, invalid shader_id: %d", name, par.shader);
    return false;
  }

  float collisionBounceFactor = blk.getReal("collisionBounceFactor", 0.f);
  srcSys->sdata.collisionBounceFactor = collisionBounceFactor;

  float collisionBounceDecay = blk.getReal("collisionBounceDecay", 1.f);
  srcSys->sdata.collisionBounceDecay = collisionBounceDecay;

  dafx::SystemDesc desc;
  desc.emissionData.type = dafx::EmissionType::SHADER;
  desc.simulationData.type = dafx::SimulationType::SHADER;

  if (collisionBounceFactor > 0 || collisionBounceDecay < 0.99)
  {
    desc.emissionData.shader = "dafx_flowps2_emission_gpu_features";
    desc.simulationData.shader = "dafx_flowps2_simulation_gpu_features";
  }
  else
  {
    desc.emissionData.shader = "dafx_flowps2_emission";
    desc.simulationData.shader = "dafx_flowps2_simulation";
  }

  desc.renderElemSize = DAFX_REN_DATA_SIZE;
  desc.simulationElemSize = DAFX_SIM_DATA_SIZE;

  desc.renderSortDepth = sort_depth;

  if (blk.paramExists("isTrail")) // fx.blk can override isTrail
    srcSys->sdata.emitter.isTrail = blk.getBool("isTrail", false);

  if (srcSys->sdata.emitter.isTrail)
  {
    desc.serviceDataSize = sizeof(Point3) + sizeof(Point3) + sizeof(int);
    desc.serviceData.resize(desc.serviceDataSize, 0);
  }

  if (par.burstMode)
  {
    G_ASSERT(par.life >= 0 && par.randomLife >= 0);

    desc.emitterData.type = dafx::EmitterType::BURST;
    desc.emitterData.burstData.lifeLimit = par.life + par.randomLife;
    desc.emitterData.burstData.cycles = 1;
    desc.emitterData.burstData.countMin = par.numParts;
    desc.emitterData.burstData.countMax = par.numParts;
  }
  else
  {
    desc.emitterData.type = dafx::EmitterType::LINEAR;
    desc.emitterData.linearData.lifeLimit = par.life + par.randomLife;
    desc.emitterData.linearData.countMin = par.numParts;
    desc.emitterData.linearData.countMax = par.numParts;
  }

  desc.texturesPs.clear();
  desc.texturesPs.push_back({(TEXTUREID)(intptr_t)par.texture, false});

  if (par.normal && !desc.texturesPs.empty())
    desc.texturesPs.push_back({(TEXTUREID)(intptr_t)par.normal, false});

#define GDATA(a) \
  desc.reqGlobalValues.push_back(dafx::ValueBindDesc(#a, offsetof(dafx_ex::GlobalData, a), sizeof(dafx_ex::GlobalData::a)))

  GDATA(dt);
  GDATA(zn_zfar);
  GDATA(globtm);
  GDATA(depth_size);
  GDATA(depth_size_rcp);
  GDATA(normals_size);
  GDATA(normals_size_rcp);

  int defaultShaderId = par.shader;
  if (par.shader == FX_STD_ABLEND || par.shader == FX_STD_ADDITIVE)
    defaultShaderId = FX_STD_PREMULTALPHA;

  if (par.shader == FX_STD_GBUFFERATEST)
    desc.renderDescs.push_back({render_tags[ERT_TAG_GBUFFER], g_shader_names[FX_STD_GBUFFERATEST]});
  else if (par.shader == FX_STD_ATEST_REFRACTION)
    desc.renderDescs.push_back({render_tags[ERT_TAG_ATEST], g_shader_names[FX_STD_ATEST_REFRACTION]});
  else if (par.shader == FX_STD_GBUFFER_PATCH)
    desc.renderDescs.push_back({render_tags[ERT_TAG_GBUFFER], g_shader_names[FX_STD_GBUFFER_PATCH]});
  else if (group == FX_RENDER_GROUP_WATER_PROJ)
    desc.renderDescs.push_back({render_tags[ERT_TAG_WATER_PROJ], g_shader_names[defaultShaderId]});
  else if (par.distortion)
    desc.renderDescs.push_back({render_tags[ERT_TAG_HAZE], g_shader_haze});

  else if (group == FX_RENDER_GROUP_LOWRES || group == FX_RENDER_GROUP_HIGHRES) // meaning modfx is low res for "combined" as well (so
                                                                                // sparks can be high res all the time)
    desc.renderDescs.push_back({render_tags[ERT_TAG_LOWRES], g_shader_names[defaultShaderId]});

  if (tags & (1 << ERT_TAG_VOLMEDIA))
    desc.renderDescs.push_back({render_tags[ERT_TAG_VOLMEDIA], g_shader_volmedia});

  if (group == FX_RENDER_GROUP_LOWRES || group == FX_RENDER_GROUP_HIGHRES)
    desc.renderDescs.push_back({render_tags[ERT_TAG_BVH], g_shader_bvh});

  if (tags & (1 << ERT_TAG_FOM))
  {
    if (par.shader == FX_STD_ADDITIVE || par.shader == FX_STD_ADDSMOOTH)
      desc.renderDescs.push_back({render_tags[ERT_TAG_FOM], g_shader_fom_additive});
    else
      desc.renderDescs.push_back({render_tags[ERT_TAG_FOM], g_shader_fom});
  }

  G_STATIC_ASSERT(sizeof(ParentRenData) == DAFX_PARENT_REN_DATA_SIZE);
  G_STATIC_ASSERT(sizeof(ParentSimData) == DAFX_PARENT_SIM_DATA_SIZE);

  dafx::SystemDesc parentDesc;
  parentDesc.renderElemSize = DAFX_PARENT_REN_DATA_SIZE;
  parentDesc.simulationElemSize = DAFX_PARENT_SIM_DATA_SIZE;
  parentDesc.emitterData.type = dafx::EmitterType::FIXED;
  parentDesc.emitterData.fixedData.count = 1;
  parentDesc.emissionData.type = dafx::EmissionType::REF_DATA;
  parentDesc.simulationData.type = dafx::SimulationType::NONE;

  parentDesc.emissionData.renderRefData.resize(parentDesc.renderElemSize);
  parentDesc.emissionData.simulationRefData.resize(parentDesc.simulationElemSize);

  memcpy(parentDesc.emissionData.renderRefData.data(), &srcSys->rdata, sizeof(ParentRenData));
  memcpy(parentDesc.emissionData.simulationRefData.data(), &srcSys->sdata, sizeof(ParentSimData));

  parentDesc.localValuesDesc.push_back(dafx::ValueBindDesc("colorScale", offsetof(ParentRenData, colorScale), 4));
  parentDesc.localValuesDesc.push_back(dafx::ValueBindDesc("fadeScale", offsetof(ParentRenData, fadeScale), 4));

  int sofs = DAFX_PARENT_REN_DATA_SIZE;
  parentDesc.localValuesDesc.push_back(
    dafx::ValueBindDesc("collisionBounceFactor", sofs + offsetof(ParentSimData, collisionBounceFactor), 4));
  parentDesc.localValuesDesc.push_back(
    dafx::ValueBindDesc("collisionBounceDecay", sofs + offsetof(ParentSimData, collisionBounceDecay), 4));

  parentDesc.subsystems.push_back(desc);

  srcSys->sid = dafx::register_system(g_dafx_ctx, parentDesc, name);
  return (bool)srcSys->sid;
}

class DafxFlowPs2Factory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new DafxFlowPs2System(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "FlowPs2"; }
};

static DafxFlowPs2Factory g_dafx_flowps2_factory;

static bool g_dafx_flowps2_registered = false;
void register_dafx_factory()
{
  register_effect_factory(&g_dafx_flowps2_factory);
  g_dafx_flowps2_registered = true;
  ShaderGlobal::set_int(::get_shader_variable_id("use_dafx", /*opt*/ true), 1);
}

void dafx_flowps2_set_context(dafx::ContextId ctx) { g_dafx_ctx = ctx; }
