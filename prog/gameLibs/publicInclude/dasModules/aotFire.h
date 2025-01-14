//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <daECS/core/entityId.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotDm.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/game/dm/fire.h>
#include <damageModel/fireDamage.h>
#include <EASTL/vector.h>
#include <dasModules/dasManagedTab.h>

using FireDamageStates = eastl::vector<dm::FireDamageState::State>;
using FireDataList = Tab<dm::FireData>;

MAKE_TYPE_FACTORY(FireDamageStateState, dm::FireDamageState::State);
MAKE_TYPE_FACTORY(FireDamageState, dm::FireDamageState);
DAS_BIND_VECTOR(FireDamageStates, FireDamageStates, dm::FireDamageState::State, " FireDamageStates");

MAKE_TYPE_FACTORY(FireParams, dm::fire::Properties);
MAKE_TYPE_FACTORY(FireData, dm::FireData);
MAKE_TYPE_FACTORY(FireDamageComponent, dm::FireDamageComponent);
MAKE_TYPE_FACTORY(StartBurnDesc, dm::StartBurnDesc);
DAS_BIND_VECTOR(FireDataList, FireDataList, dm::FireData, " ::FireDataList");

namespace bind_dascript
{
inline void startburndesc_setNodeId(dm::StartBurnDesc &c, int n) { c.nodeId = dag::Index16(n); }
} // namespace bind_dascript
