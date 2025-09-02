// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define USE_BULLET_PHYSICS 1
#include <phys/dag_physics.h>
#include <phys/dag_physObject.h>
#include <phys/dag_vehicle.h>
#include <vehiclePhys/physCar.h>
#undef USE_BULLET_PHYSICS

#include "phys.inc.cpp"

void phys_bullet_simulate(real dt) { phys_simulate(dt); }

void phys_bullet_clear_phys_world() { phys_clear_phys_world(); }
void phys_bullet_create_phys_world(DynamicPhysObjectData *base_obj) { phys_create_phys_world(base_obj); }
void *phys_bullet_get_phys_world() { return pw; }
void phys_bullet_set_phys_body(void *phbody) { phys_set_phys_body(phbody); }

void phys_bullet_init() { phys_init(); }
void phys_bullet_close() { phys_close(); }

void phys_bullet_before_render() { phys_before_render(); }
void phys_bullet_render() { phys_render(IDynRenderService::Stage::STG_RENDER_DYNAMIC_OPAQUE); }
void phys_bullet_render_trans() { phys_render(IDynRenderService::Stage::STG_RENDER_DYNAMIC_TRANS); }

bool phys_bullet_get_phys_tm(int body_id, TMatrix &phys_tm, bool &obj_active)
{
  return phys_get_phys_tm(body_id, phys_tm, obj_active);
}

void phys_bullet_add_impulse(int body_ind, const Point3 &pos, const Point3 &delta, real spring_factor, real damper_factor, real dt)
{
  add_impulse(body_ind, pos, delta, spring_factor, damper_factor, dt);
}

bool phys_bullet_load_collision(IGenLoad &crd) { return pw && pw->loadSceneCollision(crd, 0); }

void phys_bullet_install_tracer(bool (*traceray)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid))
{
  IPhysVehicle::bulletSetStaticTracer(traceray);
}
