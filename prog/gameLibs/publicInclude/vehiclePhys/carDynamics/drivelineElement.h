//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum DrivelineElements
{
  Engine = 0,
  Gearbox,
  Diff0, // Usually rear
  Diff1, // Usually front
  Num
};

class DrivelineElementState
{
public:
  // Semi-Static data
  float inertia = 0.f;               // Inertia of this component
  float ratio = 1.f, invRatio = 1.f; // Gearing ratio (gearbox/diff)

  float rotV = 0.f;      // Rotational velocity
  float rotA = 0.f;      // Rotational acceleration
  float tReaction = 0.f, // Reaction torque
    tBraking = 0.f;      // Braking torque (also a reaction torque)

  // Semi-static data (recalculated when gear is changed)
  float effectiveInertiaDownStream = 0.f; // Inertia of component + children
  float cumulativeRatio = 1.f;            // Ratio at this point of the driveline
};
