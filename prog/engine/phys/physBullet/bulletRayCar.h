#pragma once
#include <phys/dag_vehicle.h>
#include <phys/dag_physics.h>
#include <generic/dag_tab.h>

class BulletRbRayCar : public IPhysVehicle, public IPhysVehicle::ICalcWheelsAngAcc, public IPhysVehicle::ICalcWheelContactForces
{
public:
  class Spring
  {
  public:
    btVector3 axis = btVector3(0, -1, 0), fixPt = btVector3(0, 0, 0), forcePt;
    float l0 = 0.5, lmin = 0.18, lmax = 0.48, lrest = 0;
    float k = 18500.0;
    float kdUp = 1000.0, kdDown = 50.0;

    float curL = 0, curV = 0;

  public:
    Spring()
    {
      curL = lrest = (lmax + lmin) / 2;
      forcePt = fixPt + axis * lmin;
    }

    float calcForce() const { return (l0 - curL) * k - curV * (curV > 0 ? kdDown : kdUp); }
    float calcSpringOnlyForce() const { return (l0 - curL) * k; }
    btVector3 wheelPos() const { return fixPt + axis * curL; }
    btVector3 wheelBottomPos(float radius) const { return fixPt + axis * (curL + radius); }
  };

  class Wheel : public IWheelData
  {
  public:
    struct Hit
    {
      btVector3 worldNormal;
      float distance = 0;
      int materialIndex = -1;
      PhysBody *body = nullptr;
    };
    Wheel(int id, PhysWorld *pw) : wid(id), physWorld(pw) {}

    void init(PhysBody *phbody, PhysBody *wheel, float wheel_rad, const btVector3 &axis, int physmat_id);
    void setParameters(float mass);

    void addRayQuery(float ifps);
    void onWheelContact(bool has_hit, const Hit &hit, float ifps);
    void response();
    void calculateAcc(float ifps, float &acc);

    void applySpringForces(float ifps)
    {
      const btMatrix3x3 &m33 = body->getCenterOfMassTransform().getBasis();
      body->applyForce((m33 * spring.axis) * (suspForce), m33 * spring.fixPt);
    }
    void integrateAngVel(float ifps, float acc)
    {
      angular_velocity += acc * ifps;
      rotation_angle += angular_velocity * ifps;
      add_torque = 0;
    }

    void integratePosition(float /*ifps*/)
    {
      wPos = to_point3(spring.wheelPos());

      if (!wheel)
        return;

      TMatrix tm = makeTM(to_point3(spring.axis), steering_angle * DEG_TO_RAD) * makeTM(to_point3(axis), rotation_angle * DEG_TO_RAD);
      TMatrix tm_trans;
      tm_trans.identity();
      tm_trans.setcol(3, to_point3(spring.wheelPos()));
      btTransform body_m44;
      body->getMotionState()->getWorldTransform(body_m44);
      wheel->setCenterOfMassTransform(body_m44 * to_btTransform(tm_trans * tm));
    }

    const Point3 &getLocalPos() const override { return wPos; }
    float getAxisRotAng() const override { return rotation_angle; }

  protected:
    void processContactHandler(int pmid, const btVector3 &pos, const btVector3 &norm);

  public:
    int wid, axleId = -1;
    btRigidBody *body = nullptr, *wheel = nullptr;
    PhysWorld *physWorld = nullptr;
    BulletRbRayCar::Spring spring;

    btVector3 axis;  //< rotation axis
    btVector3 upDir; //< wheel up dir (=-spring.axis for non-rigid axles, or is calculated dynamically)
    btVector3 wSpringAxis = btVector3(0, -1, 0);
    float radius = 0;  //< wheel radius
    float mass = 1;    //< wheel mass
    float inertia = 1; //< wheel inertia relative rotation axis

    float angular_velocity = 0; //< wheel angular velocity
    float rotation_angle = 0;   //< wheel rotation angle (deg.)
    float steering_angle = 0;   //< wheel steering angle (deg.)

    float damping = 0, freeWheelFriction = 0;
    float force_torque = 0, brake_torque = 0, add_torque = 0;

    // currect contact properties
    bool contact = false;
    btVector3 groundNorm = btVector3(0, 1, 0), forcePt = btVector3(0, 0, 0);
    btVector3 longForce = btVector3(0, 0, 0), latForce = btVector3(0, 0, 0);
    Point3 wPos = Point3(0, 0, 0);
    float suspForce = 0, wheelFz = 0;
    int contactPmid = -1, wheelPmid = -1;

    IPhysVehicle::ICalcWheelContactForces *cwcf = nullptr;
    IPhysVehicle::IWheelContactCB *wheelContactCb = nullptr;
  };

  struct Axle
  {
    int left, right;
    bool rigid;
    float arbK;
  };

public:
  BulletRbRayCar(PhysBody *o, int iter);
  virtual ~BulletRbRayCar();

  // IPhysVehicle interface
  void destroyVehicle() override;

  int addWheel(PhysBody *wheel, float wheel_rad, const Point3 &axis, int physmat_id) override;
  void setSpringPoints(int wheel_id, const Point3 &fixpt, const Point3 &spring, float lmin, float lmax, float l0, float forcept,
    float restpt, float long_force_pt_in_ofs, float long_force_pt_down_ofs) override;
  void setSpringHardness(int wheel_id, float k, float kd_up, float kd_down) override;
  void setWheelParams(int wheel_id, float mass, float friction) override;
  void setWheelDamping(int wheel_id, float damping) override;
  void setAirDrag(float air_drag_sq) override { setAirDrag(air_drag_sq * 30.0, air_drag_sq); }
  void setAirDrag(float air_drag, float air_drag_sq) override
  {
    airDrag = air_drag;
    airDragSq = air_drag_sq;
  }
  void setAirLift(float air_lift, float air_lift_sq, float max_lift_force) override
  {
    airLift = air_lift;
    airLiftSq = air_lift_sq;
    airMaxLiftForce = max_lift_force;
  }
  void setSideAirDrag(float air_drag, float air_drag_sq) override
  {
    airDragSide = air_drag;
    airDragSqSide = air_drag_sq;
  }
  void setSideAirLift(float air_lift, float air_lift_sq) override
  {
    airLiftSide = air_lift;
    airLiftSqSide = air_lift_sq;
  }
  void setAxleParams(int left_wid, int right_wid, bool rigid_axle, float arb_k) override;

  void resetWheels() override;

  void setCwcf(unsigned wheels_mask, IPhysVehicle::ICalcWheelContactForces *cwcf) override;
  void setCwaa(IPhysVehicle::ICalcWheelsAngAcc *cwaa) override { this->cwaa = cwaa ? cwaa : this; }

  void addWheelTorque(int wheel_id, float torque) override;
  void setWheelSteering(int wheel_id, float angle) override;
  float getWheelSteering(int wheel_id) override;
  void setWheelBrakeTorque(int wheel_id, float torque) override;
  void setWheelAngularVelocity(int wid, float vel) override { wheels[wid]->angular_velocity = vel; }
  void setWheelContactCb(int wheel_id, IWheelContactCB *cb) override { wheels[wheel_id]->wheelContactCb = cb; }

  IWheelData *getWheelData(int wheel_id) override { return wheels[wheel_id]; }
  float getWheelAngularVelocity(int wheel_id) override;
  float getWheelFeedbackTorque(int wheel_id) override;
  float getWheelInertia(int wheel_id) override;
  bool getWheelContact(int wheel_id) override;
  bool getWheelDrift(int wheel_id, float &long_vel, float &lat_vel) override;

  void update(float ifps) override
  {
    findContacts(ifps);
    calculateForce(ifps);
    integratePosition(ifps);
  }
  void updateInactiveVehicle(float ifps, float wheel_vel) override;

  void debugRender() override;

  // IPhysVehicle::ICalcWheelsAngAcc interface
  unsigned calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt) override;
  void postCalcWheelsAcc(float /*wheel_acc*/[], int /*wheel_num*/, float /*dt*/) override {}

  // IPhysVehicle::ICalcWheelContactForces interface
  void calcWheelContactForces(int wid, float norm_force, float load_rad, const Point3 wheel_basis[3], const Point3 &ground_norm,
    float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force, float &out_long_force) override;

  void findContacts(float ifps);
  void calculateForce(float ifps);
  void integratePosition(float ifps);

protected:
  float airDrag, airDragSq;
  float airLift, airLiftSq, airMaxLiftForce;
  float airDragSide, airDragSqSide, airLiftSide, airLiftSqSide;
  btRigidBody *object;
  PhysBody *physBody;

  Tab<Wheel *> wheels;
  Tab<Axle> axle;
  IPhysVehicle::ICalcWheelsAngAcc *cwaa;
  int iterCount;

  int getAxle(int l_wid, int r_wid);
};
