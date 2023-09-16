#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <math/random/dag_random.h>
#include <generic/dag_DObject.h>
#include <generic/dag_span.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_baseFxClasses.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <gameRes/dag_gameResources.h>

#include <emitterFlowPs_decl.h>
#include <math/dag_noise.h>
#include <stdEmitter.h>
#include <staticVisSphere.h>

#define PARTICLES_RESERVE_VALUE 0 // 0..1
#define NUM_TURBULENCE_BUCKETS  20

#define CALC_CHILDREN_PARTICLES

class FlowPs2Effect;

extern bool isFlowPs2Active(const FlowPs2Effect *effect);

class EmitterFlowPsEffect : public BaseParticleEffect
{
public:
  TMatrix fxTm;
  real fxScale;
  bool wasBurstSpawned;
  int turbulenceBucketNo;

  EmitterFlowPsParams par;
  Ptr<BaseParticleFxEmitter> emitter;
  StaticVisSphere staticVisibilitySphere;
  BaseEffectObject *subFx;
  BaseParticleFxEmitter *subFxEmitter;

  struct PartBase //-V730
  {
    Point3 pos, vel;
    TMatrix prevTm;
    real life = -1, maxLife;
    real angle, wvel;
    real phase;
  };

  struct Part : PartBase
  {
    Ptr<BaseParticleFxEmitter> emitter;
    BaseEffectObject *effect = NULL;

    Part() { prevTm.identity(); }
    Part(const Part &p)
    {
      static_cast<PartBase &>(*this) = p;
      emitter = p.emitter;
      effect = NULL;
    }
    ~Part() { del_it(effect); }

    bool isAlive() const { return life > 0; }

    void unsetEmit()
    {
      if (effect)
        effect->setParam(HUID_EMITTER, NULL);

      emitter = NULL;
    }

    __forceinline void update(real dt, const TMatrix &fxTm)
    {
      if (emitter)
      {
        TMatrix emTm = TMatrix::IDENT;
        if (vel.lengthSq() != 0)
        {
          emTm.setcol(2, normalize(vel));
          emTm.setcol(1, normalize(emTm.getcol(2) % Point3(1, 0, 0)));
          emTm.setcol(0, normalize(emTm.getcol(2) % emTm.getcol(1)));
          prevTm = emTm;
        }
        else
        {
          emTm = prevTm;
        }

        TMatrix rotTm;
        rotTm.rotzTM(angle);
        emTm = emTm % rotTm;

        emTm.setcol(3, pos * fxTm);
        emitter->setParam(HUID_TM, &emTm);
        // emitter->setParam(HUID_VELOCITY, &vel);

        updateEffect(dt);
      }
    }

    __forceinline void updateEffect(real dt)
    {
      if (effect)
        effect->update(dt);
    }

    __forceinline void render(unsigned int rtype)
    {
      if (effect)
        effect->render(rtype);
    }
  };

  SmallTab<Part> parts;
  real spawnCounter, spawnRate, nominalRate;
  real startPhase;
  double noiseTime;

  EmitterFlowPsEffect() :
    subFx(NULL),
    wasBurstSpawned(false),
    spawnCounter(0),
    spawnRate(0),
    nominalRate(0),
    startPhase(0),
    subFxEmitter(NULL),
    turbulenceBucketNo(0)
  {
    par.numParts = 0;
    fxTm.identity();
    fxScale = 1.f;
    par.seed = grnd();
    noiseTime = 0;
  }


  virtual ~EmitterFlowPsEffect() { del_it(subFx); }


  virtual BaseParamScript *clone()
  {
    EmitterFlowPsEffect *eff = new EmitterFlowPsEffect(*this);
    eff->par.seed = grnd();

    if (emitter)
      eff->emitter = (BaseParticleFxEmitter *)emitter->clone();

    if (subFx)
    {
      eff->subFx = (BaseEffectObject *)subFx->clone();
      subFxEmitter = (BaseParticleFxEmitter *)subFx->getParam(HUID_EMITTER, NULL);

      for (int j = 0; j < eff->parts.size(); ++j)
      {
        eff->parts[j].effect = (BaseEffectObject *)subFx->clone();
        eff->parts[j].emitter = (BaseParticleFxEmitter *)subFxEmitter->clone();
      }
    }

    return eff;
  }


  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    int oldNum = par.numParts;
    real oldLife = par.life;

    par.load(ptr, len, load_cb);

    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter = new StdParticleEmitter;
    emitter->loadParamsData(ptr, len, load_cb);

    par.rotSpeed *= TWOPI;

    if (par.numParts != oldNum || par.life != oldLife)
      setNumParts(par.numParts);

    del_it(subFx);
    subFx = NULL;

    if (par.fx1)
    {
      subFx = (BaseEffectObject *)((BaseEffectObject *)par.fx1)->clone();
#ifdef CALC_CHILDREN_PARTICLES
      float subSpawnRate = 0;
      subFx->getParam(HUID_SPAWN_RATE, &subSpawnRate);
      int num = (int)(subSpawnRate * par.life);
      num += (int)(num * PARTICLES_RESERVE_VALUE);
      subFx->setParam(HUID_SET_NUMPARTS, &num);
#endif
      release_game_resource((GameResource *)par.fx1);
    }
    else
      return;

    subFxEmitter = (BaseParticleFxEmitter *)subFx->getParam(HUID_EMITTER, NULL);
    if (!subFx || !subFxEmitter)
      return;

    for (int i = 0; i < parts.size(); ++i)
    {
      del_it(parts[i].effect);
      parts[i].effect = (BaseEffectObject *)subFx->clone();
      BaseParticleFxEmitter *emitClone = (BaseParticleFxEmitter *)subFxEmitter->clone();
      parts[i].effect->setParam(HUID_EMITTER, emitClone);

      parts[i].emitter = emitClone;
    }

    wasBurstSpawned = false;
  }

  void setNumParts(int num)
  {
    parts.resize(num);

    if (par.life != 0)
      spawnRate = num / par.life;
    else
      spawnRate = 0;

    nominalRate = spawnRate;

    spawnCounter = 0;
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
    else if (id == HUID_VELOCITY && emitter)
      emitter->setParam(id, value);
    else if (id == HUID_EMITTER)
      emitter = (BaseParticleFxEmitter *)value;
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
          parts[i].life = 0.f;
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
  }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      *(TMatrix *)value = fxTm;
    else if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_STATICVISSPHERE)
      *(BSphere3 *)value = BSphere3(staticVisibilitySphere.par.center, staticVisibilitySphere.par.radius);
    else if (id == HUID_VELOCITY && emitter)
      return emitter->getParam(id, value);
    else if (id == HUID_BURST)
      return &par.burstMode;
    else if (id == HUID_BURST_DONE)
    {
      if (!wasBurstSpawned)
      {
        *((bool *)value) = false;
        return value;
      }

      for (int i = 0; i < parts.size(); i++)
        if (parts[i].isAlive() || isFlowPs2Active((FlowPs2Effect *)parts[i].effect))
        {
          if (value)
            *((bool *)value) = false;
          return value;
        }
      if (value)
        *((bool *)value) = true;
      return value;
    }
    return NULL;
  }

  void spawn(int i, const Point3 &pos, const Point3 &normal, const Point3 &vel)
  {
    Part &p = parts[i];

    p.pos = pos;

    p.vel = vel + normalize(Point3(_srnd(par.seed), _srnd(par.seed), _srnd(par.seed))) * par.randomVel + par.startVel +
            normal * par.normalVel;

    p.maxLife = p.life = par.life + _srnd(par.seed) * par.randomLife;
    p.angle = _srnd(par.seed) * PI * par.randomRot;
    p.wvel = _srnd(par.seed) * par.rotSpeed;
    p.phase = _frnd(par.seed) * par.randomPhase + startPhase;

    del_it(p.effect);
    BaseParticleFxEmitter *emitClone = (BaseParticleFxEmitter *)subFxEmitter->clone();
    p.effect = (BaseEffectObject *)subFx->clone();
    p.effect->setParam(HUID_EMITTER, emitClone);
    p.emitter = emitClone;
  }

  void spawn(const Point3 &pos, const Point3 &normal, const Point3 &vel)
  {
    for (int i = 0; i < parts.size(); ++i)
    {
      Part &p = parts[i];
      if (p.isAlive() || isFlowPs2Active((FlowPs2Effect *)p.effect))
        continue;

      spawn(i, pos, normal, vel);
      break;
    }
  }

  virtual void spawn(BaseParticleFxEmitter *em, real amount)
  {
    if (!em || amount <= 0)
      return;

    spawnCounter += amount;

    while (spawnCounter >= 1)
    {
      spawnCounter -= 1;

      Point3 pos, norm, vel;
      em->emitPoint(pos, norm, vel, _frnd(par.seed), _frnd(par.seed), _frnd(par.seed));
      spawn(pos, norm, vel);
    }
  }

  virtual void spawnParticles(BaseParticleFxEmitter *em, real amount) { spawn(em, amount * par.amountScale); }


  virtual void update(real dt)
  {
    if (!par.burstMode)
    {
      spawn(emitter, spawnRate * dt);
    }
    else if (emitter && !wasBurstSpawned)
    {
      wasBurstSpawned = true;

      for (int i = 0; i < parts.size(); ++i)
      {
        Point3 pos, norm, vel;
        emitter->emitPoint(pos, norm, vel, _frnd(par.seed), _frnd(par.seed), _frnd(par.seed));
        spawn(i, pos, norm, vel);
      }
    }

    real vk = 1 - par.viscosity * dt;
    if (vk < 0)
      vk = 0;

    real wk = 1 - par.rotViscosity * dt;
    if (wk < 0)
      wk = 0;

    for (int i = 0; i < parts.size(); ++i)
    {
      Part &p = parts[i];

      if (!p.isAlive())
      {
        if (p.effect && isFlowPs2Active((FlowPs2Effect *)p.effect))
          p.updateEffect(dt);
        continue;
      }


      p.pos += p.vel * dt;
      p.vel *= vk;
      p.vel.y -= par.gravity * dt;

      p.angle += p.wvel * dt;
      p.wvel *= wk;

      p.life -= dt;
      p.update(dt, fxTm);

      if (!p.isAlive())
        p.unsetEmit();
    }

    if (par.turbulenceMultiplier > 0.f)
    {
      noiseTime += dt * par.turbulenceTimeScale;
      while (noiseTime > 1000.f)
        noiseTime -= 1000.f;

      int startParticleNo = turbulenceBucketNo * parts.size() / NUM_TURBULENCE_BUCKETS;
      int pastLastParticleNo = (turbulenceBucketNo + 1) * parts.size() / NUM_TURBULENCE_BUCKETS;
      float turbulenceMultiplier = dt * par.turbulenceMultiplier * NUM_TURBULENCE_BUCKETS;
      for (int i = startParticleNo; i < pastLastParticleNo; i++)
      {
        Part &p = parts[i];
        if (!p.isAlive())
          continue;

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
  }


  virtual void render(unsigned rtype)
  {
    for (int i = 0; i < parts.size(); i++)
    {
      Part &p = parts[i];
      if (p.effect && isFlowPs2Active((FlowPs2Effect *)p.effect))
      {
        parts[i].render(rtype);
      }
    }
  }
};
DAG_DECLARE_RELOCATABLE(EmitterFlowPsEffect::Part);


class EmitterFlowPsFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new EmitterFlowPsEffect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "EmitterFlowPs"; }
};

static EmitterFlowPsFactory factory;


void register_emitterflow_ps_fx_factory() { register_effect_factory(&factory); }
