//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <dag/dag_vector.h>
#include <util/dag_index16.h>

class DataBlock;
class GeomNodeTree;


class IPhysVehicle
{
public:
  class ICalcWheelContactForces
  {
  public:
    enum
    {
      B_FWD,
      B_SIDE,
      B_UP
    };

    // called by vehicle for each wheel to calculate forces in contact point
    virtual void calcWheelContactForces(int wid, float norm_force, float load_rad, const Point3 wheel_basis[3],
      const Point3 &ground_norm, float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force,
      float &out_long_force) = 0;
  };

  class ICalcWheelsAngAcc
  {
  public:
    // called by vehicle to calculate angular accelerations for wheels
    // returns bitmask of calculated wheels (e.g., 0 for none, 0xF for first 4 wheels)
    virtual unsigned calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt) = 0;

    // called by vehicle after calculating angular accelerations for all wheels
    // can be used for validating acc before they are applied to wheels
    virtual void postCalcWheelsAcc(float wheel_acc[], int wheel_num, float dt) = 0;
  };

  class IWheelContactCB
  {
  public:
    virtual void onWheelContact(int wheel_id, int pmid, const Point3 &pos, const Point3 &norm, const Point3 &move_dir,
      const Point3 &wheel_dir, float slip_speed) = 0;
  };

  class IWheelData
  {
    int classLabel;

  public:
    IWheelData() { classLabel = 'PWHL'; }
    bool checkClassLabel() { return classLabel == 'PWHL'; }

    virtual const Point3 &getLocalPos() const = 0;
    virtual float getAxisRotAng() const = 0;

  protected:
    virtual ~IWheelData() {}
  };

public:
  virtual void destroyVehicle() = 0;

  // car setup methods
  virtual int addWheel(PhysBody *wheel, float wheel_rad, const Point3 &axis, int physmat_id) = 0;
  virtual void setSpringPoints(int wheel_id, const Point3 &fixpt, const Point3 &spring, float lmin, float lmax, float l0,
    float forcept, float restpt, float long_force_pt_in_ofs = 0, float long_force_pt_down_ofs = 0) = 0;
  virtual void setSpringHardness(int wheel_id, float k, float kd_up, float kd_down) = 0;
  virtual void setWheelParams(int wheel_id, float mass, float friction) = 0;
  virtual void setWheelDamping(int wheel_id, float damping) = 0;
  virtual void setAirDrag(float air_drag_sq) = 0;
  virtual void setAirDrag(float air_drag, float air_drag_sq) = 0;
  virtual void setAirLift(float air_lift, float air_lift_sq, float max_lift_force) = 0;
  virtual void setSideAirDrag(float air_drag, float air_drag_sq) = 0;
  virtual void setSideAirLift(float air_lift, float air_lift_sq) = 0;
  virtual void setAxleParams(int left_wid, int right_wid, bool rigid_axle, float arb_k) = 0;

  // Resets wheel position to local and rotation velocity to zero
  virtual void resetWheels() = 0;

  virtual void setCwcf(unsigned wheel_mask, ICalcWheelContactForces *cwcf) = 0;
  virtual void setCwaa(ICalcWheelsAngAcc *cwaa) = 0;

  virtual IWheelData *getWheelData(int wheel_id) = 0;

  // car control methods
  virtual void addWheelTorque(int wheel_id, float torque) = 0;
  virtual void setWheelSteering(int wheel_id, float angle) = 0;
  virtual float getWheelSteering(int wheel_id) = 0;
  virtual void setWheelBrakeTorque(int wheel_id, float torque) = 0;
  virtual void setWheelAngularVelocity(int wheel_id, float ang_vel) = 0;
  virtual void setWheelContactCb(int wheel_id, IWheelContactCB *cb) = 0;

  // wheel state methods
  virtual float getWheelAngularVelocity(int wheel_id) = 0;
  virtual float getWheelFeedbackTorque(int wheel_id) = 0;
  virtual float getWheelInertia(int wheel_id) = 0;
  virtual bool getWheelContact(int wheel_id) = 0;
  virtual bool getWheelDrift(int wheel_id, float &long_vel, float &lat_vel) = 0;

  virtual void update(float ifps) = 0;
  virtual void updateInactiveVehicle(float ifps, float wheel_lin_vel) = 0;

  // debug vehicle rendering
  virtual void debugRender() = 0;

public:
  // creators for different car types / physics engines
  static IPhysVehicle *createRayCarBullet(PhysBody *car, int iter_count = 1);
  static IPhysVehicle *createRayCarJolt(PhysBody *car, int iter_count = 1);

  static void bulletSetStaticTracer(bool (*traceray)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid));
  static void joltSetStaticTracer(bool (*traceray)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid));
};


// Two-wheeled vehicle simulated by the physics engine's own vehicle solver (wheels belong to the vehicle constraint,
// they are not separate PhysBody instances as in IPhysVehicle). Wheel indices: 0 = front, 1 = rear.
class IPhysMotorcycle
{
public:
  struct WheelSettings
  {
    // chassis space: forward=+X, up=+Y, left=+Z, as DNG vehicles
    Point3 position = Point3(0, 0, 0);             // suspension attachment point in chassis space
    Point3 suspensionDirection = Point3(0, -1, 0); // chassis space, normalized on creation
    Point3 steeringAxis = Point3(0, 1, 0);         // chassis space, normalized on creation
    float radius = 0.3f;
    float width = 0.1f;
    float suspensionMinLength = 0.3f;
    float suspensionMaxLength = 0.5f;
    float suspensionFrequency = 1.5f; // Hz
    float suspensionDamping = 0.5f;   // damping ratio, 1 is critical
    float maxBrakeTorque = 500.f;     // N*m
    float maxHandBrakeTorque = 0.f;   // N*m, 0 keeps the hand brake off this wheel
    float maxSteerAngleDeg = 0.f;     // 0 makes the wheel non-steerable
    // apply suspension force at the fully compressed suspension point instead of the wheel contact point
    bool overrideSuspensionForcePoint = false;
  };

  struct EngineSettings
  {
    float maxTorque = 150.f; // N*m
    float minRPM = 1000.f;
    float maxRPM = 10000.f;
  };

  struct TransmissionSettings
  {
    float shiftDownRPM = 2000.f;
    float shiftUpRPM = 8000.f;
    float clutchStrength = 2.f;
  };

  struct Settings
  {
    WheelSettings frontWheel, rearWheel;
    EngineSettings engine;
    TransmissionSettings transmission;
    float differentialRatio = 1.f;     // gearbox rpm / rear wheel rpm, combined primary and final drive
    float maxPitchRollAngleDeg = 60.f; // solver stabilizes the vehicle below this tilt; loadDesc() defaults it from the wheelie
    // the lean controller turns the chassis toward the lean of the ground reaction by a torque no rider has, so a softer spring looks
    // more natural; the defaults are the physics engine ones
    float maxLeanAngleDeg = 45.f; // the controller aims at most this far from upright
    float leanSpring = 5000.f;    // N*m per radian of lean error
    float leanDamping = 1000.f;   // N*m per radian/s of roll rate
  };

  // the constants poseNodes() needs, from the skeleton rest pose and the wheel settings, and chassisInModel; empty poses nothing
  struct BikeGeometrySetup
  {
    // the node adds the motions from motionBegin to its own motion, both included, to those of its nearest moving ancestor;
    // the motions make two chains: steer, front travel, front spin; rear travel, rear spin
    struct Node
    {
      dag::Index16 node;
      int motionBegin = 0;
      TMatrix chassisInParent = TMatrix::IDENT; // chassis frame in the rest frame of the parent node
      TMatrix restInChassis = TMatrix::IDENT;   // rest frame of the node in chassis space
    };
    Node nodes[4];                                                        // steering, front suspension, front wheel, rear wheel
    Point3 suspensionDirection[2] = {Point3(0, -1, 0), Point3(0, -1, 0)}; // chassis space, normalized
    float restLength[2] = {};                                             // mid travel
    Point3 steeringAxis = Point3(0, 1, 0);                                // chassis space, normalized
    TMatrix chassisInModel = TMatrix::IDENT;                              // Desc::chassisInModel, also in an empty setup

    bool empty() const { return !nodes[0].node; }
  };

  // Settings plus the chassis body and the skeleton nodes, as loadDesc() fills them.
  // The chassis origin is the box center; the caller creates the chassis PhysBody with mass, inertia and comOffset.
  struct Desc
  {
    float mass = 0.f;
    Point3 inertia = Point3(0, 0, 0);   // kg*m^2 about chassis x (roll), y (yaw), z (pitch)
    Point3 comOffset = Point3(0, 0, 0); // from the chassis origin
    Point3 boxSize = Point3(0, 0, 0);
    TMatrix chassisInModel = TMatrix::IDENT; // chassis frame in skeleton model space, which is the root node space
    Settings settings;
    dag::Vector<float> gearRatios, reverseGearRatios; // empty keeps the physics engine defaults
    dag::Index16 nodeBody, nodeSteering, nodeSuspensionFront, nodeWheelFront, nodeWheelRear;
    const GeomNodeTree *restTree = nullptr; // the skeleton loadDesc() read, kept unchanged by the caller
  };

  // the solver values that pose the steering and the wheels, in physics engine units and signs; index 0 = front, 1 = rear
  struct RunningGearState
  {
    float steerAngle = 0;           // front wheel, radians
    float suspensionLength[2] = {}; // from WheelSettings::position along the suspension direction; the setup restLength at rest
    float rotationAngle[2] = {};    // radians about the axle, growing as the wheel rolls forward, wrapped by fmod to (-2 pi, 2 pi)
    float angularVelocity[2] = {};  // radians/s, the rate of rotationAngle
  };

public:
  virtual void destroyVehicle() = 0;

  // wakes up the chassis body; call on driver input so a sleeping vehicle reacts to it
  virtual void activate() = 0;

  // normalized driver input: forward and left in [-1..1], brake and hand_brake in [0..1];
  // left > 0 turns left, the same sign as IPhysVehicle wheel steering
  virtual void setDriverInput(float forward, float left, float brake, float hand_brake = 0.f) = 0;
  // lean controller is enabled by default on creation; disabling it also frees pitch and roll, so the vehicle can fall over
  virtual void enableLeanController(bool enable) = 0;
  // gravity of this vehicle alone, such as in a gravity zone: the chassis falls along it and leans and rights against it;
  // the physics world gravity applies until the first call
  virtual void setGravity(const Point3 &gravity) = 0;

  // absolute sine of the current lean angle relative to gravity up; 0 = upright or upside down, lean direction is not reported
  virtual float getAbsSinLeanAngle() const = 0;
  // whether the wheel (0 = front, 1 = rear) touched the ground in the last step; read it while no step runs
  virtual bool hasWheelContact(int wheel) const = 0;
  // game thread, after fetchSimRes()
  virtual RunningGearState getRunningGearState() const = 0;

  // called on the physics thread before each substep with its dt, while the step holds all body locks; nullptr removes it
  typedef void (*PreStepCallback)(void *ctx, float dt);
  virtual void setPreStepCallback(PreStepCallback cb, void *ctx) = 0;

public:
  // fills desc from blk and the tree rest pose: hubs and the steering axis from the nodes, springs for a rest at mid travel;
  // logerr()s suspicious values but still loads; false only for a missing mandatory value or node; Jolt only, reads its gravity
  static bool loadDesc(const DataBlock &blk, const GeomNodeTree &tree, Desc &desc);
  // desc.restTree must be valid during the call; false leaves dest_setup empty, apart from chassisInModel; Jolt only
  static bool initBikeGeometrySetup(BikeGeometrySetup &dest_setup, const Desc &desc);

  // Jolt only; nullptr for an invalid chassis; the caller owns the chassis, which must outlive the vehicle;
  // desc.restTree must be valid during the call; destroy the vehicle before the physics engine closes
  static IPhysMotorcycle *createJolt(PhysBody *chassis, const Desc &desc);
  // what initBikeGeometrySetup() made from the desc in createJolt()
  virtual const BikeGeometrySetup &getBikeGeometrySetup() const = 0;
  // writes the local tms of the moving nodes (steering, travel, wheel spin) into a tree with the skeleton of desc.restTree,
  // such as an animChar tree, and invalidates their wtm; game thread, after fetchSimRes(); no-op if getBikeGeometrySetup() is empty;
  // the caller sets the root to chassis tm * inverse(getBikeGeometrySetup().chassisInModel), then calls invalidateWtm() and calcWtm()
  virtual void syncNodes(GeomNodeTree &tree) = 0;
  // syncNodes() for a copy with no vehicle, from a BikeGeometrySetup and solver values such as those of a network snapshot; Jolt only
  static void poseNodes(GeomNodeTree &tree, const BikeGeometrySetup &setup, const RunningGearState &state);
};
