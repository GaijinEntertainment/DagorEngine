//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace rendinst
{
struct RendInstDesc;
}
class CollisionResource;
class TMatrix;
class Point3;

namespace dacoll
{
class CollisionInstances;

void *register_collision_cb(const CollisionResource *collRes, const char *debug_name);
void unregister_collision_cb(void *&handle);
CollisionInstances *get_collision_instances_by_handle(void *handle);
void clear_ri_instances();
void clear_ri_apex_instances();
void invalidate_ri_instance(const rendinst::RendInstDesc &desc);
void enable_disable_ri_instance(const rendinst::RendInstDesc &desc, bool flag);
bool is_ri_instance_enabled(const CollisionInstances *instance, const rendinst::RendInstDesc &desc);
void move_ri_instance(const rendinst::RendInstDesc &desc, const Point3 &vel, const Point3 &omega);
bool check_ri_collision_filtered(const rendinst::RendInstDesc &desc, const TMatrix &initial_tm, const TMatrix &new_tm, int filter);
float get_ri_instances_time();

void update_ri_instances(float dt);
}; // namespace dacoll
