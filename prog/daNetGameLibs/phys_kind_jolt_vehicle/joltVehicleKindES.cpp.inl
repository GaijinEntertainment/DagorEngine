// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys_kind/physKind.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/anim/animComponent.h>
#include <gamePhys/collision/collisionLib.h>
#include <ioSys/dag_dataBlock.h>
#include <phys/dag_physics.h>
#include <phys/dag_vehicle.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include "phys/physEvents.h"
#include <EASTL/algorithm.h>
#include <EASTL/array.h>

// Remote copies read it from the state ext
struct JoltVehicleControls
{
  int8_t forward;
  int8_t right;
  uint8_t brake;
  uint8_t handBrake;
};

// Zero is the rest pose; a standing bike's zero speeds and steer come last, so the wire trims them
struct JoltVehicleStateExt
{
  JoltVehicleControls controls;
  int8_t travel[2];
  int16_t angularVelocity[2];
  int8_t steer;
};

struct BikeGeometry
{
  IPhysMotorcycle::BikeGeometrySetup setup;
  float halfTravel[2] = {};
  float maxSteer = 0.f;
};

static const float WAKE_GRAVITY_TURN_COS = cosf(DegToRad(1.f));

static float half_travel(const IPhysMotorcycle::WheelSettings &w) { return 0.5f * (w.suspensionMaxLength - w.suspensionMinLength); }

static BikeGeometry make_bike_geometry(const IPhysMotorcycle::Desc &desc)
{
  const IPhysMotorcycle::Settings &s = desc.settings;
  BikeGeometry geometry = {{}, {half_travel(s.frontWheel), half_travel(s.rearWheel)}, DegToRad(s.frontWheel.maxSteerAngleDeg)};
  IPhysMotorcycle::initBikeGeometrySetup(geometry.setup, desc);
  return geometry;
}

static void write_bike_wheels(JoltVehicleStateExt &ext, const BikeGeometry &geometry, const IPhysMotorcycle::RunningGearState &w)
{
  const auto toByte = [](float unit) { return int8_t(lrintf(clamp(unit, -1.f, 1.f) * 127.f)); };
  ext.steer = toByte(safediv(w.steerAngle, geometry.maxSteer));
  for (int i = 0; i < 2; i++)
  {
    ext.angularVelocity[i] = int16_t(lrintf(clamp(w.angularVelocity[i] * 64.f, -32767.f, 32767.f)));
    ext.travel[i] = toByte(safediv(w.suspensionLength[i] - geometry.setup.restLength[i], geometry.halfTravel[i]));
  }
}

static IPhysMotorcycle::RunningGearState read_bike_wheels(const JoltVehicleStateExt &ext, const BikeGeometry &geometry)
{
  IPhysMotorcycle::RunningGearState w;
  w.steerAngle = ext.steer / 127.f * geometry.maxSteer;
  for (int i = 0; i < 2; i++)
  {
    w.angularVelocity[i] = ext.angularVelocity[i] / 64.f;
    w.suspensionLength[i] = geometry.setup.restLength[i] + ext.travel[i] / 127.f * geometry.halfTravel[i];
  }
  return w;
}

// suspension: min and max length, frequency, damping
static JPH::WheelSettingsWV *make_wheel(
  JPH::Vec3Arg mid_pos, float radius, float width, float max_steer, JPH::Vec3Arg suspension_dir, const Point4 &suspension)
{
  auto *wheel = new JPH::WheelSettingsWV;
  wheel->mPosition = mid_pos - suspension_dir * (0.5f * (suspension.x + suspension.y));
  wheel->mWheelForward = JPH::Vec3::sAxisX();
  wheel->mSuspensionDirection = suspension_dir;
  wheel->mSteeringAxis = -suspension_dir;
  wheel->mSuspensionMinLength = suspension.x;
  wheel->mSuspensionMaxLength = suspension.y;
  wheel->mSuspensionSpring = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, suspension.z, suspension.w);
  wheel->mRadius = radius;
  wheel->mWidth = width;
  wheel->mMaxSteerAngle = max_steer;
  return wheel;
}

static IPhysMotorcycle::WheelSettings make_bike_wheel(
  const Point3 &mid_pos, float radius, float max_steer_deg, const Point3 &suspension_dir, const Point4 &suspension)
{
  IPhysMotorcycle::WheelSettings wheel;
  wheel.position = mid_pos - suspension_dir * (0.5f * (suspension.x + suspension.y));
  wheel.suspensionDirection = suspension_dir;
  wheel.steeringAxis = -suspension_dir;
  wheel.radius = radius;
  wheel.width = 0.05f;
  wheel.suspensionMinLength = suspension.x;
  wheel.suspensionMaxLength = suspension.y;
  wheel.suspensionFrequency = suspension.z;
  wheel.suspensionDamping = suspension.w;
  // As the car wheels
  wheel.maxBrakeTorque = 1500.f;
  wheel.maxSteerAngleDeg = max_steer_deg;
  return wheel;
}

// The skeleton places the chassis and the wheels, the BLK motorcycle{} holds the rest
static bool load_bike_desc(ecs::EntityId eid, IPhysMotorcycle::Desc &desc)
{
  const char *blkName = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__bikeDesc"), ecs::nullstr);
  if (!blkName || !*blkName)
    return false;
  const auto *animchar = g_entity_mgr->getNullable<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
  if (!animchar || !animchar->getOriginalNodeTree())
  {
    logerr("jolt_vehicle: <%s> has no animchar skeleton for %s", g_entity_mgr->getEntityTemplateName(eid), blkName);
    return false;
  }
  DataBlock blk;
  return blk.load(blkName) && IPhysMotorcycle::loadDesc(*blk.getBlockByNameEx("motorcycle"), *animchar->getOriginalNodeTree(), desc);
}

class JoltVehicleKind
{
public:
  ~JoltVehicleKind() { destroy(); }

  bool needsRecreate(bool dynamic) const { return !body || isDynamic() != dynamic; }
  bool isDynamic() const { return constraint || bike; }
  float getMass() const { return body->getMass(); }

  void create(ecs::EntityId eid, const TMatrix &tm, bool dynamic)
  {
    destroy();
    PhysWorld *world = dacoll::get_phys_world();
    if (!world)
      return;
    const bool motorcycle = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__motorcycle"), false);
    const Point3 half = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__halfSize"), Point3(2.f, 0.3f, 0.9f));
    const float mass = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__mass"), 1200.f);
    const float wheelRadius = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__wheelRadius"), 0.35f);
    const Point3 centerOfMass = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__centerOfMass"), Point3(0.f, -half.y, 0.f));
    const Point4 suspension = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__suspension"), Point4(0.1f, 0.35f, 1.6f, 0.5f));
    const float maxSteerDeg = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__maxSteerDeg"), 30.f);

    const float caster = DegToRad(30.f);
    // Bike wheels outside the box sink into walls; the caster moves the front one on its travel
    const float bikeReach = 2.f * wheelRadius + 0.5f * sinf(caster) * (suspension.y - suspension.x);
    IPhysMotorcycle::Desc desc;
    const bool skeletal = motorcycle && load_bike_desc(eid, desc);
    // A copy that stops or starts simulating keeps its wheel pose
    const bool hadBikeGeometry = !bikeGeometry.setup.empty();
    bikeGeometry = make_bike_geometry(desc);
    if (!hadBikeGeometry)
      shownWheels = read_bike_wheels(JoltVehicleStateExt{}, bikeGeometry);
    // A new solver wheel spins from 0
    for (int i = 0; i < 2; i++)
      spinOffset[i] = shownWheels.rotationAngle[i];
    if (!skeletal)
    {
      desc.mass = mass;
      desc.comOffset = centerOfMass;
      desc.boxSize = Point3((half.x + (motorcycle ? bikeReach : 0.f)) * 2.f, half.y * 2.f, half.z * 2.f);
    }
    PhysBoxCollision box(desc.boxSize.x, desc.boxSize.y, desc.boxSize.z);
    PhysBodyCreationData creation;
    creation.autoInertia = lengthSq(desc.inertia) == 0.f;
    creation.momentOfInertia = desc.inertia;
    creation.kinematic = !dynamic;
    creation.linearDamping = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__linearDamping"), 0.05f);
    creation.centerOfMassOffset = desc.comOffset;
    const TMatrix &chassisInModel = bikeGeometry.setup.chassisInModel;
    modelInChassis = inverse(chassisInModel);
    body = eastl::make_unique<PhysBody>(world, dynamic ? desc.mass : 0.f, &box, tm * chassisInModel, creation);
    if (!body->isValid())
    {
      body.reset();
      return;
    }
    // The state moves the model root
    comOffset = chassisInModel * desc.comOffset;
    if (!dynamic)
      return;
    body->setContinuousCollisionMode(true);

    const float maxPitchRollDeg = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__maxPitchRollDeg"), 40.f);
    const float maxTorque = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__maxTorque"), 500.f);
    const float differentialRatio = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__differentialRatio"), 3.42f);
    maxPitchRoll = DegToRad(maxPitchRollDeg);
    isUpright = true;
    bikeStepped = false;
    recentLags.fill(1.f);
    const float wheelY = -0.9f * half.y;
    if (motorcycle)
    {
      IPhysMotorcycle::Settings &s = desc.settings;
      if (!skeletal)
      {
        const Point3 casterDir(sinf(caster), -cosf(caster), 0.f);
        s.frontWheel = make_bike_wheel(Point3(half.x + wheelRadius, wheelY, 0.f), wheelRadius, maxSteerDeg, casterDir, suspension);
        s.rearWheel =
          make_bike_wheel(Point3(-half.x - wheelRadius, wheelY, 0.f), wheelRadius, 0.f, Point3(0.f, -1.f, 0.f), suspension);
        // The rear wheel locks for a slide
        s.rearWheel.maxHandBrakeTorque = 4000.f;
        s.engine.maxTorque = maxTorque;
        s.differentialRatio = differentialRatio;
        s.maxPitchRollAngleDeg = maxPitchRollDeg;
        // One gear: max RPM caps speed
        desc.gearRatios = {2.27f};
        desc.reverseGearRatios = {-4.f};
      }
      springQuarterPeriod = 0.25f / min(s.frontWheel.suspensionFrequency, s.rearWheel.suspensionFrequency);
      bike = IPhysMotorcycle::createJolt(body.get(), desc);
      if (bike)
        bike->setPreStepCallback(
          [](void *ctx, float dt) {
            JoltVehicleKind &kind = *static_cast<JoltVehicleKind *>(ctx);
            kind.bodyLag -= dt;
            kind.bikeStepped = true;
          },
          this);
      return;
    }
    springQuarterPeriod = 0.25f / suspension.z;
    JPH::VehicleConstraintSettings settings;
    settings.mForward = JPH::Vec3::sAxisX();
    settings.mMaxPitchRollAngle = maxPitchRoll;
    const float frontX = half.x - 2.f * wheelRadius;
    const float maxSteer = DegToRad(maxSteerDeg);
    const JPH::Vec3 down(0.f, -1.f, 0.f);
    settings.mWheels = {make_wheel(JPH::Vec3(frontX, wheelY, -half.z), wheelRadius, 0.2f, maxSteer, down, suspension),
      make_wheel(JPH::Vec3(frontX, wheelY, half.z), wheelRadius, 0.2f, maxSteer, down, suspension),
      make_wheel(JPH::Vec3(-frontX, wheelY, -half.z), wheelRadius, 0.2f, 0.f, down, suspension),
      make_wheel(JPH::Vec3(-frontX, wheelY, half.z), wheelRadius, 0.2f, 0.f, down, suspension)};
    auto *controller = new JPH::WheeledVehicleControllerSettings;
    const float antiRollStiffness = g_entity_mgr->getOr(eid, ECS_HASH("jolt_vehicle__antiRollStiffness"), 1000.f);
    settings.mAntiRollBars.resize(2);
    for (int i = 0; i < 2; ++i)
    {
      settings.mAntiRollBars[i].mLeftWheel = 2 * i;
      settings.mAntiRollBars[i].mRightWheel = 2 * i + 1;
      settings.mAntiRollBars[i].mStiffness = antiRollStiffness;
    }
    controller->mEngine.mMaxTorque = maxTorque;
    controller->mDifferentials.resize(1);
    controller->mDifferentials[0].mLeftWheel = 0;
    controller->mDifferentials[0].mRightWheel = 1;
    controller->mDifferentials[0].mDifferentialRatio = differentialRatio;
    settings.mController = controller;
    PhysBody::lock_body_rw(body->bodyId, [&](JPH::Body &b) { constraint = new JPH::VehicleConstraint(b, settings); });
    // Rays skip walls; casts hit them
    constraint->SetVehicleCollisionTester(
      new JPH::VehicleCollisionTesterCastCylinder(PhysBody::api().GetObjectLayer(body->bodyId), 1.f));
    constraint->SetPreStepCallback(
      [this](JPH::VehicleConstraint &, const JPH::PhysicsStepListenerContext &ctx) { bodyLag -= ctx.mDeltaTime; });
    jolt_api::phys_sys().AddConstraint(constraint);
    jolt_api::phys_sys().AddStepListener(constraint);
  }

  void destroy()
  {
    if (!body)
      return;
    dacoll::fetch_sim_res(true);
    if (constraint)
    {
      jolt_api::phys_sys().RemoveStepListener(constraint);
      jolt_api::phys_sys().RemoveConstraint(constraint);
      constraint = nullptr;
    }
    if (bike)
      eastl::exchange(bike, nullptr)->destroyVehicle();
    body.reset();
  }

  void setInput(const JoltVehicleControls &input)
  {
    const float forward = max(input.forward / 127.f, -1.f), right = max(input.right / 127.f, -1.f);
    const float brake = input.brake / 255.f, handBrake = input.handBrake / 255.f;
    // A released handbrake must wake parked vehicles
    const bool handBrakeChanged = eastl::exchange(lastHandBrake, handBrake) != handBrake;
    // Jolt steers right-handed, Dagor is left-handed
    if (bike)
      bike->setDriverInput(forward, -right, brake, handBrake);
    else
      static_cast<JPH::WheeledVehicleController *>(constraint->GetController())->SetDriverInput(forward, -right, brake, handBrake);
    if (forward != 0.f || right != 0.f || input.brake || handBrakeChanged)
      body->wakeUp();
  }

  // A gravity zone turns the fall, the lean and the pitch and roll limit; zones keep the world gravity strength
  void setGravity(const Point3 &dir)
  {
    const Point3 gravity = dir * jolt_api::phys_sys().GetGravity().Length();
    if (bike)
      bike->setGravity(gravity);
    else
      constraint->OverrideGravity(to_jVec3(gravity));
    // Jolt pulls only an active body
    if (dir * wokenGravityDir < WAKE_GRAVITY_TURN_COS)
    {
      wokenGravityDir = dir;
      body->wakeUp();
    }
  }

  // A fallen vehicle can tip over
  void setUpright(bool upright)
  {
    if (upright == isUpright)
      return;
    isUpright = upright;
    if (bike)
      bike->enableLeanController(upright);
    else
      constraint->SetMaxPitchRollAngle(upright ? maxPitchRoll : JPH::JPH_PI);
    // A sleeping bike would never tip over
    body->wakeUp();
  }

  // RendInsts join the world only on request: the camera adds them on a client alone, so both sides request the reach of the next step
  void addRendInstsAround(float dt) const
  {
    BBox3 box;
    body->lockRO([&](const JPH::Body &b) {
      JPH::AABox bounds = b.GetWorldSpaceBounds();
      // the bike wheels cast below the box
      bounds.ExpandBy(b.GetLinearVelocity().Abs() * dt + JPH::Vec3::sReplicate(1.f));
      box = BBox3(to_point3(bounds.mMin), to_point3(bounds.mMax));
    });
    dacoll::update_ri_cache_in_volume_to_phys_world(box);
  }

  // World steps per frame: extrapolate lag jitter
  void readState(PhysKindState &state, float dt)
  {
    bodyLag = clamp(bodyLag + dt, 0.f, 4.f * dt);
    recentLags[lagReads++ % recentLags.size()] = bodyLag;
    const float ahead = bodyLag - *eastl::min_element(recentLags.begin(), recentLags.end());
    body->lockRO([&](const JPH::Body &b) {
      readTm = modelTm(b);
      readVelocity = to_point3(b.GetPointVelocity(to_jVec3(readTm.getcol(3))));
      readOmega = to_point3(b.GetAngularVelocity());
    });
    const Point3 pos = readTm.getcol(3) + readVelocity * ahead;
    readTm = makeTM(readOmega, length(readOmega) * ahead) * readTm;
    readTm.setcol(3, pos);
    state.location.fromTM(readTm);
    state.velocity = readVelocity;
    state.omega = Point3(readTm.getcol(0) * readOmega, readTm.getcol(1) * readOmega, readTm.getcol(2) * readOmega);
    // Until the first step the ext keeps the snapshot or rest values
    if (bike && bikeStepped && !bikeGeometry.setup.empty())
      write_bike_wheels(phys_kind_ext_as<JoltVehicleStateExt>(state.ext), bikeGeometry, bike->getRunningGearState());
  }

  // The world steps the body after readState
  void writeState(const PhysKindState &state, bool keep_body_motion, float correction_age = 0.f)
  {
    const TMatrix stateTm = state.location.makeTM();
    TMatrix tm = stateTm;
    Point3 velocity = state.velocity, omegaStep(0.f, 0.f, 0.f);
    if (keep_body_motion)
      body->lockRO([&](const JPH::Body &b) {
        const TMatrix bodyTm = modelTm(b);
        const Point3 bodyVelocity = to_point3(b.GetPointVelocity(to_jVec3(bodyTm.getcol(3))));
        tm = tm * inverse(readTm) * bodyTm;
        velocity += bodyVelocity - readVelocity;
        omegaStep = to_point3(b.GetAngularVelocity()) - readOmega;
        // An old error would pump the springs
        if (correction_age > springQuarterPeriod)
        {
          const Point3 up = bodyTm.getcol(1);
          tm.setcol(3, tm.getcol(3) - up * ((tm.getcol(3) - bodyTm.getcol(3)) * up));
          velocity -= up * ((velocity - bodyVelocity) * up);
        }
      });
    else
      bodyLag = 0.f;
    body->setTm(tm * bikeGeometry.setup.chassisInModel);
    // A later write keeps only newer motion
    readTm = stateTm;
    readVelocity = state.velocity;
    readOmega = stateTm % state.omega;
    const Point3 omega = readOmega + omegaStep;
    // Jolt moves the mass center
    const Point3 comVelocity = velocity + omega % (tm % comOffset);
    PhysBody::api().SetLinearAndAngularVelocity(body->bodyId, to_jVec3(comVelocity), to_jVec3(omega));
  }

  void followKinematic(const TMatrix &tm, const Point3 &velocity, const Point3 &omega)
  {
    dacoll::set_obj_motion(CollisionObject(body.get()), tm * bikeGeometry.setup.chassisInModel,
      velocity + (tm % omega) % (tm % comOffset), modelInChassis % omega);
  }

  // After fetch_sim_res; the animchar places the root from the transform
  void syncNodes(GeomNodeTree &tree, const JoltVehicleStateExt &ext, float dt, float smooth_time)
  {
    if (bikeGeometry.setup.empty())
      return;
    if (bike)
    {
      // Before its first step a new vehicle has the max travel
      if (bikeStepped)
      {
        shownWheels = bike->getRunningGearState();
        for (int i = 0; i < 2; i++)
          shownWheels.rotationAngle[i] = norm_ang(shownWheels.rotationAngle[i] + spinOffset[i]);
      }
    }
    else
    {
      // The ext holds the last snapshot: its steps must not show
      const IPhysMotorcycle::RunningGearState target = read_bike_wheels(ext, bikeGeometry);
      shownWheels.steerAngle = approach(shownWheels.steerAngle, target.steerAngle, dt, smooth_time);
      for (int i = 0; i < 2; i++)
      {
        shownWheels.suspensionLength[i] = approach(shownWheels.suspensionLength[i], target.suspensionLength[i], dt, smooth_time);
        shownWheels.rotationAngle[i] = norm_ang(shownWheels.rotationAngle[i] + target.angularVelocity[i] * dt);
      }
    }
    IPhysMotorcycle::poseNodes(tree, bikeGeometry.setup, shownWheels);
  }

private:
  TMatrix modelTm(const JPH::Body &b) const { return to_tmatrix(b.GetWorldTransform()) * modelInChassis; }

  eastl::unique_ptr<PhysBody> body;
  JPH::Ref<JPH::VehicleConstraint> constraint;
  IPhysMotorcycle *bike = nullptr;
  BikeGeometry bikeGeometry;
  IPhysMotorcycle::RunningGearState shownWheels;
  TMatrix modelInChassis = TMatrix::IDENT;
  TMatrix readTm = TMatrix::IDENT;
  Point3 readVelocity = Point3(0.f, 0.f, 0.f);
  Point3 readOmega = Point3(0.f, 0.f, 0.f);
  Point3 comOffset = Point3(0.f, 0.f, 0.f);
  Point3 wokenGravityDir = Point3(0.f, -1.f, 0.f);
  eastl::array<float, 16> recentLags;
  uint32_t lagReads = 0;
  float bodyLag = 0.f;
  float springQuarterPeriod = 0.f;
  float maxPitchRoll = 0.f;
  float lastHandBrake = 0.f;
  float spinOffset[2] = {};
  bool isUpright = true;
  bool bikeStepped = false;
};

ECS_DECLARE_BOXED_TYPE(JoltVehicleKind);
ECS_REGISTER_BOXED_TYPE(JoltVehicleKind, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(JoltVehicleKind, "jolt_vehicle", nullptr, 0, "phys_kind_net_phys");

static void jolt_vehicle_step_es_event_handler(const CmdPhysKindStep &evt,
  ecs::EntityId eid,
  PhysKindActor &phys_kind_net_phys,
  JoltVehicleKind &jolt_vehicle,
  bool jolt_vehicle__upright = true)
{
  PhysKind &phys = phys_kind_net_phys.phys;
  dacoll::fetch_sim_res(true);
  const bool fresh = jolt_vehicle.needsRecreate(true);
  if (fresh)
    jolt_vehicle.create(eid, phys.currentState.location.makeTM(), true);
  if (!jolt_vehicle.isDynamic())
    return;
  if (fresh)
  {
    jolt_vehicle.writeState(phys.currentState, false);
    // DNG actor contacts read the kind mass
    phys.mass = jolt_vehicle.getMass();
  }
  jolt_vehicle.setUpright(jolt_vehicle__upright);
  jolt_vehicle.setGravity(phys.currentState.gravDirection);
  auto &input = phys_kind_ext_as<JoltVehicleControls>(phys.currentState.ext);
  if (evt.get<3>())
    input = phys_kind_ext_as<JoltVehicleControls>(phys.appliedCT.ext);
  jolt_vehicle.setInput(input);
  jolt_vehicle.readState(phys.currentState, evt.get<1>());
}

static void jolt_vehicle_apply_state_es_event_handler(
  const CmdPhysKindApplyState &evt, const PhysKindActor &phys_kind_net_phys, JoltVehicleKind &jolt_vehicle)
{
  if (!jolt_vehicle.isDynamic())
    return;
  dacoll::fetch_sim_res(true);
  const PhysKind &phys = phys_kind_net_phys.phys;
  // A correction keeps the history from its tick
  const int correctedTick = phys.historyStates.empty() ? phys.currentState.atTick : phys.historyStates.front().atTick;
  jolt_vehicle.writeState(phys.currentState, evt.get<0>(), (phys.currentState.atTick - correctedTick) * phys.timeStep);
  // A loaded snapshot steps no tick this frame
  jolt_vehicle.setInput(phys_kind_ext_as<JoltVehicleControls>(phys.currentState.ext));
}

// The ticks of a frame do not move the body: the world steps it once after them, by the frame time
ECS_AFTER(after_net_phys_sync)
ECS_BEFORE(after_collision_sync)
static void jolt_vehicle_request_rendinsts_es(const UpdatePhysEvent &info, const JoltVehicleKind &jolt_vehicle)
{
  dacoll::fetch_sim_res(true);
  if (jolt_vehicle.isDynamic())
    jolt_vehicle.addRendInstsAround(info.dt);
}

ECS_TAG(gameClient)
ECS_AFTER(after_net_phys_sync)
static void jolt_vehicle_follow_remote_es(const UpdatePhysEvent &,
  ecs::EntityId eid,
  const PhysKindActor &phys_kind_net_phys,
  JoltVehicleKind &jolt_vehicle,
  const TMatrix &transform)
{
  if (phys_kind_net_phys.getRole() != IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW || phys_kind_net_phys.phys.shadowSim)
    return;
  dacoll::fetch_sim_res(true);
  if (jolt_vehicle.needsRecreate(false))
    jolt_vehicle.create(eid, transform, false);
  if (!jolt_vehicle.needsRecreate(false))
    jolt_vehicle.followKinematic(transform, phys_kind_net_phys.phys.currentState.velocity, phys_kind_net_phys.phys.currentState.omega);
}

ECS_AFTER(after_net_phys_sync)
static void jolt_vehicle_sync_nodes_es(const UpdatePhysEvent &info,
  const PhysKindActor &phys_kind_net_phys,
  JoltVehicleKind &jolt_vehicle,
  AnimV20::AnimcharBaseComponent &animchar)
{
  dacoll::fetch_sim_res(true);
  const PhysKind &phys = phys_kind_net_phys.phys;
  jolt_vehicle.syncNodes(animchar.getNodeTree(), phys_kind_ext_as<JoltVehicleStateExt>(phys.currentState.ext), info.dt, phys.timeStep);
}
