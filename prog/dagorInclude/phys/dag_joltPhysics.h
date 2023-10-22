//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>

#include <phys/dag_physCollision.h>
#include <phys/dag_physBodyCreationData.h>
#include <phys/dag_physUserData.h>
#include <phys/dag_physShapeQueryResult.h>
#include <phys/dag_joltHelpers.h>

using PhysConstraint = JPH::Constraint;
#include <phys/dag_physJoint.h>
#include <phys/dag_joltHelpers.h>


// forward declarations for external classes
class IGenLoad;


class IJoltPhysContactData
{
public:
  virtual ~IJoltPhysContactData() = default;
};

namespace jolt_api
{
extern JPH::PhysicsSystem *physicsSystem;

inline JPH::PhysicsSystem &phys_sys() { return *physicsSystem; }
inline JPH::BodyInterface &body_api() { return physicsSystem->GetBodyInterface(); }
inline const JPH::BodyLockInterface &body_lock() { return physicsSystem->GetBodyLockInterface(); }
inline float min_convex_rad_for_box(const Point3 &ext)
{
  return eastl::min(eastl::min(JPH::cDefaultConvexRadius, ext.x * 0.5f), eastl::min(ext.y * 0.5f, ext.z * 0.5f));
}
} // namespace jolt_api

class PhysBody
{
public:
  PhysBody(PhysWorld *w, float mass, const PhysCollision *coll, const TMatrix &tm = TMatrix::IDENT,
    const PhysBodyCreationData &s = {});
  ~PhysBody();


  static auto &api() { return jolt_api::body_api(); }

  struct ObjFilter : public JPH::ObjectLayerFilter
  {
    ObjFilter(unsigned short gm, unsigned short lm) : groupMask(gm), layerMask(lm) {}
    bool ShouldCollide(JPH::ObjectLayer inLayer) const override final
    {
      return (objlayer_to_group_mask(inLayer) & layerMask) && (groupMask & objlayer_to_layer_mask(inLayer));
    }
    unsigned short groupMask, layerMask;
  };

  template <typename C>
  struct BodyFilterEx : public JPH::BodyFilter
  {
    BodyFilterEx(const C &needcoll_cb) : c(needcoll_cb) {}
    bool ShouldCollideLocked(const JPH::Body &inBody) const override final
    {
      if (auto *physBody = (PhysBody *)(void *)(uintptr_t)inBody.GetUserData())
        return c.needsCollision(physBody, physBody->getUserData());
      return true;
    }
    const C &c;
  };

  template <typename Callable>
  static void lock_body_rw(JPH::BodyID body_id, Callable f)
  {
    JPH::BodyLockWrite lock(jolt_api::body_lock(), body_id);
    if (lock.Succeeded())
      f(lock.GetBody());
  }
  template <typename Callable>
  void lockRW(Callable f)
  {
    lock_body_rw(bodyId, f);
  }

  template <typename Callable>
  static void lock_body_ro(JPH::BodyID body_id, Callable f)
  {
    JPH::BodyLockRead lock(jolt_api::body_lock(), body_id);
    if (lock.Succeeded())
      f(lock.GetBody());
  }
  template <typename Callable>
  void lockRO(Callable f) const
  {
    lock_body_ro(bodyId, f);
  }

  void setTm(const TMatrix &wtm);
  void getTm(TMatrix &wtm) const;
  void setTmInstant(const TMatrix &local_to_world, bool = false) { setTm(local_to_world); }
  void getTmInstant(TMatrix &local_to_world) const { getTm(local_to_world); }

  void disableGravity(bool disable);
  bool isGravityDisabled();

  void activateBody(bool active = true)
  {
    if (active)
      api().ActivateBody(bodyId);
    else
      api().DeactivateBody(bodyId);
  }
  bool isActive() const { return api().IsActive(bodyId); }
  bool isInWorld() const { return api().IsAdded(bodyId); }
  inline void disableDeactivation() {}
  bool isDeactivationDisabled() const { return false; };
  bool doesBodyWantDeactivation() const { return false; };


  // Set both tm and velocities - useful for animated bodies.
  void setTmWithDynamics(const TMatrix &wtm, real dt, bool activate = true);

  // Zero mass makes body static.
  void setMassMatrix(real mass, real ixx, real iyy, real izz);
  real getMass() const
  {
    JPH::BodyLockRead lock(jolt_api::body_lock(), bodyId);
    if (lock.Succeeded() && !lock.GetBody().IsStatic())
      return safeinv(lock.GetBody().GetMotionProperties()->GetInverseMass());
    return 0;
  }
  void getMassMatrix(real &mass, real &ixx, real &iyy, real &izz);
  void getInvMassMatrix(real &mass, real &ixx, real &iyy, real &izz);


  void setCollision(const PhysCollision *collision, bool allow_fast_inaccurate_coll_tm = true);
  PhysCollision *getCollisionScaledCopy(const Point3 &scale = Point3(1, 1, 1));
  void patchCollisionScaledCopy(const Point3 &s, PhysBody *o);

  bool isConvexCollisionShape() const;
  void getShapeAabb(Point3 &out_bb_min, Point3 &out_bb_max) const;
  void setSphereShapeRad(float rad);
  void setBoxShapeExtents(const Point3 &ext);
  void setVertCapsuleShapeSize(float cap_rad, float cap_cyl_ht);

  int getMaterialId() const { return safeMaterialId; }
  void setMaterialId(int id);

  // Should be called after setCollision() to set layer mask for all collision elements.
  void setGroupAndLayerMask(unsigned int group, unsigned int mask);
  unsigned int getInteractionLayer() const { return layerMask; }
  unsigned int getGroupMask() const { return groupMask; }

  void setVelocity(const Point3 &v, bool = true) { api().SetLinearVelocity(bodyId, to_jVec3(v)); }
  Point3 getVelocity() const { return to_point3(api().GetLinearVelocity(bodyId)); }

  void setAngularVelocity(const Point3 &w, bool = true) { api().SetAngularVelocity(bodyId, to_jVec3(w)); }
  Point3 getAngularVelocity() const { return to_point3(api().GetAngularVelocity(bodyId)); }

  Point3 getPointVelocity(const Point3 &world_pt) { return to_point3(api().GetPointVelocity(bodyId, to_jVec3(world_pt))); }

  void setAcceleration(const Point3 & /*acc*/, bool /*activate*/ = true) { G_ASSERT(0); }
  void setGravity(const Point3 &grav, bool activate = true);

  void addImpulse(const Point3 &p, const Point3 &force_x_dt, bool activate = true);

  void setContinuousCollisionMode(bool use);

  void setCollisionCallback(IPhysBodyCollisionCallback *cb) { ccb = cb; }
  IPhysBodyCollisionCallback *getCollisionCallback() { return ccb; }

  void setPreSolveCallback(PhysBodyPreSolveCallback cb) { pcb = cb; }
  PhysBodyPreSolveCallback getPreSolveCallback() { return pcb; }

  void *getUserData() const { return userPtr; }
  void setUserData(void *p) { userPtr = p; }

  PhysWorld *getPhysWorld() const { return world; }

  void setCcdParams(float rad, float threshold);

  void wakeUp() { api().ActivateBody(bodyId); }

  JPH::BodyID bodyId;

  static PhysBody *from_body_id(JPH::BodyID bid) { return (PhysBody *)(void *)(uintptr_t)api().GetUserData(bid); }
  static JPH::RefConst<JPH::Shape> get_shape(JPH::BodyID bid) { return api().GetShape(bid); }
  JPH::RefConst<JPH::Shape> getShape() const { return api().GetShape(bodyId); }

protected:
  static JPH::RefConst<JPH::Shape> create_jolt_collision_shape(const PhysCollision *c);

  PhysWorld *world = nullptr;
  IPhysBodyCollisionCallback *ccb = nullptr;
  PhysBodyPreSolveCallback pcb = nullptr;

  int safeMaterialId = 0;
  unsigned short groupMask = 0xFFFFu, layerMask = 0xFFFFu;
  void *userPtr = nullptr;

  friend class PhysWorld;
};

template <>
struct PhysBody::BodyFilterEx<void *> : public JPH::BodyFilter
{
  BodyFilterEx(void *const &) {}
};


class PhysRayCast
{
public:
  PhysRayCast(const Point3 &pos, const Point3 &dir, real max_length, PhysWorld *world);

  void setFilterMask(unsigned fmask) { filterMask = fmask; }

  void setPos(const Point3 &p);
  const Point3 &getPos() const;

  void setDir(const Point3 &d);
  const Point3 &getDir();

  void setMaxLength(real len);
  real getMaxLength() const;

  // Force update for auto-updated PhysRayCast.
  void forceUpdate()
  {
    if (ownWorld)
      forceUpdate(ownWorld);
  }

  // Update manually-updated PhysRayCast.
  void forceUpdate(PhysWorld *world);

  bool hasContact() const { return hasHit; }

  real getLength() const { return hit.mFraction * maxLength; }
  Point3 getPoint() const { return to_point3(pos + dir * hit.mFraction); }
  Point3 getNormal() const;
  PhysBody *getBody() const { return !hit.mBodyID.IsInvalid() ? PhysBody::from_body_id(hit.mBodyID) : nullptr; }
  int getMaterial() { return 0; }

protected:
  PhysWorld *ownWorld;

  JPH::Vec3 pos;
  JPH::Vec3 dir;
  float maxLength;
  JPH::RayCastResult hit;

  bool hasHit = false;
  uint16_t filterMask = 0xFFFF;
};

class PhysWorld
{
public:
  static void init_engine(bool single_threaded = false);
  static void term_engine();
  static JPH::PhysicsSystem &phys_sys() { return jolt_api::phys_sys(); }
  static JPH::BodyInterface &body_api() { return jolt_api::body_api(); }

  PhysWorld(real default_static_friction, real default_dynamic_friction, real default_restitution, real default_softness);
  ~PhysWorld();

  // interaction layers
  void setInteractionLayers(unsigned int mask1, unsigned int mask2, bool make_contacts);

public:
  void startSim(real dt, bool wake_up_thread = true);
  bool fetchSimRes(bool wait, PhysBody * = nullptr);

  constexpr bool canExecActionNow() const { return true; }
  void addAfterPhysAction(AfterPhysUpdateAction *) {}

  inline void simulate(real dt)
  {
    startSim(dt);
    fetchSimRes(true);
  }

  void clear() {}

  int getDefaultMaterialId() { return 0; }

  int createNewMaterialId();

  // parameters: for simple material system
  int createNewMaterialId(real static_friction, real dynamic_friction, real restitution, real softness);

  void setMaterialPairInteraction(int mat_id1, int mat_id2, bool collide);

  // add body to this world (may require fetchSimRes(true) called before adding body)
  void addBody(PhysBody *b, bool kinematic, bool update_aabb = true);
  // removed body from this world (may require fetchSimRes(true) called before removing body)
  void removeBody(PhysBody *b);

  PhysJoint *createRagdollBallJoint(PhysBody *body1, PhysBody *body2, const TMatrix &tm, const Point3 &min_limit,
    const Point3 &max_limit, real damping, real twist_damping, real /*stiffness*/, real sleep_threshold = 8.0)
  {
    return regJoint(new PhysRagdollBallJoint(body1, body2, tm, min_limit, max_limit, damping, twist_damping, sleep_threshold));
  }

  PhysJoint *createRagdollHingeJoint(PhysBody *body1, PhysBody *body2, const Point3 &pos, const Point3 &axis, const Point3 &mid_axis,
    const Point3 &x_axis, real ang_limit, real damping, real /*stiffness*/, real sleep_threshold = 8.0)
  {
    return regJoint(new PhysRagdollHingeJoint(body1, body2, pos, axis, mid_axis, x_axis, ang_limit, damping, sleep_threshold));
  }

  PhysJoint *createRevoluteJoint(PhysBody * /*body1*/, PhysBody * /*body2*/, const Point3 & /*pos*/, const Point3 & /*axis*/,
    real /*min_ang*/, real /*max_ang*/, real /*min_rest*/, real /*max_rest*/, real /*spring*/, short /*proj_type*/,
    real /*proj_angle*/, real /*proj_dist*/, real /*damping*/, short /*flags*/, real /*sleep_threshold*/ = 8.0)
  {
    return NULL;
  }

  PhysJoint *createSphericalJoint(PhysBody * /*body1*/, PhysBody * /*body2*/, const Point3 & /*pos*/, const Point3 & /*dir*/,
    const Point3 & /*axis*/, real /*min_ang*/, real /*max_ang*/, real /*min_rest*/, real /*max_rest*/, real /*sw_val*/,
    real /*sw_rest*/, real /*spring*/, real /*damp*/, real /*sw_spr*/, real /*sw_damp*/, real /*tw_spr*/, real /*tw_damp*/,
    short /*proj_type*/, real /*proj_dist*/, short /*flags*/, real /*sleep_threshold*/ = 8.f)
  {
    return NULL;
  }

  PhysJoint *create6DofSpringJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2)
  {
    return regJoint(new Phys6DofSpringJoint(body1, body2, frame1, frame2));
  }

  PhysJoint *create6DofJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2)
  {
    return regJoint(new Phys6DofJoint(body1, body2, frame1, frame2));
  }

  void *loadSceneCollision(IGenLoad &crd, int scene_mat_id = -1);
  bool unloadSceneCollision(void *handle);

  Point3 getGravity() const { return to_point3(phys_sys().GetGravity()); }
  void setGravity(const Point3 &gravity) { phys_sys().SetGravity(to_jVec3(gravity)); }

  struct Material
  {
    real friction;
    real restitution;
  };

  const Material *getMaterial(int id) const;

  void updateContactReports();

  void setMaxSubSteps(int num_sub_steps) { maxSubSteps = num_sub_steps; }
  void setFixedTimeStep(float fts) { fixedTimeStep = fts; }

  static JPH::PhysicsSystem *getScene() { return jolt_api::physicsSystem; }

  void shapeQuery(const PhysBody *body, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
    PhysShapeQueryResult &out, int filter_grp = 0x1, int filter_mask = 0xFFFF);
  void shapeQuery(const PhysCollision &shape, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
    PhysShapeQueryResult &out, int filter_grp = 0x1, int filter_mask = 0xFFFF);

  template <typename C>
  bool convexSweepTest(const PhysBody *shape, const Point3 &from, const Point3 &to, const C &needcoll_cb, PhysShapeQueryResult &out,
    int filter_grp = 0x1, int filter_mask = 0xFFFF)
  {
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> closestCb;
    PhysBody::ObjFilter objf(filter_grp, filter_mask);
    PhysBody::BodyFilterEx bodyf(needcoll_cb);
    return convex_shape_sweep_test(shape->getShape(), from, to, closestCb, out, objf, bodyf);
  }
  template <typename C>
  bool convexSweepTest(const PhysCollision &shape, const Point3 &from, const Point3 &to, const C &needcoll_cb,
    PhysShapeQueryResult &out, int filter_grp = 0x1, int filter_mask = 0xFFFF)
  {
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> closestCb;
    PhysBody::ObjFilter objf(filter_grp, filter_mask);
    PhysBody::BodyFilterEx bodyf(needcoll_cb);
    switch (shape.collType)
    {
      case PhysCollision::TYPE_SPHERE:
      {
        JPH::SphereShape convex(static_cast<const PhysSphereCollision &>(shape).getRadius());
        return convex_shape_sweep_test(&convex, from, to, closestCb, out, objf, bodyf);
      }
      case PhysCollision::TYPE_BOX:
      {
        auto ext = static_cast<const PhysBoxCollision &>(shape).getSize();
        JPH::BoxShape convex(to_jVec3(ext / 2), jolt_api::min_convex_rad_for_box(ext));
        return convex_shape_sweep_test(&convex, from, to, closestCb, out, objf, bodyf);
      }
      default: G_ASSERTF(0, "unsupported collType=%d in %s", shape.collType, __FUNCTION__);
    }
    return false;
  }

  template <typename C>
  struct ContactResultCB final : public JPH::CollideShapeCollector, public JPH::BodyFilter
  {
    // typename C::contact_data_t
    // typename C::obj_user_data_t
    // float C::addSingleResult(const C::contact_data_t &cp, C::obj_user_data_t *objA, C::obj_user_data_t *objB, <ANY> *objInfo)
    // void  C::visualDebugForSingleResult(const PhysBody *bodyA, const PhysBody *bodyB, const C::contact_data_t &c)
    // bool  C::needsCollision(C::obj_user_data_t *objB, bool b_is_static)
    C &c;
    const PhysBody *pbA, *pb2;

    ContactResultCB(C &_c, const PhysBody *pb_a, const PhysBody *pb_b = nullptr) : c(_c), pbA(pb_a), pb2(pb_b) {}
    void AddHit(const JPH::CollideShapeResult &cp) override
    {
      auto *pbB = pb2 ? pb2 : PhysBody::from_body_id(cp.mBodyID2);
      void *userPtrA = pbA->getUserData();
      void *userPtrB = pbB ? pbB->getUserData() : nullptr;

      typename C::contact_data_t cdata;
      cdata.depth = -cp.mPenetrationDepth;
      cdata.wpos = to_point3(cp.mContactPointOn1);
      cdata.wposB = to_point3(cp.mContactPointOn2);
      cdata.wnormB = to_point3(-cp.mPenetrationAxis.Normalized());
      cdata.posA = to_point3(body_api().GetWorldTransform(pbA->bodyId).Inversed() * cp.mContactPointOn1);
      cdata.posB = to_point3(body_api().GetWorldTransform(pb2 ? pb2->bodyId : cp.mBodyID2).Inversed() * cp.mContactPointOn2);
      c.visualDebugForSingleResult(pbA, pbB, cdata);
      c.addSingleResult(cdata, (typename C::obj_user_data_t *)userPtrA, (typename C::obj_user_data_t *)userPtrB, nullptr);
    }
    bool ShouldCollideLocked(const JPH::Body &inBody) const override
    {
      if (auto *pb = (PhysBody *)(void *)(uintptr_t)inBody.GetUserData())
        return c.needsCollision((typename C::obj_user_data_t *)pb->getUserData(), inBody.IsStatic());
      return true;
    }
  };

  template <typename C>
  void contactTest(const PhysBody *shape, C &contact_cb, int filter_grp = 1u, int filter_mask = 0xFFFFu)
  {
    ContactResultCB<C> cb(contact_cb, shape);
    phys_sys().GetNarrowPhaseQuery().CollideShape(shape->getShape(), JPH::Vec3::sReplicate(1.0f),
      body_api().GetCenterOfMassTransform(shape->bodyId), defCollideShapeStg, JPH::Vec3::sZero(), cb, {},
      PhysBody::ObjFilter(filter_grp, filter_mask), cb);
  }
  template <typename C>
  void contactTestPair(const PhysBody *shape, const PhysBody *shapeB, C &contact_cb, int filter_grp = 1u, int filter_mask = 0xFFFFu)
  {
    ContactResultCB<C> cb(contact_cb, shape, shapeB);
    if (needsCollision(shapeB, contact_cb, filter_grp, filter_mask))
      JPH::CollisionDispatch::sCollideShapeVsShape(shape->getShape(), shapeB->getShape(), JPH::Vec3::sReplicate(1.0f),
        JPH::Vec3::sReplicate(1.0f), body_api().GetCenterOfMassTransform(shape->bodyId),
        body_api().GetCenterOfMassTransform(shapeB->bodyId), {}, {}, defCollideShapeStg, cb);
  }

  template <typename C, typename UDT = typename C::obj_user_data_t>
  static bool needsCollision(const PhysBody *bodyB, C &c, int filter_grp = 1, int filter_mask = 0xFFFFu)
  {
    if (!bodyB || !(bodyB->groupMask & filter_mask) || !(filter_grp & bodyB->layerMask))
      return false;
    return c.needsCollision((UDT *)bodyB->getUserData(), body_api().GetMotionType(bodyB->bodyId) == JPH::EMotionType::Static);
  }

protected:
  int maxSubSteps = 3;
  float fixedTimeStep = 1.f / 60.f;

  // materials
  Tab<Material> materials;

  PhysJoint *regJoint(PhysJoint *j);

  friend class PhysBody;
  friend class PhysCollision;

  static void shape_query(const JPH::Shape *shape, const TMatrix &from, const Point3 &dir, PhysWorld *,
    dag::ConstSpan<PhysBody *> bodies, PhysShapeQueryResult &out, int filter_grp, int filter_mask);
  static bool convex_shape_sweep_test(const JPH::Shape *shape, const Point3 &from, const Point3 &to,
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> &closestCb, PhysShapeQueryResult &out,
    const JPH::ObjectLayerFilter &obj_filter, const JPH::BodyFilter &body_filter);

  static JPH::ShapeCastSettings defShapeCastStg;
  static JPH::CollideShapeSettings defCollideShapeStg;
};

inline void init_jolt_physics_engine(bool single_threaded = false) { PhysWorld::init_engine(single_threaded); }
inline void close_jolt_physics_engine() { PhysWorld::term_engine(); }

int phys_body_get_hmap_step(PhysBody *b);
int phys_body_set_hmap_step(PhysBody *b, int step);
