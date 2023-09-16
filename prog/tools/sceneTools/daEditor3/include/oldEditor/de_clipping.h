//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_e3dColor.h>

#include <generic/dag_tab.h>

#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <sceneRay/dag_sceneRayDecl.h>


// forward declarations for external classes
class IGenLoad;
class Point3;
class BBox3;
class FastRtDump;

typedef class PropertyContainerControlBase PropPanel2;

struct TpsPhysmatInfo;
struct Capsule;


// phys material management
void init_tps_physmat(const char *physmatFileName);
void close_tps_physmat();
void get_tps_physmat_info(TpsPhysmatInfo &info);


class IDagorEdCustomCollider
{
public:
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) = 0;
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) = 0;
  virtual const char *getColliderName() const = 0;

  virtual bool isColliderVisible() const = 0;
  virtual bool setupColliderParams(int /*mode*/, const BBox3 & /*area*/) { return true; }
  virtual void prepareCollider() {}
  virtual void clipCapsule(const Capsule & /*c*/, Point3 & /*lpt*/, Point3 & /*wpt*/, real & /*md*/, const Point3 & /*norm*/)
  {
    return;
  }
};


// custom colliders register/unregister
void register_custom_collider(IDagorEdCustomCollider *coll);
void unregister_custom_collider(IDagorEdCustomCollider *coll);
int get_custom_colliders_count();
IDagorEdCustomCollider *get_custom_collider(int idx);

// enable/disable colliders
void enable_custom_collider(const char *name);
void disable_custom_collider(const char *name);
bool is_custom_collider_enabled(const IDagorEdCustomCollider *collider);

// enable colliders from 'names' array only
void set_enabled_colliders(const Tab<String> &names);
// disable colliders from 'names' array only
void set_disabled_colliders(const Tab<String> &names);

void get_enabled_colliders(Tab<String> &names);
void get_disabled_colliders(Tab<String> &names);

// enable/disable shadow tracers
void enable_custom_shadow(const char *name);
void disable_custom_shadow(const char *name);
bool is_custom_shadow_enabled(const IDagorEdCustomCollider *collider);

void set_custom_colliders(dag::ConstSpan<IDagorEdCustomCollider *> colliders, unsigned filter_mask);
void restore_editor_colliders();

dag::ConstSpan<IDagorEdCustomCollider *> get_current_colliders(unsigned &filter_mask);

// custom colliders Property Panel routine
// collider_pid -- first collider checkbox's PID
// you have to reserve some number of PIDs to hold all colliders PIDs
// shadow means custom shadows instead of custom colliders
bool fill_custom_colliders_list(PropPanel2 &panel, const char *grp_caption, int grp_pid, int collider_pid, bool shadow,
  bool open_grp = false);
// check collider checkbox on Property Panel and enable/disable collider
// collider_pid -- first collider checkbox's PID
// shadow means custom shadows instead of custom colliders
// return true if collider state changed
bool on_pp_collider_check(int pid, const PropPanel2 &panel, int collider_pid, bool shadow);


// dagor phys engine clipping
namespace DagorPhys
{
extern bool use_only_visible_colliders;

StaticSceneRayTracer *load_binary_raytracer(IGenLoad &crd);

void setup_collider_params(int mode, const BBox3 &area);
bool trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt);
bool trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm);
bool ray_hit_static(const Point3 &p, const Point3 &dir, real maxt);

real clip_capsule_static(Capsule &c, Point3 &cap_pt, Point3 &world_pt);
real clip_capsule_static(Capsule &c, Point3 &cap_pt, Point3 &world_pt, Point3 &norm);

FastRtDump *getFastRtDump();

void init_clipping_binary(StaticSceneRayTracer *rt);

void close_clipping();

BBox3 get_bounding_box();

const char *get_collision_name();
} // namespace DagorPhys
