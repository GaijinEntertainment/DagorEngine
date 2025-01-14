// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionInstances.h>
#include <dag/dag_vector.h>
#include <EASTL/iterator.h>
#include <perfMon/dag_statDrv.h>
#if ENABLE_APEX
#include "apexCollisionInstances.h"
#endif
#include <gamePhys/collision/physLayers.h>
#include <phys/dag_physics.h>
#include <EASTL/type_traits.h>
#include <rendInst/rendInstAccess.h>
#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <gamePhys/collision/cachedCollisionObject.h>
#include <gamePhys/collision/rendinstCollisionWrapper.h>
#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gameMath/interpolateTabUtils.h>
#include <memory/dag_framemem.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>
#include "riCollisionCallback.h"
#include "collisionGlobals.h"

namespace dacoll
{

#if ENABLE_APEX
typedef ApexCollisionInstances CollisionInstancesType;
#else
typedef CollisionInstances CollisionInstancesType;
G_STATIC_ASSERT(!eastl::is_polymorphic<CollisionInstances>::value); // Don't need it (for perfomance reasons)
#endif
static dag::Vector<CollisionInstancesType> ri_instances;
static constexpr int NON_EMPTY_HEAD_INDEX = -2;
static int last_non_empty_ri_instance = NON_EMPTY_HEAD_INDEX; // Tail of list
static int num_non_empty_ri_instances = 0;
static float ri_instances_time = 0;

struct PairCollisionCB
{
  CollisionObject obj;
  WrapperContactResultCB &contactCB;

  int &collMatId;

  PairCollisionCB(const CollisionObject &in_obj, WrapperContactResultCB &callback) :
    obj(in_obj), contactCB(callback), collMatId(callback.collMatId)
  {}

  void onContact(const CollisionObject &contact, const rendinst::RendInstDesc &)
  {
    if (get_phys_world()->needsCollision(contact.body, contactCB))
      get_phys_world()->contactTestPair(obj.body, contact.body, contactCB);
  }
};

struct RIAddToWorldCB : public rendinst::RendInstCollisionCB
{
  void processCollisionInstance(const rendinst::CollisionInfo &info) const
  {
    if (info.handle)
    {
      dacoll::CollisionInstances *instance = dacoll::get_collision_instances_by_handle(info.handle);
      G_ASSERTF(instance, "Cannot find collision for ri at " FMT_P3, P3D(info.tm.getcol(3)));
      if (instance)
        instance->updateTm(info.desc, info.tm);
    }
  }

  virtual void addCollisionCheck(const rendinst::CollisionInfo &info) { processCollisionInstance(info); }

  virtual void addTreeCheck(const rendinst::CollisionInfo &info) { processCollisionInstance(info); }
};


void update_ri_cache_in_volume_to_phys_world(const BBox3 &box)
{
  if (!get_phys_world())
    return;

  get_phys_world()->fetchSimRes(true);

  RIAddToWorldCB callback;
  rendinst::testObjToRIGenIntersection(box, callback, rendinst::GatherRiTypeFlag::RiGenAndExtra, nullptr /*cache*/, PHYSMAT_INVALID,
    true /*unlock_in_cb*/);
}

bool test_collision_ri(const CollisionObject &co, const BBox3 &box, Tab<gamephys::CollisionContactData> &out_contacts,
  const TraceMeshFaces *trace_cache, int mat_id)
{
  if (!get_phys_world())
    return false;

  get_phys_world()->fetchSimRes(true);

  int prev_cont = out_contacts.size();

  TMatrix tm;
  co.body->getTmInstant(tm);

  WrapperContactResultCB contactCb(out_contacts, mat_id, dacoll::get_collision_object_collapse_threshold(co));
  PairCollisionCB pairColl(co, contactCb);
  RICollisionCB<PairCollisionCB> callback(pairColl);

  if (trace_cache)
  {
    mat44f vtm;
    v_mat44_make_from_43cu_unsafe(vtm, tm.array);
    bbox3f castBox;
    v_bbox3_init(castBox, vtm, v_ldu_bbox3(box));
    bool res = try_use_trace_cache(castBox, trace_cache);

    BBox3 wbox;
    v_stu_bbox3(wbox, castBox);
    trace_utils::draw_trace_handle_debug_cast_result(trace_cache, wbox, res, false);
    if (!res)
      trace_cache = nullptr;
  }

  rendinst::testObjToRIGenIntersection(box, tm, callback, rendinst::GatherRiTypeFlag::RiGenAndExtra, trace_cache, mat_id,
    true /*unlock_in_cb*/);
  return out_contacts.size() > prev_cont;
}

bool test_collision_ri(const CollisionObject &co, const BBox3 &box, Tab<gamephys::CollisionContactData> &out_contacts,
  bool provide_coll_info, float at_time, const TraceMeshFaces *trace_cache, int mat_id, bool process_tree_behaviour)
{
  if (!get_phys_world())
    return false;

  get_phys_world()->fetchSimRes(true);

  int prev_cont = out_contacts.size();

  TMatrix tm;
  co.body->getTmInstant(tm);

  WrapperRendinstContactResultCB contactCb(out_contacts, 0, mat_id, process_tree_behaviour);
  WrapperRendInstCollisionCB<WrapperRendinstContactResultCB> callback(co, contactCb, provide_coll_info, at_time);

  if (trace_cache)
  {
    mat44f vtm;
    v_mat44_make_from_43cu_unsafe(vtm, tm.array);
    bbox3f castBox;
    v_bbox3_init(castBox, vtm, v_ldu_bbox3(box));
    bool res = try_use_trace_cache(castBox, trace_cache);

    BBox3 wbox;
    v_stu_bbox3(wbox, castBox);
    trace_utils::draw_trace_handle_debug_cast_result(trace_cache, wbox, res, false);
    if (!res)
      trace_cache = nullptr;
  }

  rendinst::testObjToRIGenIntersection(box, tm, callback, rendinst::GatherRiTypeFlag::RiGenAndExtra, trace_cache, mat_id,
    true /*unlock_in_cb*/);

  return out_contacts.size() > prev_cont;
}

struct SweepCollisionCB
{
  const PhysBody *castShape;
  TMatrix from;
  TMatrix to;
  int collMatId = -1;
  int checkMatId;
  ShapeQueryOutput &out;
  Tab<rendinst::RendInstDesc> *desc = nullptr;
  RIFilterCB *filterCB = nullptr;
  int contactsNum = 0;

  SweepCollisionCB(const PhysBody *cast_shape, const TMatrix &from_tm, const TMatrix &to_tm, int cast_mat_id, ShapeQueryOutput &_out,
    Tab<rendinst::RendInstDesc> *in_desc, RIFilterCB *_filterCB) :
    castShape(cast_shape), checkMatId(cast_mat_id), desc(in_desc), filterCB(_filterCB), out(_out)
  {
    from = from_tm;
    to = to_tm;
  }

  void onContact(const CollisionObject &contact, const rendinst::RendInstDesc &in_desc)
  {
    if (!get_phys_world()->needsCollision<SweepCollisionCB, void>(contact.body, *this))
      return;
    ShapeQueryOutput prev = out;
    out.t = 1.0f;
    get_phys_world()->shapeQuery(castShape, from, to, make_span_const(&contact.body, 1), out);
    if (out.t >= 1.f || (filterCB && !(*filterCB)(in_desc, out.t)))
      out = prev;
    else
    {
      ++contactsNum;
      if (desc)
        desc->push_back(in_desc);
      if (out.t > prev.t)
        out = prev;
    }
  }

  bool needsCollision(void *userDataB, bool /*b_is_static*/) const
  {
    if (checkMatId >= 0)
      if (CollisionObjectUserData *ptr = CollisionObjectUserData::cast((PhysObjectUserData *)userDataB))
        if (ptr->matId >= 0)
          return PhysMat::isMaterialsCollide(checkMatId, ptr->matId);
    return true;
  }
};

void shape_query_ri(const PhysBody *shape, const TMatrix &from, const TMatrix &to, float rad, ShapeQueryOutput &out, int cast_mat_id,
  Tab<rendinst::RendInstDesc> *out_desc, const TraceMeshFaces *handle, RIFilterCB *filterCB)
{
  if (!get_phys_world())
    return;

  get_phys_world()->fetchSimRes(true);

  SweepCollisionCB sweepCb(shape, from, to, cast_mat_id, out, out_desc, filterCB);
  RICollisionCB<SweepCollisionCB> callback(sweepCb);
  Capsule capsule(from.getcol(3), to.getcol(3), rad);

  if (handle)
  {
    bbox3f castBox = capsule.getBoundingBox();
    bool res = try_use_trace_cache(castBox, handle);

    BBox3 wbox;
    v_stu_bbox3(wbox, castBox);
    trace_utils::draw_trace_handle_debug_cast_result(handle, wbox, res, false);
    if (!res)
      handle = nullptr;
  }

  rendinst::testObjToRIGenIntersection(capsule, callback, rendinst::GatherRiTypeFlag::RiGenAndExtra, handle, cast_mat_id,
    true /*unlock_in_cb*/);
#if DA_PROFILER_ENABLED
  DA_PROFILE_TAG(shape_query_ri, ": %u contacts", sweepCb.contactsNum);
#endif
}

void *register_collision_cb(const CollisionResource *collRes, const char *)
{
  CollisionObject obj =
    add_dynamic_collision_from_coll_resource(NULL, collRes, NULL, ACO_NONE, EPL_STATIC, EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC));
  if (!obj)
    return nullptr;
  ri_instances.emplace_back(obj);
  return (void *)(uintptr_t)ri_instances.size();
}

void unregister_collision_cb(void *&handle)
{
  if (!handle || ri_instances.empty()) // assume that call to this function after clear_ri_instances() is not an error
    return;
  CollisionInstances *ci = get_collision_instances_by_handle(handle);
  G_ASSERT_RETURN(ci, );

#ifndef ENABLE_APEX
  if (!ci->empty())
  {
    ci->clearInstances();
    remove_empty_ri_instance_list(*ci);
  }
#endif

  if (ci == &ri_instances.back())
    ri_instances.pop_back();
  else
    ci->clear();
  handle = nullptr;
}

CollisionInstances *get_collision_instances_by_handle(void *handle)
{
  if (!handle)
    return nullptr;
  int ri = int((uintptr_t)handle) - 1;
  G_ASSERTF_RETURN((unsigned)ri < ri_instances.size(), nullptr, "%d >= %d", ri, ri_instances.size());
  return &ri_instances[ri];
}

void flush_ri_instances()
{
  for (auto &ri : ri_instances)
  {
    ri.clearInstances();
    ri.unlink();
  }
  last_non_empty_ri_instance = NON_EMPTY_HEAD_INDEX;
  num_non_empty_ri_instances = 0;
}

void clear_ri_instances()
{
  clear_and_shrink(ri_instances);
  flush_ri_instances();
  ri_instances_time = 0;
}

void clear_ri_apex_instances()
{
#if ENABLE_APEX
  for (auto &ri : ri_instances)
    ri.clearApexInstances();
#endif
}

void enable_disable_ri_instance(const rendinst::RendInstDesc &desc, bool flag)
{
  void *handle = rendinst::getCollisionResourceHandle(desc);
  if (CollisionInstances *instance = handle ? get_collision_instances_by_handle(handle) : nullptr)
    instance->enableDisableCollisionObject(desc, flag);
}

bool is_ri_instance_enabled(const CollisionInstances *instance, const rendinst::RendInstDesc &desc)
{
  return instance->isCollisionObjectEnabled(desc);
}


void invalidate_ri_instance(const rendinst::RendInstDesc &desc)
{
  void *handle = rendinst::getCollisionResourceHandle(desc);
  if (CollisionInstances *instance = handle ? get_collision_instances_by_handle(handle) : nullptr)
    instance->removeCollisionObject(desc);
}

void move_ri_instance(const rendinst::RendInstDesc &desc, const Point3 &vel, const Point3 &omega)
{
  void *handle = rendinst::getCollisionResourceHandle(desc);
  if (CollisionInstances *instance = handle ? get_collision_instances_by_handle(handle) : nullptr)
    instance->updateTm(desc, vel, omega);
}

bool check_ri_collision_filtered(const rendinst::RendInstDesc &desc, const TMatrix &initial_tm, const TMatrix &new_tm, int filter)
{
  void *handle = rendinst::getCollisionResourceHandle(desc);
  CollisionInstances *instance = handle ? get_collision_instances_by_handle(handle) : nullptr;
  if (!instance)
    return false;

  CollisionObject obj = instance->updateTm(desc, new_tm);
  Tab<gamephys::CollisionContactData> newContacts(framemem_ptr());
  test_collision_world(obj, newContacts, -1, EPL_DEFAULT, filter);

  if (newContacts.empty())
    return false;

  instance->updateTm(desc, initial_tm);
  Tab<gamephys::CollisionContactData> oldContacts(framemem_ptr());
  test_collision_world(obj, oldContacts, -1, EPL_DEFAULT, filter);
  if (oldContacts.empty())
    return true; // we haven't got contacts and now we have - definetely we should restrain

  // check if we get worser with this new tm
  for (const gamephys::CollisionContactData &contact : oldContacts)
  {
    Point3 newPos = new_tm * contact.posA;
    if ((newPos - contact.wpos) * contact.wnormB < 0.f)
      return true;
  }
  return false;
}

float get_ri_instances_time() { return ri_instances_time; }

#ifndef ENABLE_APEX
void push_non_empty_ri_instance_list(CollisionInstances &ci)
{
  G_ASSERT(size_t(&ci - ri_instances.data()) < ri_instances.size());
  G_ASSERT(ci.prevNotEmpty < 0 && ci.nextNotEmpty < 0);
  G_ASSERT(!ci.empty());
  ci.prevNotEmpty = last_non_empty_ri_instance;
  last_non_empty_ri_instance = &ci - ri_instances.data();
  if (ci.prevNotEmpty >= 0)
    ri_instances[ci.prevNotEmpty].nextNotEmpty = last_non_empty_ri_instance;
  num_non_empty_ri_instances++;
}

void remove_empty_ri_instance_list(CollisionInstances &ci)
{
  G_ASSERT(size_t(&ci - ri_instances.data()) < ri_instances.size());
  G_ASSERT(ci.empty());
  int i = &ci - ri_instances.data();
  if (ci.nextNotEmpty >= 0)
  {
    G_ASSERT(ri_instances[ci.nextNotEmpty].prevNotEmpty == i);
    ri_instances[ci.nextNotEmpty].prevNotEmpty = ci.prevNotEmpty;
  }
  else
  {
    G_ASSERT(last_non_empty_ri_instance == i);
    last_non_empty_ri_instance = ci.prevNotEmpty;
  }
  if (ci.prevNotEmpty >= 0)
  {
    G_ASSERT(ri_instances[ci.prevNotEmpty].nextNotEmpty == i);
    ri_instances[ci.prevNotEmpty].nextNotEmpty = ci.nextNotEmpty;
  }
  else
    G_ASSERT(ci.prevNotEmpty == NON_EMPTY_HEAD_INDEX);
  G_ASSERT(last_non_empty_ri_instance != i);
  G_UNUSED(i);
  ci.prevNotEmpty = ci.nextNotEmpty = -1;
  num_non_empty_ri_instances--;
  G_ASSERT(num_non_empty_ri_instances >= 0);
}
#endif

void update_ri_instances(float dt)
{
  TIME_PROFILE_DEV(update_ri_instances);
  ri_instances_time += dt;
#if ENABLE_APEX // TODO: implement for apex code path
  for (auto &ri : ri_instances)
    ri.update(ri_instances_time);
#else
  int n = 0;
  for (int i = last_non_empty_ri_instance; i >= 0;)
  {
    G_ASSERT(!ri_instances[i].empty());
    int pi = ri_instances[i].prevNotEmpty;
    if (!ri_instances[i].update(ri_instances_time))
      remove_empty_ri_instance_list(ri_instances[i]);
    else
      ++n;
    i = pi;
  }
  G_UNUSED(n);
  G_ASSERTF(n == num_non_empty_ri_instances, "%d != %d", n, num_non_empty_ri_instances); // Broken list?
#endif
}

} // namespace dacoll
