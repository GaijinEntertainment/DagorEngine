//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class PhysJoint
{
public:
  enum JointType
  {
    PJ_HINGE = 0,
    PJ_6DOF,
    PJ_6DOF_SPRING,
    PJ_CONE_TWIST,
    PJ_FIXED,
    PJ_UNKNOWN
  };
  JointType jointType = PJ_UNKNOWN;

  PhysJoint(JointType joint_type) : jointType(joint_type) {}

  virtual ~PhysJoint();
  PhysConstraint *getJoint() const { return joint; }

protected:
  PhysConstraint *joint = nullptr;
};

class PhysRagdollHingeJoint : public PhysJoint
{
public:
  PhysRagdollHingeJoint(PhysBody *body1, PhysBody *body2, const Point3 &pos, const Point3 &axis, const Point3 & /*mid_axis*/,
    const Point3 & /*x_axis*/, real ang_limit, real /*damping*/, real /*sleep_threshold*/);

  static PhysRagdollHingeJoint *cast(PhysJoint *j)
  {
    if (!j)
      return NULL;
    if (j->jointType == PhysJoint::PJ_HINGE)
      return static_cast<PhysRagdollHingeJoint *>(j);
    return NULL;
  }
};

class Phys6DofJoint : public PhysJoint
{
public:
  Phys6DofJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2);

  static Phys6DofJoint *cast(PhysJoint *j)
  {
    if (!j)
      return NULL;
    if (j->jointType == PhysJoint::PJ_6DOF)
      return static_cast<Phys6DofJoint *>(j);
    return NULL;
  }

  void setLimit(int index, const Point2 &limits);
  void setParam(int num, float value, int axis);
  void setAxisDamping(int index, float damping); // not the same as in setSpring!
};

class Phys6DofSpringJoint : public PhysJoint
{
public:
  Phys6DofSpringJoint(PhysBody *body1, PhysBody *body2, const TMatrix &frame1, const TMatrix &frame2);
  Phys6DofSpringJoint(PhysBody *body1, PhysBody *body2, const TMatrix &in_tm);

  static Phys6DofSpringJoint *cast(PhysJoint *j)
  {
    if (!j)
      return NULL;
    if (j->jointType == PhysJoint::PJ_6DOF_SPRING)
      return static_cast<Phys6DofSpringJoint *>(j);
    return NULL;
  }

  void setSpring(int index, bool on, float stiffness, float damping, const Point2 &limits);
  void setLimit(int index, const Point2 &limits);
  void setEquilibriumPoint();
  void setParam(int num, float value, int axis);
  void setAxisDamping(int index, float damping); // not the same as in setSpring!
};

class PhysRagdollBallJoint : public PhysJoint
{
public:
  PhysRagdollBallJoint(PhysBody *body1, PhysBody *body2, const TMatrix &_tm, const Point3 &min_limit, const Point3 &max_limit,
    real damping, real /*twist_damping*/, real /*sleep_threshold*/);

  static PhysRagdollBallJoint *cast(PhysJoint *j)
  {
    if (!j)
      return NULL;
    if (j->jointType == PhysJoint::PJ_CONE_TWIST)
      return static_cast<PhysRagdollBallJoint *>(j);
    return NULL;
  }

  void setTargetOrientation(const TMatrix &tm);
  void setTwistSwingMotorSettings(float twistFrequency, float twistDamping, float swingFrequency, float swingDamping);
};

class PhysFixedJoint : public PhysJoint
{
public:
  PhysFixedJoint(PhysBody * /*body1*/, PhysBody * /*body2*/, real /*sleep_threshold*/, real /*max_force*/ = -1,
    real /*max_torque*/ = -1) :
    PhysJoint(PhysJoint::PJ_FIXED)
  {}
  bool isBroken() { return false; }

  static PhysFixedJoint *cast(PhysJoint *j)
  {
    return (j && j->jointType == PhysJoint::PJ_FIXED) ? static_cast<PhysFixedJoint *>(j) : nullptr;
  }
};
