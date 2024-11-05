//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathAng.h>
#include <generic/dag_tab.h>
#include <gamePhys/phys/commonPhysBase.h>
#include <daGame/netWeapon.h>

template <typename PhysState, typename ControlState, typename PartialState>
class UnitPhysicsBase : public PhysicsBase<PhysState, ControlState, PartialState>
{
public:
  typedef typename PhysicsBase<PhysState, ControlState, PartialState>::custom_resync_cb_t custom_resync_cb_t;

  UnitPhysicsBase(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step,
    float extrapolation_time_mult = DEFAULT_EXTRAPOLATION_TIME_MULT);

  virtual void saveCurrentStateTo(PhysState &state, int32_t tick) const override
  {
    PhysicsBase<PhysState, ControlState, PartialState>::saveCurrentStateTo(state, tick);

    dag::ConstSpan<NetWeapon *> allGuns = this->getActor()->getAllWeapons();
    state.weaponStates.resize(min(allGuns.size(), state.MAX_WEAPON_STATES_COUNT));
    for (uint32_t gunNo = 0; gunNo < state.weaponStates.size(); gunNo++)
      allGuns[gunNo]->saveWeaponState(state.weaponStates[gunNo]);
  }
  virtual void setCurrentState(const PhysState &state) override
  {
    PhysicsBase<PhysState, ControlState, PartialState>::setCurrentState(state);

    dag::ConstSpan<NetWeapon *> allGuns = this->getActor()->getAllWeapons();
    int maxGunNo = min((uint32_t)state.weaponStates.size(), min(allGuns.size(), state.MAX_WEAPON_STATES_COUNT));
    for (int gunNo = 0; gunNo < maxGunNo; gunNo++)
      allGuns[gunNo]->restoreWeaponState(state.weaponStates[gunNo]);
  }
  static void doCustomResync(IPhysBase *base, const PhysState & /*prev_desynced_state*/, const PhysState &desynced_state,
    const PhysState &incoming_state, const PhysState *matching_state)
  {
    auto self = static_cast<UnitPhysicsBase *>(base);
    dag::ConstSpan<NetWeapon *> allGuns = self->getActor()->getAllWeapons();
    if (matching_state)
    {
      uint32_t minwssz = (uint32_t)min(matching_state->weaponStates.size(), incoming_state.weaponStates.size());
      int numberOfGuns = min(allGuns.size(), minwssz);
      for (unsigned int gunNo = 0; gunNo < numberOfGuns; gunNo++)
      {
        if (!allGuns[gunNo]->correctInternalState(incoming_state.weaponStates[gunNo], matching_state->weaponStates[gunNo]))
          continue;
        for (int i = 0; i < self->historyStates.size(); ++i)
        {
          if (gunNo < self->historyStates[i].weaponStates.size())
            self->historyStates[i].weaponStates[gunNo].flags |= WeaponState::WS_INVALID;
        }
      }
    }

    int numberOfGuns = min(allGuns.size(), (uint32_t)desynced_state.weaponStates.size());
    for (unsigned int gunNo = 0; gunNo < numberOfGuns; gunNo++)
      allGuns[gunNo]->onGunStateChanged(desynced_state.weaponStates[gunNo]);

    self->getActor()->validateGunsLists();
  }

  custom_resync_cb_t getCustomResyncCb() const override
  {
    return this->getActor()->getAllWeapons().size() ? &doCustomResync : nullptr;
  }
};

template <typename PhysState, typename ControlState, typename PartialState>
UnitPhysicsBase<PhysState, ControlState, PartialState>::UnitPhysicsBase(ptrdiff_t physactor_offset, PhysVars *phys_vars,
  float time_step, float extrapolation_time_mult) :
  PhysicsBase<PhysState, ControlState, PartialState>(physactor_offset, phys_vars, time_step, extrapolation_time_mult)
{}
