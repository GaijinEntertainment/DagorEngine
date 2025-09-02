//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasDataBlock.h>
#include <walkerAi/agent.h>
#include <walkerAi/library.h>
#include <walkerAi/agentObstacle.h>
#include <walkerAi/aiTarget.h>
#include <walkerAi/entityDangers.h>
#include <walkerAi/paramProxy.h>
#include <math/dag_Point3.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(ai::Danger::DangerType, DangerType);

typedef eastl::vector<ai::Danger> AgentDangersDangers;
typedef eastl::vector<walkerai::ObstacleSegment> ObstacleSegments;

MAKE_TYPE_FACTORY(Obstacle, walkerai::Obstacle);
MAKE_TYPE_FACTORY(AgentDangers, ai::AgentDangers);
MAKE_TYPE_FACTORY(ObstacleSegment, walkerai::ObstacleSegment);
MAKE_TYPE_FACTORY(Target, walkerai::Target);
MAKE_TYPE_FACTORY(Danger, ai::Danger);
MAKE_TYPE_FACTORY(RealParamProxy, RealParamProxy);
DAS_BIND_VECTOR(AgentDangersDangers, AgentDangersDangers, ai::Danger, " AgentDangersDangers");
DAS_BIND_VECTOR(ObstacleSegments, ObstacleSegments, walkerai::ObstacleSegment, " ObstacleSegments");

static_assert(sizeof(RealParamProxy) <= sizeof(vec4f));

struct RealParamProxy_JitAbiWrap : RealParamProxy
{
  RealParamProxy_JitAbiWrap(vec4f v) { ::memcpy(this, &v, sizeof(RealParamProxy)); }
  operator vec4f()
  {
    vec4f result;
    ::memcpy(&result, this, sizeof(RealParamProxy));
    return result;
  }
};

namespace das
{
template <>
struct cast<RealParamProxy>
{
  static __forceinline RealParamProxy to(vec4f x)
  {
    RealParamProxy result;
    ::memcpy(&result, &x, sizeof(RealParamProxy));
    return result;
  }
  static __forceinline vec4f from(RealParamProxy x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(RealParamProxy));
    return result;
  }
};

template <>
struct WrapType<RealParamProxy>
{
  enum
  {
    value = true
  };
  typedef vec4f type;
  typedef vec4f rettype;
};


template <>
struct WrapArgType<RealParamProxy>
{
  typedef RealParamProxy_JitAbiWrap type;
};
template <>
struct WrapRetType<RealParamProxy>
{
  typedef RealParamProxy_JitAbiWrap type;
};

template <>
struct SimPolicy<RealParamProxy>
{
  static __forceinline auto to(vec4f a) { return das::cast<RealParamProxy>::to(a); }
};
} // namespace das

namespace bind_dascript
{
inline walkerai::ObstacleSegment make_obstacle_segment(das::float3 start, das::float3 end, das::float3 dir)
{
  return walkerai::ObstacleSegment(reinterpret_cast<Point3 &>(start), reinterpret_cast<Point3 &>(end),
    reinterpret_cast<Point3 &>(dir));
}
inline void RealParamProxy_loadFromBlk(RealParamProxy &proxy, const DataBlock &blk, const char *ecs_name, ecs::EntityId eid,
  const char *bb_int_name, const char *bb_real_Name, DataBlock &blackBoard)
{
  proxy.loadFromBlk(&blk, ecs_name, eid, bb_int_name, bb_real_Name, &blackBoard);
}
inline double RealParamProxy_get(RealParamProxy &proxy, ecs::EntityId eid, DataBlock &blackBoard)
{
  return proxy.get(eid, &blackBoard);
}
inline void RealParamProxy_setFloat(RealParamProxy &proxy, float value, ecs::EntityId eid, DataBlock &blackBoard)
{
  proxy.set(value, eid, &blackBoard);
}
inline void RealParamProxy_setEid(RealParamProxy &proxy, ecs::EntityId value, ecs::EntityId eid, DataBlock &blackBoard)
{
  proxy.set(double(ecs::entity_id_t(value)), eid, &blackBoard);
}
} // namespace bind_dascript
