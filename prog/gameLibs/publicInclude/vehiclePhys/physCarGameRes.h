//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>

#include <vehiclePhys/physCarParams.h>

enum
{
  VehicleGameResClassId = 0x62FD2074u,     // Vehicle
  VehicleDescGameResClassId = 0x580504D4u, // VehicleDesc
};

struct PhysCarSettings2
{
  static constexpr int RES_VERSION = 5;

  TMatrix logicTm;
  PhysCarSuspensionParams frontSusp;
  PhysCarSuspensionParams rearSusp;
};

extern void register_phys_car_gameres_factory();
