//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct EspSensors
{
  float yawRate;
  float yawAngle;
  float slipRatio[4], slipTanA[4];
  float steerAngle;
};
