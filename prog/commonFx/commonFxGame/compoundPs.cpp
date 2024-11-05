// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <generic/dag_DObject.h>
#include <generic/dag_span.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResources.h>

#include <compoundPs_decl.h>
#include <stdEmitter.h>
#include <staticVisSphere.h>


class CompoundPsEffect : public BaseParticleEffect
{
private:
  CompoundPsParams par;
  float time;

public:
  enum
  {
    NUM_FX = 8
  };

  BaseEffectObject *subFx[NUM_FX];
  TMatrix baseSubFxTm[NUM_FX];
  TMatrix subFxTm[NUM_FX];

  Ptr<BaseParticleFxEmitter> emitter;
  bool useCommonEmitter;
  bool useSubFxRotationSpeed[NUM_FX];
  bool useSubFxRotationSpeedTotal;

  StaticVisSphere staticVisibilitySphere;


  CompoundPsEffect() : useCommonEmitter(true)
  {
    time = 0;
    useSubFxRotationSpeedTotal = false;
    for (int i = 0; i < NUM_FX; ++i)
    {
      subFx[i] = NULL;
      baseSubFxTm[i].identity();
      subFxTm[i].identity();
      useSubFxRotationSpeed[i] = false;
    }
  }
  CompoundPsEffect(CompoundPsEffect &&) = default;
  CompoundPsEffect(const CompoundPsEffect &) = default; // should be used with caution only in clone()


  ~CompoundPsEffect()
  {
    for (int i = 0; i < NUM_FX; ++i)
      if (subFx[i])
      {
        // release_game_resource((GameResource *)subFx[i]);
        delete subFx[i];
      }
  }


  virtual BaseParamScript *clone()
  {
    CompoundPsEffect *o = new CompoundPsEffect(*this);

    if (emitter)
      o->emitter = (BaseParticleFxEmitter *)emitter->clone();

    for (int i = 0; i < NUM_FX; ++i)
      if (subFx[i])
      {
        o->subFx[i] = (BaseEffectObject *)subFx[i]->clone();
        o->baseSubFxTm[i] = baseSubFxTm[i];
        o->subFxTm[i] = subFxTm[i];
        if (o->subFx[i] && useCommonEmitter)
          o->subFx[i]->setParam(HUID_EMITTER, o->emitter);
      }

    return o;
  }


  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    par.load(ptr, len, load_cb);
    useSubFxRotationSpeedTotal = false;

#define SETFX(i)                                                                                                                  \
  if (subFx[i - 1])                                                                                                               \
  {                                                                                                                               \
    delete subFx[i - 1];                                                                                                          \
    subFx[i - 1] = NULL;                                                                                                          \
  }                                                                                                                               \
  if (par.fx##i)                                                                                                                  \
  {                                                                                                                               \
    TMatrix scl(1);                                                                                                               \
    scl.m[0][0] = par.fx##i##_scale.x;                                                                                            \
    scl.m[1][1] = par.fx##i##_scale.y;                                                                                            \
    scl.m[2][2] = par.fx##i##_scale.z;                                                                                            \
    subFx[i - 1] = (BaseEffectObject *)((BaseEffectObject *)par.fx##i)->clone();                                                  \
    baseSubFxTm[i - 1] = rotxTM(par.fx##i##_rot.x) * rotyTM(par.fx##i##_rot.y) * rotzTM(par.fx##i##_rot.z) * scl;                 \
    baseSubFxTm[i - 1].setcol(3, par.fx##i##_offset);                                                                             \
    subFxTm[i - 1] = baseSubFxTm[i - 1];                                                                                          \
    useSubFxRotationSpeed[i - 1] =                                                                                                \
      float_nonzero(par.fx##i##_rot_speed.x) || float_nonzero(par.fx##i##_rot_speed.y) || float_nonzero(par.fx##i##_rot_speed.z); \
    if (useSubFxRotationSpeed[i - 1])                                                                                             \
      useSubFxRotationSpeedTotal = true;                                                                                          \
  }

    SETFX(1)
    SETFX(2)
    SETFX(3)
    SETFX(4)
    SETFX(5)
    SETFX(6)
    SETFX(7)
    SETFX(8)

#undef SETFX

    time = 0;

    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter = new StdParticleEmitter;
    emitter->loadParamsData(ptr, len, load_cb);


    useCommonEmitter = par.useCommonEmitter;

    for (int i = 0; i < NUM_FX; ++i)
      if (subFx[i] && useCommonEmitter)
        subFx[i]->setParam(HUID_EMITTER, emitter);


    release_game_resource((GameResource *)par.fx1);
    release_game_resource((GameResource *)par.fx2);
    release_game_resource((GameResource *)par.fx3);
    release_game_resource((GameResource *)par.fx4);
    release_game_resource((GameResource *)par.fx5);
    release_game_resource((GameResource *)par.fx6);
    release_game_resource((GameResource *)par.fx7);
    release_game_resource((GameResource *)par.fx8);
    if (!useCommonEmitter)
      setParam(HUID_EMITTER_TM, (void *)&TMatrix::IDENT);
  }


  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_EMITTER_TM && emitter)
    {
      emitter->setParam(HUID_TM, value);

      if (!useCommonEmitter)
      {
        TMatrix tm;
        for (int i = 0; i < NUM_FX; ++i)
          if (subFx[i])
          {
            tm = (*(TMatrix *)value) * subFxTm[i];
            subFx[i]->setParam(id, &tm);
          }
      }
    }
    else
    {
      if (id == HUID_EMITTER)
        emitter = (BaseParticleFxEmitter *)value;

      if (id == HUID_EMITTER_TM)
      {
        TMatrix tm;
        for (int i = 0; i < NUM_FX; ++i)
          if (subFx[i])
          {
            tm = (*(TMatrix *)value) * subFxTm[i];
            subFx[i]->setParam(id, &tm);
          }
      }
      else if (useCommonEmitter || id != HUID_EMITTER)
      {
        for (int i = 0; i < NUM_FX; ++i)
          if (subFx[i])
            subFx[i]->setParam(id, value);
      }
    }

    if (id == _MAKE4C('WIND') || id == _MAKE4C('CLOU') || id == HUID_BURST)
    {
      for (int i = 0; i < NUM_FX; ++i)
      {
        if (subFx[i])
          subFx[i]->setParam(id, value);
      }
    }
  }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_EMITTER)
      return emitter;
    else if (id == HUID_STATICVISSPHERE)
      *(BSphere3 *)value = BSphere3(staticVisibilitySphere.par.center, staticVisibilitySphere.par.radius);
    else if (id == _MAKE4C('BBOX'))
    {
      G_ASSERT(value);
      BBox3 *bbox = (BBox3 *)value;
      bbox->setempty();
      for (unsigned int subFxNo = 0; subFxNo < NUM_FX; subFxNo++)
      {
        if (subFx[subFxNo])
        {
          BBox3 subFxBbox;
          subFx[subFxNo]->getParam(_MAKE4C('BBOX'), &subFxBbox);
          *bbox += subFxBbox;
        }
      }
      return value;
    }
    else
    {
      for (int i = 0; i < NUM_FX; ++i)
      {
        void *res = subFx[i] ? subFx[i]->getParam(id, value) : NULL;
        if (res)
          return res;
      }
    }

    return NULL;
  }

  // virtual bool isSubOf(unsigned id) { return  || __super::isSubOf(id); }

  inline float getFxDelay(int i)
  {
    float fxDelay = 0.f;
    switch (i)
    {
      case 0: fxDelay = par.fx1_delay; break;
      case 1: fxDelay = par.fx2_delay; break;
      case 2: fxDelay = par.fx3_delay; break;
      case 3: fxDelay = par.fx4_delay; break;
      case 4: fxDelay = par.fx5_delay; break;
      case 5: fxDelay = par.fx6_delay; break;
      case 6: fxDelay = par.fx7_delay; break;
      case 7: fxDelay = par.fx8_delay; break;
      default: G_ASSERT(0);
    }
    return fxDelay;
  }

  inline float getFxEmitterLifeTime(int i)
  {
    float fxEmitterLifeTime = 0.f;
    switch (i)
    {
      case 0: fxEmitterLifeTime = par.fx1_emitter_lifetime; break;
      case 1: fxEmitterLifeTime = par.fx2_emitter_lifetime; break;
      case 2: fxEmitterLifeTime = par.fx3_emitter_lifetime; break;
      case 3: fxEmitterLifeTime = par.fx4_emitter_lifetime; break;
      case 4: fxEmitterLifeTime = par.fx5_emitter_lifetime; break;
      case 5: fxEmitterLifeTime = par.fx6_emitter_lifetime; break;
      case 6: fxEmitterLifeTime = par.fx7_emitter_lifetime; break;
      case 7: fxEmitterLifeTime = par.fx8_emitter_lifetime; break;
      default: G_ASSERT(0);
    }
    return fxEmitterLifeTime;
  }


  virtual void spawnParticles(BaseParticleFxEmitter *em, real amount)
  {
    if (!em || amount <= 0)
      return;

    for (int i = 0; i < NUM_FX; ++i)
    {
      float fxDelay = getFxDelay(i);
      if (subFx[i] && subFx[i]->isSubOf(HUID_BaseParticleEffect) && time >= fxDelay)
        ((BaseParticleEffect *)subFx[i])->spawnParticles(em, amount);
    }
  }


  virtual void update(real dt)
  {
    time += dt;

    TMatrix baseTm;
    if (useSubFxRotationSpeedTotal && emitter && !useCommonEmitter)
      emitter->getParam(HUID_TM, &baseTm);

    for (int i = 0; i < NUM_FX; ++i)
      if (subFx[i])
      {
        float fxLocalTime = time - getFxDelay(i);
        if (fxLocalTime < 0.f)
          continue;

        float fxEmitterLifeTime = getFxEmitterLifeTime(i);
        if (fxEmitterLifeTime > 0.f && fxLocalTime > fxEmitterLifeTime)
        {
          float spawnRate = 0.f;
          subFx[i]->setParam(HUID_SPAWN_RATE, &spawnRate);
        }

        if (useSubFxRotationSpeed[i])
        {
          Point3 rotationSpeed;
          switch (i)
          {
            case 0: rotationSpeed = par.fx1_rot_speed; break;
            case 1: rotationSpeed = par.fx2_rot_speed; break;
            case 2: rotationSpeed = par.fx3_rot_speed; break;
            case 3: rotationSpeed = par.fx4_rot_speed; break;
            case 4: rotationSpeed = par.fx5_rot_speed; break;
            case 5: rotationSpeed = par.fx6_rot_speed; break;
            case 6: rotationSpeed = par.fx7_rot_speed; break;
            case 7: rotationSpeed = par.fx8_rot_speed; break;
            default: rotationSpeed.zero(); G_ASSERT(0);
          }

          TMatrix rotation = rotxTM(TWOPI * rotationSpeed.x * fxLocalTime) * rotyTM(TWOPI * rotationSpeed.y * fxLocalTime) *
                             rotzTM(TWOPI * rotationSpeed.z * fxLocalTime);

          subFxTm[i] = rotation * baseSubFxTm[i];

          if (emitter && !useCommonEmitter)
          {
            TMatrix tm = baseTm * subFxTm[i];
            subFx[i]->setParam(HUID_EMITTER_TM, &tm);
          }
        }

        subFx[i]->update(dt);
      }
  }


  virtual void render(unsigned rtype, const TMatrix &view_itm)
  {
    for (int i = 0; i < NUM_FX; ++i)
    {
      if (subFx[i])
      {
        float fxDelay = getFxDelay(i);
        if (time >= fxDelay)
          subFx[i]->render(rtype, view_itm);
      }
    }
  }
};


class CompoundPsFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new CompoundPsEffect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "CompoundPs"; }
};

static CompoundPsFactory factory;


void register_compound_ps_fx_factory() { register_effect_factory(&factory); }
