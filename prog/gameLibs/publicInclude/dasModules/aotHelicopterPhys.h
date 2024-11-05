//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <gamePhys/phys/helicopter/helicopterPhys.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotGamePhys.h>
#include <gamePhys/phys/helicopter/helicopterPhys.h>

typedef decltype(HelicopterPhys::explosionDamageImpulses) HelicopterPhysExplosionDamageImpulses;

MAKE_TYPE_FACTORY(HelicopterControlState, HelicopterControlState);
MAKE_TYPE_FACTORY(HelicopterDamage, HelicopterState::HelicopterDamage);
MAKE_TYPE_FACTORY(HelicopterState, HelicopterState);
MAKE_TYPE_FACTORY(HelicopterProps, HelicopterProps);
MAKE_TYPE_FACTORY(HelicopterPhys, HelicopterPhys);
MAKE_TYPE_FACTORY(HelicopterPhysExplosionDamageImpulse, HelicopterPhys::ExplosionDamageImpulse);

DAS_BIND_VECTOR(HelicopterPhysExplosionDamageImpulses, HelicopterPhysExplosionDamageImpulses, HelicopterPhys::ExplosionDamageImpulse,
  "HelicopterPhysExplosionDamageImpulses");

namespace bind_dascript
{
inline void helicopter_phys_set_engine_started(HelicopterPhys &phys) { phys.setEnginesStarted(); }
} // namespace bind_dascript
