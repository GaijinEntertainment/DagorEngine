//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_carray.h>
#include <shaders/dag_renderStateId.h>
#include <shaders/dag_overrideStateId.h>

namespace debug3d
{
enum StateZFunc
{
  ZFUNC_GREATER_EQUAL = 0,
  ZFUNC_LESS,
};

union StateKey
{
  StateKey() : k(0) {}
  StateKey(bool test_z, bool write_z, StateZFunc z_func) : k(0)
  {
    s.testZ = test_z ? 1u : 0u;
    s.writeZ = write_z ? 1u : 0u;
    s.zfunc = z_func;
  }

  uint32_t k;
  struct
  {
    uint32_t testZ : 1;
    uint32_t writeZ : 1;
    uint32_t zfunc : 1;
  } s;
  static constexpr int VARIANTS_COUNT = 8;
};

class CachedStates
{
  shaders::RenderStateId renderStateId = shaders::RenderStateId::Invalid;
  carray<shaders::UniqueOverrideStateId, StateKey::VARIANTS_COUNT> stateOverrides = {};
  shaders::OverrideStateId savedOverride;
  void makeState(const StateKey &key);
  shaders::UniqueOverrideStateId &getState(const StateKey &key);

public:
  void setState(const StateKey &key);
  void resetState();
  void clearStateOverrides() { stateOverrides = {}; }
};
} // namespace debug3d
