//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>

namespace gamephys
{
const float hpToW = 746.f;                 // Horse powers to Watts
const float wattToHP = 1.f / 746.f;        // Watts to horse powers
const float mpsToKmh = 3.6f;               // meters a second to km per hour
const float kmhToMps = 1.f / mpsToKmh;     // km per hour to meters a second
const float rpmToRadPerSec = TWOPI / 60.f; // RPM to radians per second
const float radPerSecToRpm = 60.f / TWOPI; // Radians per second to RPM
const float secondsToHour = 3600.f;        // Seconds in hour
const float hourToSeconds = 1.f / 3600.f;
const float gasolineEnergy = 41.87f * 1e6f;
const float keroseneEnergy = 43.54f * 1e6f;
const float airSpecificHeatCapacity = 1.01f; // Air specific heat capacity (Cm)
const float zeroKelvinInCelsium = -273.15f;
const float reciNormalDensity = 1.f / 1.225f;
}; // namespace gamephys
