//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <math/dag_plane3.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btMaterial.h>
#include <phys/dag_physUserData.h>
#include <phys/dag_physCollision.h>
#include <phys/dag_physBodyCreationData.h>
#include <phys/dag_physShapeQueryResult.h>

using PhysConstraint = btTypedConstraint;
#include <phys/dag_physJoint.h>

// forward declarations for external classes
class IGenLoad;


void init_bullet_physics_engine(bool = false);
void close_bullet_physics_engine();
extern int64_t bt_control_tid;


#ifndef BT_USE_DOUBLE_PRECISION
__forceinline const Point3 &to_point3(const btVector3 &v) { return (const Point3 &)v; }
__forceinline Point3 &to_point3(btVector3 &v) { return (Point3 &)v; }
__forceinline btVector3 to_btVector3(const Point3 &v) { return btVector3(v.x, v.y, v.z); }
#else
__forceinline btVector3 to_btVector3(const Point3 &v) { return btVector3(v.x, v.y, v.z); }
__forceinline Point3 to_point3(const btVector3 &v) { return Point3(v.x, v.y, v.z); }
#endif


__forceinline btTransform to_btTransform(const TMatrix &v)
{
  return btTransform(btMatrix3x3(v.m[0][0], v.m[1][0], v.m[2][0], v.m[0][1], v.m[1][1], v.m[2][1], v.m[0][2], v.m[1][2], v.m[2][2]),
    to_btVector3(v.getcol(3)));
}

__forceinline TMatrix to_tmatrix(const btTransform &v)
{
  TMatrix out;
  out.setcol(0, to_point3(v.getBasis().getColumn(0)));
  out.setcol(1, to_point3(v.getBasis().getColumn(1)));
  out.setcol(2, to_point3(v.getBasis().getColumn(2)));
  out.setcol(3, to_point3(v.getOrigin()));
  return out;
}

struct SetActivationStateAfterPhysUpdateAction final : AfterPhysUpdateAction
{
  int actState;
  SetActivationStateAfterPhysUpdateAction(btRigidBody *b, int as) : AfterPhysUpdateAction(b), actState(as) {}
  void doAction(PhysWorld &, bool) override { body->setActivationState(actState); }
};

class IBulletPhysBodyCollisionCallback
{
public:
  virtual void onCollision(PhysBody *other, const Point3 &pt, const Point3 &norm, int this_matId, int other_matId,
    const Point3 &norm_imp, const Point3 &friction_imp, const Point3 &this_old_vel, const Point3 &other_old_vel) = 0;
};

class IBulletPhysContactData
{
public:
  virtual ~IBulletPhysContactData() = default;
};


class PhysBody
{
public:
  PhysBody(PhysWorld *w, float mass, const PhysCollision *coll, const TMatrix &tm = TMatrix::IDENT,
    const PhysBodyCreationData &s = {});
  ~PhysBody();


  // set/get world transform (uses motion state if available)
  void setTm(const TMatrix &wtm);
  void getTm(TMatrix &wtm) const;
  // uses instant setWorldTransform()/getWorldTransform()/ (motion state is ignored)
  void setTmInstant(const TMatrix &wtm, bool upd_aabb = false);
  void getTmInstant(TMatrix &wtm) const;

  void disableGravity(bool disable);
  bool isGravityDisabled() { return body->getFlags() & BT_DISABLE_WORLD_GRAVITY; }

  void activateBody(bool active = true);
  bool isActive() const { return body->isActive(); }
  bool isInWorld() const { return body->isInWorld(); }
  inline void disableDeactivation();
  bool isDeactivationDisabled() const { return getActor()->getActivationState() == DISABLE_DEACTIVATION; };
  bool doesBodyWantDeactivation() const { return getActor()->getActivationState() == WANTS_DEACTIVATION; };


  // Set both tm and velocities - useful for animated bodies.
  void setTmWithDynamics(const TMatrix &wtm, real dt, bool activate = true);

  // Zero mass makes body static.
  void setMassMatrix(real mass, real ixx, real iyy, real izz);

  real getMass() const { return safeinv(body->getInvMass()); /*safeinv(0)==0*/ }
  void getMassMatrix(real &mass, real &ixx, real &iyy, real &izz);

  void getInvMassMatrix(real &mass, real &ixx, real &iyy, real &izz);


  void setCollision(const PhysCollision *collision, bool allow_fast_inaccurate_coll_tm = true);
  PhysCollision *getCollisionScaledCopy(const Point3 &scale = Point3(1, 1, 1));
  void patchCollisionScaledCopy(const Point3 &s, PhysBody *o);

  bool isConvexCollisionShape() const;
  void getShapeAabb(Point3 &out_bb_min, Point3 &out_bb_max) const;
  void setSphereShapeRad(float rad)
  {
    if (auto *shape = body->getCollisionShape())
      if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
        static_cast<btSphereShape *>(shape)->setUnscaledRadius(rad);
  }
  void setBoxShapeExtents(const Point3 &ext)
  {
    if (auto *shape = body->getCollisionShape())
      if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
        static_cast<btBoxShape *>(shape)->setImplicitShapeDimensions(to_btVector3(ext));
  }
  inline void setVertCapsuleShapeSize(float cap_rad, float cap_cyl_ht)
  {
    if (auto *shape = body->getCollisionShape())
      if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
      {
        auto cap_shape = static_cast<btCapsuleShape *>(shape);
        btVector3 scale = cap_shape->getLocalScaling();
        G_ASSERTF_RETURN(cap_shape->getUpAxis() == 1, , "shape=Capsule(rad=%d half_ht=%d up_axis=%d)", cap_shape->getRadius(),
          cap_shape->getHalfHeight(), cap_shape->getUpAxis());
        float rad_s = cap_rad * scale.x() / cap_shape->getRadius();
        float ht_s = cap_cyl_ht / 2 * scale.y() / cap_shape->getHalfHeight();
        shape->setLocalScaling(btVector3(rad_s, ht_s, rad_s));
      }
  }

  int getMaterialId() const { return safeMaterialId; }
  void setMaterialId(int id);

  // Should be called after setCollision() to set layer mask for all collision elements.
  void setGroupAndLayerMask(unsigned int group, unsigned int mask);
  unsigned int getInteractionLayer() const { return layerMask; }
  unsigned int getGroupMask() const { return groupMask; }

  void setVelocity(const Point3 &v, bool = true);
  Point3 getVelocity() const { return to_point3(body->getLinearVelocity()); }

  void setAngularVelocity(const Point3 &w, bool = true);
  Point3 getAngularVelocity() const { return to_point3(body->getAngularVelocity()); }

  Point3 getPointVelocity(const Point3 &world_pt)
  {
    return to_point3(body->getVelocityInLocalPoint(to_btVector3(world_pt) - body->getWorldTransform().getOrigin()));
  }

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

  // Bullet-specific:
  PhysWorld *getPhysWorld() const { return world; }
  btRigidBody *getActor() const { return body; }

  void setCcdParams(float rad, float threshold);

  void wakeUp();

  constexpr bool isValid() const { return true; }

  static PhysBody *from_bt_body(const btCollisionObject *b) { return b ? (PhysBody *)b->getUserPointer() : nullptr; }

protected:
  void ctor();
  static btCollisionShape *create_bt_collision_shape_no_margin(const PhysCollision *c, float *out_minCollisionSz, TMatrix *out_multTm,
    bool *out_multTmNonIdent, bool allow_fast_inaccurate_coll_tm);
  static btCollisionShape *create_bt_collision_shape(const PhysCollision *c, float *out_minCollisionSz = nullptr,
    TMatrix *out_multTm = nullptr, bool *out_multTmNonIdent = nullptr, bool allow_fast_inaccurate_coll_tm = false)
  {
    btCollisionShape *shape =
      create_bt_collision_shape_no_margin(c, out_minCollisionSz, out_multTm, out_multTmNonIdent, allow_fast_inaccurate_coll_tm);
    if (shape && c->getMargin() > 0.f)
      shape->setMargin(c->getMargin());
    return shape;
  }

  PhysWorld *world;
  btRigidBody *body;
  btMotionState *motionState;
  IPhysBodyCollisionCallback *ccb = nullptr;
  PhysBodyPreSolveCallback pcb = nullptr;

  TMatrix multTm = TMatrix::IDENT;

  int safeMaterialId = -1;
  unsigned short groupMask, layerMask;
  float minCollisionSz = 0.0f;
  bool multTmNonIdent = false;
  void *userPtr = nullptr;

  friend class PhysWorld;
};

class PhysRayCast
{
public:
  // Specify PhysWorld to make auto-updated PhysRayCast, or pass NULL to
  // create manually-updated PhysRayCast.
  PhysRayCast(const Point3 &pos, const Point3 &dir, real max_length, PhysWorld *world);

  void setFilterMask(unsigned fmask) { filterMask = fmask; }

  void setPos(const Point3 &p) { pos = to_btVector3(p); }
  const Point3 &getPos() const { return to_point3(pos); }

  void setDir(const Point3 &d)
  {
    dir = to_btVector3(d);
    if (is_equal_float(d.x, 0) && is_equal_float(d.y, 0) && is_equal_float(d.z, 0))
      invalid |= 2;
    else
      invalid &= ~2;

    float dirLenSq = lengthSq(d);
    if (dirLenSq != 0.0f && fabsf(dirLenSq - 1.0f) > 0.01f)
    {
#if DAGOR_DBGLEVEL > 0
      DAG_FATAL("Invalid dir " FMT_P3 ", len = %f", P3D(d), length(d));
#endif
      invalid |= 2;
    }
  }
  const Point3 &getDir() { return to_point3(dir); }

  void setMaxLength(real len)
  {
    maxLength = len;
    if (maxLength < 0.001)
      invalid |= 1;
    else
      invalid &= (~1);
  }
  real getMaxLength() const { return maxLength; }

  // Force update for auto-updated PhysRayCast.
  void forceUpdate()
  {
    if (ownWorld)
      forceUpdate(ownWorld);
  }

  // Update manually-updated PhysRayCast.
  void forceUpdate(PhysWorld *world);

  bool hasContact() const { return hasHit; }

  real getLength() const { return hit.m_hitFraction * maxLength; }
  Point3 getPoint() const { return to_point3(pos + dir * hit.m_hitFraction); }
  Point3 getNormal() const { return to_point3(hit.m_hitNormalLocal); }
  PhysBody *getBody() const { return PhysBody::from_bt_body(hit.m_collisionObject); }
  int getMaterial() { return 0; }

protected:
  PhysWorld *ownWorld;

  btVector3 pos;
  btVector3 dir;
  btScalar maxLength;
  btCollisionWorld::LocalRayResult hit;

  short invalid, hasHit;
  unsigned filterMask = ~0u;
};


class PhysWorld : public cpujobs::IJob
{
  class DActionsTab : public Tab<AfterPhysUpdateAction *>
  {
    friend class PhysWorld; // For direct access to `mCount`
  } delayedActions;
  OSSpinlock delayedActionsMutex;

public:
  PhysWorld(real default_static_friction, real default_dynamic_friction, real default_restitution, real default_softness);

  ~PhysWorld();

  // interaction layers
  void setInteractionLayers(unsigned int mask1, unsigned int mask2, bool make_contacts);

  int64_t controlTID = get_main_thread_id();
  bool isControledByCurThread() const { return controlTID == get_current_thread_id(); }
  void setControlThreadId(int64_t tid) { controlTID = bt_control_tid = tid; }
#if DAGOR_DBGLEVEL > 0
  int64_t invalidFetchSimResTID = -1;
  void setInvalidFetchSimResThread(int64_t tid) { invalidFetchSimResTID = tid ? tid : get_current_thread_id(); }
#else
  void setInvalidFetchSimResThread(int64_t) {}
#endif

  void startSim(real dt, bool wake_up_thread = true);
  bool fetchSimRes(bool wait, PhysBody *destroying_body = nullptr);
  void clear();

  virtual void doJob();

  bool canExecActionNow() const
  {
    return interlocked_acquire_load(done) && isControledByCurThread() &&
           // TODO: make methods in dag::Vector for this kind of access
           interlocked_relaxed_load(*(volatile int *)&delayedActions.mCount) == 0;
  }
  void addAfterPhysAction(AfterPhysUpdateAction *action);

public:
  void updateAabb(btRigidBody *obj);

  inline void simulate(real dt)
  {
    startSim(dt);
    fetchSimRes(true);
  }


  int getDefaultMaterialId() { return 0; }

  int createNewMaterialId();

  // parameters: for simple material system
  int createNewMaterialId(real static_friction, real dynamic_friction, real restitution, real softness);

  void setMaterialPairInteraction(int mat_id1, int mat_id2, bool collide);

  // add body to this world (may require fetchSimRes(true) called before adding body)
  void addBody(PhysBody *b, bool kinematic, bool update_aabb = true)
  {
    if (kinematic)
      getScene()->addRigidBody(b->getActor(), b->groupMask = btBroadphaseProxy::KinematicFilter,
        b->layerMask = btBroadphaseProxy::AllFilter & (~(btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::StaticFilter)));
    else
      getScene()->addRigidBody(b->getActor(), b->groupMask, b->layerMask);
    if (update_aabb)
      getScene()->updateSingleAabb(b->getActor());
  }

  // removed body from this world (may require fetchSimRes(true) called before removing body)
  void removeBody(PhysBody *b) { getScene()->removeRigidBody(b->getActor()); }

  PhysJoint *createRagdollBallJoint(PhysBody *body1, PhysBody *body2, const TMatrix &tm, const Point3 &min_limit,
    const Point3 &max_limit, real damping, real twist_damping, real stiffness, real sleep_threshold = 8.0);

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
    /*
    return regJoint(new PhysRevoluteJoint(body1, body2, pos, axis, min_ang, max_ang,
      min_rest, max_rest, spring, proj_type, proj_angle, proj_dist, damping, flags,
        sleep_threshold));
    */
  }

  PhysJoint *createSphericalJoint(PhysBody * /*body1*/, PhysBody * /*body2*/, const Point3 & /*pos*/, const Point3 & /*dir*/,
    const Point3 & /*axis*/, real /*min_ang*/, real /*max_ang*/, real /*min_rest*/, real /*max_rest*/, real /*sw_val*/,
    real /*sw_rest*/, real /*spring*/, real /*damp*/, real /*sw_spr*/, real /*sw_damp*/, real /*tw_spr*/, real /*tw_damp*/,
    short /*proj_type*/, real /*proj_dist*/, short /*flags*/, real /*sleep_threshold*/ = 8.f)
  {
    return NULL;
    /*
    return regJoint(new PhysSphericalJoint(body1, body2, pos, dir, axis, min_ang, max_ang,
      min_rest, max_rest, sw_val, sw_rest, spring, damp, sw_spr, sw_damp, tw_spr, tw_damp,
        proj_type, proj_dist, flags, sleep_threshold));
    */
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

  btDiscreteDynamicsWorld *getScene() { return &scene; }
  const btDiscreteDynamicsWorld *getScene() const { return &scene; }

  static float getLastSimDt() { return lastSimDt; }

  Point3 getGravity() const { return to_point3(scene.getGravity()); }
  void setGravity(const Point3 &gravity) { scene.setGravity(to_btVector3(gravity)); }

  struct Material
  {
    real friction;
    real restitution;
  };

  const Material *getMaterial(int id) const;

  void updateContactReports();

  void setMultithreadedUpdate(bool flag) { multithreadedUpdate = flag; }
  void setMaxSubSteps(int max_sub_steps);
  void setFixedTimeStep(float fts);

  template <typename C>
  struct ShapeQueryResultCB : public btCollisionWorld::ClosestConvexResultCallback
  {
    // bool C::needsCollision(const PhysBody *objB, void *userDataB) const
    const C &c;

    ShapeQueryResultCB(const C &_c, int filter_grp, int filter_mask) :
      c(_c), ClosestConvexResultCallback(btVector3(0.f, 0.f, 0.f), btVector3(0.f, 0.f, 0.f))
    {
      m_collisionFilterGroup = filter_grp;
      m_collisionFilterMask = filter_mask;
    }
    bool needsCollision(btBroadphaseProxy *proxy0) const override final
    {
      if (!proxy0 || !btCollisionWorld::ClosestConvexResultCallback::needsCollision(proxy0))
        return false;
      if (auto *physBody = PhysBody::from_bt_body((btCollisionObject *)proxy0->m_clientObject))
        return c.needsCollision(physBody, physBody->getUserData());
      return true;
    }
  };

  void shapeQuery(const PhysBody *body, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
    PhysShapeQueryResult &out, int filter_grp = btBroadphaseProxy::DefaultFilter, int filter_mask = btBroadphaseProxy::AllFilter);
  void shapeQuery(const PhysCollision &shape, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
    PhysShapeQueryResult &out, int filter_grp = btBroadphaseProxy::DefaultFilter, int filter_mask = btBroadphaseProxy::AllFilter);

  template <typename C>
  bool convexSweepTest(const PhysBody *shape, const Point3 &from, const Point3 &to, const C &needcoll_cb, PhysShapeQueryResult &out,
    int filter_grp = btBroadphaseProxy::DefaultFilter, int filter_mask = btBroadphaseProxy::AllFilter)
  {
    ShapeQueryResultCB<C> closestCb(needcoll_cb, filter_grp, filter_mask);
    return convexShapeSweepTest(static_cast<btConvexShape *>(shape->getActor()->getCollisionShape()), from, to, closestCb, out);
  }
  template <typename C>
  bool convexSweepTest(const PhysCollision &shape, const Point3 &from, const Point3 &to, const C &needcoll_cb,
    PhysShapeQueryResult &out, int filter_grp = btBroadphaseProxy::DefaultFilter, int filter_mask = btBroadphaseProxy::AllFilter)
  {
    ShapeQueryResultCB<C> closestCb(needcoll_cb, filter_grp, filter_mask);
    switch (shape.collType)
    {
      case PhysCollision::TYPE_SPHERE:
      {
        btSphereShape convex(static_cast<const PhysSphereCollision &>(shape).getRadius());
        return convexShapeSweepTest(&convex, from, to, closestCb, out);
      }
      case PhysCollision::TYPE_BOX:
      {
        btBoxShape convex(to_btVector3(static_cast<const PhysBoxCollision &>(shape).getSize() / 2));
        return convexShapeSweepTest(&convex, from, to, closestCb, out);
      }
      default: G_ASSERTF(0, "unsupported collType=%d in %s", shape.collType, __FUNCTION__);
    }
    return false;
  }

  template <typename C>
  struct ContactResultCB : public btCollisionWorld::ContactResultCallback
  {
    // typename C::contact_data_t
    // typename C::obj_user_data_t
    // float C::addSingleResult(const C::contact_data_t &cp, C::obj_user_data_t *objA, C::obj_user_data_t *objB, <ANY> *objInfo)
    // void  C::visualDebugForSingleResult(const PhysBody *bodyA, const PhysBody *bodyB, const C::contact_data_t &c)
    // bool  C::needsCollision(C::obj_user_data_t *objB, bool b_is_static)
    C &c;
    btCollisionObject *objA;

    ContactResultCB(C &_c, btCollisionObject *obj_a) : c(_c), objA(obj_a) {}
    btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0, int /*partId0*/, int /*index0*/,
      const btCollisionObjectWrapper *colObj1, int /*partId1*/, int /*index1*/) override final
    {
      bool isSwapped = colObj0->getCollisionObject() != objA;
      auto bodyA = (isSwapped ? colObj1 : colObj0)->getCollisionObject();
      auto bodyB = (isSwapped ? colObj0 : colObj1)->getCollisionObject();
      auto *pbA = PhysBody::from_bt_body(bodyA);
      auto *pbB = PhysBody::from_bt_body(bodyB);
      void *userPtrA = pbA ? pbA->getUserData() : nullptr;
      void *userPtrB = pbB ? pbB->getUserData() : nullptr;

      typename C::contact_data_t cdata;
      cdata.depth = cp.getDistance();
      cdata.wpos = to_point3(isSwapped ? cp.getPositionWorldOnB() : cp.getPositionWorldOnA());
      cdata.wposB = to_point3(isSwapped ? cp.getPositionWorldOnA() : cp.getPositionWorldOnB());
      cdata.wnormB = to_point3(cp.m_normalWorldOnB) * (isSwapped ? -1.f : 1.f);
      cdata.posA = to_point3(isSwapped ? cp.m_localPointB : cp.m_localPointA);
      cdata.posB = to_point3(isSwapped ? cp.m_localPointA : cp.m_localPointB);
      c.visualDebugForSingleResult(pbA, pbB, cdata);
      return c.addSingleResult(cdata, (typename C::obj_user_data_t *)userPtrA, (typename C::obj_user_data_t *)userPtrB, nullptr);
    }
    bool needsCollision(btBroadphaseProxy *proxy0) const override final
    {
      if (!proxy0 || !btCollisionWorld::ContactResultCallback::needsCollision(proxy0))
        return false;
      if (auto body = (btCollisionObject *)proxy0->m_clientObject)
      {
        auto *pb = PhysBody::from_bt_body(body);
        return c.needsCollision((typename C::obj_user_data_t *)(pb ? pb->getUserData() : nullptr), body->isStaticObject());
      }
      return true;
    }
  };

  template <typename C>
  void contactTest(const PhysBody *shape, C &contact_cb, int filter_grp = btBroadphaseProxy::DefaultFilter,
    int filter_mask = btBroadphaseProxy::AllFilter)
  {
    ContactResultCB<C> cb(contact_cb, shape->getActor());
    cb.m_collisionFilterGroup = filter_grp;
    cb.m_collisionFilterMask = filter_mask;
    getScene()->contactTest(shape->getActor(), cb);
  }
  template <typename C>
  void contactTestPair(const PhysBody *shape, const PhysBody *shapeB, C &contact_cb, int filter_grp = btBroadphaseProxy::DefaultFilter,
    int filter_mask = btBroadphaseProxy::AllFilter)
  {
    ContactResultCB<C> cb(contact_cb, shape->getActor());
    cb.m_collisionFilterGroup = filter_grp;
    cb.m_collisionFilterMask = filter_mask;
    getScene()->contactPairTest(shape->getActor(), shapeB->getActor(), cb);
  }

  template <typename C, typename UDT = typename C::obj_user_data_t>
  bool needsCollision(const PhysBody *bodyB, C &c, int filter_grp = btBroadphaseProxy::DefaultFilter,
    int filter_mask = btBroadphaseProxy::AllFilter)
  {
    if (!bodyB || !(bodyB->groupMask & filter_mask) || !(filter_grp & bodyB->layerMask))
      return false;
    return c.needsCollision((UDT *)bodyB->getUserData(), bodyB->getActor()->isStaticObject());
  }

protected:
  btDiscreteDynamicsWorld scene;
  static float lastSimDt;

  float fixedTimeStep = 1.f / 60.f;
  bool multithreadedUpdate = true;
  uint8_t maxSubSteps = 3;

  btMaterial *defaultMaterial;

  // materials
  Tab<Material> materials;

  PhysJoint *regJoint(PhysJoint *j);

  static btRigidBody *create_bt_rigidbody(const btRigidBody::btRigidBodyConstructionInfo &ctor_info);
  static void destroy_bt_collobject(btRigidBody *obj);

  bool convexShapeSweepTest(btConvexShape *shape, const Point3 &from, const Point3 &to,
    btCollisionWorld::ClosestConvexResultCallback &closestCb, PhysShapeQueryResult &out)
  {
    TMatrix fromTm = TMatrix::IDENT;
    fromTm.setcol(3, from);
    TMatrix toTm = TMatrix::IDENT;
    toTm.setcol(3, to);

    closestCb.m_closestHitFraction = out.t;
    getScene()->convexSweepTest(shape, to_btTransform(fromTm), to_btTransform(toTm), closestCb);

    if (closestCb.m_closestHitFraction < out.t)
    {
      out.t = closestCb.m_closestHitFraction;
      out.res = to_point3(closestCb.m_hitPointWorld);
      out.norm = to_point3(closestCb.m_hitNormalWorld);
      const btRigidBody *hitBody = closestCb.m_hitCollisionObject ? btRigidBody::upcast(closestCb.m_hitCollisionObject) : nullptr;
      out.vel = hitBody ? to_point3(hitBody->getLinearVelocity()) : Point3(0.f, 0.f, 0.f);
    }
    return out.t < 1.f;
  }

  friend class PhysBody;
  friend class PhysCollision;

  friend void init_bullet_physics_engine(bool);
  friend void close_bullet_physics_engine();
};

template <>
struct PhysWorld::ShapeQueryResultCB<void *> : public btCollisionWorld::ClosestConvexResultCallback
{
  ShapeQueryResultCB(void *const &, int filter_grp, int filter_mask) :
    ClosestConvexResultCallback(btVector3(0.f, 0.f, 0.f), btVector3(0.f, 0.f, 0.f))
  {
    m_collisionFilterGroup = filter_grp;
    m_collisionFilterMask = filter_mask;
  }
};

inline void PhysBody::disableDeactivation()
{
  exec_or_add_after_phys_action<SetActivationStateAfterPhysUpdateAction>(*world, getActor(), DISABLE_DEACTIVATION);
}

int phys_body_get_hmap_step(PhysBody *b);
int phys_body_set_hmap_step(PhysBody *b, int step);

void free_all_bt_collobjects_memory(); // All objects expected to be destroyed before this call
