// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderStates.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/sort.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <hash/xxh3.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_renderStateId.h>
#include "concurrentElementPool.h"

namespace shaders
{

static uint8_t stencil_ref = 0;
void set_stencil_ref(uint8_t ref) // todo: to be replaced with d3d function
{
  d3d::setstencil(stencil_ref = ref);
}

namespace render_states
{

static ska::flat_hash_map</*hash*/ uint64_t, RenderStateId, ska::power_of_two_std_hash<uint64_t>> renderStatesHash;
static ConcurrentElementPool<RenderStateId, eastl::pair<RenderState, DriverRenderStateId>, 5> renderStates;
static eastl::vector_map<uint64_t, uint32_t> renderStateOverrides; // Key: (OverrideStateId<<32)|RenderStateId -> index in
                                                                   // overriddenRenderStates
static ska::flat_hash_map</*hash*/ uint64_t, DriverRenderStateId, ska::power_of_two_std_hash<uint64_t>> overriddenRenderStatesHash;
static dag::Vector<DriverRenderStateId> overriddenRenderStates;
static int numFreeOverridenRenderStates = 0;

// @TODO: add ability to set blendfactor & disable/enable it's use from override states.

static RenderState apply_override(RenderState state, const OverrideState &override_state)
{
  if (override_state.isOn(OverrideState::BLEND_SRC_DEST))
  {
    for (auto &blendParam : state.blendParams)
    {
      blendParam.ablend = 1;
      blendParam.ablendFactors.src = override_state.sblend;
      blendParam.ablendFactors.dst = override_state.dblend;
    }
  }
  if (override_state.isOn(OverrideState::FLIP_CULL) && state.cull != CULL_NONE)
    state.cull = state.cull == CULL_CW ? CULL_CCW : CULL_CW;
  if (override_state.isOn(OverrideState::CULL_NONE))
    state.cull = CULL_NONE;
  if (override_state.isOn(OverrideState::Z_BIAS))
  {
    state.zBias = override_state.zBias;
    state.slopeZBias = override_state.slopeZBias;
  }
  if (override_state.isOn(OverrideState::FORCED_SAMPLE_COUNT))
    state.forcedSampleCount = override_state.forcedSampleCount;
  if (override_state.isOn(OverrideState::Z_FUNC))
    state.zFunc = override_state.zFunc;
  if (override_state.isOn(OverrideState::BLEND_OP))
  {
    for (auto &blendParam : state.blendParams)
    {
      blendParam.blendOp = override_state.blendOp;
    }
  }
  if (override_state.isOn(OverrideState::BLEND_OP_A))
  {
    for (auto &blendParam : state.blendParams)
    {
      blendParam.sepablendOp = override_state.blendOpA;
    }
  }
  if (override_state.isOn(OverrideState::BLEND_SRC_DEST_A))
  {
    for (auto &blendParam : state.blendParams)
    {
      blendParam.sepablend = 1;
      blendParam.sepablendFactors.src = override_state.sblenda;
      blendParam.sepablendFactors.dst = override_state.dblenda;
    }
  }
  const bool forcedSampleCountEnabled =
    override_state.isOn(OverrideState::FORCED_SAMPLE_COUNT) && override_state.forcedSampleCount > 0;
  if (override_state.isOn(OverrideState::Z_TEST_DISABLE) || forcedSampleCountEnabled)
    state.ztest = 0;
  if (override_state.isOn(OverrideState::CONSERVATIVE))
  {
    if (d3d::get_driver_desc().caps.hasConservativeRassterization)
      state.conservativeRaster = 1;
    else
    {
      state.conservativeRaster = 0;
      LOGERR_ONCE("Attempt to set state with conservative rasterization. This GPU doesn't support it!");
    }
  }
  if (override_state.isOn(OverrideState::Z_BOUNDS_ENABLED))
    state.depthBoundsEnable = 1;

  G_ASSERT(!override_state.isOn(OverrideState::Z_WRITE_ENABLE) || !override_state.isOn(OverrideState::Z_WRITE_DISABLE));
  if (override_state.isOn(OverrideState::Z_WRITE_DISABLE) || forcedSampleCountEnabled)
  {
    state.zwrite = 0;
  }
  if (override_state.isOn(OverrideState::Z_WRITE_ENABLE) && !forcedSampleCountEnabled)
  {
    state.zwrite = 1;
  }
  if (override_state.isOn(OverrideState::Z_CLAMP_ENABLED))
    state.zClip = 0;
  if (override_state.isOn(OverrideState::STENCIL))
  {
    state.stencil = override_state.stencil;
  }
  state.colorWr &= override_state.colorWr;
  if (override_state.isOn(OverrideState::SCISSOR_ENABLED))
    state.scissorEnabled = 1;
  if (override_state.isOn(OverrideState::ALPHA_TO_COVERAGE))
    state.alphaToCoverage = 1;
  return state;
}

static DriverRenderStateId create_overriden_state(const RenderState &state)
{
  uint64_t hash = XXH3_64bits(&state, sizeof(state));
  auto it = overriddenRenderStatesHash.find(hash);
  if (it != overriddenRenderStatesHash.end())
    return it->second;
  auto ret = d3d::create_render_state(state);
  G_FAST_ASSERT(ret != DriverRenderStateId());
  overriddenRenderStatesHash[hash] = ret;
  return ret;
}

static uint32_t create_overridden(const RenderState &state, OverrideStateId override_id)
{
  RenderState overriddenState = apply_override(state, overrides::get(override_id));
  if (numFreeOverridenRenderStates > overriddenRenderStates.size() / 4)
  {
    for (auto &ovrs : overriddenRenderStates)
      if (ovrs == DriverRenderStateId()) // free?
      {
        ovrs = create_overriden_state(overriddenState);
        G_VERIFY(--numFreeOverridenRenderStates >= 0);
        return &ovrs - overriddenRenderStates.data();
      }
  }
  uint32_t ret = overriddenRenderStates.size();
  overriddenRenderStates.emplace_back(create_overriden_state(overriddenState));
  return ret;
}

RenderStateId create(const RenderState &state)
{
  uint64_t hash = XXH3_64bits(&state, sizeof(state));
  auto it = renderStatesHash.find(hash);
  if (DAGOR_LIKELY(it != renderStatesHash.end()))
    return it->second;
#if DAGOR_DBGLEVEL > 0
  // must be unique
  G_ASSERT(renderStates.findIf([&](auto &rs) { return rs.first == state; }) == RenderStateId::Invalid);
#endif
  auto statePair = eastl::make_pair(state, d3d::create_render_state(state));
  G_FAST_ASSERT(statePair.second != DriverRenderStateId());
  RenderStateId ret = renderStates.allocate();
  renderStates[ret] = eastl::move(statePair);
  renderStatesHash[hash] = ret;
  return ret;
}

void on_override_state_destroyed(OverrideStateId overrideId)
{
  auto it = renderStateOverrides.lower_bound(uint64_t((uint32_t)overrideId) << 32), itb = it;
  for (; it != renderStateOverrides.end(); ++it)
    if (uint32_t(it->first >> 32) == (uint32_t)overrideId)
      overriddenRenderStates[it->second] = DriverRenderStateId(); // mark as free
    else
      break;
  numFreeOverridenRenderStates += it - itb;
  renderStateOverrides.erase(itb, it);
#if DAGOR_DBGLEVEL > 1
  int numFreeCalculated = 0;
  for (auto &ors : overriddenRenderStates)
    if (ors == DriverRenderStateId())
      ++numFreeCalculated;
  G_ASSERT(numFreeCalculated == numFreeOverridenRenderStates);
  G_ASSERT(eastl::is_sorted(renderStateOverrides.begin(), renderStateOverrides.end()));
#endif
}

void set(RenderStateId id)
{
  G_FAST_ASSERT(id != RenderStateId::Invalid);
  if (DAGOR_UNLIKELY(id == RenderStateId::Invalid))
  {
    LOGERR_ONCE("Attempt to set invalid render state");
    return;
  }

  auto &rstate = renderStates[id];
  DriverRenderStateId drsid = rstate.second;
  uint8_t stencilRef = rstate.first.stencilRef;

  if (OverrideStateId overrideId = overrides::get_current_with_master())
  {
    uint64_t key = (uint64_t((uint32_t)overrideId) << 32) | uint32_t(id);
    decltype(overriddenRenderStates)::value_type *ors;
    auto it = renderStateOverrides.lower_bound(key);
    if (DAGOR_LIKELY(it != renderStateOverrides.end() && it->first == key))
      ors = &overriddenRenderStates[it->second];
    else // not found -> create new
    {
      uint32_t oid = create_overridden(rstate.first, overrideId);
      ors = &overriddenRenderStates[oid];
      renderStateOverrides.emplace_hint(it, key, oid);
    }
    G_FAST_ASSERT(*ors != DriverRenderStateId());
    drsid = *ors;
    OverrideState overrideState = overrides::get(overrideId);
    if (overrideState.isOn(OverrideState::STENCIL))
      stencilRef = stencil_ref;
  }

  d3d::set_render_state(drsid);

  if (DAGOR_UNLIKELY(rstate.first.blendFactorUsed))
    d3d::set_blend_factor(rstate.first.blendFactor);

#if !_TARGET_C1 && !_TARGET_C2
  d3d::setstencil(stencilRef);
#else

#endif
}

uint32_t get_count() { return renderStates.totalElements(); }

const RenderState &get(RenderStateId render_id) { return renderStates[render_id].first; }

} // namespace render_states
} // namespace shaders
