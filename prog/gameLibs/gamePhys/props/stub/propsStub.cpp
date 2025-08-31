// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/atmosphere.h>

float gamephys::atmosphere::_g = 0.f;
float gamephys::atmosphere::_water_density = 0.f;

#include <gamePhys/props/physMatDamageModelProps.h>
/*static*/ const PhysMatDamageModelProps *PhysMatDamageModelProps::get_props(int)
{
  G_ASSERT(0);
  return nullptr;
}

#include <gamePhys/props/deformMatProps.h>
namespace physmat
{
void init() { G_ASSERT(0); }
const DeformMatProps *DeformMatProps::get_props(int)
{
  G_ASSERT(0);
  return nullptr;
}
} // namespace physmat
