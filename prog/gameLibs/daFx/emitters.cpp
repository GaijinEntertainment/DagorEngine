// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "emitters.h"
#include "context.h"
#include <math/random/dag_random.h>

namespace dafx
{
unsigned int get_emitter_limit(const EmitterData &data, bool skip_error)
{
#define CHECK(a)                 \
  if (!(a))                      \
  {                              \
    G_ASSERT((skip_error || a)); \
    return 0;                    \
  }

  unsigned int res = 0;

  CHECK(data.delay >= 0);

  if (data.type == EmitterType::FIXED)
  {
    res = data.fixedData.count;
  }
  else if (data.type == EmitterType::BURST)
  {
    const EmitterData::BurstData &burstData = data.burstData;

    CHECK(burstData.countMax > 0);
    CHECK(burstData.countMax >= burstData.countMin);
    CHECK(burstData.period >= 0);
    CHECK(burstData.period > 0 || burstData.cycles == 1);
    CHECK(burstData.lifeLimit != 0);
    CHECK(burstData.lifeLimit > 0 || burstData.elemLimit > 0 || burstData.cycles > 0);

    if (burstData.lifeLimit < 0) // immortal, using external limits
    {
      res = burstData.cycles > 0 ? burstData.countMax * burstData.cycles : burstData.elemLimit;
    }
    else if (burstData.period > 0) // calculate saturation point
    {
      unsigned int ratio = (unsigned int)(burstData.lifeLimit / burstData.period + 1.f);
      if (burstData.cycles > 0)
        ratio = min(ratio, burstData.cycles);

      res = ratio * burstData.countMax;
    }
    else // 0 period, 1 cycle burst
    {
      res = burstData.countMax;
    }

    if (burstData.elemLimit > 0)
      res = min(res, burstData.elemLimit);

    CHECK(res > 0);
  }
  else if (data.type == EmitterType::LINEAR)
  {
    const EmitterData::LinearData &linearData = data.linearData;

    CHECK(linearData.lifeLimit > 0);
    CHECK(linearData.countMax > 0);
    CHECK(linearData.countMax >= linearData.countMin);

    res = linearData.countMax;
  }
  else if (data.type == EmitterType::DISTANCE_BASED)
  {
    CHECK(data.distanceBasedData.elemLimit > 0);
    CHECK(data.distanceBasedData.distance > 0);
    CHECK(data.distanceBasedData.idlePeriod >= 0);
    CHECK(data.distanceBasedData.lifeLimit >= 0);

    res = data.distanceBasedData.elemLimit;
  }
  else
  {
    G_ASSERT(false);
  }

#undef CHECK

  G_UNREFERENCED(skip_error);
  return res;
}

void create_emitter_state(EmitterState &state, const EmitterData &data, unsigned int elem_limit, float emission_factor)
{
  emission_factor = max(emission_factor, data.minEmissionFactor);
  float delay = data.delay;
  if (data.type == EmitterType::LINEAR) // we only need that for linear ticks
    delay *= emission_factor;           // we need to compensate for altered dt

  state.emissionLimit = elem_limit;
  state.localTickRate = 1.f;
  state.totalTickRate = state.localTickRate;
  state.globalLifeLimit = max(data.globalLifeLimitMin, 0.f);
  state.globalLifeLimit += state.globalLifeLimit > 0 ? delay : 0;
  state.globalLifeLimitRef = state.globalLifeLimit;

  if (data.type == EmitterType::FIXED)
  {
    state.isDistanceBased = false;
    state.lifeLimit = -1;
    state.batchSize = max(int(data.fixedData.count * emission_factor), 1);
    state.cyclesCount = 1;

    state.spawnTick = 0;
    state.shrinkTick = 0;
    state.tickLimit = 0;
  }
  else if (data.type == EmitterType::BURST)
  {
    state.isDistanceBased = false;
    state.lifeLimit = data.burstData.lifeLimit;
    state.batchSize = max(int(data.burstData.countMax * emission_factor), 1);
    state.cyclesCount = data.burstData.cycles > 0 ? data.burstData.cycles : -1;

    state.tickLimit = data.burstData.period;

    state.spawnTick = state.tickLimit - delay;
    state.shrinkTick = state.tickLimit - data.burstData.lifeLimit - delay;
  }
  else if (data.type == EmitterType::LINEAR)
  {
    state.isDistanceBased = false;
    state.lifeLimit = data.linearData.lifeLimit;

    state.batchSize = 1;
    state.cyclesCount = -1;

    int count = max(int(data.linearData.countMax * emission_factor), 1);
    state.tickLimit = data.linearData.lifeLimit / count;

    state.spawnTick = -delay;
    state.shrinkTick = -data.linearData.lifeLimit - delay;

    // we can't force this for all fx, because old system (flowps2.cpp) was waiting for the first tick
    if (data.linearData.instant)
    {
      state.spawnTick += state.tickLimit;
      state.shrinkTick += state.tickLimit;
    }
  }
  else if (data.type == EmitterType::DISTANCE_BASED)
  {
    state.isDistanceBased = true;
    state.lifeLimit = data.distanceBasedData.lifeLimit;
    state.batchSize = 1;
    state.cyclesCount = -1;
    state.spawnTick = -delay;
    state.shrinkTick = data.distanceBasedData.idlePeriod / emission_factor;
    state.dist = data.distanceBasedData.distance / emission_factor;
    state.distSq = state.dist * state.dist;
    state.lastEmittedPosValid = false;
  }
  else
  {
    G_ASSERT(false);
  }

  state.cyclesCountRef = state.cyclesCount;
  state.spawnTickRef = state.spawnTick;
  state.shrinkTickRef = state.shrinkTick;
  state.garbageTick = 0;
}

void create_emitter_randomizer(EmitterRandomizer &randomizer, const EmitterData &data, float emission_factor)
{
  randomizer.emitterType = data.type;
  if (randomizer.emitterType == EmitterType::BURST)
  {
    randomizer.countMin = data.burstData.countMin > 0 ? max(int(data.burstData.countMin * emission_factor), 1) : 0;
    randomizer.countMax = max(int(data.burstData.countMax * emission_factor), 1);
  }
  else if (randomizer.emitterType == EmitterType::LINEAR)
  {
    randomizer.countMin = data.linearData.countMin > 0 ? max(int(data.linearData.countMin * emission_factor), 1) : 0;
    randomizer.countMax = max(int(data.linearData.countMax * emission_factor), 1);
  }
  else
  {
    randomizer.countMin = 1;
    randomizer.countMax = 1;
  }

  randomizer.globalLifeLimitMin = data.globalLifeLimitMin;
  randomizer.globalLifeLimitMax = data.globalLifeLimitMax;
}

void apply_emitter_randomizer(const EmitterRandomizer &randomizer, EmitterState &state, int &rng)
{
  if (randomizer.countMin != randomizer.countMax)
  {
    if (randomizer.emitterType == EmitterType::BURST)
    {
      state.batchSize = _rnd_int(rng, randomizer.countMin, randomizer.countMax);
    }
    else if (randomizer.emitterType == EmitterType::LINEAR)
    {
      float countf = _rnd_float(rng, float(randomizer.countMin), float(randomizer.countMax));
      state.tickLimit = state.lifeLimit / max(countf, FLT_EPSILON);
    }
  }

  if (randomizer.globalLifeLimitMin != randomizer.globalLifeLimitMax)
  {
    state.globalLifeLimit =
      _rnd_float(rng, randomizer.globalLifeLimitMin, max(randomizer.globalLifeLimitMin, randomizer.globalLifeLimitMax));
    state.globalLifeLimitRef = state.globalLifeLimit;
  }
}

void update_emitters(Context &ctx, float dt, int begin_sid, int end_sid)
{
  TIME_PROFILE(dafx_update_emitters);
  InstanceGroups &stream = ctx.instances.groups;

  for (int i = begin_sid; i < end_sid; ++i)
  {
    uint32_t &flags = stream.get<INST_FLAGS>(i);

    if (!(flags & SYS_ENABLED))
      continue;

    G_FAST_ASSERT(flags & SYS_VALID);
    stat_inc(ctx.stats.updateEmitters);

    EmitterState &emitter = stream.get<INST_EMITTER_STATE>(i);
    if (emitter.globalLifeLimitRef > 0)
      emitter.globalLifeLimit -= dt;

    if (!(flags & SYS_EMITTER_REQ))
      continue;

    float ldt = dt * emitter.totalTickRate;

    InstanceState &instState = stream.get<INST_ACTIVE_STATE>(i);
    SimulationState &simState = stream.get<INST_SIMULATION_STATE>(i);

    int spawnStep = 0;
    int shrinkStep = 0;

    bool allowSpawn = ldt > 0 && emitter.globalLifeLimit >= 0;

    if (emitter.isDistanceBased)
    {
      Point3 np = Point3::xyz(stream.get<INST_POSITION>(i));

      if (!(flags & SYS_POS_VALID))
        continue;

      if (!emitter.lastEmittedPosValid)
      {
        emitter.lastEmittedPosValid = true;
        emitter.lastEmittedPos = np;
      }

      emitter.generation += ldt;
      Point3 &lp = emitter.lastEmittedPos;
      Point3 dp = lp - np;
      float distSq = dp.lengthSq();

      // spawn (static idle + dynamic distance based)
      emitter.spawnTick += allowSpawn ? ldt : 0;
      spawnStep = emitter.shrinkTick > 0 ? int(emitter.spawnTick / emitter.shrinkTick) : 0;

      allowSpawn &= emitter.spawnTick >= 0; // delay
      if (!allowSpawn)                      // first anchor point should be when delay is done, not the acutal first one
        lp = np;

      if (allowSpawn && (distSq > emitter.distSq || spawnStep > 0))
      {
        lp = np;
        int distSpawnStep = int(sqrtf(distSq) / emitter.dist);
        spawnStep = max(spawnStep, ctx.app_was_inactive ? min(distSpawnStep, 1) : distSpawnStep);

        emitter.timeStamps.get_container().resize(emitter.timeStamps.size() + spawnStep, emitter.generation);

        emitter.spawnTick = 0;
      }

      // shrink
      auto firstToNotDelete = eastl::find_if(emitter.timeStamps.get_container().begin(), emitter.timeStamps.get_container().end(),
        [&emitter](float g) { return g + emitter.lifeLimit >= emitter.generation; });
      shrinkStep = firstToNotDelete - emitter.timeStamps.get_container().begin();
      emitter.timeStamps.get_container().erase(emitter.timeStamps.get_container().begin(), firstToNotDelete);

      // force shrink if we are over the limit
      int overLimit = (instState.aliveCount + spawnStep - shrinkStep) - instState.aliveLimit;
      if (overLimit > 0)
      {
        shrinkStep += overLimit;
        int stampsIntSize = emitter.timeStamps.size();
        int deleteNum = min(overLimit, stampsIntSize);
        emitter.timeStamps.get_container().erase(emitter.timeStamps.get_container().begin(),
          emitter.timeStamps.get_container().begin() + deleteNum);
      }
    }

    emitter.garbageTick = emitter.totalTickRate > 0 ? 0 : emitter.garbageTick + dt;
    if (emitter.garbageTick > emitter.lifeLimit && emitter.lifeLimit > 0)
    {
      // emission_rate = 0 mean that no particles will be spawn, but also that
      // garbage collector will not move the 'tail' of active parts.
      // so it will seems that there is alive parts, even if they all in fact dead.
      // so we need manually skip those instances.
      flags &= ~SYS_RENDER_REQ;
      instState.aliveCount = 0;
      instState.aliveStart = 0;
      simState.start = 0;
      simState.count = 0;
      emitter.garbageTick = 0;
      emitter.spawnTick = emitter.spawnTickRef;
      emitter.shrinkTick = emitter.shrinkTickRef;
      allowSpawn = false;

      emitter.timeStamps.get_container().clear();
    }

    // if emitter cannot emit then finish early
    if (!emitter.isDistanceBased && !emitter.batchSize)
    {
      flags &= ~(SYS_RENDER_REQ | SYS_EMITTER_REQ | SYS_CPU_SIMULATION_REQ | SYS_GPU_SIMULATION_REQ);
      continue;
    }

    if (!emitter.isDistanceBased)
    {
      emitter.spawnTick += allowSpawn ? ldt : 0;
      emitter.shrinkTick += ldt;
    }

    // shrink
    if (!emitter.isDistanceBased && emitter.shrinkTick >= emitter.tickLimit && emitter.lifeLimit > 0)
    {
      unsigned int count = emitter.tickLimit > 0 ? emitter.shrinkTick / emitter.tickLimit : 1;
      emitter.shrinkTick -= count * emitter.tickLimit;
      emitter.shrinkTick = max(0.f, emitter.shrinkTick);

      shrinkStep = count * emitter.batchSize;
      shrinkStep = max(0, shrinkStep);
    }

    // spawn
    if (!emitter.isDistanceBased && allowSpawn && emitter.spawnTick >= emitter.tickLimit && emitter.cyclesCount != 0)
    {
      unsigned int count = emitter.tickLimit == 0 ? 1 : emitter.spawnTick / emitter.tickLimit;
      emitter.spawnTick -= count * emitter.tickLimit;
      emitter.spawnTick = emitter.spawnTick > 0 ? emitter.spawnTick : 0;

      if (emitter.cyclesCount > 0)
        emitter.cyclesCount--;

      spawnStep = count * emitter.batchSize;
      spawnStep = max(0, spawnStep);
    }

    if (shrinkStep == 0 && spawnStep == 0)
      continue;

    uint32_t newFlags = flags;

    if (shrinkStep > 0 && instState.aliveCount > 0)
    {
      shrinkStep = min(shrinkStep, instState.aliveCount);
      instState.aliveCount -= shrinkStep;
      instState.aliveStart = (instState.aliveStart + shrinkStep) % instState.aliveLimit;

      // shrink should always be first, to avoid simulating freshly emitted parts
      simState.start = instState.aliveStart;
      simState.count = instState.aliveCount;
    }

    if (spawnStep > 0 && instState.aliveCount < instState.aliveLimit)
    {
      EmissionState &emission = stream.get<INST_EMISSION_STATE>(i);
      G_FAST_ASSERT((emission.start == 0 && emission.count == 0) || !(flags & (SYS_CPU_EMISSION_REQ | SYS_GPU_EMISSION_REQ))); // ref
                                                                                                                               // data

      spawnStep = min(spawnStep, instState.aliveLimit - instState.aliveCount);
      emission.start = (instState.aliveStart + instState.aliveCount) % instState.aliveLimit;
      emission.count = spawnStep;
      instState.aliveCount += spawnStep;
    }

    newFlags &= ~SYS_RENDER_REQ;

    if ((newFlags & SYS_RENDERABLE) && instState.aliveCount > 0)
      newFlags |= SYS_RENDER_REQ;

    // emitter is finished
    if (emitter.cyclesCount == 0)
    {
      bool allDead = emitter.lifeLimit > 0 && instState.aliveCount == 0;
      bool allSpawned = emitter.lifeLimit < 0 && instState.aliveCount == instState.aliveLimit;
      if (allDead || allSpawned)
        newFlags &= ~SYS_EMITTER_REQ;

      if (allDead)
        newFlags &= ~(SYS_CPU_SIMULATION_REQ | SYS_GPU_SIMULATION_REQ);
    }

    flags = newFlags;
  }
}
} // namespace dafx