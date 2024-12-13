// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <util/dag_threadPool.h>
#include <generic/dag_tab.h>
#include <phys/dag_physics.h>
#include <phys/dag_physStat.h>
#include <perfMon/dag_statDrv.h>
#include "bulletPrivate.h"
#include <math/dag_mathUtils.h>
#include <EASTL/tuple.h>
#include <LinearMath/btConvexHullComputer.h>
#include <util/dag_convar.h>
#include "btDagorHeightfieldTerrainShape.h"
#include <heightmap/heightmapHandler.h>
#include <scene/dag_physMat.h>
#include <LinearMath/btScalar.h>
#if BT_BULLET_VERSION >= 283
#include <LinearMath/btCpuFeatureUtility.h>
#define IF_BT_CPU_FEATURE(X, STR_YES, STR_NO) (btCpuFeatureUtility::getCpuFeatures() & btCpuFeatureUtility::X) ? STR_YES : STR_NO
#else
#define IF_BT_CPU_FEATURE(X, STR_YES, STR_NO) STR_NO
#endif

#include <debug/dag_debug.h>

#define PROFILE_PHYS 0
#if PROFILE_PHYS
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_files.h>
#endif

#if DAGOR_DBGLEVEL > 0
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>
#include <memory/dag_framemem.h>
#endif

#if DAGOR_DBGLEVEL > 0
static CONSOLE_INT_VAL("phys", max_sub_steps, 3, 0, 10);
static CONSOLE_INT_VAL("phys", fixed_time_step_hz, 60, 1, 100);
#endif

static bool is_bullet_inited = false;
Tab<PhysBody *> bullet::staticBodies(midmem);
Tab<bullet::StaticCollision *> bullet::swcRegistry(inimem);

float PhysWorld::lastSimDt = 0;

static btDefaultCollisionConfiguration *collisionConfiguration = NULL;
static btCollisionDispatcher *dispatcher = NULL;
static btBroadphaseInterface *overlappingPairCache = NULL;
static btSequentialImpulseConstraintSolver *solver = NULL;

int64_t bt_control_tid = -1;
bool bt_is_control_thread() { return bt_control_tid == get_current_thread_id(); }

static void bt_call_presolve(PhysBody *body, btManifoldPoint &cp, void *other, bool inv_dir)
{
  IPhysContactData *&contactData = reinterpret_cast<IPhysContactData *&>(cp.m_userPersistentData);
  Point3 norm = to_point3(inv_dir ? -cp.m_normalWorldOnB : cp.m_normalWorldOnB);
  body->getPreSolveCallback()(body, other, to_point3(cp.m_positionWorldOnB), norm, contactData);
  cp.m_normalWorldOnB = to_btVector3(inv_dir ? -norm : norm);
}

static bool bt_contact_processed_callback(btManifoldPoint &cp, void *body0, void *body1)
{
  if (!body0 || !body1)
  {
    // users can call manifold.refreshContactPoints without bodies being set, handle this.
    return true;
  }
  if (auto *b0 = PhysBody::from_bt_body(static_cast<btCollisionObject *>(body0)))
  {
    if (b0->getPreSolveCallback())
      bt_call_presolve(b0, cp, body1, false);
  }
  if (auto *b1 = PhysBody::from_bt_body(static_cast<btCollisionObject *>(body1)))
  {
    if (b1->getPreSolveCallback())
    {
      // contact point is from body0 to body1, but we're reporting to body1
      // here, inverse signs of collision data.
      bt_call_presolve(b1, cp, body0, true);
    }
  }
  return true;
}

static bool bt_contact_destroyed_callback(void *userData)
{
  IPhysContactData *contactData = static_cast<IPhysContactData *>(userData);
  delete contactData;
  return true;
}

static inline const char *getConfigurationString()
{
  return
#ifdef BT_USE_DOUBLE_PRECISION
    "Double"
#else
    "Single"
#endif
    " precision"
#if _TARGET_SIMD_NEON
    " ARM64"
#elif _TARGET_64BIT
    " x64"
#else
    " x86"
#endif
#if _TARGET_64BIT
    " 64-bit"
#else
    " 32-bit"
#endif
    " with instructions:"
#ifdef BT_USE_NEON
    " NEON"
#endif
#ifdef BT_USE_SSE
    " SSE2"
#endif
#ifdef BT_ALLOW_SSE4
    " SSE4"
#endif
#if BT_DEBUG
    " (Debug)"
#endif
    ;
}

void init_bullet_physics_engine(bool)
{
  if (::is_bullet_inited)
    return;

  ::is_bullet_inited = true;
  bt_control_tid = get_main_thread_id();

  /// collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
  btDefaultCollisionConstructionInfo collCfgInfo;
#if DAGOR_PREFER_HEAP_ALLOCATION
  collCfgInfo.m_defaultMaxPersistentManifoldPoolSize = 1;
  collCfgInfo.m_defaultMaxCollisionAlgorithmPoolSize = 1;
#endif
  collisionConfiguration = new btDefaultCollisionConfiguration(collCfgInfo);

  /// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
  dispatcher = new btCollisionDispatcher(collisionConfiguration);

  /// btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
  overlappingPairCache = new btDbvtBroadphase();

  /// the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
  solver = new btSequentialImpulseConstraintSolver;

  gContactProcessedCallback = &bt_contact_processed_callback;
  gContactDestroyedCallback = &bt_contact_destroyed_callback;

  static bool printed_once = false;
  if (!printed_once)
  {
    debug("using phys engine: Bullet v%d.%d.%d [%s%s%s%s]", btGetVersion() / 100, (btGetVersion() / 10) % 10, btGetVersion() % 10,
      getConfigurationString(), IF_BT_CPU_FEATURE(CPU_FEATURE_SSE4_1, " SSE4.1", ""), IF_BT_CPU_FEATURE(CPU_FEATURE_FMA3, " FMA3", ""),
      IF_BT_CPU_FEATURE(CPU_FEATURE_NEON_HPFP, " NEON_HPFP", ""));
    printed_once = true;
  }
}


void close_bullet_physics_engine()
{
  if (!::is_bullet_inited)
    return;
  ::is_bullet_inited = false;

  for (int i = bullet::swcRegistry.size() - 1; i >= 0; i--)
    bullet::swcRegistry[i]->pw->unloadSceneCollision(bullet::swcRegistry[i]);
  G_ASSERT(!bullet::swcRegistry.size());

  // delete solver
  del_it(solver);

  // delete broadphase
  del_it(overlappingPairCache);

  // delete dispatcher
  del_it(dispatcher);

  del_it(collisionConfiguration);

  free_all_bt_collobjects_memory();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

struct AddBodyToWorldAction final : public AfterPhysUpdateAction
{
  uint16_t grp, mask;

  AddBodyToWorldAction(PhysBody *in_body, unsigned int in_grp, unsigned int in_mask) :
    AfterPhysUpdateAction(in_body->getActor()), grp(in_grp), mask(in_mask)
  {}
  void doAction(PhysWorld &pw, bool) override { pw.getScene()->addRigidBody(body, grp, mask); }
};

PhysBody::PhysBody(PhysWorld *w, float mass, const PhysCollision *coll, const TMatrix &tm, const PhysBodyCreationData &s) : world(w)
{
  btCollisionShape *shape =
    PhysBody::create_bt_collision_shape(coll, &minCollisionSz, &multTm, &multTmNonIdent, s.allowFastInaccurateCollTm);
  btTransform btWtm = to_btTransform(multTmNonIdent ? tm * multTm : tm);
  motionState = s.useMotionState ? new btDefaultMotionState(btWtm) : nullptr;
  btVector3 momentOfInertia = to_btVector3(s.momentOfInertia);
  if (s.autoInertia && shape)
    shape->calculateLocalInertia(mass, momentOfInertia);
  btRigidBody::btRigidBodyConstructionInfo cinfo(mass, motionState, shape, momentOfInertia);
  if (const PhysWorld::Material *material = world->getMaterial(s.materialId))
  {
    cinfo.m_friction = material->friction;
    cinfo.m_restitution = material->restitution;
    cinfo.m_rollingFriction = 0.1f;
    safeMaterialId = s.materialId;
  }
  else
    safeMaterialId = 0;
  if (s.friction >= 0)
    cinfo.m_friction = s.friction;
  if (s.restitution >= 0)
    cinfo.m_restitution = s.restitution;
  if (s.rollingFriction >= 0)
    cinfo.m_rollingFriction = s.rollingFriction;
  cinfo.m_startWorldTransform = btWtm;
  body = PhysWorld::create_bt_rigidbody(cinfo);

  userPtr = s.userPtr;
  body->setUserPointer(this);
  body->setDamping(s.linearDamping, s.angularDamping);
  if (!mass)
    body->setCollisionFlags(s.kinematic ? btCollisionObject::CF_KINEMATIC_OBJECT : btCollisionObject::CF_STATIC_OBJECT);

  if (s.autoMask)
  {
    bool isDynamic = !(body->isStaticObject() || body->isKinematicObject());
    groupMask = isDynamic ? short(btBroadphaseProxy::DefaultFilter) : short(btBroadphaseProxy::StaticFilter);
    layerMask =
      isDynamic ? short(btBroadphaseProxy::AllFilter) : short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter);
  }
  else
    groupMask = s.group, layerMask = s.mask;
  if (s.addToWorld)
  {
    if (world->isControledByCurThread())
    {
      exec_or_add_after_phys_action<AddBodyToWorldAction>(*world, this, groupMask, layerMask);
      if (shape)
        world->updateAabb(body);
    }
    else
    {
      world->fetchSimRes(true);
      world->getScene()->addRigidBody(body, groupMask, layerMask);
      if (shape)
        world->getScene()->updateSingleAabb(body);
    }
  }
}


#if DAGOR_DBGLEVEL > 0
template <typename ShapeT, int ShapeE, typename F>
static void draw_shape_debug_case(const btCollisionShape *shape, F &&f)
{
  if (shape->getShapeType() == ShapeE)
    f(*static_cast<const ShapeT *>(shape));
}

static void draw_shape_debug(const TMatrix &tm, const btCollisionShape *s, const unsigned color, bool rand_children_colors)
{
  draw_shape_debug_case<btBoxShape, BOX_SHAPE_PROXYTYPE>(s, [&](const btBoxShape &shape) {
    const Point3 halfExtent = to_point3(shape.getHalfExtentsWithMargin());
    draw_cached_debug_box(BBox3(-halfExtent, halfExtent), color);
  });
  draw_shape_debug_case<btSphereShape, SPHERE_SHAPE_PROXYTYPE>(s,
    [&](const btSphereShape &shape) { draw_cached_debug_sphere(tm.getcol(3), shape.getRadius(), color); });
  draw_shape_debug_case<btCapsuleShape, CAPSULE_SHAPE_PROXYTYPE>(s, [&](const btCapsuleShape &shape) {
    Point3 halfHeight = Point3::ZERO;
    halfHeight[shape.getUpAxis()] = shape.getHalfHeight();
    draw_cached_debug_capsule(Capsule(-halfHeight, halfHeight, shape.getRadius() + shape.getMargin()), color, tm);
  });
  draw_shape_debug_case<btCylinderShape, CYLINDER_SHAPE_PROXYTYPE>(s, [&](const btCylinderShape &shape) {
    // TODO: this assumes it is vertical
    draw_cached_debug_cylinder(tm, shape.getRadius(), shape.getImplicitShapeDimensions()[shape.getUpAxis()], color);
  });
  draw_shape_debug_case<btBvhTriangleMeshShape, TRIANGLE_MESH_SHAPE_PROXYTYPE>(s, [&](const btBvhTriangleMeshShape &shape) {
    const auto *meshes = static_cast<const btTriangleIndexVertexArray *>(shape.getMeshInterface());

    for (int meshI = 0; meshI < meshes->getIndexedMeshArray().size(); meshI++)
    {
      const auto &mesh = meshes->getIndexedMeshArray()[meshI];
      if (mesh.m_vertexType != PHY_FLOAT)
        continue;
      if (mesh.m_indexType != PHY_INTEGER)
        continue;
      const Point3 *vertices = reinterpret_cast<const Point3 *>(mesh.m_vertexBase);
      const int *indices = reinterpret_cast<const int *>(mesh.m_triangleIndexBase);
      const Point3 scale = to_point3(shape.getLocalScaling());
      for (int i = 0; i < mesh.m_numTriangles; i++)
      {
        Point3 p[3];
        p[0] = tm * hadamard_product(scale, vertices[indices[i * 3 + 0]]);
        p[1] = tm * hadamard_product(scale, vertices[indices[i * 3 + 1]]);
        p[2] = tm * hadamard_product(scale, vertices[indices[i * 3 + 2]]);
        draw_cached_debug_line(p[0], p[1], color);
        draw_cached_debug_line(p[1], p[2], color);
        draw_cached_debug_line(p[2], p[0], color);
        draw_cached_debug_solid_triangle(p, (color & 0xffffffu) | 0x66000000u);
      }
    }
  });

  draw_shape_debug_case<btConvexHullShape, CONVEX_HULL_SHAPE_PROXYTYPE>(s, [&](const btConvexHullShape &shape) {
    btConvexHullComputer hullComp;
    hullComp.compute((const float *)shape.getUnscaledPoints(), sizeof(btVector3), shape.getNumVertices(), 0.f, 0.f);
    for (int i = 0; i < hullComp.faces.size(); i++)
    {
      const int face = hullComp.faces[i];
      const btConvexHullComputer::Edge *firstEdge = &hullComp.edges[face];
      const btConvexHullComputer::Edge *edge = firstEdge;

      int numVertices = 0;
      Point3 vertices[128];
      do
      {
        vertices[numVertices++] = tm * to_point3(hullComp.vertices[edge->getSourceVertex()]);
        edge = edge->getNextEdgeOfFace();
      } while (edge != firstEdge);

      for (int i = 0; i < numVertices; i++)
      {
        draw_cached_debug_line(vertices[i], vertices[(i + 1) % numVertices], color);
        if (i > 0)
        {
          Point3 p[3];
          p[0] = vertices[0];
          p[1] = vertices[i];
          p[2] = vertices[(i + 1) % numVertices];
          draw_cached_debug_solid_triangle(p, (color & 0xffffffu) | 0x66000000u);
        }
      }
    }
  });

  draw_shape_debug_case<btCompoundShape, COMPOUND_SHAPE_PROXYTYPE>(s, [&](const btCompoundShape &shape) {
    for (int i = 0; i < shape.getNumChildShapes(); i++)
    {
      TMatrix localTm = to_tmatrix(shape.getChildTransform(i));
      draw_shape_debug(tm * localTm, shape.getChildShape(i),
        rand_children_colors ? (color & 0xff000000u) | ((color & 0xffffffu) + (i + 1) * 0x7890123u) : color, rand_children_colors);
    }
  });

  // STATIC_PLANE_PROXYTYPE
  // TYPE_HEIGHTFIELD
  // TYPE_NATIVE_SHAPE
}
#endif

void PhysBody::drawDebugCollision(unsigned color, bool rand_children_colors) const
{
  G_UNUSED(color);
  G_UNUSED(rand_children_colors);

#if DAGOR_DBGLEVEL > 0
  TMatrix tm;
  getTmInstant(tm);
  draw_shape_debug(tm, getActor()->getCollisionShape(), color, rand_children_colors);
#endif
}

struct UpdateSingleAabbAction final : public AfterPhysUpdateAction
{
  UpdateSingleAabbAction(btRigidBody *coll_obj, PhysWorld *in_world) : AfterPhysUpdateAction(coll_obj) {}

  void doAction(PhysWorld &pw, bool) override { pw.getScene()->updateSingleAabb(body); }
};

struct SetTmAction final : public AfterPhysUpdateAction
{
  btTransform tm;
  bool upMs;
  bool upAabb;

  SetTmAction(btRigidBody *b, const btTransform &tm_, bool ums = true, bool ua = true) :
    AfterPhysUpdateAction(b), tm(tm_), upMs(ums), upAabb(ua)
  {}

  void doAction(PhysWorld &pw, bool) override
  {
    if (btMotionState *motionState = upMs ? body->getMotionState() : nullptr)
    {
      motionState->setWorldTransform(tm);
      body->setInterpolationWorldTransform(tm);
    }
    body->setWorldTransform(tm);
    if (upAabb && body->getRootCollisionShape())
      pw.getScene()->updateSingleAabb(body);
  }
};

void PhysBody::setTm(const TMatrix &wtm)
{
  btTransform finalTm = to_btTransform(multTmNonIdent ? (wtm * multTm) : wtm);
  if (body->isInWorld())
    exec_or_add_after_phys_action<SetTmAction>(*world, body, finalTm);
  else
    SetTmAction(body, finalTm, /*upMs*/ true, /*upAabb*/ false).doAction(*world, true);
}

void PhysBody::getTm(TMatrix &wtm) const
{
  if (motionState)
    wtm = to_tmatrix(static_cast<btDefaultMotionState *>(motionState)->m_graphicsWorldTrans);
  else
    wtm = to_tmatrix(body->getWorldTransform());
  if (multTmNonIdent)
    wtm *= inverse(multTm);
}

void PhysBody::setTmInstant(const TMatrix &wtm, bool upd_aabb)
{
  btTransform finalTm = to_btTransform(multTmNonIdent ? (wtm * multTm) : wtm);
  if (body->isInWorld())
    exec_or_add_after_phys_action<SetTmAction>(*world, body, finalTm, /*upMs*/ false, upd_aabb);
  else
    body->setWorldTransform(finalTm);
}

void PhysBody::getTmInstant(TMatrix &wtm) const
{
  wtm = to_tmatrix(body->getWorldTransform());
  if (multTmNonIdent)
    wtm *= inverse(multTm);
}

static void free_collision_shape(btCollisionShape *shape, bool root = true)
{
  if (btCompoundShape *compShape = shape->isCompound() ? static_cast<btCompoundShape *>(shape) : NULL)
    for (int i = 0; i < compShape->getNumChildShapes(); ++i)
      free_collision_shape(compShape->getChildShape(i), false);
  else if (!root && shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
    delete static_cast<btTriangleIndexVertexArray *>(static_cast<btBvhTriangleMeshShape *>(shape)->getMeshInterface());
  if (!root) // root shape is destroyed within destroy_bt_collobject()
    delete shape;
}

PhysBody::~PhysBody()
{
  world->fetchSimRes(true, this);
  G_ASSERT(body);
  if (btMotionState *ms = body->getMotionState())
    delete ms;
  if (btCollisionShape *shape = body->getRootCollisionShape())
    free_collision_shape(shape);
  world->scene.removeCollisionObject(body);
  PhysWorld::destroy_bt_collobject(body);
}

struct SetInteractionLayerAction final : public AfterPhysUpdateAction
{
  uint16_t group, mask;

  SetInteractionLayerAction(PhysBody *in_body, unsigned int gr, unsigned int msk) :
    AfterPhysUpdateAction(in_body->getActor()), group(gr), mask(msk)
  {}

  void doAction(PhysWorld &pw, bool) override
  {
    btBroadphaseProxy *proxy = body->getBroadphaseProxy();
    if (proxy != nullptr)
    {
      proxy->m_collisionFilterGroup = group;
      proxy->m_collisionFilterMask = mask;
      overlappingPairCache->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(proxy, dispatcher);
    }
    else
    {
      pw.getScene()->removeCollisionObject(body);
      pw.getScene()->addRigidBody(body, group, mask);
    }
  }
};

void PhysBody::setGroupAndLayerMask(unsigned int group, unsigned int mask)
{
  groupMask = group;
  layerMask = mask;
  if (body->isInWorld())
    exec_or_add_after_phys_action<SetInteractionLayerAction>(*world, this, groupMask, layerMask);
}

struct ReAddToWorldAction final : public AfterPhysUpdateAction
{
  uint16_t grp, mask;

  ReAddToWorldAction(PhysBody *in_body, unsigned int in_grp, unsigned int in_mask) :
    AfterPhysUpdateAction(in_body->getActor()), grp(in_grp), mask(in_mask)
  {}

  void doAction(PhysWorld &pw, bool) override
  {
    pw.getScene()->removeRigidBody(body);
    pw.getScene()->addRigidBody(body, grp, mask);
  }
};

struct ActivateBodyAction final : public AfterPhysUpdateAction
{
  bool forceActivation;

  ActivateBodyAction(btRigidBody *body_, bool force_a = false) : AfterPhysUpdateAction(body_), forceActivation(force_a) {}

  void doAction(PhysWorld &, bool) override { body->activate(forceActivation); }
};

void PhysBody::activateBody(bool active)
{
  if (active)
    exec_or_add_after_phys_action<ActivateBodyAction>(*world, body);
  else
    exec_or_add_after_phys_action<SetActivationStateAfterPhysUpdateAction>(*world, body, WANTS_DEACTIVATION);
}

void PhysBody::setMassMatrix(real mass, real ixx, real iyy, real izz)
{
  // Bullet doesn't like big ratios of inertia tensor
  // so we need to patch it, if we don't want to modify it's code
  const float inertiaTensorRatio = 15.f;
  Point3 inertia(ixx, iyy, izz);
  for (int i = 0; i < 3; ++i)
    for (int j = 1; j < 3; ++j)
      if (safediv(inertia[i], inertia[(i + j) % 3]) > inertiaTensorRatio)
        inertia[(i + j) % 3] = inertia[i] / inertiaTensorRatio;
  body->setMassProps(mass, to_btVector3(inertia));
  body->updateInertiaTensor();
  if (mass > 0.f)
    exec_or_add_after_phys_action<ActivateBodyAction>(*world, body);
  else // re-add it to world but as static body
    exec_or_add_after_phys_action<ReAddToWorldAction>(*world, this, groupMask, layerMask);
}


void PhysBody::getMassMatrix(real &mass, real &ixx, real &iyy, real &izz)
{
  float im = body->getInvMass();
  btVector3 ij = body->getInvInertiaDiagLocal();

  if (im)
  {
    mass = 1.0f / im;
    ixx = float_nonzero(ij.x()) ? 1.0f / ij.x() : 0.0f;
    iyy = float_nonzero(ij.y()) ? 1.0f / ij.y() : 0.0f;
    izz = float_nonzero(ij.z()) ? 1.0f / ij.z() : 0.0f;
  }
  else
  {
    mass = 0;
    ixx = 0;
    iyy = 0;
    izz = 0;
  }
}


void PhysBody::getInvMassMatrix(real &imass, real &ixx, real &iyy, real &izz)
{
  float im = body->getInvMass();
  btVector3 ij = body->getInvInertiaDiagLocal();

  if (im)
  {
    imass = im;
    ixx = ij.x();
    iyy = ij.y();
    izz = ij.z();
  }
  else
  {
    imass = 0;
    ixx = 0;
    iyy = 0;
    izz = 0;
  }
}

struct SetCollisionAction : public AfterPhysUpdateAction
{
  btCollisionShape *collider;
  uint16_t gmask, ilayer;

  SetCollisionAction(PhysBody *in_body, btCollisionShape *in_collider) :
    AfterPhysUpdateAction(in_body->getActor()),
    collider(in_collider),
    gmask(in_body->getGroupMask()),
    ilayer(in_body->getInteractionLayer())
  {}

  void doAction(PhysWorld &pw, bool) override
  {
    pw.getScene()->removeCollisionObject(body);
    if (!collider)
      delete body->getCollisionShape();
    body->setCollisionShape(collider);
    pw.getScene()->addRigidBody(body, gmask, ilayer);
  }
};

struct BulletPhysNativeShape : public PhysCollision
{
  const btCollisionShape *shape;
  btVector3 localScale;

  BulletPhysNativeShape(const btCollisionShape *s, btVector3 ls) : PhysCollision(TYPE_NATIVE_SHAPE), shape(s), localScale(ls)
  {
    setMargin(s->getMargin());
  }
};
void PhysCollision::clearNativeShapeData(PhysCollision &c) { static_cast<BulletPhysNativeShape &>(c).shape = nullptr; }

btCollisionShape *PhysBody::create_bt_collision_shape_no_margin(const PhysCollision *c, float *out_minCollisionSz, TMatrix *out_multTm,
  bool *out_multTmNonIdent, bool allow_fast_inaccurate_coll_tm)
{
  if (!c || c->collType == c->TYPE_EMPTY ||
      (c->collType == c->TYPE_COMPOUND && !static_cast<const PhysCompoundCollision *>(c)->coll.size()))
    return nullptr;

  switch (c->collType)
  {
    case PhysCollision::TYPE_SPHERE:
    {
      auto sphereColl = static_cast<const PhysSphereCollision *>(c);
      if (out_minCollisionSz)
        *out_minCollisionSz = sphereColl->radius;
      return new btSphereShape(sphereColl->radius);
    }
    break;

    case PhysCollision::TYPE_BOX:
    {
      auto boxColl = static_cast<const PhysBoxCollision *>(c);
      if (out_minCollisionSz)
        *out_minCollisionSz = min(boxColl->size.x, min(boxColl->size.y, boxColl->size.z));
      return new btBoxShape(to_btVector3(boxColl->size / 2));
    }
    break;

    case PhysCollision::TYPE_CAPSULE:
    {
      auto capsuleColl = static_cast<const PhysCapsuleCollision *>(c);
      if (out_minCollisionSz)
        *out_minCollisionSz = min(capsuleColl->radius, capsuleColl->height);
      switch (capsuleColl->dirIdx)
      {
        case 0: return new btCapsuleShapeX(capsuleColl->radius, capsuleColl->height);
        case 1: return new btCapsuleShape(capsuleColl->radius, capsuleColl->height);
        case 2: return new btCapsuleShapeZ(capsuleColl->radius, capsuleColl->height);
        default: G_ASSERTF_RETURN(false, nullptr, "capsuleColl->dirIdx=%d, must be [0..2]", capsuleColl->dirIdx);
      }
    }
    break;

    case PhysCollision::TYPE_CYLINDER:
    {
      auto cylColl = static_cast<const PhysCylinderCollision *>(c);
      if (out_minCollisionSz)
        *out_minCollisionSz = min(cylColl->radius, cylColl->height);
      switch (cylColl->dirIdx)
      {
        case 0: return new btCylinderShapeX(btVector3(cylColl->height * 0.5f, cylColl->radius, cylColl->radius));
        case 1: return new btCylinderShape(btVector3(cylColl->radius, cylColl->height * 0.5f, cylColl->radius));
        case 2: return new btCylinderShapeZ(btVector3(cylColl->radius, cylColl->radius, cylColl->height * 0.5f));
        default: G_ASSERTF_RETURN(false, nullptr, "cylColl->dirIdx=%d, must be [0..2]", cylColl->dirIdx);
      }
    }
    break;

    case PhysCollision::TYPE_PLANE:
    {
      auto planeColl = static_cast<const PhysPlaneCollision *>(c);
      return new btStaticPlaneShape(to_btVector3(planeColl->plane.n), planeColl->plane.d);
    }
    break;

    case PhysCollision::TYPE_HEIGHTFIELD:
    {
      auto hmapColl = static_cast<const PhysHeightfieldCollision *>(c);
      auto hmap = hmapColl->hmap;
      return new btDagorHeightfieldTerrainShape(hmap->getCompressedData(), hmap->getHeightmapCellSize() * hmapColl->scale,
        hmap->getHeightMin() * hmapColl->scale, hmap->getHeightScaleRaw() * hmapColl->scale, hmapColl->ofs, hmapColl->box,
        hmapColl->holes, hmapColl->hmStep);
    }
    break;

    case PhysCollision::TYPE_TRIMESH:
    {
      auto meshColl = static_cast<const PhysTriMeshCollision *>(c);
      btIndexedMesh part;

      part.m_vertexBase = (const unsigned char *)meshColl->vdata;
      part.m_vertexStride = meshColl->vstride;
      part.m_numVertices = meshColl->vnum;
      part.m_triangleIndexBase = (const unsigned char *)meshColl->idata;
      part.m_triangleIndexStride = 3 * meshColl->istride;
      part.m_numTriangles = meshColl->inum / 3;
      part.m_vertexType = meshColl->vtypeShort ? PHY_SHORT : PHY_FLOAT;

      btTriangleIndexVertexArray *meshInterface = new btTriangleIndexVertexArray;
      meshInterface->addIndexedMesh(part, (meshColl->istride == 2) ? PHY_SHORT : PHY_INTEGER);
      if (lengthSq(meshColl->scale - Point3(1, 1, 1)) > 1e-12)
        meshInterface->setScaling(to_btVector3(meshColl->scale));

      return new btBvhTriangleMeshShape(meshInterface, true, /*buildBvh*/ false); // bvh is built within create_bt_rigidbody()
    }
    break;

    case PhysCollision::TYPE_CONVEXHULL:
    {
      auto convexColl = static_cast<const PhysConvexHullCollision *>(c);
      if (convexColl->build)
      {
        btConvexHullComputer hullComp;
        hullComp.compute(convexColl->vdata, convexColl->vstride, convexColl->vnum, 0.f, 0.f);
        G_ASSERTF(hullComp.vertices.size(), "Cannot make hull vertices from %p,%d (stride=%d)", convexColl->vdata, convexColl->vnum,
          convexColl->vstride);
        return new btConvexHullShape(&hullComp.vertices.at(0).getX(), hullComp.vertices.size(), sizeof(btVector3));
      }
      return new btConvexHullShape(convexColl->vdata, convexColl->vnum, convexColl->vstride);
    }
    break;

    case PhysCollision::TYPE_COMPOUND:
    {
      auto compColl = static_cast<const PhysCompoundCollision *>(c);
      const bool isSingleNode = compColl->coll.size() == 1;
      const int collType = compColl->coll[0].c->collType;
      const TMatrix &tm = compColl->coll[0].m;
      const bool isIdent = memcmp(&tm, &TMatrix::IDENT, sizeof(Point3) * 3) == 0; //-V1014 //-V1086
      const bool hasTranslation = tm.getcol(3).lengthSq() > 1e-6;
      const bool isValidForSingleNode = collType == c->TYPE_SPHERE     ? !hasTranslation
                                        : collType == c->TYPE_BOX      ? isIdent
                                        : collType == c->TYPE_CAPSULE  ? !hasTranslation
                                        : collType == c->TYPE_CYLINDER ? !hasTranslation
                                                                       : true;
      if (isSingleNode && (allow_fast_inaccurate_coll_tm || isValidForSingleNode) && out_multTm && out_multTmNonIdent)
      {
        *out_multTm = tm;
        *out_multTmNonIdent = !isIdent || hasTranslation;
        btCollisionShape *collider = PhysBody::create_bt_collision_shape(compColl->coll[0].c, out_minCollisionSz);
        G_ASSERTF_RETURN(collider, nullptr, "failed to create collision type=%d", collType);
        return collider;
      }

      btCompoundShape *collider = new btCompoundShape;
      for (int n = 0; n < compColl->coll.size(); n++)
      {
        const btTransform tm = to_btTransform(compColl->coll[n].m);

        switch (compColl->coll[n].c->collType)
        {
          case PhysCollision::TYPE_SPHERE:
          case PhysCollision::TYPE_BOX:
          case PhysCollision::TYPE_CAPSULE:
          case PhysCollision::TYPE_CYLINDER:
          case PhysCollision::TYPE_TRIMESH:
          case PhysCollision::TYPE_CONVEXHULL:
          case PhysCollision::TYPE_NATIVE_SHAPE:
            if (auto c = PhysBody::create_bt_collision_shape(compColl->coll[n].c))
              collider->addChildShape(tm, c);
            break;

          default: G_ASSERTF(false, "cannot create bt subshape[%d] collision type=%d for compound", n, compColl->coll[n].c->collType);
        }
      }
      return collider;
    }
    break;

    case PhysCollision::TYPE_NATIVE_SHAPE:
    {
      auto ns = static_cast<const BulletPhysNativeShape *>(c);
      switch (ns->shape->getShapeType())
      {
        case CONVEX_HULL_SHAPE_PROXYTYPE:
        {
          const btConvexHullShape *originalShape = static_cast<const btConvexHullShape *>(ns->shape);
          btConvexHullShape *newShape = new btConvexHullShape(*originalShape);
          newShape->setLocalScaling(ns->localScale);
          return newShape;
        }
        case TRIANGLE_MESH_SHAPE_PROXYTYPE:
        {
          const btBvhTriangleMeshShape *originalShape = static_cast<const btBvhTriangleMeshShape *>(ns->shape);
          btScaledBvhTriangleMeshShape *newShape =
            new btScaledBvhTriangleMeshShape(const_cast<btBvhTriangleMeshShape *>(originalShape), ns->localScale);
          return newShape;
        }
        default:
          G_ASSERTF(false, "native shape cloning not implemented for type=%d (%s)", ns->shape->getShapeType(), ns->shape->getName());
      }
    }
    break;

    default: G_ASSERTF(false, "cannot create bt shape for collision type=%d", c->collType);
  }
  return nullptr;
}

static PhysCollision *create_bt_scaled_copy(const btCollisionShape *s, btVector3 scale);
static PhysCollision *create_bt_scaled_copy_no_margin(const btCollisionShape *s, btVector3 scale)
{
  switch (s->getShapeType())
  {
    case CONVEX_HULL_SHAPE_PROXYTYPE:
    case TRIANGLE_MESH_SHAPE_PROXYTYPE: return new BulletPhysNativeShape(s, scale);

    case COMPOUND_SHAPE_PROXYTYPE:
    {
      auto shape = static_cast<const btCompoundShape *>(s);
      PhysCompoundCollision *newShape = new PhysCompoundCollision;
      for (int i = 0; i < shape->getNumChildShapes(); ++i)
      {
        btTransform newTrans = shape->getChildTransform(i);
        newTrans.setOrigin(newTrans.getOrigin() * scale);
        btMatrix3x3 scaleMatrix(scale.getX(), 0.f, 0.f, 0.f, scale.getY(), 0.f, 0.f, 0.f, scale.getZ());
        btMatrix3x3 scaledMatrix = scaleMatrix * newTrans.getBasis();
        btVector3 newScale = scale;
        newScale.setX(scaledMatrix.getColumn(0).length());
        newScale.setY(scaledMatrix.getColumn(1).length());
        newScale.setZ(scaledMatrix.getColumn(2).length());
        newShape->addChildCollision(create_bt_scaled_copy(shape->getChildShape(i), newScale), to_tmatrix(newTrans));
      }
      return newShape;
    }

    case BOX_SHAPE_PROXYTYPE:
    {
      auto sz = static_cast<const btBoxShape *>(s)->getHalfExtentsWithoutMargin() * 2;
      return new PhysBoxCollision(scale.x() * sz.x(), scale.y() * sz.y(), scale.z() * sz.z());
    }

    case SPHERE_SHAPE_PROXYTYPE:
    {
      auto shape = static_cast<const btSphereShape *>(s);
      return new PhysSphereCollision(shape->getRadius() * scale.x());
    }

    case CAPSULE_SHAPE_PROXYTYPE:
    {
      auto shape = static_cast<const btCapsuleShape *>(s);
      float rad = shape->getRadius() * scale.x(), ht = shape->getHalfHeight() * 2 * scale.y();
      return new PhysCapsuleCollision(rad, ht + rad * 2, shape->getUpAxis());
    }

    default: G_ASSERTF(false, "unknown shape type for object of debugtype '%s'", s->getName());
  };
  return nullptr;
}
static PhysCollision *create_bt_scaled_copy(const btCollisionShape *s, btVector3 scale)
{
  PhysCollision *c = create_bt_scaled_copy_no_margin(s, scale);
  if (c)
    c->setMargin(s->getMargin());
  return c;
}
PhysCollision *PhysBody::getCollisionScaledCopy(const Point3 &s)
{
  PhysCollision *c = create_bt_scaled_copy(getActor()->getRootCollisionShape(), to_btVector3(s));
  if (!multTmNonIdent)
    return c;
  PhysCompoundCollision *compound = new PhysCompoundCollision;
  compound->addChildCollision(c, multTm);
  return compound;
}

struct CompoundShapeChildTransformRes
{
  btTransform trans;
  btVector3 scale;
};
static CompoundShapeChildTransformRes patch_compound_child_transform(const btVector3 &scale, int i, const btCompoundShape *compound)
{
  CompoundShapeChildTransformRes res;
  res.trans = compound->getChildTransform(i);
  res.trans.setOrigin(res.trans.getOrigin() * scale);
  btMatrix3x3 scaleMatrix(scale.getX(), 0.f, 0.f, 0.f, scale.getY(), 0.f, 0.f, 0.f, scale.getZ());
  btMatrix3x3 scaledMatrix = scaleMatrix * res.trans.getBasis();
  res.scale = scale;
  res.scale.setX(scaledMatrix.getColumn(0).length());
  res.scale.setY(scaledMatrix.getColumn(1).length());
  res.scale.setZ(scaledMatrix.getColumn(2).length());
  return res;
}
static void patch_bullet_scaled_copy(const btVector3 &scale, const btCollisionShape *original_shape, btCollisionShape *shape)
{
  switch (shape->getShapeType())
  {
    case COMPOUND_SHAPE_PROXYTYPE:
    {
      const btCompoundShape *originalShape = static_cast<const btCompoundShape *>(original_shape);
      btCompoundShape *patchedShape = static_cast<btCompoundShape *>(shape);
      G_ASSERT_RETURN(originalShape->getNumChildShapes() == patchedShape->getNumChildShapes(), );
      for (int i = 0; i < originalShape->getNumChildShapes(); ++i)
      {
        CompoundShapeChildTransformRes transRes = patch_compound_child_transform(scale, i, originalShape);
        patchedShape->getChildTransform(i) = transRes.trans;

        const btCollisionShape *originalChild = originalShape->getChildShape(i);
        btCollisionShape *patchedChild = patchedShape->getChildShape(i);
        patch_bullet_scaled_copy(transRes.scale, originalChild, patchedChild);
      }
    }
    break;
    default: shape->setLocalScaling(scale); break;
  };
}

void PhysBody::patchCollisionScaledCopy(const Point3 &scale, PhysBody *orig)
{
  multTmNonIdent = orig->multTmNonIdent;
  if (multTmNonIdent)
    multTm = orig->multTm;
  patch_bullet_scaled_copy(to_btVector3(scale), orig->getActor()->getRootCollisionShape(), getActor()->getRootCollisionShape());
}

void PhysBody::setCollision(const PhysCollision *coll, bool allow_fast_inaccurate_coll_tm)
{
  exec_or_add_after_phys_action<SetCollisionAction>(*world, this,
    PhysBody::create_bt_collision_shape(coll, &minCollisionSz, &multTm, &multTmNonIdent, allow_fast_inaccurate_coll_tm));
}

bool PhysBody::isConvexCollisionShape() const
{
  const btCollisionShape *collisionShape = body ? body->getCollisionShape() : nullptr;
  return collisionShape && collisionShape->isConvex();
}
void PhysBody::getShapeAabb(Point3 &out_bb_min, Point3 &out_bb_max) const
{
  btVector3 aabbMin, aabbMax;
  body->getCollisionShape()->getAabb(to_btTransform(TMatrix::IDENT), aabbMin, aabbMax);
  out_bb_min = to_point3(aabbMin), out_bb_max = to_point3(aabbMax);
}

void PhysBody::setContinuousCollisionMode(bool use)
{
  // 2e-3f (2mm) This is a minimal possible size of a collision object
  // We assume that if any body is moved more than 2mm than CCD will be performed
  // Very low values of the threshold leads to a crash during CCD.
  // The movement is so small that is leads to CCD check along zero distance.
  // Also, we assume that minCollisionSz must be valid for a body and 2mm threshold never happens
  body->setCcdMotionThreshold(use ? max(minCollisionSz * 0.25f, 2e-3f) : 0.f);
  body->setCcdSweptSphereRadius(use ? minCollisionSz : 0.f);
}

void PhysBody::setMaterialId(int id)
{
  const PhysWorld::Material *material = world->getMaterial(id);
  safeMaterialId = id;

  if (!material)
  {
    material = world->getMaterial(0);
    if (!material)
    {
      debug("material id %d", id);
      G_ASSERT(0);
    }
  }

  //== softness and dynamicFriction is not used
  body->setRestitution(material->restitution);
  body->setFriction(material->friction);
  body->setRollingFriction(0.1);
}

template <typename FT, FT fn>
struct SetStuffAndActivate;
template <typename... Args, void (btRigidBody::*setter)(Args...)>
struct SetStuffAndActivate<void (btRigidBody::*)(Args...), setter> final : public AfterPhysUpdateAction
{
  eastl::tuple<eastl::remove_cvref_t<Args>...> args;
  bool activate;

  SetStuffAndActivate(btRigidBody *b, bool act, Args... args_) : AfterPhysUpdateAction(b), activate(act), args(args_...) {}

  void doAction(PhysWorld &, bool) override
  {
    auto invokeSetter = [&](auto... xs) { ((*body).*(setter))(xs...); };
    eastl::apply(invokeSetter, args);
    if (activate)
      body->activate();
  }
};

#define BT_MEMBER_FN(x) decltype(&x), &x

void PhysBody::setVelocity(const Point3 &vel, bool activate)
{
  using actCls = SetStuffAndActivate<BT_MEMBER_FN(btRigidBody::setLinearVelocity)>;
  exec_or_add_after_phys_action<actCls>(*world, body, activate, to_btVector3(vel));
}


void PhysBody::setAngularVelocity(const Point3 &vel, bool activate)
{
  using actCls = SetStuffAndActivate<BT_MEMBER_FN(btRigidBody::setAngularVelocity)>;
  exec_or_add_after_phys_action<actCls>(*world, body, activate, to_btVector3(vel));
}

void PhysBody::disableGravity(bool disable)
{
  if (disable)
  {
    body->setFlags(body->getFlags() | BT_DISABLE_WORLD_GRAVITY);
    body->setGravity(ZERO<btVector3>());
  }
  else
  {
    body->setFlags(body->getFlags() & ~BT_DISABLE_WORLD_GRAVITY);
    body->setGravity(world->getScene()->getGravity());
    exec_or_add_after_phys_action<ActivateBodyAction>(*world, body);
  }
}

void PhysBody::setGravity(const Point3 &grav, bool activate)
{
  using actCls = SetStuffAndActivate<BT_MEMBER_FN(btRigidBody::setGravity)>;
  exec_or_add_after_phys_action<actCls>(*world, body, activate, to_btVector3(grav));
}


struct AddImpulseAction final : public AfterPhysUpdateAction
{
  Point3 pos;
  Point3 imp;
  bool activate;

  AddImpulseAction(btRigidBody *b, const Point3 &p, const Point3 &i, bool a) : AfterPhysUpdateAction(b), pos(p), imp(i), activate(a) {}

  void doAction(PhysWorld &pw, bool) override
  {
    body->applyImpulse(to_btVector3(imp), to_btVector3(pos) - body->getCenterOfMassPosition());
    if (activate)
      body->activate();
  }
};

void PhysBody::addImpulse(const Point3 &p, const Point3 &force_x_dt, bool activate)
{
  exec_or_add_after_phys_action<AddImpulseAction>(*world, body, p, force_x_dt, activate);
}


void PhysBody::setTmWithDynamics(const TMatrix &wtm, real dt, bool activate)
{
  TMatrix oldTm;
  getTm(oldTm);
  setTm(wtm);
  Point3 vel = (wtm.getcol(3) - oldTm.getcol(3)) / dt;
  const float maxDynamicSpeed = 100.f;
  if (lengthSq(vel) > sqr(maxDynamicSpeed))
    vel *= maxDynamicSpeed / length(vel);
  setVelocity(vel, activate);
}

void PhysBody::wakeUp() { exec_or_add_after_phys_action<ActivateBodyAction>(*world, body, /*forceActivation*/ true); }

struct DestroyJointAction final : public AfterPhysUpdateAction
{
  btTypedConstraint *joint;

  DestroyJointAction(btTypedConstraint *j) : AfterPhysUpdateAction(nullptr), joint(j) {}

  void doAction(PhysWorld &pw, bool) override
  {
    pw.getScene()->removeConstraint(joint);
    delete joint;
  }
};

PhysJoint::~PhysJoint()
{
  G_ASSERT(joint);

  PhysBody *body1 = PhysBody::from_bt_body(&joint->getRigidBodyA());
  if (!body1)
    body1 = PhysBody::from_bt_body(&joint->getRigidBodyB());
  G_ASSERT(body1);
  PhysWorld *w = body1->getPhysWorld();
  exec_or_add_after_phys_action<DestroyJointAction>(*w, joint);
}

void Phys6DofJoint::setParam(int num, float value, int axis)
{
  btGeneric6DofConstraint *dofJoint = (btGeneric6DofConstraint *)joint;
  dofJoint->setParam(num, value, axis);
}

void Phys6DofJoint::setAxisDamping(int index, float damping)
{
  btGeneric6DofConstraint *dofJoint = (btGeneric6DofConstraint *)joint;
  if (index < 3)
  {
    dofJoint->getTranslationalLimitMotor()->m_damping = damping;
  }
  else
  {
    dofJoint->getRotationalLimitMotor(index - 3)->m_damping = damping;
  }
}


void Phys6DofJoint::setLimit(int index, const Point2 &limits)
{
  btGeneric6DofConstraint *dofJoint = (btGeneric6DofConstraint *)joint;
  dofJoint->setLimit(index, limits.x, limits.y);
}

void Phys6DofSpringJoint::setSpring(int index, bool on, float stiffness, float damping, const Point2 &limits)
{
  btGeneric6DofSpringConstraint *dofSpring = (btGeneric6DofSpringConstraint *)joint;
  dofSpring->enableSpring(index, on);
  if (on)
  {
    dofSpring->setStiffness(index, stiffness);
    dofSpring->setDamping(index, damping);
    setLimit(index, limits);
  }
}

void Phys6DofSpringJoint::setLimit(int index, const Point2 &limits)
{
  btGeneric6DofSpringConstraint *dofSpring = (btGeneric6DofSpringConstraint *)joint;
  dofSpring->setLimit(index, limits.x, limits.y);
}

void Phys6DofSpringJoint::setEquilibriumPoint()
{
  btGeneric6DofSpringConstraint *dofSpring = (btGeneric6DofSpringConstraint *)joint;
  dofSpring->setEquilibriumPoint();
}

void Phys6DofSpringJoint::setParam(int num, float value, int axis)
{
  btGeneric6DofSpringConstraint *dofSpring = (btGeneric6DofSpringConstraint *)joint;
  dofSpring->setParam(num, value, axis);
}


void Phys6DofSpringJoint::setAxisDamping(int index, float damping)
{
  btGeneric6DofSpringConstraint *dofSpring = (btGeneric6DofSpringConstraint *)joint;
  if (index < 3)
  {
    dofSpring->getTranslationalLimitMotor()->m_damping = damping;
  }
  else
  {
    dofSpring->getRotationalLimitMotor(index - 3)->m_damping = damping;
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

PhysRayCast::PhysRayCast(const Point3 &p, const Point3 &d, real max_length, PhysWorld *world) :
  hit(NULL, NULL, btVector3(0, 0, 0), 2.0f)
{
  ownWorld = world;
  pos = to_btVector3(p);
  dir = to_btVector3(d);
  maxLength = max_length;

  PhysCounters::rayCastObjCreated++;
  hasHit = 0;
}


void PhysRayCast::forceUpdate(PhysWorld *world)
{
  if (!world)
    world = ownWorld;
  G_ASSERT(world);

  hit.m_hitFraction = maxLength;
  hit.m_collisionObject = NULL;

  PhysCounters::rayCastUpdate++;

  btVector3 p0 = pos;
  btVector3 p1 = pos + dir * maxLength;

  btCollisionWorld::ClosestRayResultCallback resultCallback(p0, p1);
  resultCallback.m_collisionFilterGroup = resultCallback.m_collisionFilterMask = filterMask;
  world->getScene()->rayTest(p0, p1, resultCallback);
  hasHit = resultCallback.hasHit();
  if (hasHit)
  {
    hit.m_hitFraction = resultCallback.m_closestHitFraction;
    hit.m_collisionObject = resultCallback.m_collisionObject;
    hit.m_hitNormalLocal = resultCallback.m_hitNormalWorld;
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

static constexpr int PHYS_WORLD_DA_RESV_CNT = 128;

PhysWorld::PhysWorld(real default_static_friction, real default_dynamic_friction, real default_restitution, real default_softness) :
  scene(dispatcher, overlappingPairCache, solver, collisionConfiguration)
{
  G_UNREFERENCED(default_softness);
  G_UNREFERENCED(default_dynamic_friction);
  PhysCounters::resetCounters();

  scene.setGravity(btVector3(0, -9.80665f, 0));
  defaultMaterial = nullptr;

  delayedActions.reserve(PHYS_WORLD_DA_RESV_CNT);

  // scene.setDamping(0.12f, 0.06f);

  Material material;
  material.friction = default_static_friction;
  material.restitution = default_restitution * 0.5;
  materials.push_back(material);
}


PhysWorld::~PhysWorld()
{
  if (multithreadedUpdate)
    threadpool::wait(this);
  for (int i = bullet::swcRegistry.size() - 1; i >= 0; i--)
    if (bullet::swcRegistry[i]->pw == this)
      unloadSceneCollision(bullet::swcRegistry[i]);

  clear_all_ptr_items(bullet::staticBodies);

  // remove the rigidbodies from the dynamics world and delete them
  for (int i = scene.getNumCollisionObjects() - 1; i >= 0; i--)
  {
    btCollisionObject *obj = scene.getCollisionObjectArray()[i];
    btRigidBody *body = btRigidBody::upcast(obj);
    if (body)
    {
      if (body->getMotionState())
        delete body->getMotionState();
      if (auto *physBody = PhysBody::from_bt_body(body))
        physBody->body = nullptr;
    }

    scene.removeCollisionObject(obj);
    PhysWorld::destroy_bt_collobject(body);
  }
}

void PhysWorld::startSim(real dt, bool wake_up_thread /* = true */)
{
  fetchSimRes(true);
  lastSimDt = dt;
  if (multithreadedUpdate)
    threadpool::add(this, threadpool::PRIO_DEFAULT, wake_up_thread);
  else
    doJob();
}

bool PhysWorld::fetchSimRes(bool wait, PhysBody *destroying_body)
{
  G_UNUSED(destroying_body);
  if (wait)
  {
#if DAGOR_DBGLEVEL > 0
    if (invalidFetchSimResTID >= 0 && invalidFetchSimResTID == get_current_thread_id())
      logerr("Attempt to call %s from invalid thread. Forgotten start_async_phys_sim_es dependency?", __FUNCTION__);
#endif
#if TIME_PROFILER_ENABLED
    if (interlocked_acquire_load(done) && delayedActions.empty())
      return true;
    TIME_PROFILE(PhysWorld__fetchSimRes);
#endif
    if (multithreadedUpdate)
      threadpool::wait(this);
    if (delayedActions.empty())
      return true;
    if (isControledByCurThread())
    {
      OSSpinlockScopedLock lock(delayedActionsMutex);
      // Make sure that this->delayedActions is empty to avoid adding new ones from within doAction
      decltype(delayedActions) delayedActionsLocal;
      delayedActionsLocal.swap(delayedActions);
      for (auto act : delayedActionsLocal)
      {
        act->doAction(*this, /*immediate*/ false);
        delete act;
      }
      delayedActionsLocal.clear();
      delayedActionsLocal.swap(delayedActions);
    }
#if DAGOR_DBGLEVEL > 0
    else if (destroying_body) // Make sure that no delayed actions that refs this destroying body are left
    {
      OSSpinlockScopedLock lock(delayedActionsMutex);
      for (auto act : delayedActions)
        G_ASSERT(act->body != destroying_body->getActor());
    }
#endif
  }
  return true;
}

void PhysWorld::doJob()
{
#if PROFILE_PHYS
  int64_t refTime = ref_time_ticks();
#endif
  {
    TIME_PROFILE(bullet_phys_simulate)
#if DAGOR_DBGLEVEL > 0
    getScene()->stepSimulation(lastSimDt, max_sub_steps.get(), 1.f / fixed_time_step_hz.get());
#else
    getScene()->stepSimulation(lastSimDt, maxSubSteps, fixedTimeStep);
#endif
  }
#if PROFILE_PHYS
  int totalTime = get_time_usec(refTime);
  if (totalTime / 1000 > 10)
  {
    // Invoke serialization, so we can find out what's going on
    static int serializationId = 0;
    btDefaultSerializer *serializer = new btDefaultSerializer(1024 * 1024 * 64);
    btCollisionObjectArray &collObjs = scene.getCollisionObjectArray();
    for (int i = 0; i < collObjs.size(); ++i)
    {
      String collObjName(128, "collObj%d", i);
      serializer->registerNameForPointer(collObjs[i], collObjName);
    }
    scene.serialize(serializer);
    String fname(128, "file%d(%dms).bullet", serializationId, totalTime / 1000);
    file_ptr_t file = df_open(fname.str(), DF_WRITE | DF_CREATE);
    df_write(file, serializer->getBufferPointer(), serializer->getCurrentBufferSize());
    df_close(file);
    serializationId++;
    delete serializer;
  }
#endif
}

void PhysWorld::addAfterPhysAction(AfterPhysUpdateAction *action)
{
  OSSpinlockScopedLock lock(delayedActionsMutex);
  // Still use interlocked here since in `exec_or_add_after_phys_action` we reading it without mutex
  int nextCount = interlocked_increment(*(volatile int *)&delayedActions.mCount);
  if (DAGOR_UNLIKELY(nextCount > delayedActions.capacity()))
  {
    G_FAST_ASSERT(delayedActions.capacity());
    delayedActions.reserve(delayedActions.capacity() * 2);
  }
  delayedActions[nextCount - 1] = action;
}

void PhysWorld::clear()
{
  fetchSimRes(true);
  if (delayedActions.capacity() > PHYS_WORLD_DA_RESV_CNT)
  {
    delayedActions = {};
    delayedActions.reserve(PHYS_WORLD_DA_RESV_CNT);
  }
}

void PhysWorld::updateAabb(btRigidBody *obj) { exec_or_add_after_phys_action<UpdateSingleAabbAction>(*this, obj, this); }

void PhysWorld::setInteractionLayers(unsigned int mask1, unsigned int mask2, bool make_contacts)
{
  G_UNREFERENCED(mask1);
  G_UNREFERENCED(mask2);
  G_UNREFERENCED(make_contacts);
  //  simulator->setupContactGroups(mask1, mask2, make_contacts);
}


int PhysWorld::createNewMaterialId()
{
  Material material = materials[0];
  materials.push_back(material);
  return materials.size() - 1;
}


int PhysWorld::createNewMaterialId(real static_friction, real /*dynamic_friction*/, real restitution, real /*softness*/)
{
  Material material;
  material.friction = static_friction;
  material.restitution = restitution * 0.5;
  materials.push_back(material);
  return materials.size() - 1;
}

PhysJoint *PhysWorld::createRagdollBallJoint(PhysBody *body1, PhysBody *body2, const TMatrix &tm, const Point3 &min_limit,
  const Point3 &max_limit, real damping, real twist_damping, real /*stiffness*/, real sleep_threshold)
{
  G_UNREFERENCED(sleep_threshold);
  Phys6DofSpringJoint *j = new Phys6DofSpringJoint(body1, body2, tm);
  float totalMass = body1->getMass() + body2->getMass();
  for (int i = 0; i < 3; ++i)
  {
    j->setLimit(i, ZERO<Point2>());
    float minLim = min_limit[(i + 1) % 3];
    float maxLim = max_limit[(i + 1) % 3];
    j->setSpring(i + 3, true, totalMass * 0.f, 0.01f, Point2(-fabsf(minLim), maxLim));
  }
  j->setAxisDamping(3, damping);
  j->setAxisDamping(4, damping);
  j->setAxisDamping(5, twist_damping);
  body1->getActor()->setSleepingThresholds(1.6, 2.5);
  body2->getActor()->setSleepingThresholds(1.6, 2.5);
  body1->getActor()->setDeactivationTime(0.3);
  body2->getActor()->setDeactivationTime(0.3);
  return regJoint(j);
}


void PhysWorld::setMaterialPairInteraction(int mat_id1, int mat_id2, bool collide)
{
  G_UNREFERENCED(mat_id1);
  G_UNREFERENCED(mat_id2);
  G_UNREFERENCED(collide);
  // BulletPhysics::setMaterialPairInteraction ( mat_id1, mat_id2, collide );
}

struct AddConstraintToWorldAction final : public AfterPhysUpdateAction
{
  btTypedConstraint *joint;

  AddConstraintToWorldAction(PhysJoint *in_joint) : AfterPhysUpdateAction(nullptr), joint(in_joint->getJoint()) {}

  void doAction(PhysWorld &pw, bool) override { pw.getScene()->addConstraint(joint, /*disableCollisionsBetweenLinkedBodies*/ true); }
};

PhysJoint *PhysWorld::regJoint(PhysJoint *j)
{
  exec_or_add_after_phys_action<AddConstraintToWorldAction>(*this, j);
  return j;
}

void PhysWorld::setMaxSubSteps(int mss)
{
#if DAGOR_DBGLEVEL > 0
  max_sub_steps.set(mss);
#endif
  maxSubSteps = mss;
}

void PhysWorld::setFixedTimeStep(float fts)
{
#if DAGOR_DBGLEVEL > 0
  fixed_time_step_hz.set((int)floorf(1.f / fabsf(fts) + 0.5f));
#endif
  fixedTimeStep = fabsf(fts);
}

const PhysWorld::Material *PhysWorld::getMaterial(int id) const
{
  if (id >= 0 && id < materials.size())
    return &materials[id];
  else
    return NULL;
}

PhysRagdollHingeJoint::PhysRagdollHingeJoint(PhysBody *body1, PhysBody *body2, const Point3 &pos, const Point3 &axis,
  const Point3 & /*mid_axis*/, const Point3 & /*x_axis*/, real ang_limit, real /*damping*/, real /*sleep_threshold*/) :
  PhysJoint(PJ_HINGE)
{
  btHingeConstraint *j = new btHingeConstraint(*body1->getActor(), *body2->getActor(),
    body1->getActor()->getCenterOfMassTransform().inverse() * to_btVector3(pos),
    body2->getActor()->getCenterOfMassTransform().inverse() * to_btVector3(pos),
    body1->getActor()->getCenterOfMassTransform().getBasis().inverse() * to_btVector3(axis),
    body2->getActor()->getCenterOfMassTransform().getBasis().inverse() * to_btVector3(axis), false);
  j->setLimit(j->getHingeAngle(), j->getHingeAngle() + ang_limit * 2);
  body1->getActor()->setSleepingThresholds(1.6, 2.5);
  body2->getActor()->setSleepingThresholds(1.6, 2.5);
  body1->getActor()->setDamping(0.15, 0.95);
  body2->getActor()->setDamping(0.15, 0.95);
  body1->getActor()->setDeactivationTime(0.3);
  body2->getActor()->setDeactivationTime(0.3);
  joint = j;
}

Phys6DofJoint::Phys6DofJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2) : PhysJoint(PJ_6DOF)
{
  if (body2)
  {
    joint = new btGeneric6DofConstraint(*body1->getActor(), *body2->getActor(), to_btTransform(frame1), to_btTransform(frame2), true);
  }
  else
  {
    joint = new btGeneric6DofConstraint(*body1->getActor(), to_btTransform(frame1), false);
  }
}

Phys6DofSpringJoint::Phys6DofSpringJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2) :
  PhysJoint(PJ_6DOF_SPRING)
{
  joint =
    new btGeneric6DofSpringConstraint(*body1->getActor(), *body2->getActor(), to_btTransform(frame1), to_btTransform(frame2), true);
}

Phys6DofSpringJoint::Phys6DofSpringJoint(PhysBody *body1, PhysBody *body2, const TMatrix &in_tm) : PhysJoint(PJ_6DOF_SPRING)
{
  btTransform tm = to_btTransform(in_tm);
  joint = new btGeneric6DofSpringConstraint(*body1->getActor(), *body2->getActor(),
    body1->getActor()->getCenterOfMassTransform().inverse() * tm, body2->getActor()->getCenterOfMassTransform().inverse() * tm, true);
}

PhysRagdollBallJoint::PhysRagdollBallJoint(PhysBody *body1, PhysBody *body2, const TMatrix &_tm, const Point3 &min_limit,
  const Point3 &max_limit, real damping, real /*twist_damping*/, real /*sleep_threshold*/) :
  PhysJoint(PJ_CONE_TWIST)
{
  btTransform tm = to_btTransform(_tm);

  btConeTwistConstraint *j = new btConeTwistConstraint(*body1->getActor(), *body2->getActor(),
    body1->getActor()->getCenterOfMassTransform().inverse() * tm, body2->getActor()->getCenterOfMassTransform().inverse() * tm);

  j->setLimit(min(max_limit.y, min_limit.y), min(max_limit.z, min_limit.z), min(fabsf(max_limit.x), fabsf(min_limit.x)));
  j->setDamping(damping);
  body1->getActor()->setDamping(0.15, 0.95);
  body1->getActor()->setDeactivationTime(0.3);
  body2->getActor()->setDamping(0.15, 0.95);
  body2->getActor()->setDeactivationTime(0.3);
  body1->getActor()->setSleepingThresholds(1.6, 2.5);
  body2->getActor()->setSleepingThresholds(1.6, 2.5);
  joint = j;
}

void PhysRagdollBallJoint::setTargetOrientation(const TMatrix &tm)
{
  G_UNUSED(tm);
  LOGERR_ONCE("PhysRagdollBallJoint::setTargetOrientation not implemented");
}

void PhysRagdollBallJoint::setTwistSwingMotorSettings(float twistFrequency, float twistDamping, float swingFrequency,
  float swingDamping)
{
  G_UNUSED(twistFrequency);
  G_UNUSED(twistDamping);
  G_UNUSED(swingFrequency);
  G_UNUSED(swingDamping);
  LOGERR_ONCE("PhysRagdollBallJoint::setTwistSwingMotorSettings not implemented");
}

int phys_body_get_hmap_step(PhysBody *b)
{
  if (!b)
    return -1;

  const btCollisionShape *shape = b->getActor()->getCollisionShape();
  if (!shape || shape->getShapeType() != TERRAIN_SHAPE_PROXYTYPE)
    return -1;

  G_ASSERT(::strcmp(shape->getName(), "DHEIGHTFIELD") == 0);
  return static_cast<const btDagorHeightfieldTerrainShape *>(shape)->getStep();
}
int phys_body_set_hmap_step(PhysBody *b, int step)
{
  if (!b)
    return -1;

  btCollisionShape *shape = b->getActor()->getCollisionShape();
  if (!shape || shape->getShapeType() != TERRAIN_SHAPE_PROXYTYPE)
    return -1;

  G_ASSERT(::strcmp(shape->getName(), "DHEIGHTFIELD") == 0);

  btDagorHeightfieldTerrainShape *terrain = static_cast<btDagorHeightfieldTerrainShape *>(shape);
  const int prevStep = terrain->getStep();
  if (step != prevStep)
  {
    b->getPhysWorld()->fetchSimRes(/*wait*/ true);
    terrain->setStep(step);
  }
  return prevStep;
}

static void shape_query(btConvexShape *shape, const TMatrix &from, const TMatrix &to, PhysWorld *pw, dag::ConstSpan<PhysBody *> bodies,
  PhysShapeQueryResult &out, int filter_grp, int filter_mask)
{
  btTransform btFromTm = to_btTransform(from);
  btTransform btToTm = to_btTransform(to);
  btCollisionWorld::ClosestConvexResultCallback closestCb(btVector3(0.f, 0.f, 0.f), btVector3(0.f, 0.f, 0.f));
  closestCb.m_collisionFilterGroup = filter_grp;
  closestCb.m_collisionFilterMask = filter_mask;
  closestCb.m_closestHitFraction = out.t;
  for (auto b : bodies)
    pw->getScene()->objectQuerySingle(shape, btFromTm, btToTm, b->getActor(), b->getActor()->getCollisionShape(),
      b->getActor()->getWorldTransform(), closestCb, 0.f);
  if (closestCb.m_closestHitFraction < out.t)
  {
    out.t = closestCb.m_closestHitFraction;
    out.res = to_point3(closestCb.m_hitPointWorld);
    out.norm = to_point3(closestCb.m_hitNormalWorld);
    const btRigidBody *hitBody = closestCb.m_hitCollisionObject ? btRigidBody::upcast(closestCb.m_hitCollisionObject) : nullptr;
    out.vel = hitBody ? to_point3(hitBody->getLinearVelocity()) : Point3(0.f, 0.f, 0.f);
  }
}

void PhysWorld::shapeQuery(const PhysBody *body, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
  PhysShapeQueryResult &out, int filter_grp, int filter_mask)
{
  auto shape = body->getActor()->getCollisionShape();
  if (!shape || !shape->isConvex())
    return;
  auto convex = static_cast<btConvexShape *>(shape);
  shape_query(convex, from, to, this, bodies, out, filter_grp, filter_mask);
}

void PhysWorld::shapeQuery(const PhysCollision &shape, const TMatrix &from, const TMatrix &to, dag::ConstSpan<PhysBody *> bodies,
  PhysShapeQueryResult &out, int filter_grp, int filter_mask)
{
  switch (shape.collType)
  {
    case PhysCollision::TYPE_SPHERE:
    {
      btSphereShape convex(static_cast<const PhysSphereCollision &>(shape).getRadius());
      return shape_query(&convex, from, to, this, bodies, out, filter_grp, filter_mask);
    }
    case PhysCollision::TYPE_BOX:
    {
      btBoxShape convex(to_btVector3(static_cast<const PhysBoxCollision &>(shape).getSize() / 2));
      return shape_query(&convex, from, to, this, bodies, out, filter_grp, filter_mask);
    }
    default: G_ASSERTF(0, "unsupported collType=%d in %s", shape.collType, __FUNCTION__);
  }
}
