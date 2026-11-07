// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys/dag_vehicle.h>
#include <phys/dag_physics.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <debug/dag_log.h>
#include <Jolt/Physics/Vehicle/MotorcycleController.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>

class JoltMotorcycle final : public IPhysMotorcycle
{
public:
  JoltMotorcycle(JPH::Body &body, const Desc &desc)
  {
    using namespace JPH;
    const Settings &s = desc.settings;

    VehicleConstraintSettings vehicle;
    vehicle.mForward = Vec3::sAxisX();
    vehicle.mMaxPitchRollAngle = maxPitchRollRad = s.maxPitchRollAngleDeg * DEG_TO_RAD;
    vehicle.mWheels = {makeWheel(s.frontWheel), makeWheel(s.rearWheel)};

    MotorcycleControllerSettings *controller = new MotorcycleControllerSettings;
    controller->mEngine.mMaxTorque = s.engine.maxTorque;
    controller->mEngine.mMinRPM = s.engine.minRPM;
    controller->mEngine.mMaxRPM = s.engine.maxRPM;
    controller->mTransmission.mShiftDownRPM = s.transmission.shiftDownRPM;
    controller->mTransmission.mShiftUpRPM = s.transmission.shiftUpRPM;
    controller->mTransmission.mClutchStrength = s.transmission.clutchStrength;
    if (!desc.gearRatios.empty())
      controller->mTransmission.mGearRatios.assign(desc.gearRatios.begin(), desc.gearRatios.end());
    if (!desc.reverseGearRatios.empty())
      controller->mTransmission.mReverseGearRatios.assign(desc.reverseGearRatios.begin(), desc.reverseGearRatios.end());
    // the controller applies engine torque only through differentials,
    // so a single one driving the rear wheel is required even for a motorcycle
    controller->mDifferentials.resize(1);
    controller->mDifferentials[0].mLeftWheel = -1;
    controller->mDifferentials[0].mRightWheel = 1;
    controller->mDifferentials[0].mDifferentialRatio = s.differentialRatio;
    controller->mMaxLeanAngle = s.maxLeanAngleDeg * DEG_TO_RAD;
    controller->mLeanSpringConstant = s.leanSpring;
    controller->mLeanSpringDamping = s.leanDamping;
    vehicle.mController = controller;

    constraint = new VehicleConstraint(body, vehicle);
    // cast a rounded cylinder (convex radius = half wheel width) instead of ray probes
    constraint->SetVehicleCollisionTester(new VehicleCollisionTesterCastCylinder(body.GetObjectLayer(), 1.0f));
    jolt_api::phys_sys().AddConstraint(constraint);
    jolt_api::phys_sys().AddStepListener(constraint);

    initBikeGeometrySetup(geometrySetup, desc);
  }

  ~JoltMotorcycle()
  {
    jolt_api::phys_sys().RemoveStepListener(constraint);
    jolt_api::phys_sys().RemoveConstraint(constraint);
  }

  void destroyVehicle() override { delete this; }

  void activate() override { jolt_api::body_api().ActivateBody(constraint->GetVehicleBody()->GetID()); }

  void setDriverInput(float forward, float left, float brake, float hand_brake) override
  {
    auto lock = lockChassis();
    // Jolt's right is right in its right-handed frame, which is left in Dagor's left-handed one
    getController().SetDriverInput(forward, left, brake, hand_brake);
  }

  void enableLeanController(bool enable) override
  {
    auto lock = lockChassis();
    getController().EnableLeanController(enable);
    constraint->SetMaxPitchRollAngle(enable ? maxPitchRollRad : JPH::JPH_PI);
  }

  void setGravity(const Point3 &gravity) override
  {
    auto lock = lockChassis();
    constraint->OverrideGravity(to_jVec3(gravity));
    worldUp = (-to_jVec3(gravity)).NormalizedOr(worldUp);
  }

  float getAbsSinLeanAngle() const override
  {
    JPH::Quat rot = jolt_api::body_api().GetRotation(constraint->GetVehicleBody()->GetID()); // locked read, step may run
    JPH::Vec3 up = rot * constraint->GetLocalUp();
    JPH::Vec3 fwd = rot * constraint->GetLocalForward();
    return fabsf(worldUp.Cross(up).Dot(fwd));
  }

  bool hasWheelContact(int wheel) const override { return constraint->GetWheel(wheel)->HasContact(); }

  RunningGearState getRunningGearState() const override
  {
    RunningGearState state;
    state.steerAngle = constraint->GetWheel(0)->GetSteerAngle();
    for (int i = 0; i < 2; i++)
    {
      state.suspensionLength[i] = constraint->GetWheel(i)->GetSuspensionLength();
      state.rotationAngle[i] = constraint->GetWheel(i)->GetRotationAngle();
      state.angularVelocity[i] = constraint->GetWheel(i)->GetAngularVelocity();
    }
    return state;
  }

  void setPreStepCallback(PreStepCallback cb, void *ctx) override
  {
    auto lock = lockChassis();
    if (cb)
      constraint->SetPreStepCallback(
        [cb, ctx](JPH::VehicleConstraint &, const JPH::PhysicsStepListenerContext &step) { cb(ctx, step.mDeltaTime); });
    else
      constraint->SetPreStepCallback(nullptr);
  }

  const BikeGeometrySetup &getBikeGeometrySetup() const override { return geometrySetup; }
  void syncNodes(GeomNodeTree &tree) override { poseNodes(tree, geometrySetup, getRunningGearState()); }

private:
  JPH::MotorcycleController &getController() { return *static_cast<JPH::MotorcycleController *>(constraint->GetController()); }
  // a step reads the controller inputs while it holds all body locks, so the chassis lock serializes input writes with it
  JPH::BodyLockWrite lockChassis() const { return JPH::BodyLockWrite(jolt_api::body_lock(), constraint->GetVehicleBody()->GetID()); }

  static JPH::WheelSettingsWV *makeWheel(const WheelSettings &w)
  {
    auto *jw = new JPH::WheelSettingsWV;
    jw->mPosition = to_jVec3(w.position);
    jw->mWheelForward = JPH::Vec3::sAxisX();
    jw->mSuspensionDirection = to_jVec3(normalize(w.suspensionDirection));
    jw->mSteeringAxis = to_jVec3(normalize(w.steeringAxis));
    jw->mRadius = w.radius;
    jw->mWidth = w.width;
    jw->mSuspensionMinLength = w.suspensionMinLength;
    jw->mSuspensionMaxLength = w.suspensionMaxLength;
    jw->mSuspensionSpring.mFrequency = w.suspensionFrequency;
    jw->mSuspensionSpring.mDamping = w.suspensionDamping;
    jw->mMaxBrakeTorque = w.maxBrakeTorque;
    jw->mMaxHandBrakeTorque = w.maxHandBrakeTorque;
    jw->mMaxSteerAngle = w.maxSteerAngleDeg * DEG_TO_RAD;
    if (w.overrideSuspensionForcePoint)
    {
      jw->mEnableSuspensionForcePoint = true;
      jw->mSuspensionForcePoint = jw->mPosition + jw->mSuspensionDirection * jw->mSuspensionMinLength;
    }
    return jw;
  }

  JPH::Ref<JPH::VehicleConstraint> constraint;
  // the step writes the constraint world up, so the game thread reads this copy
  JPH::Vec3 worldUp = -jolt_api::phys_sys().GetGravity().Normalized();
  float maxPitchRollRad = 0;
  BikeGeometrySetup geometrySetup;
};

// model space is the root space: the caller places the root, as animChar does from its transform
static TMatrix rest_tm_in_model(const GeomNodeTree &tree, dag::Index16 node)
{
  TMatrix tm = TMatrix::IDENT;
  for (; node && tree.getParentNodeIdx(node); node = tree.getParentNodeIdx(node))
  {
    TMatrix ntm;
    tree.getNodeTmScalar(node, ntm);
    tm = ntm * tm;
  }
  return tm;
}

IPhysMotorcycle *IPhysMotorcycle::createJolt(PhysBody *chassis, const Desc &desc)
{
  if (!chassis || !chassis->isValid())
    return nullptr;
  // no body lock, as for joints: a running step holds the mutexes the ctor takes and then waits for body locks
  JPH::Body *body = jolt_api::body_lock().TryGetBody(chassis->bodyId);
  return body ? new JoltMotorcycle(*body, desc) : nullptr;
}

// two chains: steer, front travel, front spin; rear travel, rear spin
enum
{
  MOTION_STEER,
  MOTION_TRAVEL_FRONT,
  MOTION_SPIN_FRONT,
  MOTION_TRAVEL_REAR,
  MOTION_SPIN_REAR,
  MOTION_COUNT
};
static constexpr int chainEnd[] = {MOTION_TRAVEL_FRONT, MOTION_SPIN_FRONT, MOTION_TRAVEL_REAR, MOTION_COUNT};

void IPhysMotorcycle::poseNodes(GeomNodeTree &tree, const BikeGeometrySetup &setup, const RunningGearState &state)
{
  if (setup.empty())
    return;
  using namespace JPH;
  // node motion in chassis space, as in VehicleConstraint::GetWheelLocalTransform but with the spin negated:
  // its turn about forward x up rolls the wheel backward; loadDesc() puts each hub at its wheel node origin
  Mat44 motion[MOTION_COUNT];
  for (int i = 0; i < 2; i++)
  {
    Vec3 hub = to_jVec3(setup.nodes[2 + i].restInChassis.getcol(3));
    motion[MOTION_TRAVEL_FRONT + 2 * i] =
      Mat44::sTranslation(to_jVec3(setup.suspensionDirection[i]) * (state.suspensionLength[i] - setup.restLength[i]));
    motion[MOTION_SPIN_FRONT + 2 * i] =
      Mat44::sTranslation(hub) * Mat44::sRotation(Vec3::sAxisZ(), -state.rotationAngle[i]) * Mat44::sTranslation(-hub);
  }
  // steering turns about the steering axis through the node origin; the solver keeps the hub fixed,
  // so the rendered hub swings by the fork offset (a few cm at full lock)
  Vec3 pivot = to_jVec3(setup.nodes[0].restInChassis.getcol(3));
  motion[MOTION_STEER] =
    Mat44::sTranslation(pivot) * Mat44::sRotation(to_jVec3(setup.steeringAxis), state.steerAngle) * Mat44::sTranslation(-pivot);

  for (int i = 0; i < countof(setup.nodes); i++)
  {
    const BikeGeometrySetup::Node &n = setup.nodes[i];
    Mat44 own = motion[n.motionBegin];
    for (int m = n.motionBegin + 1; m < chainEnd[i]; m++)
      own = own * motion[m];
    tree.setNodeTmScalar(n.node, n.chassisInParent * to_tmatrix(own) * n.restInChassis);
    tree.invalidateWtm(n.node.preceeding()); // the wtm stays valid up to the index given, so it is the node before
  }
}

// relative to the body node, because an export may mirror it (a Y/Z swap), and with it every node below
static void check_basis(const GeomNodeTree &tree, dag::Index16 node, const TMatrix &body_tm, const TMatrix &tm)
{
  TMatrix rel = inverse(body_tm) * tm;
  bool ok = rel.det() > 0.f;
  for (int i = 0; i < 3; i++)
    ok = ok && fabsf(length(rel.getcol(i)) - 1.f) < 1e-3f && fabsf(rel.getcol(i) * rel.getcol((i + 1) % 3)) < 1e-3f;
  if (!ok)
    logerr("IPhysMotorcycle::loadDesc: node <%s> basis is not orthonormal, or is mirrored relative to the body node",
      tree.getNodeName(node));
}

// inverts Jolt's spring sizing (effective mass at the mid-travel point, suspension tilt; VehicleConstraint::SetupVelocityConstraint),
// so the static load compresses the spring by half its travel
static float spring_frequency_for_mid_travel(const IPhysMotorcycle::Desc &desc, const IPhysMotorcycle::WheelSettings &w, float load)
{
  float halfTravel = 0.5f * (w.suspensionMaxLength - w.suspensionMinLength);
  Point3 forcePoint = w.position + w.suspensionDirection * (0.5f * (w.suspensionMinLength + w.suspensionMaxLength));
  Point3 a = forcePoint % Point3(0, -1, 0);
  const Point3 &i = desc.inertia;
  float effectiveMass = 1.f / (1.f / desc.mass + sqr(a.x) / i.x + sqr(a.y) / i.y + sqr(a.z) / i.z);
  float stiffness = load * max(0.1f, -w.suspensionDirection.y) / halfTravel;
  return sqrtf(stiffness / effectiveMass) / TWOPI;
}

// a missing block keeps the physics engine defaults, a present one must give at least one gear
static void load_ratios(const DataBlock *blk, dag::Vector<float> &ratios)
{
  if (!blk)
    return;
  int ratioNid = blk->getNameId("ratio");
  for (uint32_t i = 0; i < blk->paramCount(); i++)
    if (blk->getParamNameId(i) == ratioNid && blk->getParamType(i) == DataBlock::TYPE_REAL)
      ratios.push_back(blk->getReal(i));
  if (ratios.empty())
    logerr("IPhysMotorcycle::loadDesc: <%s> holds no real 'ratio'", blk->getBlockName());
}

// the motion of a node is a chain prefix; that of its nearest moving ancestor must be a shorter one of the same chain
bool IPhysMotorcycle::initBikeGeometrySetup(BikeGeometrySetup &dest_setup, const Desc &desc)
{
  dest_setup = BikeGeometrySetup();
  dest_setup.chassisInModel = desc.chassisInModel;
  if (!desc.restTree)
    return false;
  const GeomNodeTree &tree = *desc.restTree;
  const dag::Index16 nodes[] = {desc.nodeSteering, desc.nodeSuspensionFront, desc.nodeWheelFront, desc.nodeWheelRear};
  const int chainBegin[] = {MOTION_STEER, MOTION_STEER, MOTION_STEER, MOTION_TRAVEL_REAR};
  const TMatrix modelToChassis = inverse(desc.chassisInModel);
  BikeGeometrySetup setup = dest_setup;
  for (int i = 0; i < countof(nodes); i++)
  {
    int ancestor = -1;
    for (dag::Index16 p = tree.getParentNodeIdx(nodes[i]); p && ancestor < 0; p = tree.getParentNodeIdx(p))
      for (int j = 0; j < countof(nodes); j++)
        if (nodes[j] == p)
          ancestor = j;
    if (ancestor >= 0 && (chainBegin[ancestor] != chainBegin[i] || chainEnd[ancestor] >= chainEnd[i]))
    {
      logerr("IPhysMotorcycle::initBikeGeometrySetup: node <%s> is below the moving node <%s> but does not follow all its motion;"
             " the nodes keep the rest pose",
        tree.getNodeName(nodes[i]), tree.getNodeName(nodes[ancestor]));
      return false;
    }
    BikeGeometrySetup::Node &n = setup.nodes[i];
    n.node = nodes[i];
    n.motionBegin = ancestor >= 0 ? chainEnd[ancestor] : chainBegin[i];
    n.chassisInParent = inverse(rest_tm_in_model(tree, tree.getParentNodeIdx(nodes[i]))) * desc.chassisInModel;
    n.restInChassis = modelToChassis * rest_tm_in_model(tree, nodes[i]);
  }
  // as makeWheel() gives them to the physics engine
  for (int i = 0; i < 2; i++)
  {
    const WheelSettings &w = i ? desc.settings.rearWheel : desc.settings.frontWheel;
    setup.suspensionDirection[i] = normalize(w.suspensionDirection);
    setup.restLength[i] = 0.5f * (w.suspensionMinLength + w.suspensionMaxLength);
  }
  setup.steeringAxis = normalize(desc.settings.frontWheel.steeringAxis);
  dest_setup = setup;
  return true;
}

bool IPhysMotorcycle::loadDesc(const DataBlock &blk, const GeomNodeTree &tree, Desc &desc)
{
  bool ok = true;
  desc = Desc();
  auto mandatory = [&](const DataBlock &b, const char *name) {
    if (!b.paramExists(name))
    {
      logerr("IPhysMotorcycle::loadDesc: mandatory <%s> is missing", name);
      ok = false;
    }
  };
  auto find_node = [&](const char *param, const char *def_name) {
    const char *name = blk.getStr(param, def_name);
    dag::Index16 node = tree.findNodeIndex(name);
    if (!node)
    {
      logerr("IPhysMotorcycle::loadDesc: skeleton has no node <%s>", name);
      ok = false;
    }
    return node;
  };
  desc.nodeBody = find_node("bodyNode", "body");
  desc.nodeSteering = find_node("steeringNode", "steering");
  desc.nodeSuspensionFront = find_node("suspensionFrontNode", "suspension_front");
  desc.nodeWheelFront = find_node("wheelFrontNode", "wheel_front");
  desc.nodeWheelRear = find_node("wheelRearNode", "wheel_rear");
  const DataBlock &frontBlk = *blk.getBlockByNameEx("frontWheel"), &rearBlk = *blk.getBlockByNameEx("rearWheel");
  mandatory(blk, "mass");
  mandatory(blk, "inertia");
  mandatory(blk, "box");
  mandatory(frontBlk, "radius");
  mandatory(rearBlk, "radius");
  if (!ok)
    return false;

  desc.mass = blk.getReal("mass");
  desc.inertia = blk.getPoint3("inertia");
  desc.boxSize = blk.getPoint3("box");
  desc.comOffset = blk.getPoint3("comOffset", Point3(0, 0, 0));
  if (desc.mass <= 0.f || desc.inertia.x <= 0.f || desc.inertia.y <= 0.f || desc.inertia.z <= 0.f)
    logerr("IPhysMotorcycle::loadDesc: mass %g and inertia (%g %g %g) must be positive", desc.mass, P3D(desc.inertia));

  TMatrix bodyTm = rest_tm_in_model(tree, desc.nodeBody), steeringTm = rest_tm_in_model(tree, desc.nodeSteering),
          suspensionTm = rest_tm_in_model(tree, desc.nodeSuspensionFront), frontTm = rest_tm_in_model(tree, desc.nodeWheelFront),
          rearTm = rest_tm_in_model(tree, desc.nodeWheelRear);
  for (dag::Index16 node : {desc.nodeSteering, desc.nodeSuspensionFront, desc.nodeWheelFront, desc.nodeWheelRear})
    check_basis(tree, node, bodyTm, rest_tm_in_model(tree, node));

  // chassis frame: model up, forward from the rear hub to the front hub, origin at the box center
  const Point3 up(0, 1, 0);
  Point3 forward = frontTm.getcol(3) - rearTm.getcol(3);
  forward.y = 0.f;
  forward.normalize();
  Point3 boxCenter = blk.getPoint3("boxCenter", Point3(0, 0, 0)); // in chassis axes, from the model origin
  TMatrix &chassis = desc.chassisInModel;
  chassis.setcol(0, forward);
  chassis.setcol(1, up);
  chassis.setcol(2, forward % up);
  chassis.setcol(3, forward * boxCenter.x + up * boxCenter.y + chassis.getcol(2) * boxCenter.z);
  TMatrix toChassis = inverse(chassis);

  Point3 steeringAxis = normalize(toChassis % steeringTm.getcol(2));
  if (fabsf(steeringAxis.z) > 0.01f || steeringAxis.y < 0.5f || steeringAxis.x > 0.01f)
    logerr("IPhysMotorcycle::loadDesc: node <%s> local Z should be the steering axis, up and tilted back in the symmetry plane;"
           " it is (%g %g %g) in chassis axes",
      tree.getNodeName(desc.nodeSteering), P3D(steeringAxis));
  if (normalize(toChassis % suspensionTm.getcol(2)) * steeringAxis < 0.999f)
    logerr("IPhysMotorcycle::loadDesc: node <%s> local Z should run along the steering axis",
      tree.getNodeName(desc.nodeSuspensionFront));

  Settings &s = desc.settings;
  auto load_wheel = [&](WheelSettings &w, const DataBlock &wblk, dag::Index16 node, const TMatrix &tm, const Point3 &dir,
                      float def_steer_deg) {
    Point3 hub = toChassis * tm.getcol(3), axle = normalize(toChassis % tm.getcol(1));
    if (fabsf(axle.x) > 0.05f || fabsf(axle.y) > 0.05f)
      logerr("IPhysMotorcycle::loadDesc: node <%s> local Y should be the wheel axle, it is (%g %g %g) in chassis axes",
        tree.getNodeName(node), P3D(axle));
    if (fabsf(hub.z) > 0.01f)
      logerr("IPhysMotorcycle::loadDesc: node <%s> is %g off the symmetry plane", tree.getNodeName(node), hub.z);
    float restLength = wblk.getReal("restLength", 0.3f), halfTravel = wblk.getReal("halfTravel", 0.1f);
    if (restLength <= halfTravel || halfTravel <= 0.f)
      logerr("IPhysMotorcycle::loadDesc: wheel node <%s> needs restLength %g > halfTravel %g > 0", tree.getNodeName(node), restLength,
        halfTravel);
    w.suspensionDirection = dir;
    w.position = hub - dir * restLength;
    w.suspensionMinLength = restLength - halfTravel;
    w.suspensionMaxLength = restLength + halfTravel;
    w.radius = wblk.getReal("radius");
    w.width = wblk.getReal("width", w.width);
    w.suspensionDamping = wblk.getReal("damping", w.suspensionDamping);
    w.maxBrakeTorque = wblk.getReal("maxBrakeTorque", w.maxBrakeTorque);
    w.maxHandBrakeTorque = wblk.getReal("maxHandBrakeTorque", w.maxHandBrakeTorque);
    w.maxSteerAngleDeg = wblk.getReal("maxSteerAngle", def_steer_deg);
    return hub;
  };
  s.frontWheel.steeringAxis = steeringAxis;
  Point3 frontHub = load_wheel(s.frontWheel, frontBlk, desc.nodeWheelFront, frontTm, -steeringAxis, 30.f);
  Point3 rearHub = load_wheel(s.rearWheel, rearBlk, desc.nodeWheelRear, rearTm, Point3(0, -1, 0), 0.f);
  if (fabsf((frontHub.y - s.frontWheel.radius) - (rearHub.y - s.rearWheel.radius)) > 0.02f)
    logerr("IPhysMotorcycle::loadDesc: the wheels touch the ground at different heights, %g front and %g rear",
      frontHub.y - s.frontWheel.radius, rearHub.y - s.rearWheel.radius);

  // static wheel loads by the lever rule, then spring frequencies for a rest at mid travel
  float gravity = jolt_api::phys_sys().GetGravity().Length();
  float wheelBase = frontHub.x - rearHub.x;
  if (!(desc.comOffset.x > rearHub.x && desc.comOffset.x < frontHub.x)) // also true for NaN hubs from coincident wheel nodes
    logerr("IPhysMotorcycle::loadDesc: COM x %g is not between the hubs, %g rear and %g front", desc.comOffset.x, rearHub.x,
      frontHub.x);
  float frontLoad = desc.mass * gravity * (desc.comOffset.x - rearHub.x) / wheelBase;
  s.frontWheel.suspensionFrequency = spring_frequency_for_mid_travel(desc, s.frontWheel, max(frontLoad, 1.f));
  s.rearWheel.suspensionFrequency = spring_frequency_for_mid_travel(desc, s.rearWheel, max(desc.mass * gravity - frontLoad, 1.f));

  const DataBlock &engineBlk = *blk.getBlockByNameEx("engine");
  s.engine.maxTorque = engineBlk.getReal("maxTorque", s.engine.maxTorque);
  s.engine.minRPM = engineBlk.getReal("minRPM", s.engine.minRPM);
  s.engine.maxRPM = engineBlk.getReal("maxRPM", s.engine.maxRPM);
  const DataBlock &transmissionBlk = *blk.getBlockByNameEx("transmission");
  s.transmission.shiftUpRPM = transmissionBlk.getReal("shiftUpRPM", s.transmission.shiftUpRPM);
  s.transmission.shiftDownRPM = transmissionBlk.getReal("shiftDownRPM", s.transmission.shiftDownRPM);
  s.transmission.clutchStrength = transmissionBlk.getReal("clutchStrength", s.transmission.clutchStrength);
  load_ratios(transmissionBlk.getBlockByName("gearRatios"), desc.gearRatios);
  load_ratios(transmissionBlk.getBlockByName("reverseGearRatios"), desc.reverseGearRatios);
  s.differentialRatio = blk.getReal("differentialRatio", s.differentialRatio);
  // past this pitch the mass center is behind the rear contact, so a wheelie resting against a higher limit would never come down
  float wheelieDeg = atan2f(desc.comOffset.x - rearHub.x, desc.comOffset.y - (rearHub.y - s.rearWheel.radius)) * RAD_TO_DEG;
  s.maxPitchRollAngleDeg = blk.getReal("maxPitchRollAngle", wheelieDeg - 3.f);
  s.maxLeanAngleDeg = blk.getReal("maxLeanAngle", s.maxLeanAngleDeg);
  s.leanSpring = blk.getReal("leanSpring", s.leanSpring);
  s.leanDamping = blk.getReal("leanDamping", s.leanDamping);
  desc.restTree = &tree; // only a complete desc poses the tree
  return true;
}
