#pragma once
#include <daFx/dafx.h>
#include <fx/dag_baseFxClasses.h>
#include <dafxEmitter_decl.h>

struct DafxEmitterInfo
{
  dafx::EmitterData emitterData;

  bool loadParamsData(dafx::ContextId ctx, const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    DafxEmitterParams par;
    par.load(ptr, len, load_cb);

    if (par.count <= 0)
    {
      logerr("dafx::emitter, part count");
      return false;
    }

    if (par.cycles == 0 && par.period == 0)
    {
      logerr("dafx::emitter, infinite part count");
      return false;
    }

    dafx::EmitterData em;

    if (par.type == 0) // fixed
    {
      em.type = dafx::EmitterType::FIXED;
      em.fixedData.count = par.count;
    }
    else if (par.type == 1) // burst
    {
      em.type = dafx::EmitterType::BURST;
      em.burstData.countMin = par.count;
      em.burstData.countMax = par.count;
      em.burstData.cycles = par.cycles;
      em.burstData.period = par.period;
      em.burstData.lifeLimit = par.life;
      em.burstData.elemLimit = 0;
    }
    else if (par.type == 2) // linear
    {
      em.type = dafx::EmitterType::LINEAR;
      em.linearData.countMin = par.count;
      em.linearData.countMax = par.count;
      em.linearData.lifeLimit = par.life;
    }

    em.delay = par.delay;

    const dafx::Config &cfg = dafx::get_config(ctx);
    unsigned int lim = dafx::get_emitter_limit(em, true);
    if (lim >= cfg.emission_limit)
    {
      logerr("dafx::emitter, over emitter limit");
      return false;
    }

    eastl::swap(emitterData, em);
    return true;
  }
};