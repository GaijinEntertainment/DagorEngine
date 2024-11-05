//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityComponent.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physRagdoll.h>

struct ProjectileImpulse
{
  struct ImpulseData
  {
    int nodeId;
    Point3 pos;
    Point3 impulse;
    ImpulseData(int node_id, const Point3 &_pos, const Point3 &_impulse) : nodeId(node_id), pos(_pos), impulse(_impulse) {}
  };
  eastl::vector<eastl::pair<float, ImpulseData>> data;
};

namespace das
{
struct ProjectileImpulseDataPair
{
  float time;
  const ProjectileImpulse::ImpulseData *data;
};
} // namespace das

ECS_DECLARE_RELOCATABLE_TYPE(ProjectileImpulse);
ECS_DECLARE_RELOCATABLE_TYPE(ProjectileImpulse::ImpulseData);
ECS_DECLARE_RELOCATABLE_TYPE(das::ProjectileImpulseDataPair);

ECS_DECLARE_BOXED_TYPE(PhysRagdoll);

inline void save_projectile_impulse(float cur_time, ProjectileImpulse &projectile_impulse, int node_id, const Point3 &pos,
  const Point3 &impulse, float save_delta_time)
{
  projectile_impulse.data.erase(projectile_impulse.data.begin(),
    eastl::remove_if(projectile_impulse.data.begin(), projectile_impulse.data.end(),
      [cur_time, save_delta_time](auto &time) { return time.first > cur_time - save_delta_time; }));
  projectile_impulse.data.emplace_back(cur_time, ProjectileImpulse::ImpulseData(node_id, pos, impulse));
}