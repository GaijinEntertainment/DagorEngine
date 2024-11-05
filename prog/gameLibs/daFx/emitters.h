// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include <EASTL/queue.h>

namespace dafx
{
struct Context;
struct EmitterState
{
  bool isDistanceBased;
  int emissionLimit;

  unsigned int batchSize;
  int cyclesCount;
  int cyclesCountRef;
  float lifeLimit;
  float globalLifeLimit;

  float spawnTick;
  float spawnTickRef;
  float shrinkTick;
  float shrinkTickRef;
  float globalLifeLimitRef;

  float tickLimit;

  float totalTickRate;
  float localTickRate;

  float garbageTick;

  // distance based
  float generation;
  float dist;
  float distSq;
  Point3 lastEmittedPos;
  bool lastEmittedPosValid;
  eastl::queue<float> timeStamps;
};
struct EmitterRandomizer
{
  EmitterType emitterType;
  unsigned int countMax;
  unsigned int countMin;

  float globalLifeLimitMin;
  float globalLifeLimitMax;
};

inline void set_emitter_emission_rate(EmitterState &state, float tick_rate)
{
  state.localTickRate = tick_rate;
  state.totalTickRate = state.localTickRate;
}

inline void reset_emitter_state(EmitterState &state)
{
  set_emitter_emission_rate(state, 1.f);
  state.cyclesCount = state.cyclesCountRef;
  state.spawnTick = state.spawnTickRef;
  state.shrinkTick = state.shrinkTickRef;
  state.globalLifeLimit = state.globalLifeLimitRef;
  state.garbageTick = 0;
  state.generation = 0;
  state.lastEmittedPosValid = false;
  state.timeStamps.get_container().clear();
}

void create_emitter_state(EmitterState &state, const EmitterData &data, unsigned int elem_limit, float emission_factor);

void create_emitter_randomizer(EmitterRandomizer &randomizer, const EmitterData &data, float emission_factor);
void apply_emitter_randomizer(const EmitterRandomizer &randomizer, EmitterState &state, int &rng);

void update_emitters(Context &ctx, float dt, int begin_sid, int end_sid);
} // namespace dafx