// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/contactSolver.h>

#include <gamePhys/collision/collisionObject.h>

#include <math/dag_geomTree.h>
#include <math/random/dag_random.h>

#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/contactData.h>

#include <gamePhys/props/atmosphere.h>
#include <gamePhys/phys/utils.h>
#include <util/dag_bitArray.h>
#include <memory/dag_framemem.h>

#include <gameRes/dag_collisionResource.h>

#include <scene/dag_physMat.h>

#include <phys/dag_physics.h>

int velocity_iterations = 5;
int position_iterations = 2;

float linear_sleep_tolerance = 0.1f;
float angular_sleep_tolerance = 0.1f;

#if defined(USE_BULLET_PHYSICS)
typedef btManifoldPoint ManifoldPoint;
typedef btPersistentManifold PersistentManifold;

#else
struct ManifoldPoint
{
  Point3 m_localPointA = Point3(0, 0, 0);
  Point3 m_localPointB = Point3(0, 0, 0);
  Point3 m_normalWorldOnB = Point3(0, 0, 0);
  Point3 m_positionWorldOnA = Point3(0, 0, 0);
  Point3 m_positionWorldOnB = Point3(0, 0, 0);
  float m_distance1 = 0;
  float m_appliedImpulse = 0;
  float m_appliedImpulseLateral1 = 0;
  float m_appliedImpulseLateral2 = 0;

  float getDistance() const { return m_distance1; }
};

struct PersistentManifold
{
  constexpr static const int MANIFOLD_CACHE_SIZE = 4;
  ManifoldPoint m_pointCache[MANIFOLD_CACHE_SIZE];
  int m_cachedPoints = 0;
  float m_contactBreakingThreshold = 0;
  float m_contactProcessingThreshold = 0;

  int getNumContacts() const { return m_cachedPoints; }
  const ManifoldPoint &getContactPoint(int index) const { return m_pointCache[index]; }
  ManifoldPoint &getContactPoint(int index) { return m_pointCache[index]; }
  int getCacheEntry(const ManifoldPoint &newPoint) const;

  void refreshContactPoints(const TMatrix &tmA, const TMatrix &tmB);

  int sortCachedPoints(const ManifoldPoint &pt);
  int addManifoldPoint(const ManifoldPoint &newPoint, bool isPredictive = false);
  void removeContactPoint(int index);
  void replaceContactPoint(const ManifoldPoint &newPoint, int insertIndex);
  bool validContactDistance(const ManifoldPoint &pt) const { return pt.m_distance1 <= getContactBreakingThreshold(); }

  void setContactBreakingThreshold(float contactBreakingThreshold) { m_contactBreakingThreshold = contactBreakingThreshold; }
  void setContactProcessingThreshold(float contactProcessingThreshold) { m_contactProcessingThreshold = contactProcessingThreshold; }
  float getContactBreakingThreshold() const { return m_contactBreakingThreshold; }
  float getContactProcessingThreshold() const { return m_contactProcessingThreshold; }
};

int PersistentManifold::getCacheEntry(const ManifoldPoint &newPoint) const
{
  float shortestDist = getContactBreakingThreshold() * getContactBreakingThreshold();
  int size = getNumContacts();
  int nearestPoint = -1;
  for (int i = 0; i < size; i++)
  {
    const ManifoldPoint &mp = m_pointCache[i];

    Point3 diffA = mp.m_localPointA - newPoint.m_localPointA;
    const float distToManiPoint = diffA * diffA;
    if (distToManiPoint < shortestDist)
    {
      shortestDist = distToManiPoint;
      nearestPoint = i;
    }
  }
  return nearestPoint;
}
void PersistentManifold::refreshContactPoints(const TMatrix &tmA, const TMatrix &tmB)
{
  /// first refresh worldspace positions and distance
  for (int i = getNumContacts() - 1; i >= 0; i--)
  {
    ManifoldPoint &manifoldPoint = m_pointCache[i];
    manifoldPoint.m_positionWorldOnA = tmA * manifoldPoint.m_localPointA;
    manifoldPoint.m_positionWorldOnB = tmB * manifoldPoint.m_localPointB;
    manifoldPoint.m_distance1 = (manifoldPoint.m_positionWorldOnA - manifoldPoint.m_positionWorldOnB) * manifoldPoint.m_normalWorldOnB;
  }

  /// then
  float distance2d;
  Point3 projectedDifference, projectedPoint;
  for (int i = getNumContacts() - 1; i >= 0; i--)
  {
    ManifoldPoint &manifoldPoint = m_pointCache[i];
    // contact becomes invalid when signed distance exceeds margin (projected on contactnormal direction)
    if (!validContactDistance(manifoldPoint))
      removeContactPoint(i);
    else
    {
      // todo: friction anchor may require the contact to be around a bit longer
      // contact also becomes invalid when relative movement orthogonal to normal exceeds margin
      projectedPoint = manifoldPoint.m_positionWorldOnA - manifoldPoint.m_normalWorldOnB * manifoldPoint.m_distance1;
      projectedDifference = manifoldPoint.m_positionWorldOnB - projectedPoint;
      distance2d = projectedDifference * projectedDifference;
      if (distance2d > getContactBreakingThreshold() * getContactBreakingThreshold())
        removeContactPoint(i);
    }
  }
}
int PersistentManifold::addManifoldPoint(const ManifoldPoint &newPoint, bool isPredictive)
{
  if (!isPredictive)
    G_ASSERT(validContactDistance(newPoint));

  int insertIndex = getNumContacts();
  if (insertIndex == MANIFOLD_CACHE_SIZE)
    insertIndex = sortCachedPoints(newPoint); // sort cache so best points come first, based on area
  else
    m_cachedPoints++;
  if (insertIndex < 0)
    insertIndex = 0;

  m_pointCache[insertIndex] = newPoint;
  return insertIndex;
}
void PersistentManifold::removeContactPoint(int index)
{
  int lastUsedIndex = getNumContacts() - 1;
  if (index != lastUsedIndex)
  {
    m_pointCache[index] = m_pointCache[lastUsedIndex];
    m_pointCache[lastUsedIndex].m_appliedImpulse = 0.f;
    m_pointCache[lastUsedIndex].m_appliedImpulseLateral1 = 0.f;
    m_pointCache[lastUsedIndex].m_appliedImpulseLateral2 = 0.f;
  }
  m_cachedPoints--;
}
void PersistentManifold::replaceContactPoint(const ManifoldPoint &newPoint, int insertIndex)
{
  G_ASSERT(validContactDistance(newPoint));

  float appliedImpulse = m_pointCache[insertIndex].m_appliedImpulse;
  float appliedLateralImpulse1 = m_pointCache[insertIndex].m_appliedImpulseLateral1;
  float appliedLateralImpulse2 = m_pointCache[insertIndex].m_appliedImpulseLateral2;

  bool replacePoint = true;
  if (replacePoint) //-V547
  {
    m_pointCache[insertIndex] = newPoint;
    m_pointCache[insertIndex].m_appliedImpulse = appliedImpulse;
    m_pointCache[insertIndex].m_appliedImpulseLateral1 = appliedLateralImpulse1;
    m_pointCache[insertIndex].m_appliedImpulseLateral2 = appliedLateralImpulse2;
  }
}
int PersistentManifold::sortCachedPoints(const ManifoldPoint &pt)
{
  // calculate 4 possible cases areas, and take biggest area
  // also need to keep 'deepest'
  int maxPenetrationIndex = -1;
  float maxPenetration = pt.getDistance();
  for (int i = 0; i < 4; i++)
    if (m_pointCache[i].getDistance() < maxPenetration)
    {
      maxPenetrationIndex = i;
      maxPenetration = m_pointCache[i].getDistance();
    }

  int maxIndex = -1;
  float maxVal = -1e30f;

  if (maxPenetrationIndex != 0)
  {
    Point3 a0 = pt.m_localPointA - m_pointCache[1].m_localPointA;
    Point3 b0 = m_pointCache[3].m_localPointA - m_pointCache[2].m_localPointA;
    float cross_sq = lengthSq(a0 % b0);
    maxVal = cross_sq;
    maxIndex = 0;
  }
  if (maxPenetrationIndex != 1)
  {
    Point3 a1 = pt.m_localPointA - m_pointCache[0].m_localPointA;
    Point3 b1 = m_pointCache[3].m_localPointA - m_pointCache[2].m_localPointA;
    float cross_sq = lengthSq(a1 % b1);
    if (cross_sq > maxVal)
      maxVal = cross_sq, maxIndex = 1;
  }

  if (maxPenetrationIndex != 2)
  {
    Point3 a2 = pt.m_localPointA - m_pointCache[0].m_localPointA;
    Point3 b2 = m_pointCache[3].m_localPointA - m_pointCache[1].m_localPointA;
    float cross_sq = lengthSq(a2 % b2);
    if (cross_sq > maxVal)
      maxVal = cross_sq, maxIndex = 2;
  }

  if (maxPenetrationIndex != 3)
  {
    Point3 a3 = pt.m_localPointA - m_pointCache[0].m_localPointA;
    Point3 b3 = m_pointCache[2].m_localPointA - m_pointCache[1].m_localPointA;
    float cross_sq = lengthSq(a3 % b3);
    if (cross_sq > maxVal)
      maxVal = cross_sq, maxIndex = 3;
  }

  return maxIndex >= 0 ? maxIndex : 0;
}

static inline const Point3 &to_point3(const Point3 &p) { return p; }
static inline const Point3 &to_btVector3(const Point3 &p) { return p; }
static inline const TMatrix &to_btTransform(const TMatrix &tm) { return tm; }
#endif

const float time_to_sleep = 0.25f;

static DPoint3 mult(const DPoint3 &lhs, const DPoint3 &rhs) { return DPoint3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z); }

static DPoint3 safeinv(const DPoint3 &p) { return DPoint3(safeinv(p.x), safeinv(p.y), safeinv(p.z)); }

void ContactSolver::Constraint::init(const BodyState &state_a, const BodyState &state_b)
{
  JB = 0.0;
  if (!state_a.isKinematic || state_b.isKinematic)
    JB += state_a.invMass * J[0] * J[0] + mult(state_a.invInertia, J[1]) * J[1];
  if (!state_b.isKinematic || state_a.isKinematic)
    JB += state_b.invMass * J[2] * J[2] + mult(state_b.invInertia, J[3]) * J[3];
  invJB = safeinv(JB);
}

struct ContactSolver::Cache::Pair
{
  int indexA = -1;
  int indexB = -1;

  PersistentManifold manifold;
};

ContactSolver::Cache::Cache(Cache &&) = default;
ContactSolver::Cache::~Cache() = default;
ContactSolver::Cache::Cache::Cache(ContactSolver &set_solver) : solver(set_solver) {}


void ContactSolver::Cache::clearData() { clear_and_shrink(pairs); }

void ContactSolver::Cache::clearData(int index)
{
  for (int i = pairs.size() - 1; i >= 0; --i)
    if (pairs[i].indexA == index || pairs[i].indexB == index)
      erase_items(pairs, i, 1);
}

void ContactSolver::Cache::update()
{
  for (auto &pair : pairs)
  {
    TMatrix tmA = TMatrix::IDENT;
    if (pair.indexA >= 0 && solver.bodies[pair.indexA].phys)
      solver.bodies[pair.indexA].phys->getCurrentStateLoc().toTM(tmA);

    TMatrix tmB = TMatrix::IDENT;
    if (pair.indexB >= 0 && solver.bodies[pair.indexB].phys)
      solver.bodies[pair.indexB].phys->getCurrentStateLoc().toTM(tmB);

    pair.manifold.refreshContactPoints(to_btTransform(tmB), to_btTransform(tmA));
  }
}

void ContactSolver::Cache::getManifold(Tab<ContactManifoldPoint> &manifold)
{
  for (int cacheId = 0; cacheId < pairs.size(); ++cacheId)
  {
    Cache::Pair &pair = pairs[cacheId];
    for (int i = 0; i < pair.manifold.getNumContacts(); ++i)
    {
      const ManifoldPoint &p = pair.manifold.getContactPoint(i);

      ContactManifoldPoint &m = manifold.push_back();

      m.indexInCache = i;
      m.cacheId = cacheId;

      m.indexA = pair.indexA;
      m.indexB = pair.indexB;

      m.depth = -p.getDistance();
      m.lambda = p.m_appliedImpulse;
      m.lambdaFriction1 = p.m_appliedImpulseLateral1;
      m.lambdaFriction2 = p.m_appliedImpulseLateral2;

      m.normal = dpoint3(to_point3(p.m_normalWorldOnB));

      m.localPosA = dpoint3(to_point3(p.m_localPointB));
      m.localPosB = dpoint3(to_point3(p.m_localPointA));

      IPhysBase *physA = pair.indexA >= 0 ? solver.bodies[pair.indexA].phys : nullptr;
      IPhysBase *physB = pair.indexB >= 0 ? solver.bodies[pair.indexB].phys : nullptr;

      m.orientA = physA ? physA->getCurrentStateLoc().O.getQuat() : ZERO<Quat>();
      m.orientB = physB ? physB->getCurrentStateLoc().O.getQuat() : ZERO<Quat>();

      m.rA = physA ? dpoint3(m.orientA * (m.localPosA - dpoint3(physA->getCenterOfMass()))) : ZERO<DPoint3>();
      m.rB = physB ? dpoint3(m.orientB * (m.localPosB - dpoint3(physB->getCenterOfMass()))) : ZERO<DPoint3>();

      m.massA = physA ? physA->getMass() : 0.0;
      m.massB = physB ? physB->getMass() : 0.0;

      m.frictionA = physA ? physA->getFriction() : 0.0;
      m.frictionB = physB ? physB->getFriction() : 0.0;
      if (m.frictionA < 0.f)
        m.frictionA = 0.05;
      if (m.frictionB < 0.f)
        m.frictionB = 0.05;

      m.bouncingA = physA ? physA->getBouncing() : 0.0;
      m.bouncingB = physB ? physB->getBouncing() : 0.0;

      m.inertiaA = physA ? physA->calcInertia() : ZERO<DPoint3>();
      m.inertiaB = physB ? physB->calcInertia() : ZERO<DPoint3>();
    }
  }
}

ContactSolver::ContactSolver() : cache(*this)
{
  layersMask.set();

  ::memset(&groundState, 0, sizeof(BodyState));
  groundState.location.O.setQuat(Quat(0.f, 0.f, 0.f, 1.f));
}

ContactSolver::~ContactSolver() {}

void ContactSolver::addBody(IPhysBase *phys, float bounding_radius, uint32_t flags, int layer)
{
  if (!phys)
    return;

  int vacantBodyInd = -1;
  for (int i = 0; i < bodies.size(); ++i)
  {
    if (bodies[i].phys == phys)
      return;
    else if (!bodies[i].phys && vacantBodyInd < 0)
      vacantBodyInd = i;
  }

  Body &b = (vacantBodyInd < 0) ? bodies.push_back() : bodies[vacantBodyInd];
  b.phys = phys;
  b.boundingRadius = bounding_radius;
  b.flags = flags;
  b.layer = layer;

  BodyState &state = (vacantBodyInd < 0) ? bodyStates.push_back() : bodyStates[vacantBodyInd];
  ::memset(&state, 0, sizeof(BodyState));

  state.shouldCheckContactWithGround = true;
  state.isKinematic = flags & Flags::Kinematic;
  state.invMass = safeinv(phys->getMass());
  state.invInertia = safeinv(phys->calcInertia());
}

void ContactSolver::removeBody(IPhysBase *phys)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    Body &body = bodies[i];
    if (body.phys == phys)
    {
      body.phys = nullptr;
      cache.clearData(i);
      wakeUpBodiesInRadius(bodyStates[i].location.P, body.boundingRadius);
      break;
    }
  }
}

void ContactSolver::setBodyFlags(IPhysBase *phys, uint32_t flags)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    Body &body = bodies[i];
    if (body.phys == phys)
    {
      body.flags = flags;
      break;
    }
  }
}

void ContactSolver::setLayersMask(int layer_a, int layer_b, bool should_solve)
{
  if (layer_a >= MAX_LAYERS_NUM || layer_b >= MAX_LAYERS_NUM)
    return;

  layersMask.set(layer_a + layer_b * MAX_LAYERS_NUM, should_solve);
  layersMask.set(layer_b + layer_a * MAX_LAYERS_NUM, should_solve);
}

bool ContactSolver::getLayersMask(int layer_a, int layer_b) const
{
  const uint32_t layerNo = layer_a + layer_b * MAX_LAYERS_NUM;
  return layerNo < layersMask.size() ? layersMask[layerNo] : false;
}

void ContactSolver::integrateVelocity(double /* dt */)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    const Body &body = bodies[i];

    BodyState &state = bodyStates[i];
    IPhysBase *phys = body.phys;
    if (!phys)
      continue;

    state.addVelocity.zero();
    state.addOmega.zero();

    state.location = phys->getCurrentStateLoc();
    state.velocity = phys->getCurrentStateVelocity();
    state.omega = dpoint3(state.location.O.getQuat() * phys->getCurrentStateOmega());
  }
}

void ContactSolver::integratePositions(double /* dt */)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    const Body &body = bodies[i];
    if (!body.phys)
      continue;

    BodyState &state = bodyStates[i];

    if (body.phys->isAsleep())
    {
      state.velocity.zero();
      state.omega.zero();
      continue;
    }

    state.velocity += state.addVelocity;
    state.omega += state.addOmega;
  }
}

void ContactSolver::addContact(int index_a, int index_b, gamephys::CollisionContactData &contact)
{
  Cache::Pair *cachePair = nullptr;
  for (auto &pair : cache.pairs)
  {
    if ((pair.indexA == index_a && pair.indexB == index_b) || (pair.indexB == index_a && pair.indexA == index_b))
    {
      cachePair = &pair;
      break;
    }
  }

  if (!cachePair)
  {
    Cache::Pair &newCache = cache.pairs.push_back();
    new (&newCache, _NEW_INPLACE) Cache::Pair();
    newCache.indexA = index_a;
    newCache.indexB = index_b;
    newCache.manifold.setContactBreakingThreshold(0.25f);
    newCache.manifold.setContactProcessingThreshold(0.0f);

    cachePair = &cache.pairs.back();
  }

  ManifoldPoint p;
  ::memset(&p, 0, sizeof(ManifoldPoint));
  p.m_localPointA = to_btVector3(contact.posA);
  p.m_localPointB = to_btVector3(contact.posB);
  p.m_normalWorldOnB = to_btVector3(contact.wnormB);
  p.m_distance1 = contact.depth;
  p.m_positionWorldOnA = to_btVector3(contact.wpos);
  p.m_positionWorldOnB = to_btVector3(contact.wposB);

  if (!cachePair->manifold.validContactDistance(p))
    return;

  // Fix normals "inside" the ground
  if (index_a == -1)
  {
    Point3 normal = to_point3(p.m_normalWorldOnB);
    if (normal * Point3(0.f, 1.f, 0.f) < 0.f)
      p.m_normalWorldOnB = -p.m_normalWorldOnB;
  }

  const int insertIndex = cachePair->manifold.getCacheEntry(p);
  if (insertIndex >= 0)
    cachePair->manifold.replaceContactPoint(p, insertIndex);
  else
    cachePair->manifold.addManifoldPoint(p);
}

void ContactSolver::addFrictionConstraint(Constraint::Type type, const ContactManifoldPoint &info, double lambda, double friction,
  const DPoint3 &u)
{
  Constraint &c = constraints.push_back();

  c.type = type;
  c.lambda = lambda;

  c.indexInCache = info.indexInCache;
  c.cacheId = info.cacheId;
  c.indexA = info.indexA;
  c.indexB = info.indexB;
  c.J[0] = c.indexA >= 0 ? -u : ZERO<DPoint3>();
  c.J[1] = c.indexA >= 0 ? -(info.rA % u) : ZERO<DPoint3>();
  c.J[2] = c.indexB >= 0 ? u : ZERO<DPoint3>();
  c.J[3] = c.indexB >= 0 ? (info.rB % u) : ZERO<DPoint3>();
  c.lambdaMax = friction * gamephys::atmosphere::g();
  c.lambdaMin = -c.lambdaMax;
  c.init(getState(c.indexA), getState(c.indexB));
}

void ContactSolver::addContactConstraint(const ContactManifoldPoint &info, double beta, double bias)
{
  Constraint &c = constraints.push_back();
  c.type = Constraint::Type::Contact;
  c.indexInCache = info.indexInCache;
  c.cacheId = info.cacheId;
  c.indexA = info.indexA;
  c.indexB = info.indexB;
  c.J[0] = info.indexA >= 0 ? -info.normal : ZERO<DPoint3>();
  c.J[1] = info.indexA >= 0 ? -(info.rA % info.normal) : ZERO<DPoint3>();
  c.J[2] = info.indexB >= 0 ? info.normal : ZERO<DPoint3>();
  c.J[3] = info.indexB >= 0 ? info.rB % info.normal : ZERO<DPoint3>();
  c.lambda = info.lambda;
  c.lambdaMin = 0.0;
  c.beta = beta;
  c.bias = bias;
  c.localPosA = info.localPosA;
  c.localPosB = info.localPosB;
  c.normal = info.normal;
  if (solverFlags & SolverFlags::SolveBouncing)
    c.bouncing = (info.bouncingA + info.bouncingB) * 0.5;
  else
    c.bouncing = 0.0;
  c.init(getState(info.indexA), getState(info.indexB));
}

void ContactSolver::initVelocityConstraints()
{
  clear_and_shrink(constraints);

  Tab<ContactManifoldPoint> manifold(framemem_ptr());
  cache.getManifold(manifold);

  for (auto &info : manifold)
  {
    const double mu = (info.frictionA + info.frictionB) * 0.5;
    const double m = (info.massA + info.massB) * 0.5;

    DPoint3 u1 = normalize(info.normal % dpoint3(info.orientB.getForward()));
    DPoint3 u2 = normalize(info.normal % u1);

    addFrictionConstraint(Constraint::Type::Friction1, info, info.lambdaFriction1, mu * m, u1);
    addFrictionConstraint(Constraint::Type::Friction2, info, info.lambdaFriction2, mu * m, u2);
  }

  for (auto &info : manifold)
    addContactConstraint(info, 0.0, 0.0);
}

void ContactSolver::initPositionConstraints()
{
  clear_and_shrink(constraints);

  Tab<ContactManifoldPoint> manifold(framemem_ptr());
  cache.getManifold(manifold);

  for (auto &info : manifold)
    addContactConstraint(info, 0.0, 0.0);
}

ContactSolver::BodyState &ContactSolver::getState(int index) { return index >= 0 ? bodyStates[index] : groundState; }

void ContactSolver::BodyState::accumulateForce(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir)
{
  appliedForce[0] += lambda * invMass * linear_dir;
  appliedForce[1] += lambda * mult(invInertia, angular_dir);
}

void ContactSolver::BodyState::applyImpulse(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir)
{
  addVelocity += lambda * invMass * linear_dir;
  addOmega += lambda * mult(invInertia, angular_dir);
}

void ContactSolver::BodyState::applyPseudoImpulse(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir)
{
  DPoint3 pseudoVel = lambda * invMass * linear_dir;
  DPoint3 pseudoOmega = lambda * mult(invInertia, angular_dir);

  location.P += pseudoVel;

  Quat omegaQuat(pseudoOmega.x, pseudoOmega.y, pseudoOmega.z, 0);
  Quat spin = omegaQuat * location.O.getQuat() * 0.5f;
  location.O.setQuat(normalize(location.O.getQuat() + spin));
}

void ContactSolver::solveVelocityConstraints(double dt)
{
  const double invDt = safeinv(dt);

  for (auto &state : bodyStates)
    mem_set_0(state.appliedForce);

  for (auto &c : constraints)
  {
    BodyState &stateA = getState(c.indexA);
    BodyState &stateB = getState(c.indexB);
    if (!stateA.isKinematic || stateB.isKinematic)
      stateA.accumulateForce(c.lambda, c.J[0], c.J[1]);
    if (!stateB.isKinematic || stateA.isKinematic)
      stateB.accumulateForce(c.lambda, c.J[2], c.J[3]);
  }

  for (int iteration = 0; iteration < 10; ++iteration)
  {
    bool solved = true;

    for (auto &c : constraints)
    {
      BodyState &stateA = getState(c.indexA);
      BodyState &stateB = getState(c.indexB);

      const double linProjA = (stateA.velocity + stateA.addVelocity) * c.J[0];
      const double angProjA = (stateA.omega + stateA.addOmega) * c.J[1];
      const double linProjB = (stateB.velocity + stateB.addVelocity) * c.J[2];
      const double angProjB = (stateB.omega + stateB.addOmega) * c.J[3];

      const double relVelAfterContact = max(c.J[0] * (stateB.velocity - stateA.velocity) * c.bouncing, 0.0);

      const double bias = c.bias * c.beta * invDt + relVelAfterContact;
      const double nu = (bias - (linProjA + angProjA + linProjB + angProjB)) * invDt;

      const double ja = stateA.appliedForce[0] * c.J[0] + stateA.appliedForce[1] * c.J[1] + stateB.appliedForce[0] * c.J[2] +
                        stateB.appliedForce[1] * c.J[3];

      double deltaLambda = (nu - ja) * c.invJB;

      double prevLambda = c.lambda;
      c.lambda = max(c.lambdaMin, min(prevLambda + deltaLambda, c.lambdaMax));
      deltaLambda = c.lambda - prevLambda;

      if (!float_nonzero(deltaLambda))
        continue;

      solved = false;

      if (!stateA.isKinematic || stateB.isKinematic)
        stateA.accumulateForce(deltaLambda, c.J[0], c.J[1]);
      if (!stateB.isKinematic || stateA.isKinematic)
        stateB.accumulateForce(deltaLambda, c.J[2], c.J[3]);
    }

    if (solved)
      break;
  }

  for (auto &c : constraints)
  {
    Cache::Pair *pair = nullptr;
    if (c.cacheId >= 0)
      pair = &cache.pairs[c.cacheId];

    if (pair && c.type == Constraint::Type::Contact)
      pair->manifold.getContactPoint(c.indexInCache).m_appliedImpulse = c.lambda;

    if (pair && c.type == Constraint::Type::Friction1)
      pair->manifold.getContactPoint(c.indexInCache).m_appliedImpulseLateral1 = c.lambda;

    if (pair && c.type == Constraint::Type::Friction2)
      pair->manifold.getContactPoint(c.indexInCache).m_appliedImpulseLateral2 = c.lambda;

    BodyState &stateA = getState(c.indexA);
    BodyState &stateB = getState(c.indexB);

    if (!stateA.isKinematic || stateB.isKinematic)
      stateA.applyImpulse(c.lambda * dt, c.J[0], c.J[1]);
    if (!stateB.isKinematic || stateA.isKinematic)
      stateB.applyImpulse(c.lambda * dt, c.J[2], c.J[3]);
  }
}

bool ContactSolver::solvePositionConstraints()
{
  for (auto &state : bodyStates)
    mem_set_0(state.appliedForce);

  double minDistance = DBL_MAX;

  for (int iteration = 0; iteration < 5; ++iteration)
  {
    bool solved = true;

    for (auto &c : constraints)
    {
      if (iteration == 0)
        c.lambda = 0.0;

      BodyState &stateA = getState(c.indexA);
      BodyState &stateB = getState(c.indexB);

      double ja = stateA.appliedForce[0] * c.J[0] + stateA.appliedForce[1] * c.J[1] + stateB.appliedForce[0] * c.J[2] +
                  stateB.appliedForce[1] * c.J[3];

      DPoint3 pA;
      TMatrix tmA;
      stateA.location.toTM(tmA);
      pA = tmA * c.localPosA;

      DPoint3 pB;
      TMatrix tmB;
      stateB.location.toTM(tmB);
      pB = tmB * c.localPosB;

      double d = (pA - pB) * c.normal;
      double C = clamp((d - 0.05) * 0.2, 0.0, 0.2);
      if (C < minDistance)
        minDistance = C;
      double deltaLambda = (C - ja) * c.invJB;

      double prevLambda = c.lambda;
      c.lambda = max(c.lambdaMin, min(prevLambda + deltaLambda, c.lambdaMax));
      deltaLambda = c.lambda - prevLambda;

      if (!float_nonzero(deltaLambda))
        continue;

      solved = false;

      if (!stateA.isKinematic || stateB.isKinematic)
        stateA.accumulateForce(deltaLambda, c.J[0], c.J[1]);
      if (!stateB.isKinematic || stateA.isKinematic)
        stateB.accumulateForce(deltaLambda, c.J[2], c.J[3]);
    }

    if (solved)
      break;
  }

  for (auto &c : constraints)
  {
    BodyState &stateA = getState(c.indexA);
    BodyState &stateB = getState(c.indexB);
    if (!stateA.isKinematic || stateB.isKinematic)
      stateA.applyPseudoImpulse(c.lambda, c.J[0], c.J[1]);
    if (!stateB.isKinematic || stateA.isKinematic)
      stateB.applyPseudoImpulse(c.lambda, c.J[2], c.J[3]);
  }

  return constraints.empty() || minDistance < 0.05;
}

void ContactSolver::clearData()
{
  atTick = 0;
  cache.clearData();
  clear_and_shrink(bodies);
  clear_and_shrink(bodyStates);
}

void ContactSolver::clearData(int index) { cache.clearData(index); }

void ContactSolver::checkStaticCollisions(int body_no, const ContactSolver::Body &body, const TMatrix &tm,
  dag::Span<CollisionObject> coll_objects)
{
  if (!(body.flags & Flags::ProcessGround) || !bodyStates[body_no].shouldCheckContactWithGround)
    return;

  bodyStates[body_no].shouldCheckContactWithGround = false;

  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  for (CollisionObject &co : coll_objects)
  {
    if (!co.isValid())
      continue;
    dacoll::set_collision_object_tm(co, tm);
    dacoll::test_collision_frt(co, contacts);
    dacoll::test_collision_ri(co, BBox3(ZERO<Point3>(), 1.f), contacts);
    dacoll::test_collision_lmesh(co, tm, 1.f, -1, contacts);
  }

  for (auto &c : contacts)
    addContact(-1, body_no, c);
}

void ContactSolver::update(double at_time, double dt)
{
  const int curTick = gamephys::ceilPhysicsTickNumber(at_time, dt);
  if (updatePolicy == UpdatePolicy::UpdateByTicks && curTick <= atTick)
    return;

  cache.update();

  if (cache.pairs.empty())
  {
    for (int i = bodies.size() - 1; i >= 0; --i)
      if (!bodies[i].phys)
      {
        erase_items(bodies, i, 1);
        erase_items(bodyStates, i, 1);
      }
  }

  atTick = gamephys::nearestPhysicsTickNumber(at_time + dt, dt);

  Bitarray checked(framemem_ptr());
  Tab<int> testPairs(framemem_ptr());

  const int bodiesCount = bodies.size();

  // Broad phase
  for (int curNo = 0; curNo < bodiesCount; ++curNo)
  {
    Body &curBody = bodies[curNo];
    IPhysBase *curBodyPhys = curBody.phys;
    if (!curBodyPhys)
      continue;

    gamephys::Loc curLoc = curBodyPhys->getCurrentStateLoc();

    for (int testNo = 0; testNo < bodiesCount; testNo++)
    {
      if (curNo == testNo)
        continue;

      Body &testBody = bodies[testNo];

      const int layerNo = curBody.layer + testBody.layer * MAX_LAYERS_NUM;
      if (!layersMask[layerNo])
        continue;

      const int pair0Idx = curNo + testNo * bodiesCount;
      const int pair1Idx = testNo + curNo * bodiesCount;

      if (pair0Idx < checked.size() && pair1Idx < checked.size() && checked[pair0Idx] && checked[pair1Idx])
        continue;

      IPhysBase *testBodyPhys = testBody.phys;
      if (!testBodyPhys)
        continue;

      if (!(curBody.flags & Flags::ProcessBody) || !(testBody.flags & Flags::ProcessBody))
        continue;

      if (curBodyPhys->isAsleep() && testBodyPhys->isAsleep())
        continue;

      gamephys::Loc testLoc = testBodyPhys->getCurrentStateLoc();

      if (lengthSq(testLoc.P - curLoc.P) > sqr(curBody.boundingRadius + testBody.boundingRadius))
        continue;

      testPairs.reserve(8);
      testPairs.push_back() = curNo;
      testPairs.push_back() = testNo;

      const int maxIdx = max(pair0Idx, pair1Idx);
      if (maxIdx >= checked.size())
        checked.resize(maxIdx + 1);

      checked.set(pair0Idx);
      checked.set(pair1Idx);
    }
  }

  // Narrow phase
  for (int i = 0; i < testPairs.size(); i += 2)
  {
    const int curNo = testPairs[i];
    const int testNo = testPairs[i + 1];

    Body &curBody = bodies[curNo];
    Body &testBody = bodies[testNo];

    IPhysBase *curBodyPhys = curBody.phys;
    IPhysBase *testBodyPhys = testBody.phys;

    TMatrix curBodyTm;
    curBodyPhys->getCurrentStateLoc().toTM(curBodyTm);
    dag::Span<CollisionObject> curCollisionObjects = curBodyPhys->getMutableCollisionObjects();

    TMatrix testBodyTm;
    testBodyPhys->getCurrentStateLoc().toTM(testBodyTm);
    dag::Span<CollisionObject> testCollisionObjects = testBodyPhys->getMutableCollisionObjects();

    Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
    dacoll::test_pair_collision(curCollisionObjects, curBodyPhys->getActiveCollisionObjectsBitMask(), curBodyTm, testCollisionObjects,
      testBodyPhys->getActiveCollisionObjectsBitMask(), testBodyTm, contacts);

    for (auto &c : contacts)
      addContact(testNo, curNo, c);

    if (!contacts.empty())
    {
      curBodyPhys->wakeUp();
      testBodyPhys->wakeUp();

      checkStaticCollisions(curNo, curBody, curBodyTm, curCollisionObjects);
      checkStaticCollisions(testNo, testBody, testBodyTm, testCollisionObjects);
    }
  }

  integrateVelocity(dt);
  initVelocityConstraints();

  for (int i = 0; i < velocity_iterations; ++i)
    solveVelocityConstraints(dt);

  integratePositions(dt);

  initPositionConstraints();

  bool positionsSolved = false;
  if (solverFlags & SolverFlags::SolvePositions)
    for (int i = 0; i < position_iterations; ++i)
    {
      positionsSolved = solvePositionConstraints();
      if (positionsSolved)
        break;
    }
  else
    positionsSolved = true;

  for (auto &c : constraints)
  {
    if (c.type != ContactSolver::Constraint::Type::Contact)
      continue;

    if (c.indexA >= 0 && bodies[c.indexA].phys)
      bodies[c.indexA].phys->rescheduleAuthorityApprovedSend();
    if (c.indexB >= 0 && bodies[c.indexB].phys)
      bodies[c.indexB].phys->rescheduleAuthorityApprovedSend();
  }

  for (int i = 0; i < bodies.size(); ++i)
  {
    Body &body = bodies[i];
    if (!body.phys)
      continue;

    BodyState &state = bodyStates[i];

    state.location.P += state.addVelocity * body.phys->getTimeStep();
    Point3 orientationInc = state.addOmega * body.phys->getTimeStep();
    Quat quatInc = Quat(orientationInc, length(orientationInc));
    state.location.O.setQuat(normalize(state.location.O.getQuat() * quatInc));

    bodies[i].phys->addSoundShockImpulse(bodyStates[i].addVelocity.length());

    state.addVelocity.zero();
    state.addOmega.zero();

    state.shouldCheckContactWithGround = true;

    body.phys->setCurrentMinimalState(state.location, state.velocity, dpoint3(inverse(state.location.O.getQuat()) * state.omega));

    if (!(body.flags & Flags::Sleepable))
      continue;

    if (state.velocity.lengthSq() < sqr(linear_sleep_tolerance) && state.omega.lengthSq() < sqr(angular_sleep_tolerance))
    {
      state.timeToSleep += dt;
    }
    else
    {
      state.timeToSleep = 0.f;
    }

    if (state.timeToSleep >= time_to_sleep && positionsSolved)
      body.phys->putToSleep();
  }
}

void ContactSolver::wakeUpBodiesInRadius(const DPoint3 &pos, float radius)
{
  const float radiusSq = sqr(radius);
  for (int i = 0; i < bodies.size(); ++i)
    if (bodies[i].phys && lengthSq(bodyStates[i].location.P - pos) <= radiusSq)
    {
      bodies[i].phys->wakeUp();
      bodyStates[i].timeToSleep = 0.f;
    }
}

bool ContactSolver::isContactBetween(IPhysBase *bodyA, IPhysBase *bodyB) const
{
  if (bodyA == nullptr || bodyB == nullptr)
    return false;
  for (const Cache::Pair &p : cache.pairs)
    if (p.indexA >= 0 && p.indexB >= 0 && p.manifold.getNumContacts() > 0 &&
        ((bodies[p.indexA].phys == bodyA && bodies[p.indexB].phys == bodyB) ||
          (bodies[p.indexA].phys == bodyB && bodies[p.indexB].phys == bodyA)))
      return true;
  return false;
}


void ContactSolver::drawDebugCollisions()
{
  for (const Body &body : bodies)
  {
    if (!body.phys)
      continue;
    const auto objects = body.phys->getCollisionObjects();
    for (const auto &co : objects)
    {
      if (!co.body)
        continue;
      co.body->drawDebugCollision(0xffffaaffu, true);
    }
  }
}

void ContactSolver::drawDebugContacts()
{
  Tab<ContactManifoldPoint> manifold;
  cache.getManifold(manifold);
  for (const auto &p : manifold)
  {
    if (unsigned(p.indexA) >= bodies.size() || unsigned(p.indexB) >= bodies.size() || bodies[p.indexA].phys == nullptr ||
        bodies[p.indexB].phys == nullptr)
      continue;

    const TMatrix &tmA = getState(p.indexA).location.makeTM();
    const TMatrix &tmB = getState(p.indexB).location.makeTM();
    const Point3 cA = tmA * bodies[p.indexA].phys->getCenterOfMass();
    const Point3 cB = tmB * bodies[p.indexB].phys->getCenterOfMass();
    draw_cached_debug_line(cA, cA + p.rA, 0xff00ff00u);
    draw_cached_debug_line(cB, cB + p.rB, 0xff00ff00u);
  }
}
