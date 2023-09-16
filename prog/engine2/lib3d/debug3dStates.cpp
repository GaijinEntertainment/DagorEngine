#include <debug/dag_debug3dStates.h>
#include <3d/dag_renderStates.h>
#include <shaders/dag_overrideStates.h>

namespace debug3d
{
void CachedStates::makeState(const StateKey &key)
{
  G_ASSERT(!stateOverrides[key.k]);
  shaders::OverrideState state;
  state.set((key.s.writeZ ? 0 : shaders::OverrideState::Z_WRITE_DISABLE) | (key.s.testZ ? 0 : shaders::OverrideState::Z_TEST_DISABLE) |
            shaders::OverrideState::BLEND_SRC_DEST | shaders::OverrideState::BLEND_SRC_DEST_A |
            (key.s.zfunc != ZFUNC_GREATER_EQUAL ? shaders::OverrideState::Z_FUNC : 0));
  state.sblend = BLEND_SRCALPHA;
  state.dblend = BLEND_INVSRCALPHA;
  state.sblenda = BLEND_ZERO;
  state.dblenda = BLEND_ZERO;

  if (key.s.zfunc == ZFUNC_LESS)
    state.zFunc = CMPF_LESS;
  stateOverrides[key.k] = shaders::overrides::create(state);
}

shaders::UniqueOverrideStateId &CachedStates::getState(const StateKey &key)
{
  G_ASSERT(stateOverrides[key.k]);
  return stateOverrides[key.k];
}

void CachedStates::setState(const StateKey &key)
{
  if (!stateOverrides[key.k])
    makeState(key);
  shaders::overrides::set(getState(key));

  if (!renderStateId)
  {
    shaders::RenderState state;
    renderStateId = shaders::render_states::create(state);
  }
  shaders::render_states::set(renderStateId);
}

void CachedStates::resetState() { shaders::overrides::reset(); }
} // namespace debug3d
