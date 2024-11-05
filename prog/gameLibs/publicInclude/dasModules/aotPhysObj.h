//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <gamePhys/phys/physObj/physObj.h>
#include <gamePhys/phys/floatingVolume.h>
#include <dasModules/aotGamePhys.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotEcs.h>

typedef Tab<BSphere3> CcdSpheresTab;
typedef Tab<Point4> ContactsLogTab;

DAS_BIND_VECTOR(CcdSpheresTab, CcdSpheresTab, BSphere3, "CcdSpheresTab");
DAS_BIND_VECTOR(ContactsLogTab, ContactsLogTab, Point4, "ContactsLogTab");
MAKE_TYPE_FACTORY(PhysObjState, PhysObjState);
MAKE_TYPE_FACTORY(PhysObjControlState, PhysObjControlState);
MAKE_TYPE_FACTORY(PhysObj, PhysObj);
MAKE_TYPE_FACTORY(FloatingVolume, gamephys::floating_volumes::FloatingVolume);

typedef Tab<PhysObjControlState> PhysObjControlStateTab;

DAS_BIND_VECTOR(PhysObjControlStateTab, PhysObjControlStateTab, PhysObjControlState, "PhysObjControlStateTab");

namespace bind_dascript
{
inline void phys_obj_addForce(PhysObj &phys, const Point3 &arm, const Point3 &force) { phys.addForce(arm, force); }

inline void phys_obj_rescheduleAuthorityApprovedSend(PhysObj &phys) { phys.rescheduleAuthorityApprovedSend(); }
} // namespace bind_dascript
