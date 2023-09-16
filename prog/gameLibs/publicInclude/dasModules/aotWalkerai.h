//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <ai/walker/agent.h>
#include <ai/walker/library.h>
#include <ai/walker/agentObstacle.h>
#include <ecs/ai/aiTarget.h>
#include <ecs/ai/entityDangers.h>
#include <math/dag_Point3.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(ai::Danger::DangerType, DangerType);

typedef eastl::vector<ai::Danger> AgentDangersDangers;
typedef eastl::vector<walkerai::ObstacleSegment> ObstacleSegments;

MAKE_TYPE_FACTORY(Obstacle, walkerai::Obstacle);
MAKE_TYPE_FACTORY(AgentDangers, ai::AgentDangers);
MAKE_TYPE_FACTORY(ObstacleSegment, walkerai::ObstacleSegment);
MAKE_TYPE_FACTORY(Target, walkerai::Target);
MAKE_TYPE_FACTORY(Danger, ai::Danger);
DAS_BIND_VECTOR(AgentDangersDangers, AgentDangersDangers, ai::Danger, " AgentDangersDangers");
DAS_BIND_VECTOR(ObstacleSegments, ObstacleSegments, walkerai::ObstacleSegment, " ObstacleSegments");

namespace bind_dascript
{
inline walkerai::ObstacleSegment make_obstacle_segment(das::float3 start, das::float3 end, das::float3 dir)
{
  return walkerai::ObstacleSegment(reinterpret_cast<Point3 &>(start), reinterpret_cast<Point3 &>(end),
    reinterpret_cast<Point3 &>(dir));
}
} // namespace bind_dascript
