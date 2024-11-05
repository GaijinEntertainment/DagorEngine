//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_generationRefId.h>

namespace shaders
{
class OverrideStateIdDummy
{};
typedef GenerationRefId<8, OverrideStateIdDummy> OverrideStateId; // weak reference

struct OverrideState;

namespace overrides
{
OverrideStateId create(const OverrideState &);
bool destroy(OverrideStateId &override_id);
bool exists(OverrideStateId override_id);

//! set current override. It will fail, if there is already one set, and override_id is not invalid.
bool set(OverrideStateId override_id);
//! reset current override.
inline bool reset() { return set(OverrideStateId()); }

OverrideState get(OverrideStateId override_id); // returns override details
OverrideStateId get_current();
OverrideStateId get_current_with_master();

void set_master_state(const OverrideState &);
void reset_master_state();

void destroy_all_managed_master_states();
} // namespace overrides

// this is owning references. It will destroy referenced data in destructor.
struct UniqueOverrideStateId
{
  ~UniqueOverrideStateId() { shaders::overrides::destroy(ref); };
  UniqueOverrideStateId() = default;
  UniqueOverrideStateId(const OverrideStateId &id) { reset(id); }
  UniqueOverrideStateId &operator=(OverrideStateId id)
  {
    reset(id);
    return *this;
  }
  bool reset(OverrideStateId id = OverrideStateId())
  {
    bool ret = overrides::destroy(ref);
    ref = id;
    return ret;
  }
  const OverrideStateId &get() const { return ref; }
  OverrideStateId &get() { return ref; }
  UniqueOverrideStateId(UniqueOverrideStateId &&id)
  {
    ref = id.ref;
    id.ref = OverrideStateId();
  }
  UniqueOverrideStateId &operator=(UniqueOverrideStateId &&id)
  {
    reset(id.ref);
    id.ref = OverrideStateId();
    return *this;
  }
  UniqueOverrideStateId(const UniqueOverrideStateId &id) = delete;
  UniqueOverrideStateId &operator=(const UniqueOverrideStateId &id) = delete;
  explicit operator bool() const { return (bool)ref; }

protected:
  OverrideStateId ref;
};

namespace overrides
{
inline bool destroy(UniqueOverrideStateId &id) { return id.reset(); }
inline void set(UniqueOverrideStateId &id) { shaders::overrides::set(id.get()); }
inline void set(const UniqueOverrideStateId &id) { shaders::overrides::set(id.get()); }
inline bool exists(UniqueOverrideStateId id) { return shaders::overrides::exists(id.get()); }
OverrideState get(const UniqueOverrideStateId &id);
} // namespace overrides

}; // namespace shaders
