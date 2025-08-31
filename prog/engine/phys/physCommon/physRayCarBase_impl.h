// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "physRayCarBase.h"
#include <scene/dag_physMat.h>
#include <scene/dag_physMatId.h>
#include <workCycle/dag_workCyclePerf.h>

#define U_EPSILON 1e-6

PhysRbRayCarBase::PhysRbRayCarBase(PhysBody *o, int iter) : physBody(o), wheels(midmem), cwaa(this), axle(midmem) { iterCount = iter; }

PhysRbRayCarBase::~PhysRbRayCarBase() {}

void PhysRbRayCarBase::destroyVehicle() { delete this; }

int PhysRbRayCarBase::addWheel(PhysBody *b, float wheel_rad, const Point3 &axis, int physmat_id)
{
  // The member "physBody" is the car itself
  // while "b" is the wheel

  if (b)
    b->setMassMatrix(0, 0, 0, 0);

  int wid = wheels.size();
  Wheel *w = new Wheel(wid, physBody->getPhysWorld());
  w->init(physBody, b, wheel_rad, axis, physmat_id);
  w->cwcf = this;
  wheels.push_back(w);
  return wheels.size() - 1;
}

void PhysRbRayCarBase::resetWheels()
{
  for (int i = 0; i < wheels.size(); i++)
  {
    Wheel &w = *wheels[i];
    w.angular_velocity = 0.0f;
    w.rotation_angle = 0.0f;
    w.steering_angle = 0.0f;

    w.force_torque = 0.0f;
    w.brake_torque = 0.0f;
    w.add_torque = 0.0;

    w.contact = false;
    w.suspForce = w.wheelFz = 0.0f;
    w.contactPmid = -1;
    w.spring.curV = 0;
    w.spring.curL = w.spring.lrest;
    w.wPos = w.spring.wheelPos();
  }
}

void PhysRbRayCarBase::initAero()
{
  // Due to virtual functions these cannot be in the constructor
  setAirDrag(0.3);
  setSideAirDrag(18.0, 0.6);
  setAirLift(0.0, 0.0, 0.0);
  setSideAirLift(0.0, 0.0);
}

void PhysRbRayCarBase::setSpringPoints(int wheel_id, const Point3 &fixpt, const Point3 &spring, float lmin, float lmax, float l0,
  float forcept, float restpt, float /*long_force_pt_in_ofs*/, float /*long_f_pt_down_ofs*/)
{
  if (wheel_id < 0 || wheel_id >= wheels.size())
    return;

  Spring &s = wheels[wheel_id]->spring;
  s.fixPt = fixpt;
  s.axis = normalize(spring);
  s.lmin = lmin;
  s.lmax = lmax;
  s.l0 = l0;
  s.lrest = restpt >= lmin ? restpt : (lmin + lmax) / 2;
  if (!float_nonzero(forcept + 1))
    forcept = s.lmin;
  s.forcePt = s.fixPt + s.axis * forcept;
  s.curL = s.lrest;

  TMatrix mtx;
  physBody->getTm(mtx);
  wheels[wheel_id]->upDir = -s.axis;
  wheels[wheel_id]->wPos = s.wheelPos();
  wheels[wheel_id]->wSpringAxis = normalize(mtx % s.axis);
}

void PhysRbRayCarBase::setSpringHardness(int wheel_id, float k, float kd_up, float kd_down)
{
  if (wheel_id < 0 || wheel_id >= wheels.size())
    return;

  Spring &s = wheels[wheel_id]->spring;
  s.k = k;
  s.kdUp = kd_up;
  s.kdDown = kd_down;
}

void PhysRbRayCarBase::setAirDrag(float air_drag, float air_drag_sq)
{
  airDrag = air_drag;
  airDragSq = air_drag_sq;
}

void PhysRbRayCarBase::setAirLift(float air_lift, float air_lift_sq, float max_lift_force)
{
  airLift = air_lift;
  airLiftSq = air_lift_sq;
  airMaxLiftForce = max_lift_force;
}

void PhysRbRayCarBase::setSideAirDrag(float air_drag, float air_drag_sq)
{
  airDragSide = air_drag;
  airDragSqSide = air_drag_sq;
}

void PhysRbRayCarBase::setSideAirLift(float air_lift, float air_lift_sq)
{
  airLiftSide = air_lift;
  airLiftSqSide = air_lift_sq;
}

void PhysRbRayCarBase::setWheelParams(int wheel_id, float mass, float /*friction*/)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->setParameters(mass);
}

void PhysRbRayCarBase::setCwcf(unsigned wheels_mask, IPhysVehicle::ICalcWheelContactForces *cwcf)
{
  for (int i = 0; i < wheels.size(); i++)
    if (wheels_mask & (1 << i))
      wheels[i]->cwcf = cwcf ? cwcf : this;
}

float PhysRbRayCarBase::getWheelInertia(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->inertia;
}

void PhysRbRayCarBase::addWheelTorque(int wheel_id, float torque)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->add_torque = torque;
}

void PhysRbRayCarBase::setWheelSteering(int wheel_id, float angle)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->steering_angle = angle;
}

float PhysRbRayCarBase::getWheelSteering(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->steering_angle;
}

float PhysRbRayCarBase::getWheelAngularVelocity(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->angular_velocity;
}

float PhysRbRayCarBase::getWheelFeedbackTorque(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->force_torque;
}

bool PhysRbRayCarBase::getWheelContact(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->contact;
}

bool PhysRbRayCarBase::getWheelDrift(int /*wheel_id*/, float & /*long_vel*/, float & /*lat_vel*/) { return false; }

void PhysRbRayCarBase::setWheelDamping(int wheel_id, float damping)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->damping = damping;
}

void PhysRbRayCarBase::setWheelBrakeTorque(int wheel_id, float torque)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->brake_torque = torque;
}

void PhysRbRayCarBase::setAxleParams(int left_wid, int right_wid, bool rigid_axle, float arb_k)
{
  int a_id = getAxle(left_wid, right_wid);
  axle[a_id].arbK = arb_k;
  axle[a_id].rigid = rigid_axle;
}

int PhysRbRayCarBase::getAxle(int left_wid, int right_wid)
{
  for (int i = 0; i < axle.size(); i++)
    if (axle[i].left == left_wid && axle[i].right == right_wid)
      return i;
  int i = append_items(axle, 1);
  axle[i].left = left_wid;
  axle[i].right = right_wid;
  axle[i].arbK = 0;
  axle[i].rigid = false;

  wheels[left_wid]->axleId = i;
  wheels[right_wid]->axleId = i;
  return i;
}

void PhysRbRayCarBase::update(float ifps)
{
  findContacts(ifps);
  calculateForce(ifps);
  integratePosition(ifps);
}

void PhysRbRayCarBase::updateInactiveVehicle(float ifps, float wheel_lin_vel)
{
  for (int i = 0; i < wheels.size(); i++)
  {
    Wheel *w = wheels[i];
    w->force_torque = 0;
    w->brake_torque = 0;
    w->add_torque = 0;
    w->angular_velocity = wheel_lin_vel / w->radius;
    w->integrateAngVel(ifps, 0);
    w->integratePosition(ifps);

    w->spring.curV = 0;
  }
}

void PhysRbRayCarBase::calcWheelContactForces(int /*wid*/, float norm_force, float load_rad, const Point3 wheel_basis[3],
  const Point3 & /*ground_norm*/, float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force,
  float &out_long_force)
{
  {
    // side force

    float slip_angle = 0.0f;
    float x = fabs(wheel_basis[B_FWD] * ground_vel);
    if (x > U_EPSILON)
      slip_angle = atan2(wheel_basis[B_SIDE] * ground_vel, x);
    else
    {
      slip_angle = wheel_basis[B_SIDE] * ground_vel;
      if (slip_angle > 0.0f)
        slip_angle = PI / 2.0f;
      else if (slip_angle < 0.0f)
        slip_angle = -PI / 2.0f;
    }
    slip_angle *= RAD_TO_DEG;

    float side_force = -slip_angle * cpt_friction / 3.0f;
    if (side_force > cpt_friction)
      side_force = cpt_friction;
    if (side_force < -cpt_friction)
      side_force = -cpt_friction;
    side_force *= norm_force * .25;

    float side_velocity = fabs(wheel_basis[B_SIDE] * ground_vel);
    if (side_velocity < 1.0f)
      side_force *= side_velocity * cpt_friction;

    out_lat_force = side_force;
  }

  {
    // forward force

    float slip_ratio = 0.0f;
    float numerator = wheel_ang_vel * load_rad;
    float denomenator = ground_vel * wheel_basis[B_FWD];

    if (fabs(denomenator) > U_EPSILON)
      slip_ratio = (numerator - denomenator) / fabs(denomenator);
    else
      slip_ratio = numerator;
    /*debug ( "%p: gnd_vel=" FMT_P3 " wheel_fwd=" FMT_P3 ", num=%.7f / denom=%.7f slip=%.7f  normF=%.4f",
    this, P3D(ground_vel), P3D(wheel_basis[B_FWD]), numerator, denomenator,
    slip_ratio, norm_force );*/
    slip_ratio *= 100.0f;

    float forward_force = slip_ratio * cpt_friction / 6.0f;
    if (forward_force > cpt_friction)
      forward_force = cpt_friction;
    if (forward_force < -cpt_friction)
      forward_force = -cpt_friction;
    forward_force *= norm_force;

    float forward_velocity = fabs(wheel_basis[B_FWD] * ground_vel - numerator);
    if (forward_velocity < 1.0f)
      forward_force *= forward_velocity * 1.0f;

    //--forcePt = body_m44 * spring.forcePt;

    out_long_force = forward_force;
  }
}

unsigned PhysRbRayCarBase::calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt)
{
  for (int i = 0; i < wheel_num; i++)
    if (wheel_need_mask & (1 << i))
      wheels[i]->calculateAcc(dt, wheel_acc[i]);
  return wheel_need_mask;
}

void PhysRbRayCarBase::integratePosition(float ifps)
{
  for (int i = 0; i < wheels.size(); i++)
    wheels[i]->integratePosition(ifps);
}

void PhysRbRayCarBase::findContacts(float ifps)
{
  int ft = workcycleperf::get_frame_timepos_usec();
  for (int i = 0; i < wheels.size(); i++)
    wheels[i]->addRayQuery(ifps);
  if (workcycleperf::debug_on)
    debug(" -- %d: trace ray: %d rays, %d usec", ft, wheels.size(), workcycleperf::get_frame_timepos_usec() - ft);

  for (int i = 0; i < axle.size(); i++)
  {
    Wheel &wl = *wheels[axle[i].left], &wr = *wheels[axle[i].right];
    float arbK = axle[i].arbK;
    float y_l = wl.spring.curL;
    float y_r = wr.spring.curL;
    float dFz = (y_l - y_r) * arbK;
    float fl = -wl.spring.calcSpringOnlyForce();
    float fr = -wr.spring.calcSpringOnlyForce();

    if (fl + dFz > 0)
    {
      float curL = (wl.spring.l0 * wl.spring.k + wr.spring.curL * arbK) / (wl.spring.k + arbK);
      if (curL > wl.spring.lmax)
        curL = wl.spring.lmax;
      else if (curL < wl.spring.lmin)
        curL = wl.spring.lmin;

      // debug("wheel %d lift off: norm=%.1f dFz=%.1f l=%.3f->%.3f",
      //   axle[i].left, fl, dFz, wl.spring.curL, curL);

      float lastL = wl.spring.curL - wl.spring.curV * ifps;
      wl.spring.curV = (curL - lastL) / ifps;
      wl.spring.curL = curL;

      y_l = curL;
      dFz = (y_l - y_r) * arbK;

      wl.suspForce = 0;
      wl.contact = false;
    }
    if (fr - dFz > 0)
    {
      float curL = (wr.spring.l0 * wr.spring.k + wl.spring.curL * arbK) / (wr.spring.k + arbK);
      if (curL > wr.spring.lmax)
        curL = wr.spring.lmax;
      else if (curL < wr.spring.lmin)
        curL = wr.spring.lmin;

      // debug("wheel %d lift off: norm=%.1f dFz=%.1f l=%.3f->%.3f",
      //   axle[i].right, fr, dFz, wr.spring.curL, curL);

      float lastL = wr.spring.curL - wr.spring.curV * ifps;
      wr.spring.curV = (curL - lastL) / ifps;
      wr.spring.curL = curL;

      y_r = curL;
      dFz = (y_l - y_r) * arbK;

      wr.suspForce = 0;
      wr.contact = false;
    }

    wl.suspForce += dFz;
    wr.suspForce += -dFz;

    if (axle[i].rigid)
    {
      wl.axis = wr.axis = normalize(wr.spring.wheelPos() - wl.spring.wheelPos());
      Point3 fwd = normalize(cross(wl.spring.axis, wl.axis));
      wl.upDir = wr.upDir = cross(fwd, wl.axis);
    }
  }
}

void PhysRbRayCarBase::calculateForce(float ifps)
{
  float iter_dt = ifps / iterCount;
  for (int iter = iterCount; iter > 0; iter--)
  {
    for (int i = 0; i < wheels.size(); i++)
      wheels[i]->response();

    float wheel_acc[32];
    unsigned wheel_mask = (1 << wheels.size()) - 1;
    wheel_mask &= ~cwaa->calcWheelsAcc(wheel_acc, wheels.size(), wheel_mask, iter_dt);
    if (wheel_mask)
      calcWheelsAcc(wheel_acc, wheels.size(), wheel_mask, iter_dt);

    cwaa->postCalcWheelsAcc(wheel_acc, wheels.size(), iter_dt);

    for (int i = 0; i < wheels.size(); i++)
      wheels[i]->integrateAngVel(iter_dt, wheel_acc[i]);
  }

  for (int i = 0; i < wheels.size(); i++)
    wheels[i]->applySpringForces(ifps);


  TMatrix mtx;
  physBody->getTm(mtx);

  bool should_activate = false;
  for (int i = 0; i < wheels.size(); i++)
    if (wheels[i]->contact)
    {
      // debug("wheel[%d] rel_pos=%@ lng=%@ lat=%@ iter_dt=%g",
      //   i, to_point3(rel_pos), to_point3(wheels[i]->longForce), to_point3(wheels[i]->latForce), iter_dt);
      physBody->addImpulse(wheels[i]->forcePt, (wheels[i]->longForce + wheels[i]->latForce) * /*iter_dt*/ ifps);
      should_activate |= fabs(wheels[i]->angular_velocity) > 1e-3;
    }
    else
      should_activate = true;

  Point3 vel = physBody->getVelocity();
  float velm = vel.length();
  float ad = airDrag, ads = airDragSq;
  float long_wt = 1, lat_wt = 0;

  if (velm > 0.1)
  {
    long_wt = fabs(dot(mtx.getcol(1), vel)) / velm;
    lat_wt = fabs(dot(mtx.getcol(2), vel)) / velm;
    ad = (airDrag * long_wt + airDragSide * lat_wt);
    ads = (airDragSq * long_wt + airDragSqSide * lat_wt);
  }

  physBody->addImpulse(mtx.getcol(3), vel * (-(ad + velm * ads) * ifps));
  if (airMaxLiftForce > 0)
  {
    Point3 wUp = wheels[0]->wSpringAxis;
    float vxz = (vel - vel * dot(vel, wUp)).length();

    if (fabs(dot(vel, wUp)) < fabs(vxz) * 2)
    {
      float fy = (airLift * long_wt + airLiftSide * lat_wt + vxz * (airLiftSq * long_wt + airLiftSqSide * lat_wt)) * vxz;
      if (fy > airMaxLiftForce)
        fy = airMaxLiftForce;
      else if (fy < -airMaxLiftForce)
        fy = -airMaxLiftForce;

#if DAGOR_DBGLEVEL > 0
      if (check_nan(fy) || fabs(fy) > 1e6)
        DAG_FATAL("bad fy=%.3f", fy);
#endif
      physBody->addImpulse(mtx.getcol(3), wUp * (fy * ifps));
    }
  }
  if (should_activate)
    physBody->activateBody(true);
}

// ---------------------------------------------------
// // Wheel related
// ---------------------------------------------------
void PhysRbRayCarBase::Wheel::setParameters(float _mass)
{
  mass = _mass;
  inertia = 1.0f / 2.0f * radius * radius * _mass;
}

void PhysRbRayCarBase::Wheel::onWheelContact(bool has_hit, const PhysRbRayCarBase::Wheel::Hit &hit, float ifps)
{
  float spring_newl;
  float cosa = 0;

  if (has_hit)
  {
    spring_newl = hit.distance - radius;
    groundNorm = hit.worldNormal;
    cosa = dot(wSpringAxis, groundNorm);

    if (spring_newl < spring.lmin)
      spring_newl = spring.lmin;
    contact = cosa <= -0.86;
  }
  else
  {
    spring_newl = spring.lmax;
    contact = false;
  }

  G_ASSERT(ifps > 1e-6);
  // float prevL = spring.curL;
  spring.curV = (spring_newl - spring.curL) / ifps;
  spring.curL = spring_newl;

  if (contact)
  {
    freeWheelFriction = damping;
    suspForce = spring.calcForce() * cosa;
    wheelFz = -spring.calcSpringOnlyForce() * cosa;

    if (hit.distance - radius < spring.lmin)
    {
      suspForce += (hit.distance - radius - spring.lmin - 0.001) * spring.k * 10;
      float maxforce = -1.2 * 9.81 / body->getInvMass();
      if (suspForce < maxforce)
        suspForce = maxforce;
    }
    contactPmid = hit.materialIndex;
    G_ASSERT(IsPhysMatID_Valid(contactPmid));
  }
  else
  {
    freeWheelFriction = damping + 1.0f;
    wheelFz = suspForce = 0;
    force_torque = 0;
    contactPmid = -1;
  }

  // debug("[%d] prevL=%.6f, curL=%.6f curV=%.6f contact=%d (cosa=%g) suspForce=%.3f wSpringAxis=%@ gn=%@",
  //   wid, prevL, spring.curL, spring.curV, contact, cosa, suspForce, to_point3(wSpringAxis), to_point3(groundNorm));
#if DAGOR_DBGLEVEL > 0
  if (check_nan(suspForce) || check_nan(wheelFz))
    DAG_FATAL("invalid suspForce=%.3f or wheelFz=%.3f", suspForce, wheelFz);
  if (fabs(suspForce) > 1e6 || fabs(wheelFz) > 1e6)
    DAG_FATAL("extra large suspForce=%.3f or wheelFz=%.3f", suspForce, wheelFz);
#endif
}

void PhysRbRayCarBase::Wheel::response()
{
  if (!contact)
    return;

  TMatrix mtx;
  body->getTm(mtx);
  Point3 wheel_basis[3];
  Point3 contact_pt = mtx * spring.wheelBottomPos(radius);
  Point3 ground_vel = body->getPointVelocity(contact_pt);

  float cptFriction = PhysMat::getInteractProps(wheelPmid, contactPmid).frict_k;
  forcePt = contact_pt;

  ////////////////////////////////
  // wheel basis
  ////////////////////////////////
  wheel_basis[B_UP] = mtx % upDir;
  wheel_basis[B_FWD] = Quat(wheel_basis[B_UP], -DEG_TO_RAD * (steering_angle + 90)) * (mtx % axis);
  wheel_basis[B_SIDE] = cross(groundNorm, wheel_basis[B_FWD]);
  wheel_basis[B_FWD] = cross(wheel_basis[B_SIDE], groundNorm);

  float f_lat, f_long;
  cwcf->calcWheelContactForces(wid, wheelFz, radius, wheel_basis, groundNorm, angular_velocity, ground_vel, cptFriction, f_lat,
    f_long);

#if DAGOR_DBGLEVEL > 0
  if (check_nan(f_lat) || check_nan(f_long))
    DAG_FATAL("invalid f_lat=%.3f or f_long=%.3f", f_lat, f_long);
  if (fabs(f_lat) > 1e6 || fabs(f_long) > 1e6)
    // DAG_FATAL("extra large f_lat=%.3f or f_long=%.3f", f_lat, f_long);
    LOGERR_CTX("extra large f_lat=%.3f or f_long=%.3f", f_lat, f_long);
#else
  if (check_nan(f_lat) || check_nan(f_long))
  {
    f_lat = 0;
    f_long = 0;
  }
  if (fabs(f_lat) > 1e6 || fabs(f_long) > 1e6)
  {
    f_lat = 0;
    f_long = 0;
  }
#endif
  latForce += f_lat * wheel_basis[B_SIDE];
  longForce += f_long * wheel_basis[B_FWD];
  force_torque = -f_long * radius;
}

void PhysRbRayCarBase::Wheel::processContactHandler(int pmid, const Point3 &pos, const Point3 &norm)
{
  G_ASSERT(wheelContactCb);

  Point3 carVel = body->getVelocity();
  if (carVel.lengthSq() > 0.01f)
  {
    TMatrix mtx;
    body->getTm(mtx);
    Point3 wheelAxis = normalize(mtx % axis);

    Point3 wUp = -wSpringAxis;
    Point3 wSide = makeTM(wUp, -steering_angle * DEG_TO_RAD) * wheelAxis;
    Point3 wFwd = -normalize(wSide % wUp);

    Point3 cpWorldVel = body->getPointVelocity(pos);
    Point3 cpLocalVel = wFwd * (-angular_velocity * radius);

    Point3 slip = cpWorldVel - cpLocalVel;
    slip -= norm * dot(slip, norm);

    //    DEBUG_CTX("wheel world vel = " FMT_P3 ", len = %f", P3D(cpWorldVel), cpWorldVel.magnitude());
    //    DEBUG_CTX("wheel local vel = " FMT_P3 ", len = %f", P3D(cpLocalVel), cpLocalVel.magnitude());
    //    DEBUG_CTX("slip = " FMT_P3 ", len = %f", P3D(slip), slip.magnitude());

    carVel.normalize();
    wheelContactCb->onWheelContact(wid, pmid, pos, norm, carVel, wFwd, slip.length());
  }
}

void PhysRbRayCarBase::Wheel::calculateAcc(float ifps, float &acc)
{
  float old_angular_velocity = angular_velocity, old_angvel = angular_velocity;
  angular_velocity += (add_torque + force_torque) * ifps / inertia;

  real max_brake_torque = -angular_velocity * inertia / ifps;
  if (brake_torque > fabs(max_brake_torque))
    angular_velocity = 0;
  else if (max_brake_torque < 0)
    angular_velocity += -brake_torque * ifps / inertia;
  else
    angular_velocity += brake_torque * ifps / inertia;
  if (fabs(old_angular_velocity) > U_EPSILON && old_angular_velocity * angular_velocity < 0.0)
    angular_velocity = 0.0f;

  // rotation damping
  old_angular_velocity = angular_velocity;
  angular_velocity -= angular_velocity * freeWheelFriction * ifps;

  if (fabs(old_angular_velocity) > U_EPSILON && old_angular_velocity * angular_velocity < 0.0)
    angular_velocity = 0.0f;

  acc = (angular_velocity - old_angvel) / ifps;
  angular_velocity = old_angvel;
}