//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>

class Point3;


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

  static void bulletSetStaticTracer(bool (*traceray)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid));
};
