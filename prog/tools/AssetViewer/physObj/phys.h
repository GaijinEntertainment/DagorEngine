// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <phys/dag_physResource.h>
#include <animChar/dag_animCharacter2.h>


class DynamicPhysObjectData;
class PhysicsResource;
class Point3;

namespace physsimulator
{
enum
{
  PHYS_DEFAULT = 1,
  PHYS_BULLET,
  PHYS_JOLT,
  PHYS_TYPE_LAST
};
enum
{
  SCENE_TYPE_GROUP,
  SCENE_TYPE_STACK,
};


extern real springFactor;
extern real damperFactor;
extern PtrTab<PhysicsResource> simObjRes;
static const float def_base_plane_ht = -2.05f;

void simulate(float dt);

int getDefaultPhys();

bool begin(DynamicPhysObjectData *base_obj, int phys_type, int inst_count, int scene_type, float interval,
  float base_plane_ht0 = def_base_plane_ht, float base_plane_ht1 = def_base_plane_ht);
void setTargetObj(void *phys_body, const char *res);
void end();
void *getPhysWorld();

void beforeRender();
void renderTrans(bool render_collision, bool render_geom, bool bodies, bool body_center, bool constraints, bool constraints_refsys);
void render();

void init();
void close();

bool connectSpring(const Point3 &pt, const Point3 &dir);
bool disconnectSpring();
void setSpringCoord(const Point3 &pt, const Point3 &dir);

bool shootAtObject(const Point3 &pt, const Point3 &dir, float bullet_impulse = 0.0061f * 315.0f);
}; // namespace physsimulator

#define DECLARE_PHYS_API(PHYS)                                                                                                     \
  extern void phys_##PHYS##_init();                                                                                                \
  extern void phys_##PHYS##_close();                                                                                               \
  extern void phys_##PHYS##_simulate(real dt);                                                                                     \
  extern void phys_##PHYS##_create_phys_world(DynamicPhysObjectData *base_obj, float base_plane_ht0, float base_plane_ht1,         \
    int inst_count, int scene_type, float interval);                                                                               \
  extern void *phys_##PHYS##_get_phys_world();                                                                                     \
  extern void phys_##PHYS##_set_phys_body(void *phbody);                                                                           \
  extern void *phys_##PHYS##_start_ragdoll(AnimV20::IAnimCharacter2 *animChar, const Point3 &start_vel);                           \
  extern void phys_##PHYS##_delete_ragdoll(void *&ragdoll);                                                                        \
  extern void phys_##PHYS##_clear_phys_world();                                                                                    \
  extern void phys_##PHYS##_before_render();                                                                                       \
  extern void phys_##PHYS##_render_trans();                                                                                        \
  extern void phys_##PHYS##_render();                                                                                              \
  extern void phys_##PHYS##_render_debug(bool bodies, bool body_center, bool constraints, bool constraints_refsys);                \
  extern bool phys_##PHYS##_get_phys_tm(int obj_idx, int sub_body_idx, TMatrix &phys_tm, bool &obj_active);                        \
  extern void phys_##PHYS##_add_impulse(int obj_idx, int sub_body_idx, const Point3 &pos, const Point3 &delta, real spring_factor, \
    real damper_factor, real dt);

DECLARE_PHYS_API(bullet)
DECLARE_PHYS_API(jolt)

#undef DECLARE_PHYS_API
