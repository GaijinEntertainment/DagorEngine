// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <gamePhys/collision/collisionLib.h>
#include <math/dag_mathUtils.h>
#include "phys/physEvents.h"

bool is_allow_underground(const Point3 &pos);

template <typename Phys>
static void underground_teleport(float dt,
  ecs::EntityId eid,
  const Phys &phys,
  float &underground_teleporter__timeToCheck,
  float underground_teleporter__timeBetweenChecks,
  float underground_teleporter__heightOffset)
{
  underground_teleporter__timeToCheck = underground_teleporter__timeToCheck - dt;
  if (underground_teleporter__timeToCheck >= 0.f)
    return;
  underground_teleporter__timeToCheck = underground_teleporter__timeBetweenChecks;
  Point3 pos = Point3::xyz(phys.currentState.location.P);
  if (check_nan(pos))
  {
    logerr("underground_teleport: entity <%s> removed because of NaN position", g_entity_mgr->getEntityTemplateName(eid));
    g_entity_mgr->destroyEntity(eid);
    return;
  }
  float ht = dacoll::traceht_lmesh(Point2::xz(pos));
  if (pos.y + underground_teleporter__heightOffset < ht)
  {
    if (is_allow_underground(pos))
      return;
    TMatrix newTm = phys.currentState.location.makeTM();
    newTm.setcol(3, Point3::xVz(pos, ht + underground_teleporter__heightOffset));
    g_entity_mgr->sendEvent(eid, CmdTeleportEntity(newTm, true));
  }
}
