//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/phys/crewState.h>

class NetCrew
{
public:
  virtual void saveCrewState(CrewState &out_crew_state) const = 0;
  virtual void restoreCrewState(const CrewState &saved_state) = 0;
  virtual bool correctInternalState(const CrewState &incoming_state, const CrewState &matching_state) = 0;
  virtual void onGunStateChanged(const CrewState &previous_state) = 0;
};
