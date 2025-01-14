// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_overrideStates.h>
#include <util/dag_generationReferencedData.h>
#include <EASTL/optional.h>
#include <EASTL/vector_map.h>
#include <osApiWrappers/dag_spinlock.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <hash/xxh3.h>
#include <debug/dag_log.h>

// for set/reset immediate
#include <shaders/dag_shaders.h>

namespace shaders
{
namespace render_states
{
extern void on_override_state_destroyed(OverrideStateId overrideId);
}
namespace overrides
{

struct OverrideStateRefCounted : OverrideState
{
  OverrideStateRefCounted(const OverrideState &s) : OverrideState(s) {}
  uint32_t refCount = 1;
};

static OSSpinlock spinlock;

using StateIdCache = ska::flat_hash_map</*hash*/ uint64_t, OverrideStateId, ska::power_of_two_std_hash<uint64_t>>;
static StateIdCache overrideStatesHash DAG_TS_GUARDED_BY(spinlock);
static StateIdCache masterOverrideStatesHash;

static GenerationReferencedData<OverrideStateId, OverrideStateRefCounted> overrideStates DAG_TS_GUARDED_BY(spinlock);

static eastl::optional<OverrideState> overrideMaster;

OverrideState get(OverrideStateId override_id)
{
  OSSpinlockScopedLock lock{spinlock};
  const OverrideStateRefCounted *state = overrideStates.cget(override_id);
  return state ? OverrideState(*state) : OverrideState();
}
OverrideState get(const UniqueOverrideStateId &id) { return get(id.get()); }

bool exists(OverrideStateId id)
{
  OSSpinlockScopedLock lock{spinlock};
  return overrideStates.doesReferenceExist(id);
}

OverrideStateId create(const OverrideState &s_)
{
  OverrideState state = s_;
  state.validate();
  uint64_t hash = XXH3_64bits(&state, sizeof(state));
  OSSpinlockScopedLock lock{spinlock};
  auto it = overrideStatesHash.find(hash);
  if (it == overrideStatesHash.end() || !overrideStates.doesReferenceExist(it->second))
  {
    OverrideStateId ret = overrideStates.emplaceOne(state);
    overrideStatesHash[hash] = ret;
    return ret;
  }
  overrideStates.get(it->second)->refCount++;
  return it->second;
}

// NOTE: our apple clang version is bugged here w.r.t. thread safety
bool destroy(OverrideStateId &override_id_) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  OverrideStateId override_id = eastl::exchange(override_id_, OverrideStateId()); // destroy reference
  OSSpinlockScopedLock lock{spinlock};
  OverrideStateRefCounted *s = overrideStates.get(override_id);
  if (!s)
    return false;
  G_ASSERT_RETURN(s->refCount != 0, false);
  if (--s->refCount == 0)
  {
    render_states::on_override_state_destroyed(override_id);
    G_VERIFY(overrideStates.destroyReference(override_id));
  }
  return true;
}

static OverrideStateId current_override;

OverrideStateId get_current() { return current_override; }

bool set(OverrideStateId override_id)
{
  const OverrideStateRefCounted *oldState;
  const OverrideStateRefCounted *state;
  {
    OSSpinlockScopedLock lock{spinlock};
    oldState = overrideStates.get(current_override);
    state = overrideStates.get(override_id);
  }
  if (!state && !!override_id)
  {
    logerr("override <%d> already destroyed", uint32_t(override_id));
    return false;
  }
  if (!oldState && !!current_override)
    logerr("override <%d> already destroyed, without being reset", uint32_t(current_override));

  if (oldState && state)
    logerr("override already set %d and not reset, resetting!, setting = %d", uint32_t(current_override), uint32_t(override_id));
  current_override = override_id;
  ShaderElement::invalidate_cached_state_block();
  return true;
}

static OverrideStateId add_master_override(OverrideStateId override_id)
{
  if (!overrideMaster)
    return override_id;

  OverrideState state = get(override_id);

  state.bits |= overrideMaster->bits;

  if (overrideMaster->isOn(OverrideState::Z_FUNC))
    state.zFunc = overrideMaster->zFunc;
  if (overrideMaster->isOn(OverrideState::FORCED_SAMPLE_COUNT))
    state.forcedSampleCount = overrideMaster->forcedSampleCount;
  if (overrideMaster->isOn(OverrideState::BLEND_OP))
    state.blendOp = overrideMaster->blendOp;
  if (overrideMaster->isOn(OverrideState::BLEND_OP_A))
    state.blendOpA = overrideMaster->blendOpA;
  if (overrideMaster->isOn(OverrideState::BLEND_SRC_DEST))
  {
    state.sblend = overrideMaster->sblend;
    state.dblend = overrideMaster->dblend;
  }
  if (overrideMaster->isOn(OverrideState::BLEND_SRC_DEST_A))
  {
    state.sblenda = overrideMaster->sblenda;
    state.dblenda = overrideMaster->dblenda;
  }
  if (overrideMaster->isOn(OverrideState::Z_BIAS))
  {
    state.zBias = overrideMaster->zBias;
    state.slopeZBias = overrideMaster->slopeZBias;
  }
  if (overrideMaster->isOn(OverrideState::STENCIL))
    state.stencil = overrideMaster->stencil;
  state.colorWr &= overrideMaster->colorWr;

  uint64_t hash = XXH3_64bits(&state, sizeof(state));
  auto it = masterOverrideStatesHash.find(hash);
  if (it != masterOverrideStatesHash.end())
    return it->second;

  OverrideStateId overriddenId = create(state);
  masterOverrideStatesHash[hash] = overriddenId;
  return overriddenId;
}

static eastl::vector_map<OverrideStateId, OverrideStateId> masterToCurrent;

void set_master_state(const OverrideState &s_)
{
  G_ASSERT(!overrideMaster);
  overrideMaster = s_;
  masterToCurrent.clear();
}

void reset_master_state()
{
  overrideMaster.reset();
  masterToCurrent.clear();
}

void destroy_all_managed_master_states() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(!overrideMaster);

  for (auto &overrideState : masterOverrideStatesHash)
    overrideStates.destroyReference(overrideState.second);
  masterOverrideStatesHash.clear();
  masterToCurrent.clear();
}

OverrideStateId get_current_with_master()
{
  auto it = masterToCurrent.find(get_current());
  if (it != masterToCurrent.end())
    return it->second;

  OverrideStateId override_id_with_master = add_master_override(get_current());
  masterToCurrent.emplace(get_current(), override_id_with_master);
  return override_id_with_master;
}

//- compatibility layer

/*
static eastl::vector<OverrideStateId> current_override;
bool set(OverrideStateId override_id)
{
  if (override_id)
  {
    if (current_override.size())
      return false;
    #if DAGOR_DBGLEVEL>0
    if (!overrideStates.doesReferenceExist(override_id))
    {
      logerr("Override <%d> already destroyed", uint32_t(override_id));
      return false;
    }
    #endif
    current_override.push_back(override_id);
    return true;
  }
  if (current_override.size() != 1)
    return false;
  current_override.clear();
  return true;
}

static void add(OverrideState &ret, const OverrideState &next)
{
  ret.bits |= next.bits;
  ret.colorWr &= next.colorWr;
  if (next.isOn(OverrideState::Z_BIAS))
    ret.zBias = next.zBias, ret.slopeZBias = next.slopeZBias;
  if (next.isOn(OverrideState::BLEND_OP))
    ret.blendOp = next.blendOp;
  if (next.isOn(OverrideState::BLEND_OP))
    ret.blendOp = next.blendOp;
  if (next.isOn(OverrideState::FORCED_SAMPLE_COUNT))
    ret.forcedSampleCount = next.forcedSampleCount;
  if (next.isOn(OverrideState::Z_FUNC))
    ret.zFunc = next.zFunc;
  if (next.isOn(OverrideState::STENCIL))
    ret.stencil = next.stencil;
}

void add(OverrideStateId override_id)
{
  #if DAGOR_DBGLEVEL>0
  if (!overrideStates.doesReferenceExist(override_id))
  {
    logerr("Override <%d> already destroyed", uint32_t(override_id));
    return;
  }
  #endif
  current_override.push_back(override_id);
}
void flush_override_stack()
{
  OverrideState result;
  for (auto &s:current_override)
    add(result, s);
}


bool remove(OverrideStateId override_id)
{
  #if DAGOR_DBGLEVEL>0
  if (!overrideStates.doesReferenceExist(override_id))
  {
    logerr("Override <%d> already destroyed", uint32_t(override_id));
    return false;
  }
  #endif
  if (!current_override.size() || current_override.back() != override_id)
  {
    logerr("Override <%d> isn't on top of stack of overrides (size of stack = %d)", uint32_t(override_id), current_override.size());
    #if DAGOR_DBGLEVEL>0
    if (current_override.find(override_id) != current_override.end())
      debug("Override <%d> is in stack of overrides, but not on top", uint32_t(override_id));
    #endif
    return false;
  }
  current_override.pop_back();
  return true;
}
*/
} // namespace overrides
}; // namespace shaders
