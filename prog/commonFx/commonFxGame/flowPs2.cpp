// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_math3d.h>
#include <math/random/dag_random.h>
#include <math/dag_color.h>
#include <generic/dag_DObject.h>
#include <generic/dag_span.h>
#include <math/dag_curveParams.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_baseFxClasses.h>
#include <debug/dag_debug.h>
#include <flowPs2_decl.h>
#include <stdEmitter.h>
#include <staticVisSphere.h>
#include <math/dag_noise.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include "randvec.h"
#include <gameRes/dag_gameResources.h>
#include <util/dag_stdint.h>
#include <supp/dag_prefetch.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <util/dag_texMetaData.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>

#define NUM_TURBULENCE_BUCKETS 50

class FlowPs2Effect;

static int total_ps = 0;
static int total_parts = 0;
static int live_parts = 0;
static int updated_parts = 0;
static int spawned_parts = 0;
static int turbulenced_parts = 0;
static uint64_t total_time_velacc_full = 0;
static uint64_t total_time_velacc_part = 0;

void get_flowps2_info(int &totalPs, int &totalParts, int &liveParts, int &updatedParts, int &spawnedParts, int &turbulencedParts,
  uint64_t &ttv_full, uint64_t &ttv_part)
{
  totalPs = total_ps;
  totalParts = total_parts;
  liveParts = live_parts;
  updatedParts = updated_parts;
  spawnedParts = spawned_parts;
  turbulencedParts = turbulenced_parts;
  ttv_full = total_time_velacc_full;
  ttv_part = total_time_velacc_part;
}

void reset_flowps2_info()
{
  total_ps = total_parts = live_parts = 0;
  updated_parts = spawned_parts = turbulenced_parts = 0;
  total_time_velacc_full = total_time_velacc_part = 0;
}

class FlowPs2Effect : public BaseParticleEffect
{
public:
  FlowPsParams2 par;

  enum ColorMultMode
  {
    COLOR_MULT_SYSTEM = 0,
    COLOR_MULT_EMITTER
  };

  Ptr<BaseParticleFxEmitter> emitter;
  StaticVisSphere staticVisibilitySphere;

  TMatrix fxTm;
  real fxScale;
  int aliveCounter;
  BBox3 bbox;
  Point3 localWindSpeed;
  bool cloudParticles;


  struct Part
  {
    Point3 pos, vel;
    real angle, wvel;
    real phase;
    bool is_alive;
    real life_, maxLife_;
    Color4 startColor;

    Part() : life_(-1), is_alive(false) {}

    bool isAlive() const { return is_alive; }
    real life() const { return life_; }
    real maxLife() const { return maxLife_; }
  };

  SmallTab<Part> parts;
  SmallTab<StandardFxParticle> renderParts;
  SmallTab<int> partInd;

  Color4 colorMult;

  real spawnCounter, spawnRate, nominalRate;
  real startPhase;
  bool wasBurstSpawned;
  float noiseTime;
  int turbulenceBucketNo;


  FlowPs2Effect() :
    spawnCounter(0),
    spawnRate(0),
    wasBurstSpawned(false),
    colorMult(1, 1, 1, 1),
    nominalRate(0),
    startPhase(0),
    noiseTime(0.),
    turbulenceBucketNo(0),
    aliveCounter(0),
    localWindSpeed(0.f, 0.f, 0.f),
    cloudParticles(false)
  {
    par.numParts = 0;
    fxTm.identity();
    fxScale = 1;
    par.seed = grnd();
  }
  FlowPs2Effect(FlowPs2Effect &&) = default;
  FlowPs2Effect(const FlowPs2Effect &) = default; // should be used with caution only in clone()


  ~FlowPs2Effect()
  {
    release_gameres_or_texres((GameResource *)par.texture);
    release_gameres_or_texres((GameResource *)par.normal);
  }


  virtual BaseParamScript *clone()
  {
    FlowPs2Effect *o = new FlowPs2Effect(*this);
    o->par.seed = grnd();
    if (emitter)
      o->emitter = (BaseParticleFxEmitter *)emitter->clone();

    acquire_managed_tex((TEXTUREID)(uintptr_t)par.texture);
    if (par.normal)
      acquire_managed_tex((TEXTUREID)(uintptr_t)par.normal);

    return o;
  }


  void setNumParts(int num)
  {
    parts.resize(num);
    renderParts.resize(num);

    partInd.clear();

    if (par.life != 0)
      spawnRate = num / par.life;
    else
      spawnRate = 0;

    nominalRate = spawnRate;

    aliveCounter = 0;

    spawnCounter = 0;
  }


  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    int oldNum = par.numParts;
    real oldLife = par.life;

    par.load(ptr, len, load_cb);

    BaseTexture *t = acquire_managed_tex((TEXTUREID)(uintptr_t)par.texture);
    if (t)
    {
      t->texfilter(TEXFILTER_LINEAR);
      release_managed_tex((TEXTUREID)(uintptr_t)par.texture);
    }

    if (par.normal)
    {
      t = acquire_managed_tex((TEXTUREID)(uintptr_t)par.normal);
      if (t)
      {
        t->texfilter(TEXFILTER_LINEAR);
        release_managed_tex((TEXTUREID)(uintptr_t)par.normal);
      }
    }


    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter = new StdParticleEmitter;
    emitter->loadParamsData(ptr, len, load_cb);

    par.rotSpeed *= TWOPI;
    if (par.framesX < 1)
      par.framesX = 1;
    if (par.framesY < 1)
      par.framesY = 1;

    if (par.numParts != oldNum || par.life != oldLife)
      setNumParts(par.numParts);

    wasBurstSpawned = false;
  }


  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
    {
      fxTm = *(TMatrix *)value;
      fxScale = length(fxTm.getcol(0));
    }
    else if (id == HUID_EMITTER_TM && emitter)
      emitter->setParam(HUID_TM, value);
    else if (id == HUID_STATICVISSPHERE)
      *(BSphere3 *)value = BSphere3(staticVisibilitySphere.par.center, staticVisibilitySphere.par.radius);
    else if (id == HUID_VELOCITY && emitter)
      emitter->setParam(id, value);
    else if (id == HUID_EMITTER)
      emitter = (BaseParticleFxEmitter *)value;
    else if (id == HUID_COLOR_MULT)
    {
      Color3 temp = *(Color3 *)value;
      colorMult.r = temp.r;
      colorMult.g = temp.g;
      colorMult.b = temp.b;
      colorMult.a = 1;
    }
    else if (id == HUID_COLOR4_MULT)
    {
      colorMult = *(Color4 *)value;
    }
    else if (id == HUID_SEED)
      par.seed = *(int *)value;
    else if (id == HUID_STARTPHASE)
      startPhase = *(real *)value;
    else if (id == HUID_RANDOMPHASE)
      par.randomPhase = *(real *)value;
    else if (id == HUID_BURST)
    {
      wasBurstSpawned = (*(bool *)value == false);
      if (!wasBurstSpawned)
        for (int i = 0; i < parts.size(); i++)
        {
          parts[i].life_ = 0;
          parts[i].is_alive = false;
        }
    }
    else if (id == HUID_LIFE)
    {
      par.life = *(real *)value;
      if (par.life != 0)
        spawnRate = parts.size() / par.life;
      else
        spawnRate = 0;
      if (spawnRate > nominalRate)
        spawnRate = nominalRate;
    }
    else if (id == HUID_SPAWN_RATE)
      spawnRate = nominalRate * (*(real *)value);
    else if (id == HUID_SET_NUMPARTS)
    {
      int num = *(int *)value;
      parts.resize(num);
      renderParts.resize(num);
      partInd.clear();
      aliveCounter = 0;
      spawnCounter = 0;
    }
    else if (id == _MAKE4C('WIND'))
    {
      localWindSpeed = inverse(fxTm) % *((Point3 *)value);
    }
    else if (id == _MAKE4C('CLOU'))
    {
      cloudParticles = *((bool *)value);
    }
  }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      *(TMatrix *)value = fxTm;
    else if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_VELOCITY && emitter)
      return emitter->getParam(id, value);
    else if (id == HUID_EMITTER)
      return emitter;
    else if (id == HUID_BURST)
      return &par.burstMode;
    else if (id == HUID_BURST_DONE)
    {
      for (int i = 0; i < parts.size(); i++)
        if (parts[i].isAlive())
        {
          if (value)
            *((bool *)value) = false;
          return value;
        }
      if (value)
        *((bool *)value) = true;
      return value;
    }
    else if (id == HUID_COLOR_MULT)
    {
      Color3 temp;
      temp.r = colorMult.r;
      temp.g = colorMult.g;
      temp.b = colorMult.b;
      if (value)
        *(Color3 *)value = temp;
      return value;
    }
    else if (id == HUID_SPAWN_RATE)
    {
      if (value)
        *(real *)value = spawnRate;
      return value;
    }
    else if (id == _MAKE4C('BBOX'))
    {
      G_ASSERT(value);
      *(BBox3 *)value = bbox;
      return value;
    }
    return NULL;
  }

  // virtual bool isSubOf(unsigned id) { return  || __super::isSubOf(id); }

  bool isActive() const { return (aliveCounter > 0); }

  void spawnPart(Part &p, const Point3 &pos, const Point3 &normal, const Point3 &vel)
  {
    ++spawned_parts;
    p.vel = vel + normalize(_rnd_svec(par.seed)) * par.randomVel + par.startVel + normal * par.normalVel;

    real radius = par.size.radius * par.size.sizeFunc.sample(0);
    p.pos = pos + p.vel * par.size.lengthDt * radius;

    p.is_alive = true;
    p.maxLife_ = p.life_ = par.life + _srnd(par.seed) * par.randomLife;
    Point3 rvec = _rnd_vec2s_1f(par.seed);
    p.angle = rvec.x * PI * par.randomRot;
    p.wvel = rvec.y * par.rotSpeed;
    p.phase = rvec.z * par.randomPhase + startPhase;
    float colorLerp = _frnd(par.seed);
    {
      const Color4 color1 = color4(par.color.color);
      const Color4 color2 = color4(par.color.color2);
      p.startColor = color1 * (1.f - colorLerp) + color2 * colorLerp;
    }
    if (par.useColorMult && (par.colorMultMode == COLOR_MULT_EMITTER))
      p.startColor *= colorMult;

    bbox += pos;

    ++aliveCounter;
  }


  void spawnPart(const Point3 &pos, const Point3 &normal, const Point3 &vel)
  {
    Part *__restrict p_ = parts.data();
    Part *__restrict p_end = p_ + parts.size();
    // for (int i=0; i<parts.size(); ++i)
    for (; p_ != p_end; ++p_)
    {
      PREFETCH_DATA(128, p_);
      Part &p = *p_; // parts[i];
      if (p.isAlive())
        continue;
      spawnPart(p, pos, normal, vel);
      break;
    }
  }


  virtual void spawnParts(BaseParticleFxEmitter *em, real amount)
  {
    if (!em || amount <= 0)
      return;

    spawnCounter += amount;

    int particlesReserve = parts.size() - aliveCounter;
    if (spawnCounter > particlesReserve)
      spawnCounter = particlesReserve;

    while (spawnCounter >= 1)
    {
      spawnCounter -= 1;

      Point3 pos, norm, vel;
      Point3 rvec = _rnd_fvec(par.seed);
      em->emitPoint(pos, norm, vel, rvec.x, rvec.y, rvec.z);
      spawnPart(pos, norm, vel);
    }
  }


  void spawnParticles(BaseParticleFxEmitter *em, real amount) { spawnParts(em, amount * par.amountScale); }


  virtual void update(real dt)
  {
    real vk = 1 - par.viscosity * dt;
    if (vk < 0)
      vk = 0;

    real wk = 1 - par.rotViscosity * dt;
    if (wk < 0)
      wk = 0;

    Point3 windOffset = localWindSpeed * par.windScale * dt;

    bbox.setempty();

    if (!parts.empty())
    {
      int old_alive_counter = aliveCounter;
      Part *__restrict p_ = parts.data();
      Part *__restrict p_end = p_ + parts.size();
      // for (int i=0; i<parts.size(); ++i)
      for (; p_ != p_end; ++p_)
      {
        PREFETCH_DATA(128, p_);
        Part &p = *p_; // parts[i];
        if (!p.isAlive())
          continue;
        ++updated_parts;
        const bool will_be_dead = p.life_ < dt;

        p.pos += p.vel * dt;
        p.pos += windOffset;
        bbox += p.pos;
        p.vel *= vk;
        p.vel.y -= par.gravity * dt;


        p.angle += p.wvel * dt;
        p.wvel *= wk;

        // if (!p.isAlive())
        //   --aliveCounter;
        p.life_ -= dt;
        if (will_be_dead)
        {
          p.is_alive = false;
          --aliveCounter;
        }

        if (--old_alive_counter <= 0)
          break;
      }
    }

    if (par.turbulenceMultiplier > 0.f)
    {
      noiseTime += dt * par.turbulenceTimeScale;
      while (noiseTime > 1000.f)
        noiseTime -= 1000.f;

      int startParticleNo = turbulenceBucketNo * parts.size() / NUM_TURBULENCE_BUCKETS;
      int pastLastParticleNo = (turbulenceBucketNo + 1) * parts.size() / NUM_TURBULENCE_BUCKETS;
      float turbulenceMultiplier = dt * par.turbulenceMultiplier * NUM_TURBULENCE_BUCKETS;
      Part *__restrict p_ = parts.data() + startParticleNo;
      Part *__restrict p_end = parts.data() + pastLastParticleNo;
      // for (int i = startParticleNo; i < pastLastParticleNo; i++)
      for (; p_ != p_end; ++p_)
      {
        PREFETCH_DATA(128, p_);
        Part &p = *p_; // parts[i];
        if (!p.isAlive())
          continue;
        ++turbulenced_parts;

        Point3 noisePos = (p.pos + fxTm.getcol(3)) * par.turbulenceScale;

        Point3 noise;

        noise.x = perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, noiseTime);
        noise.y = perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, noiseTime + 10.f);
        noise.z = perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, noiseTime + 20.f);

        noisePos.x *= 2.f;
        noise.x += 0.5f * perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, noiseTime * 2.f);
        noise.y += 0.5f * perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, (noiseTime + 10.f) * 2.f);
        noise.z += 0.5f * perlin_noise::noise4(noisePos.x, noisePos.y, noisePos.z, (noiseTime + 20.f) * 2.f);

        p.vel += noise * turbulenceMultiplier;
      }
      turbulenceBucketNo = (turbulenceBucketNo + 1) % NUM_TURBULENCE_BUCKETS;
    }

    if (!par.burstMode)
    {
      spawnParts(emitter, spawnRate * dt);
    }
    else if (emitter && !wasBurstSpawned)
    {
      wasBurstSpawned = true;

      Part *__restrict p_ = parts.data();
      Part *__restrict p_end = p_ + parts.size();
      // for (int i=0; i<parts.size(); ++i)
      for (; p_ != p_end; ++p_)
      {
        PREFETCH_DATA(128, p_);
        Part &p = *p_; // parts[i];

        Point3 pos, norm, vel;
        Point3 rvec = _rnd_fvec(par.seed);
        emitter->emitPoint(pos, norm, vel, rvec.x, rvec.y, rvec.z);
        spawnPart(p, pos, norm, vel);
      }
    }
  }


  virtual void render(unsigned rtype, const TMatrix &view_itm)
  {
    if ((rtype == FX_RENDER_TRANS && !par.distortion && par.shader != FX_STD_GBUFFERATEST) ||
        (rtype == FX_RENDER_DISTORTION && par.distortion && par.shader != FX_STD_GBUFFERATEST) ||
        (rtype == FX_RENDER_SOLID && par.shader == FX_STD_GBUFFERATEST))
    {
      total_ps++;
      total_parts += parts.size();

      TMatrix vtm = view_itm;
      TMatrix4 globTm;
      if (par.sorted)
        d3d::getglobtm(globTm); // deprecated

      if (softness_distance_var_id != -1)
        ShaderGlobal::set_real(softness_distance_var_id, par.softnessDistance);

      ShaderGlobal::set_real(::get_shader_variable_id("color_scale"), par.color.scale);

      int totalFrames = par.framesX * par.framesY;
      real stepU = 1.0f / par.framesX;
      real stepV = 1.0f / par.framesY;

      if (par.sorted)
      {
        if (partInd.size() != parts.size())
        {
          partInd.resize(parts.size());
          for (int i = 0; i < partInd.size(); ++i)
            partInd[i] = i;
        }
      }

      real colorScale = EffectsInterface::getColorScale() * par.color.scale;

      int actual_rp_count = 0;
      if (!parts.empty())
      {
        int old_alive_counter = aliveCounter;
        Part *__restrict p_ = parts.data();
        Part *__restrict p_end = p_ + parts.size();
        StandardFxParticle *__restrict rp_ = renderParts.data();

        Point3 right[2];
        Point3 up[2];
        float opacity[2];
        if (cloudParticles)
        {
          Point3 viewDirNorm = normalize(vtm.getcol(2));
          right[0] = normalize(viewDirNorm % Point3(0, 1, 0));
          up[0] = right[0] % viewDirNorm;
          opacity[0] = 1.f - fabsf(vtm.getcol(2).y);
          right[1] = normalize(viewDirNorm % Point3(1, 0, 0));
          up[1] = right[1] % viewDirNorm;
          opacity[1] = fabsf(vtm.getcol(2).y);
        }
        else
        {
          right[0] = right[1] = vtm.getcol(0);
          up[0] = up[1] = vtm.getcol(1);
          opacity[0] = opacity[1] = 1.f;
        }

        // for (int i=0; i<parts.size(); ++i)
        bool isOdd = false;
        for (; p_ != p_end; ++p_, ++rp_)
        {
          PREFETCH_DATA(128, p_);
          PREFETCH_DATA(128, rp_);

          isOdd = !isOdd;

          Part &p = *p_;                 // parts[i];
          StandardFxParticle &rp = *rp_; // renderParts[i];

          if (!p.isAlive())
          {
            rp.hide();
            continue;
          }
          ++live_parts;

          rp.pos = fxTm * p.pos;

          if (par.sorted)
            rp.calcSortOrder(rp.pos, globTm);
          else
            rp.show();

          real life = p.life_ / p.maxLife_;
          real time = 1 - life;

          // Color4 color1 = color4(par.color.color);
          // Color4 color2 = color4(par.color.color2);
          // Color4 startColor = color1 * (1.f - p.colorLerp) + color2 * p.colorLerp;
          const Color4 &startColor = p.startColor;

          Color4 color(par.color.rFunc.sample(time) * startColor.r, par.color.gFunc.sample(time) * startColor.g,
            par.color.bFunc.sample(time) * startColor.b, par.color.aFunc.sample(time) * startColor.a);

          color.clamp01();

          if (par.useColorMult && (par.colorMultMode == COLOR_MULT_SYSTEM))
          {
            color.r *= colorScale * colorMult.r;
            color.g *= colorScale * colorMult.g;
            color.b *= colorScale * colorMult.b;
            color.a *= colorMult.a;
          }
          else
          {
            color.r *= colorScale;
            color.g *= colorScale;
            color.b *= colorScale;
          }

          if (isFxShaderWithFakeHdr(par.shader))
            color.a *= par.color.fakeHdrBrightness / 4.0f;

          color.clamp01();

          real radius = par.size.radius * par.size.sizeFunc.sample(1 - life) * fxScale;

          rp.setFacingRotated(right[(int)isOdd], up[(int)isOdd], radius, p.angle);

          color.a *= opacity[(int)isOdd];
          rp.color = e3dcolor(color);
          rp.specularFade = par.color.specFunc.sample(time);


          if (par.size.lengthDt != 0)
          {
            Point3 eyeVec = normalize(vtm.getcol(3) - rp.pos);

            Point3 dp = (fxTm % p.vel) * (par.size.lengthDt / fxScale);
            dp -= (dp * eyeVec) * eyeVec;

            real ldp = length(dp);
            if (ldp != 0)
            {
              dp /= sqrtf(ldp);
              rp.uvec += (rp.uvec * dp) * dp;
              rp.vvec += (rp.vvec * dp) * dp;
            }
          }

          float curStage = totalFrames * (time * par.animSpeed + p.phase);
          int frame = floorf(curStage);
          bool blend = par.animatedFlipbook && par.animSpeed > 0 && time < (1.f - 1.f / (totalFrames * par.animSpeed));
          rp.frameBlend = blend ? curStage - frame : 0;
          frame %= totalFrames;
          if (frame < 0)
            frame += totalFrames;

          rp.setFrame(frame % par.framesX, frame / par.framesX, stepU, stepV);

          if (--old_alive_counter <= 0 && (p_ + 1 != p_end))
            break;
        }
        actual_rp_count = p_ - parts.data();
      }

      if (actual_rp_count > 0)
      {
        EffectsInterface::setStdParticleShader(par.shader);
        EffectsInterface::setStdParticleTexture((TEXTUREID)(uintptr_t)par.texture);

        TEXTUREID normalTexId = par.normal ? (TEXTUREID)(uintptr_t)par.normal : BAD_TEXTUREID;
        EffectsInterface::setStdParticleTexture(normalTexId, 1);
        EffectsInterface::setStdParticleNormalMap(normalTexId, par.shaderType);

        EffectsInterface::setLightingParams(par.ltPower, par.sunLtPower, par.ambLtPower);

        EffectsInterface::renderStdFlatParticles(&renderParts[0],
          actual_rp_count, // renderParts.size(),
          par.sorted ? &partInd[0] : NULL);

        EffectsInterface::setStdParticleNormalMap(BAD_TEXTUREID, 0); // disable normal map, by default
        EffectsInterface::setStdParticleTexture(BAD_TEXTUREID, 1);
      }
    }
  }
};


class FlowPs2Factory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new FlowPs2Effect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "FlowPs2"; }
};

static FlowPs2Factory factory;


void register_flow_ps_2_fx_factory() { register_effect_factory(&factory); }

bool isFlowPs2Active(const FlowPs2Effect *effect) { return effect->isActive(); }
