//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>

#include <generic/dag_tab.h>

#include <util/dag_globDef.h>
#include <util/dag_string.h>


// forward declarations for external classes

class IGenLoad;
class BBox3;
class CollisionResource;

struct Capsule;


class IDagorEdCustomCollider
{
public:
  // traces p + dir * [0, maxt), an empty interval answering false; a hit writes its t into maxt and, when
  // norm is not null, the hit normal
  // constraint: the HeightMap collider's near-vertical arm and the rendinst service's vertical
  // branch read and write maxt as a world-unit drop, not as the parameter t of a non-unit dir
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) = 0;
  virtual const char *getColliderName() const = 0;

  virtual bool isColliderVisible() const = 0;
  virtual bool setupColliderParams(int /*mode*/, const BBox3 & /*area*/) { return true; }
  virtual void prepareCollider() {}
  virtual void clipCapsule(const Capsule & /*c*/, Point3 & /*lpt*/, Point3 & /*wpt*/, real & /*md*/, const Point3 & /*norm*/)
  {
    return;
  }
};

// traceRay's conversion for a backend that traces a unit direction over a distance. trace takes the
// unit direction and a distance, and writes a hit's distance back into it; maxt reads and returns
// the bound in units of dir. An empty interval answers false without tracing
template <class TraceUnitDir>
inline bool trace_via_unit_dir(const Point3 &dir, real &maxt, TraceUnitDir trace)
{
  const real len = length(dir);
  if (len <= 0.f || maxt <= 0.f)
    return false;
  real t = maxt * len;
  if (!trace(dir / len, t))
    return false;
  maxt = t / len;
  return true;
}


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

void set_custom_colliders(dag::ConstSpan<IDagorEdCustomCollider *> colliders, unsigned filter_mask);
void restore_editor_colliders();

dag::ConstSpan<IDagorEdCustomCollider *> get_current_colliders(unsigned &filter_mask);


// dagor phys engine collision
namespace DagorPhys
{
extern bool use_only_visible_colliders;

// drops the clip held, then reads the editor clip stream the cook writes; false for a foreign label,
// a stream that throws or one that loads with no node, and getClipCollision answers null after it
bool load_binary_collision(IGenLoad &crd);

void setup_collider_params(int mode, const BBox3 &area);
bool trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt);
bool trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm);
bool ray_hit_static(const Point3 &p, const Point3 &dir, real maxt);

real clip_capsule_static(Capsule &c, Point3 &cap_pt, Point3 &world_pt);

// dies on close_collision, so re-fetch per use
const CollisionResource *getClipCollision();

void close_collision();

BBox3 get_bounding_box();

const char *get_collision_name();
} // namespace DagorPhys
