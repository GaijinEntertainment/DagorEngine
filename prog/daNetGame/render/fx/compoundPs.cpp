// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <generic/dag_DObject.h>
#include <generic/dag_span.h>
#include <gameRes/dag_gameResources.h>
#include <fx/dag_baseFxClasses.h>

#include <compoundPs_decl.h>
#include "stdEmitter.h"
#include <staticVisSphere.h>
#include <EASTL/string.h>

#include "render/fx/fx.h"

#define ITERATE_OVER_SUBFX() for (int i = 0; i < numEffects; ++i)

#define INVOKE_OVER_SUBFX_EXT(method, param, ext) \
  for (int i = 0; i < numEffects; ++i)            \
  {                                               \
    ext;                                          \
    subFx[i]->method(param);                      \
  }

#define INVOKE_OVER_SUBFX(method, param) INVOKE_OVER_SUBFX_EXT(method, param, )

class CompoundPsEffect : public BaseParticleEffect
{
public:
  enum
  {
    NUM_FX = 8
  };

  int numEffects;
  BaseEffectObject *subFx[NUM_FX];
  TMatrix subFxTm[NUM_FX];

  Ptr<BaseParticleFxEmitter> emitter;
  bool useCommonEmitter;

  CompoundPsEffect() : useCommonEmitter(true), numEffects(0)
  {
    for (int i = 0; i < NUM_FX; ++i)
    {
      subFx[i] = NULL;
      subFxTm[i].identity();
    }
  }
  CompoundPsEffect(const CompoundPsEffect &) = default; // should be used with caution only in clone()


  ~CompoundPsEffect()
  {
    ITERATE_OVER_SUBFX ()
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

    o->numEffects = numEffects;
    ITERATE_OVER_SUBFX ()
    {
      o->subFx[i] = (BaseEffectObject *)subFx[i]->clone();
      o->subFxTm[i] = subFxTm[i];
      if (o->subFx[i] && useCommonEmitter)
        o->subFx[i]->setParam(HUID_EMITTER, o->emitter);
    }

    return (BaseParamScript *)o;
  }

  void reduce()
  {
    BaseEffectObject *tmpFx[NUM_FX];
    TMatrix tmpTm[NUM_FX];
    numEffects = 0;
    for (int i = 0; i < NUM_FX; ++i)
      if (subFx[i])
      {
        tmpFx[numEffects] = subFx[i];
        tmpTm[numEffects++] = subFxTm[i];
      }
    memcpy(subFx, tmpFx, sizeof(tmpFx[0]) * numEffects);
    memcpy(subFxTm, tmpTm, sizeof(tmpTm[0]) * numEffects);
    for (int i = numEffects; i < NUM_FX; ++i)
    {
      subFx[i] = NULL;
      subFxTm[i].identity();
    }
  }

  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    CompoundPsParams par;
    par.load(ptr, len, load_cb);

#define SETFX(i)                                                                                              \
  if (subFx[i - 1])                                                                                           \
  {                                                                                                           \
    delete subFx[i - 1];                                                                                      \
    subFx[i - 1] = NULL;                                                                                      \
  }                                                                                                           \
  if (par.fx##i)                                                                                              \
  {                                                                                                           \
    TMatrix scl(1);                                                                                           \
    scl.m[0][0] = par.fx##i##_scale.x;                                                                        \
    scl.m[1][1] = par.fx##i##_scale.y;                                                                        \
    scl.m[2][2] = par.fx##i##_scale.z;                                                                        \
    subFx[i - 1] = (BaseEffectObject *)((BaseEffectObject *)par.fx##i)->clone();                              \
    subFxTm[i - 1] = rotxTM(par.fx##i##_rot.x) * rotyTM(par.fx##i##_rot.y) * rotzTM(par.fx##i##_rot.z) * scl; \
    subFxTm[i - 1].setcol(3, par.fx##i##_offset);                                                             \
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

    StaticVisSphere staticVisibilitySphere;
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

    reduce();
  }


  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_EMITTER_TM && emitter)
      setEmitterTm((TMatrix *)value);
    else if (id == HUID_TM)
      setTm((TMatrix *)value);
    else if (id == _MAKE4C('WIND') || id == _MAKE4C('CLOU') || id == _MAKE4C('GRND') || id == _MAKE4C('FADE') ||
             id == _MAKE4C('CLBL') || id == _MAKE4C('DENS'))
    {
      ITERATE_OVER_SUBFX ()
        subFx[i]->setParam(id, value);
    }
    else if (id == _MAKE4C('SYNC'))
    {
      G_ASSERT(subFx[0]);
      G_ASSERT(subFx[1]);
      subFx[1]->setParam(_MAKE4C('SYNC'), subFx[0]);
    }
    else
    {
      if (id == HUID_EMITTER)
        emitter = (BaseParticleFxEmitter *)value;

      if (useCommonEmitter || id != HUID_EMITTER)
      {
        ITERATE_OVER_SUBFX ()
          subFx[i]->setParam(id, value);
      }
    }
  }

  // fast setters

  virtual void setTm(const TMatrix *value)
  {
    TMatrix tm;
    INVOKE_OVER_SUBFX_EXT(setTm, &tm, tm = *value * subFxTm[i]);
  }
  virtual void setEmitterTm(const TMatrix *value)
  {
    if (emitter)
    {
      emitter->setTm(value);
      TMatrix tm;
      INVOKE_OVER_SUBFX_EXT(setEmitterTm, &tm, tm = *value * subFxTm[i]);
    }
  }
  virtual void setVelocity(const Point3 *value) { INVOKE_OVER_SUBFX(setVelocity, value); }
  virtual void setWind(const Point3 *value) { INVOKE_OVER_SUBFX(setWind, value); }
  virtual void setSpawnRate(const real *value) { INVOKE_OVER_SUBFX(setSpawnRate, value); }
  virtual void setColorMult(const Color3 *value) { INVOKE_OVER_SUBFX(setColorMult, value); }
  virtual void setColor4Mult(const Color4 *value) { INVOKE_OVER_SUBFX(setColor4Mult, value); }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_ACES_IS_ACTIVE)
    {
      if (!value)
        return NULL;

      bool isActive = false;
      ITERATE_OVER_SUBFX ()
      {
        bool isSubActive = false;
        subFx[i]->getParam(id, &isSubActive);
        isActive = isActive || isSubActive;
      }
      *((bool *)value) = isActive;
    }
    else if (id == _MAKE4C('VISM'))
    {
      G_ASSERT(value);
      unsigned int val = 0;
      bool validMask = false;
      ITERATE_OVER_SUBFX ()
      {
        unsigned int visibilityMask;
        subFx[i]->getParam(_MAKE4C('VISM'), &visibilityMask);
        if (visibilityMask != 0xFFFFFFFF) // Skip subeffects with invalid mask (newborn or dead).
        {
          validMask = true;
          val |= visibilityMask;
        }
      }

      if (!validMask) // All subeffects have invalid mask - render it as it looks like newborn effect.
        val = 0xFFFFFFFF;

      *(unsigned int *)value = val;
      return value;
    }
    else if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_EMITTER)
      return emitter;
    else if (id == _MAKE4C('WBOX'))
    {
      G_ASSERT(value);
      BBox3 *bbox = (BBox3 *)value;
      bbox->setempty();
      ITERATE_OVER_SUBFX ()
      {
        BBox3 subFxBbox;
        subFx[i]->getParam(_MAKE4C('WBOX'), &subFxBbox);
        *bbox += subFxTm[i] * subFxBbox;
      }
      return value;
    }
    else if (id == _MAKE4C('STAT'))
    {
      ITERATE_OVER_SUBFX ()
        subFx[i]->getParam(id, value);
    }
    else if (id == _MAKE4C('DSTR'))
    {
      bool hasDistortion = false;
      ITERATE_OVER_SUBFX ()
      {
        bool subHasDistortion = false;
        subFx[i]->getParam(id, &subHasDistortion);
        hasDistortion |= subHasDistortion;
      }
      *((bool *)value) = hasDistortion;
    }
    else if (id == _MAKE4C('PFXD'))
    {
      *(int *)value = 1;
      return value;
    }
    else if (id == _MAKE4C('PFXS'))
    {
      *(int *)value = 0;
      return value;
    }
    else
    {
      ITERATE_OVER_SUBFX ()
      {
        void *res = subFx[i] ? subFx[i]->getParam(id, value) : NULL;
        if (res)
          return res;
      }
    }

    return NULL;
  }

  // virtual bool isSubOf(unsigned id) { return  || __super::isSubOf(id); }


  virtual void spawnParticles(BaseParticleFxEmitter *em, real amount)
  {
    if (!em || amount <= 0)
      return;

    ITERATE_OVER_SUBFX ()
      if (subFx[i]->isSubOf(HUID_BaseParticleEffect))
        ((BaseParticleEffect *)subFx[i])->spawnParticles(em, amount);
  }

  virtual void update(real dt) { INVOKE_OVER_SUBFX(update, dt); }
  virtual void render(unsigned rtype, const TMatrix &view_itm)
  {
    ITERATE_OVER_SUBFX ()
    {
      subFx[i]->render(rtype, view_itm);
    }
  }
};

bool register_dafx_system(
  BaseEffectObject *src, uint32_t tags, FxRenderGroup group, const DataBlock &fx_blk, int sort_depth, const eastl::string &name);

bool register_dafx_compound_system(
  BaseEffectObject *src, uint32_t tags, FxRenderGroup group, const DataBlock &fx_blk, int sort_depth, const eastl::string &name)
{
  sort_depth++; // for composites of composites

  int isDafx = (int)false;
  if (src->getParam(_MAKE4C('PFXS'), &isDafx) && isDafx)
    return register_dafx_system(src, tags, group, fx_blk, sort_depth, name);

  isDafx = (int)false;
  bool ok = true;
  if (src->getParam(_MAKE4C('PFXD'), &isDafx) && isDafx)
  {
    CompoundPsEffect *comp = static_cast<CompoundPsEffect *>(src);
    for (int i = 0; i < CompoundPsEffect::NUM_FX; ++i)
    {
      if (!comp->subFx[i])
        continue;

      isDafx = (int)false;
      if (comp->subFx[i]->getParam(_MAKE4C('PFXS'), &isDafx) && isDafx)
      {
        eastl::string n = name;
        n.append_sprintf("__sub_%d", i);
        ok &= register_dafx_system(comp->subFx[i], tags, group, fx_blk, sort_depth + i, n);
      }

      isDafx = (int)false;
      if (comp->subFx[i]->getParam(_MAKE4C('PFXD'), &isDafx) && isDafx)
      {
        eastl::string n = name;
        n.append_sprintf("__sub_comp_%d", i);
        ok &= register_dafx_compound_system(comp->subFx[i], tags, group, fx_blk, sort_depth + i, n);
      }
    }
  }
  return ok;
}


class CompoundPsFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new CompoundPsEffect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "CompoundPs"; }
};

static CompoundPsFactory factory;


void register_compound_ps_fx_factory() { register_effect_factory(&factory); }
