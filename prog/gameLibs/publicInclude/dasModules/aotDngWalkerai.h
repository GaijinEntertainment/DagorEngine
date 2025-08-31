//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotHumanPhys.h>
#include <dasModules/aotPathFinder.h>
#include <gamePhys/phys/walker/humanPhys.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotWalkerai.h>
#include <dasModules/aotBehNodes.h>
#include <walkerAi/navigation.h>
#include <walkerAi/ecsAgent.h>
#include <walkerAi/aiEnums.h>

MAKE_TYPE_FACTORY(LastTarget, walkerai::LastTarget);
MAKE_TYPE_FACTORY(Aiming, walkerai::Aiming);
MAKE_TYPE_FACTORY(EntityAgent, walkerai::EntityAgent);
MAKE_TYPE_FACTORY(ObstacleEx, walkerai::ObstacleEx);
MAKE_TYPE_FACTORY(AgentObstacles, walkerai::AgentObstacles);

using EntityAgentShootFromArray = carray<Point3, 3>;
DAS_BIND_ARRAY(EntityAgentShootFromArray, EntityAgentShootFromArray, Point3);

namespace bind_dascript
{
inline walkerai::EntityAgent *beh_tree_entity_agent(const BehaviourTree &tree) { return (walkerai::EntityAgent *)tree.getUserPtr(); }

inline int walker_agent_nav_allocAreaId() { return pathfinder::CustomNav::allocAreaId(); }
inline void walker_agent_nav_areaUpdateBox(walkerai::EntityAgent &agent, int area_id, const TMatrix &tm, const BBox3 &oobb, float w1,
  float w2, bool optimize)
{
  agent.getCustomNav()->areaUpdateBox(area_id, tm, oobb, w1, w2, optimize);
}
inline void walker_agent_nav_invalidateTile(walkerai::EntityAgent &agent, uint32_t tile_id, int tx, int ty, int tlayer)
{
  agent.getCustomNav()->invalidateTile(tile_id, tx, ty, tlayer);
}
inline void walker_agent_nav_restoreTiles(walkerai::EntityAgent &agent) { agent.getCustomNav()->restoreLostTiles(); }
inline void walker_agent_nav_removeTile(walkerai::EntityAgent &agent, uint32_t tile_id) { agent.getCustomNav()->removeTile(tile_id); }
inline void walker_agent_nav_areaUpdateCylinder(walkerai::EntityAgent &agent, int area_id, const Point3 &p, const Point2 &radius,
  float w1, float w2, bool optimize)
{
  Point3 tmp(radius.x, radius.y, radius.x);
  agent.getCustomNav()->areaUpdateCylinder(area_id, BBox3(p - tmp, p + tmp), w1, w2, optimize);
}
inline void walker_custom_nav_areaUpdateCylinder(pathfinder::CustomNav &custom_nav, int area_id, const Point3 &p, const Point2 &radius,
  float w1, float w2, bool optimize)
{
  Point3 tmp(radius.x, radius.y, radius.x);
  custom_nav.areaUpdateCylinder(area_id, BBox3(p - tmp, p + tmp), w1, w2, optimize);
}
inline void walker_agent_nav_areaUpdateBoxThres(walkerai::EntityAgent &agent, int area_id, const TMatrix &tm, const BBox3 &oobb,
  float w1, float w2, bool optimize, float posThreshold, float angCosThreshold)
{
  agent.getCustomNav()->areaUpdateBox(area_id, tm, oobb, w1, w2, optimize, posThreshold, angCosThreshold);
}
inline void walker_agent_nav_areaUpdateCylinderThres(walkerai::EntityAgent &agent, int area_id, const Point3 &p, const Point2 &radius,
  float w1, float w2, bool optimize, float posThreshold, float angCosThreshold)
{
  Point3 tmp(radius.x, radius.y, radius.x);
  agent.getCustomNav()->areaUpdateCylinder(area_id, BBox3(p - tmp, p + tmp), w1, w2, optimize, posThreshold, angCosThreshold);
}
inline void walker_agent_nav_areaRemove(walkerai::EntityAgent &agent, int area_id) { agent.getCustomNav()->areaRemove(area_id); }
inline void walker_custom_nav_areaRemove(pathfinder::CustomNav &custom_nav, int area_id) { custom_nav.areaRemove(area_id); }

inline void get_agent_path(const walkerai::EntityAgent &agent,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  das::Array arr;
  arr.data = (char *)agent.path.data();
  arr.size = uint32_t(agent.path.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline bool get_agent_pathPos(const walkerai::EntityAgent &agent, int path_pos_index, Point3 &out_pos)
{
  if (path_pos_index < 0 || path_pos_index >= agent.path.size())
  {
    out_pos = Point3::ZERO;
    return false;
  }
  out_pos = agent.path[path_pos_index];
  return true;
}

inline walkerai::Obstacle *agent_add_obstacle(walkerai::EntityAgent &agent) { return &agent.obstacles.emplace_back(); }

inline walkerai::ObstacleEx *agent_obstacles_add_obstacle(walkerai::AgentObstacles &agent_obstacles)
{
  return &agent_obstacles.obstacles.emplace_back();
}
} // namespace bind_dascript