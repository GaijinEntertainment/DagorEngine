//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_staticTab.h>
#include <generic/dag_tab.h>

#include <math/dag_mathAng.h>

#include <gamePhys/phys/unitPhysBase.h>
#include <gamePhys/phys/commonPhysBase.h>
#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/collisionLib.h>

#include <EASTL/bitset.h>

class GeomNodeTree;

class ContactSolver
{
  struct Body
  {
    uint32_t flags = ContactSolver::ProcessAll;
    int layer = 0;
    float boundingRadius = 0.f;
    IPhysBase *phys = nullptr;
  };

  struct ContactManifoldPoint
  {
    int indexInCache = -1;
    int cacheId = -1;

    int indexA = -1;
    int indexB = -1;

    double depth = 0.0;
    double lambda = 0.0;
    double lambdaFriction1 = 0.0;
    double lambdaFriction2 = 0.0;

    DPoint3 normal;

    DPoint3 localPosA = ZERO<DPoint3>();
    DPoint3 localPosB = ZERO<DPoint3>();

    Quat orientA = ZERO<Quat>();
    Quat orientB = ZERO<Quat>();

    DPoint3 rA = ZERO<DPoint3>();
    DPoint3 rB = ZERO<DPoint3>();

    double massA = 0.0;
    double massB = 0.0;

    double frictionA = 0.0;
    double frictionB = 0.0;

    double bouncingA = 0.0;
    double bouncingB = 0.0;

    DPoint3 inertiaA = ZERO<DPoint3>();
    DPoint3 inertiaB = ZERO<DPoint3>();
  };

  struct Cache
  {
    struct Pair;

    ContactSolver &solver;
    Tab<Pair> pairs;
    Cache(Cache &&);
    ~Cache();
    Cache(ContactSolver &set_solver);

    void update();
    void getManifold(Tab<ContactManifoldPoint> &manifold);
    void clearData();
    void clearData(int index);
  };

  struct BodyState
  {
    bool isKinematic;
    bool shouldCheckContactWithGround;

    float timeToSleep;

    double invMass;
    DPoint3 invInertia;

    gamephys::Loc location;
    DPoint3 velocity;
    DPoint3 omega;
    DPoint3 addVelocity;
    DPoint3 addOmega;

    // appliedForce = B * lambda = inverse(M) * transpose(J) * lambda
    carray<DPoint3, 2> appliedForce;

    void accumulateForce(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir);
    void applyImpulse(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir);
    void applyPseudoImpulse(double lambda, const DPoint3 &linear_dir, const DPoint3 &angular_dir);
  };

  struct Constraint
  {
    enum class Type
    {
      Unknown,
      Friction1,
      Friction2,
      Contact
    };

    Type type = Type::Unknown;

    double lambda = 0.0;
    double lambdaMin = -1e10;
    double lambdaMax = 1e10;
    double beta = 0.0;
    double bias = 0.0;

    double bouncing = 0.0;

    // J * B = J * inv(M) * transpose(J)
    double JB = 0.0;
    double invJB = 0.0;

    carray<DPoint3, 4> J;

    int indexInCache = -1;
    int cacheId = -1;

    int indexA = -1;
    int indexB = -1;

    DPoint3 localPosA = ZERO<DPoint3>();
    DPoint3 localPosB = ZERO<DPoint3>();
    DPoint3 normal = ZERO<DPoint3>();

    Constraint() { mem_set_0(J); }

    void init(const BodyState &state_a, const BodyState &state_b);
  };

  int atTick = 0;

  Cache cache;

  Tab<Body> bodies;
  Tab<BodyState> bodyStates;
  Tab<Constraint> constraints;

  BodyState groundState;

  static const size_t MAX_LAYERS_NUM = 4;
  eastl::bitset<MAX_LAYERS_NUM * MAX_LAYERS_NUM> layersMask;

  BodyState &getState(int index);
  void addFrictionConstraint(Constraint::Type type, const ContactManifoldPoint &info, double lambda, double friction,
    const DPoint3 &u);
  void addContactConstraint(const ContactManifoldPoint &info, double beta, double bias);

  void checkStaticCollisions(int body_no, const ContactSolver::Body &body, const TMatrix &tm, dag::Span<CollisionObject> coll_objects);

  void integrateVelocity(double dt);
  void integratePositions(double dt);

  void addContact(int index_a, int index_b, gamephys::CollisionContactData &contact);
  void initVelocityConstraints();
  void initPositionConstraints();

  void solveVelocityConstraints(double dt);
  bool solvePositionConstraints();

public:
  enum Flags
  {
    ProcessGround = 1 << 0,
    ProcessFriction = 1 << 1,
    ProcessBody = 1 << 2,
    Sleepable = 1 << 3,
    Kinematic = 1 << 4,
    ProcessAll = (0xFFFFFFFF & ~Flags::Kinematic)
  };

  enum class UpdatePolicy
  {
    UpdateByTicks,
    UpdateAlways
  };

  enum SolverFlags
  {
    SolvePositions = 1 << 0,
    SolveBouncing = 1 << 1,
    SolveAll = 0xFFFFFFFF
  };

  uint32_t solverFlags = SolverFlags::SolveAll;

  UpdatePolicy updatePolicy = UpdatePolicy::UpdateByTicks;

  ContactSolver();
  ~ContactSolver();

  void addBody(IPhysBase *phys, float bounding_radius, uint32_t flags = Flags::ProcessAll, int layer = 0);
  void removeBody(IPhysBase *phys);
  void setBodyFlags(IPhysBase *phys, uint32_t flags);

  void setLayersMask(int layer_a, int layer_b, bool should_solve);
  bool getLayersMask(int layer_a, int layer_b) const;

  void wakeUpBodiesInRadius(const DPoint3 &pos, float radius);

  void clearData();
  void clearData(int index);

  void update(double at_time, double dt);
  bool isContactBetween(IPhysBase *bodyA, IPhysBase *bodyB) const;

  void drawDebugCollisions();
  void drawDebugContacts();
};
