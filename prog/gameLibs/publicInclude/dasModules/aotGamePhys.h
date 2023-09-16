//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasManagedTab.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/common/mass.h>
#include <gamePhys/props/atmosphere.h>
#include <gamePhys/phys/commonPhysBase.h>
#include <gamePhys/collision/volumetricDamageData.h>

MAKE_TYPE_FACTORY(Orient, gamephys::Orient);
MAKE_TYPE_FACTORY(Loc, gamephys::Loc);
MAKE_TYPE_FACTORY(LocalOrient, gamephys::SimpleLoc::LocalOrient);
MAKE_TYPE_FACTORY(SimpleLoc, gamephys::SimpleLoc);
MAKE_TYPE_FACTORY(CommonPhysPartialState, CommonPhysPartialState);
MAKE_TYPE_FACTORY(Mass, gamephys::Mass);
MAKE_TYPE_FACTORY(VolumetricDamageData, gamephys::VolumetricDamageData);

using VolumetricDamageDatas = Tab<gamephys::VolumetricDamageData>;

DAS_BIND_VECTOR(VolumetricDamageDatas, VolumetricDamageDatas, gamephys::VolumetricDamageData, " ::VolumetricDamageDatas");

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::gamephys::DamageReason, DamageReason);
DAS_BASE_BIND_ENUM_98(::gamephys::DamageReason, DamageReason, DMG_REASON_COLLISION, DMG_REASON_DROWN)

namespace bind_dascript
{
inline void orient_setYP0(gamephys::Orient &orient, const das::float3 &dir) { orient.setYP0(reinterpret_cast<const Point3 &>(dir)); }
inline void orient_transformInv(const gamephys::Orient &orient, das::float3 &vec)
{
  orient.transformInv(reinterpret_cast<Point3 &>(vec));
}
inline void location_toTM(const gamephys::Loc &location, das::float3x4 &tm) { location.toTM(reinterpret_cast<TMatrix &>(tm)); }
inline TMatrix location_makeTM(const gamephys::Loc &location) { return location.makeTM(); }
inline gamephys::SimpleLoc make_empty_SimpleLoc() { return gamephys::SimpleLoc(); }
} // namespace bind_dascript
