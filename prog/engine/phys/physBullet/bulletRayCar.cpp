#include "bulletRayCar.h"
#include <phys/dag_physics.h>
#include <scene/dag_physMat.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <workCycle/dag_workCyclePerf.h>

#define U_EPSILON 1e-6
const btVector3 gravity(0, -9.8, 0);

inline float dot(const btVector3 &a, const btVector3 &b) { return a.dot(b); }
inline btVector3 cross(const btVector3 &a, const btVector3 &b) { return a.cross(b); }
inline float length(const btVector3 &a) { return a.length(); }
inline btVector3 normalize(const btVector3 &a) { return a.normalized(); }
inline btVector3 getPointVelocity(btRigidBody *rb, btVector3 pt)
{
  return btCross(rb->getAngularVelocity(), pt - rb->getCenterOfMassPosition()) + rb->getLinearVelocity();
}
static bool (*custom_tracer)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid) = nullptr;


void BulletRbRayCar::Wheel::init(PhysBody *_body, PhysBody *_wheel, float wheel_rad, const btVector3 &_axis, int physmat_id)
{
  wheelPmid = physmat_id;
  body = _body->getActor();
  wheel = _wheel ? _wheel->getActor() : nullptr;

  axis = normalize(_axis);
  upDir = -spring.axis;
  radius = wheel_rad;

  integratePosition(0.0f);
  cwcf = NULL;
  wheelContactCb = NULL;
}

void BulletRbRayCar::Wheel::setParameters(float _mass)
{
  mass = _mass;
  inertia = 1.0f / 2.0f * radius * radius * _mass;
}

void BulletRbRayCar::Wheel::addRayQuery(float ifps)
{
  btTransform body_m44;
  body->getMotionState()->getWorldTransform(body_m44);
  btVector3 spring_fixpt = body_m44 * spring.fixPt;
  wSpringAxis = normalize(body_m44.getBasis() * spring.axis);

  longForce = latForce = btVector3(0, 0, 0);
  Hit hit;
  hit.distance = spring.lmax + radius;
  if (custom_tracer)
  {
    int pmid;
    if (custom_tracer(to_point3(spring_fixpt), to_point3(wSpringAxis), hit.distance, to_point3(hit.worldNormal), pmid))
    {
      hit.materialIndex = pmid;
      onWheelContact(true, hit, ifps);
    }
    else
      onWheelContact(false, hit, ifps);
  }
  else
  {
    struct RayCastCallback : public btCollisionWorld::ClosestRayResultCallback
    {
      RayCastCallback(const btVector3 &p0, const btVector3 &p1, void *rb0, void *rb1) :
        btCollisionWorld::ClosestRayResultCallback(p0, p1), excl_rb0(rb0), excl_rb1(rb1)
      {}
      bool needsCollision(btBroadphaseProxy *proxy0) const override
      {
        bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
        collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);
        void *o = proxy0->m_clientObject;
        if (collides && o)
          collides = o != excl_rb0 && o != excl_rb1;
        return collides;
      }
      void *excl_rb0, *excl_rb1;
    };

    btVector3 p0 = spring_fixpt, p1 = spring_fixpt + wSpringAxis * hit.distance;
    RayCastCallback resultCallback(p0, p1, body, wheel);
    physWorld->getScene()->rayTest(p0, p1, resultCallback);
    if (resultCallback.hasHit())
    {
      hit.worldNormal = resultCallback.m_hitNormalWorld;
      if (resultCallback.m_collisionObject)
        if (auto *physBody = PhysBody::from_bt_body(resultCallback.m_collisionObject))
          hit.body = physBody;

      hit.distance *= resultCallback.m_closestHitFraction;
      if (hit.body)
        hit.materialIndex = PhysMat::getMaterialIdByPhysBodyMaterial(hit.body->getMaterialId());
      if (hit.materialIndex == -1)
        hit.materialIndex = 0;
      // debug("hit[%d]: pt=%@ n=%@ car=%p rb=%p", wid,
      //   to_point3(spring_fixpt + wSpringAxis*hit.distance), to_point3(hit.worldNormal), body, hit.body ? hit.body->getActor() :
      //   nullptr);
      onWheelContact(true, hit, ifps);
    }
    else
      onWheelContact(false, hit, ifps);
  }

  if (contact && wheelContactCb)
  {
    btVector3 pos = spring_fixpt + wSpringAxis * (spring.curL + radius);
    processContactHandler(contactPmid, pos, groundNorm);
  }
}

void BulletRbRayCar::Wheel::onWheelContact(bool has_hit, const BulletRbRayCar::Wheel::Hit &hit, float ifps)
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

void BulletRbRayCar::Wheel::processContactHandler(int pmid, const btVector3 &pos, const btVector3 &norm)
{
  G_ASSERT(wheelContactCb);

  btVector3 carVel = body->getLinearVelocity();
  if (carVel.length2() > 0.01f)
  {
    btTransform body_m44;
    body->getMotionState()->getWorldTransform(body_m44);
    btVector3 wheelAxis = normalize(body_m44.getBasis() * axis);

    Point3 wUp = -to_point3(wSpringAxis);
    Point3 wSide = makeTM(wUp, -steering_angle * DEG_TO_RAD) * to_point3(wheelAxis);
    Point3 wFwd = -normalize(wSide % wUp);

    btVector3 cpWorldVel = getPointVelocity(body, pos);
    btVector3 cpLocalVel = to_btVector3(wFwd) * (-angular_velocity * radius);

    btVector3 slip = cpWorldVel - cpLocalVel;
    slip -= norm * (slip.dot(norm));

    //    debug_ctx("wheel world vel = " FMT_P3 ", len = %f", P3D(cpWorldVel), cpWorldVel.magnitude());
    //    debug_ctx("wheel local vel = " FMT_P3 ", len = %f", P3D(cpLocalVel), cpLocalVel.magnitude());
    //    debug_ctx("slip = " FMT_P3 ", len = %f", P3D(slip), slip.magnitude());

    carVel.normalize();
    wheelContactCb->onWheelContact(wid, pmid, to_point3(pos), to_point3(norm), to_point3(carVel), wFwd, slip.length());
  }
}

void BulletRbRayCar::Wheel::response()
{
  if (!contact)
    return;

  btTransform body_m44;
  body->getMotionState()->getWorldTransform(body_m44);
  btVector3 wheel_basis[3];
  btVector3 contact_pt = body_m44 * spring.wheelBottomPos(radius);
  btVector3 ground_vel = getPointVelocity(body, contact_pt);

  float cptFriction = PhysMat::getInteractProps(wheelPmid, contactPmid).frict_k;
  forcePt = contact_pt;

  ////////////////////////////////
  // wheel basis
  ////////////////////////////////
  wheel_basis[B_UP] = body_m44.getBasis() * upDir;
  wheel_basis[B_FWD] =
    btMatrix3x3(btQuaternion(wheel_basis[B_UP], -DEG_TO_RAD * (steering_angle + 90))) * (body_m44.getBasis() * axis);
  wheel_basis[B_SIDE] = groundNorm.cross(wheel_basis[B_FWD]);
  wheel_basis[B_FWD] = wheel_basis[B_SIDE].cross(groundNorm);

  float f_lat, f_long;
  Point3 wheel_basis_p3[3] = {to_point3(wheel_basis[0]), to_point3(wheel_basis[1]), to_point3(wheel_basis[2])};
  cwcf->calcWheelContactForces(wid, wheelFz, radius, wheel_basis_p3, to_point3(groundNorm), angular_velocity, to_point3(ground_vel),
    cptFriction, f_lat, f_long);

#if DAGOR_DBGLEVEL > 0
  if (check_nan(f_lat) || check_nan(f_long))
    DAG_FATAL("invalid f_lat=%.3f or f_long=%.3f", f_lat, f_long);
  if (fabs(f_lat) > 1e6 || fabs(f_long) > 1e6)
    // DAG_FATAL("extra large f_lat=%.3f or f_long=%.3f", f_lat, f_long);
    logerr_ctx("extra large f_lat=%.3f or f_long=%.3f", f_lat, f_long);
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

void BulletRbRayCar::Wheel::calculateAcc(float ifps, float &acc)
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

//
//
//
BulletRbRayCar::BulletRbRayCar(PhysBody *pb, int iter) : object(pb->getActor()), physBody(pb), wheels(midmem), cwaa(this), axle(midmem)
{
  setAirDrag(0.3);
  setSideAirDrag(18.0, 0.6);
  setAirLift(0.0, 0.0, 0.0);
  setSideAirLift(0.0, 0.0);
  iterCount = iter;
}

BulletRbRayCar::~BulletRbRayCar()
{
  for (int i = 0; i < wheels.size(); i++)
    delete wheels[i];
}

void BulletRbRayCar::destroyVehicle() { delete this; }

int BulletRbRayCar::addWheel(PhysBody *b, float wheel_rad, const Point3 &axis, int physmat_id)
{
  if (b)
    b->setMassMatrix(0, 0, 0, 0);

  int wid = wheels.size();
  Wheel *w = new Wheel(wid, physBody->getPhysWorld());
  w->init(physBody, b, wheel_rad, to_btVector3(axis), physmat_id);
  w->cwcf = this;
  wheels.push_back(w);
  return wheels.size() - 1;
}

void BulletRbRayCar::setSpringPoints(int wheel_id, const Point3 &fixpt, const Point3 &spring, float lmin, float lmax, float l0,
  float forcept, float restpt, float /*long_force_pt_in_ofs*/, float /*long_f_pt_down_ofs*/)
{
  if (wheel_id < 0 || wheel_id >= wheels.size())
    return;

  Spring &s = wheels[wheel_id]->spring;
  s.fixPt = to_btVector3(fixpt);
  s.axis = normalize(to_btVector3(spring));
  s.lmin = lmin;
  s.lmax = lmax;
  s.l0 = l0;
  s.lrest = restpt >= lmin ? restpt : (lmin + lmax) / 2;
  if (!float_nonzero(forcept + 1))
    forcept = s.lmin;
  s.forcePt = s.fixPt + s.axis * forcept;
  s.curL = s.lrest;
  btTransform body_m44;
  object->getMotionState()->getWorldTransform(body_m44);
  wheels[wheel_id]->upDir = -s.axis;
  wheels[wheel_id]->wPos = to_point3(s.wheelPos());
  wheels[wheel_id]->wSpringAxis = normalize(body_m44.getBasis() * s.axis);
}

void BulletRbRayCar::setSpringHardness(int wheel_id, float k, float kd_up, float kd_down)
{
  if (wheel_id < 0 || wheel_id >= wheels.size())
    return;

  Spring &s = wheels[wheel_id]->spring;
  s.k = k;
  s.kdUp = kd_up;
  s.kdDown = kd_down;
}

void BulletRbRayCar::setWheelParams(int wheel_id, float mass, float /*friction*/)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->setParameters(mass);
}
void BulletRbRayCar::setCwcf(unsigned wheels_mask, IPhysVehicle::ICalcWheelContactForces *cwcf)
{
  for (int i = 0; i < wheels.size(); i++)
    if (wheels_mask & (1 << i))
      wheels[i]->cwcf = cwcf ? cwcf : this;
}
float BulletRbRayCar::getWheelInertia(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->inertia;
}

void BulletRbRayCar::addWheelTorque(int wheel_id, float torque)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->add_torque = torque;
}
void BulletRbRayCar::setWheelSteering(int wheel_id, float angle)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->steering_angle = angle;
}

float BulletRbRayCar::getWheelSteering(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->steering_angle;
}

float BulletRbRayCar::getWheelAngularVelocity(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->angular_velocity;
}
float BulletRbRayCar::getWheelFeedbackTorque(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->force_torque;
}
bool BulletRbRayCar::getWheelContact(int wheel_id)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  return wheels[wheel_id]->contact;
}
bool BulletRbRayCar::getWheelDrift(int /*wheel_id*/, float & /*long_vel*/, float & /*lat_vel*/) { return false; }

void BulletRbRayCar::setWheelDamping(int wheel_id, float damping)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->damping = damping;
}

void BulletRbRayCar::setWheelBrakeTorque(int wheel_id, float torque)
{
  G_ASSERT(wheel_id >= 0 && wheel_id < wheels.size());
  wheels[wheel_id]->brake_torque = torque;
}
void BulletRbRayCar::setAxleParams(int left_wid, int right_wid, bool rigid_axle, float arb_k)
{
  int a_id = getAxle(left_wid, right_wid);
  axle[a_id].arbK = arb_k;
  axle[a_id].rigid = rigid_axle;
}
int BulletRbRayCar::getAxle(int left_wid, int right_wid)
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

void BulletRbRayCar::resetWheels()
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
    w.wPos = to_point3(w.spring.wheelPos());
  }
}

void BulletRbRayCar::updateInactiveVehicle(float ifps, float wheel_lin_vel)
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

void BulletRbRayCar::findContacts(float ifps)
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
    else if (fr - dFz > 0)
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
      btVector3 fwd = wl.spring.axis.cross(wl.axis);
      fwd.normalize();
      wl.upDir = wr.upDir = fwd.cross(wl.axis);
    }
  }
}

void BulletRbRayCar::calculateForce(float ifps)
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

  bool should_activate = false;
  for (int i = 0; i < wheels.size(); i++)
    if (wheels[i]->contact)
    {
      btVector3 rel_pos = wheels[i]->forcePt - object->getCenterOfMassPosition();
      // debug("wheel[%d] rel_pos=%@ lng=%@ lat=%@ iter_dt=%g",
      //   i, to_point3(rel_pos), to_point3(wheels[i]->longForce), to_point3(wheels[i]->latForce), iter_dt);
      object->applyImpulse((wheels[i]->longForce + wheels[i]->latForce) * iter_dt, rel_pos);
      should_activate |= fabs(wheels[i]->angular_velocity) > 1e-3;
    }
    else
      should_activate = true;

  btVector3 vel = object->getLinearVelocity();
  float velm = vel.length();
  float ad = airDrag, ads = airDragSq;
  float long_wt = 1, lat_wt = 0;
  btTransform body_m44;
  object->getMotionState()->getWorldTransform(body_m44);

  if (velm > 0.1)
  {
    long_wt = fabs(body_m44.getBasis().getColumn(1).dot(vel)) / velm;
    lat_wt = fabs(body_m44.getBasis().getColumn(2).dot(vel)) / velm;
    ad = (airDrag * long_wt + airDragSide * lat_wt);
    ads = (airDragSq * long_wt + airDragSqSide * lat_wt);
  }

  object->applyCentralImpulse(vel * (-(ad + velm * ads) * ifps));
  if (airMaxLiftForce > 0)
  {
    btVector3 wUp = wheels[0]->wSpringAxis;
    float vxz = (vel - vel * vel.dot(wUp)).length();

    if (fabs(vel.dot(wUp)) < fabs(vxz) * 2)
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
      object->applyCentralImpulse(wUp * (fy * ifps));
    }
  }
  if (should_activate)
    physBody->activateBody(true);
}

void BulletRbRayCar::calcWheelContactForces(int /*wid*/, float norm_force, float load_rad, const Point3 wheel_basis[3],
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

unsigned BulletRbRayCar::calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt)
{
  for (int i = 0; i < wheel_num; i++)
    if (wheel_need_mask & (1 << i))
      wheels[i]->calculateAcc(dt, wheel_acc[i]);
  return wheel_need_mask;
}

void BulletRbRayCar::integratePosition(float ifps)
{
  for (int i = 0; i < wheels.size(); i++)
    wheels[i]->integratePosition(ifps);
}

#if DAGOR_DBGLEVEL > 0
#include <debug/dag_debug3d.h>
#include <math/dag_TMatrix.h>
#endif

void BulletRbRayCar::debugRender()
{
#if DAGOR_DBGLEVEL > 0
  ::begin_draw_cached_debug_lines(false);
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  btTransform body_m44;
  object->getMotionState()->getWorldTransform(body_m44);
  float imass = 4.0 * object->getInvMass();

  for (int i = 0; i < wheels.size(); i++)
  {
    Wheel &w = *wheels[i];
    Point3 wUp = to_point3(normalize(body_m44.getBasis() * -w.upDir));
    Point3 wSide = makeTM(wUp, w.steering_angle * DEG_TO_RAD) * to_point3(normalize(body_m44.getBasis() * w.axis));
    Point3 wFwd = normalize(wUp % wSide);
    Point3 wPos = to_point3(body_m44 * w.spring.wheelPos());

    TMatrix rot;
    rot.makeTM(wSide, w.rotation_angle);

    draw_cached_debug_circle(wPos, rot * wUp, rot * wFwd, w.radius, E3DCOLOR(120, w.contact ? 255 : 0, 255, 255));

    // draw_cached_debug_sphere(to_point3(forcePt), 0.05, E3DCOLOR(255,255,255,255), 12);
    btVector3 cp = to_btVector3(wPos - wUp * w.radius);
    draw_cached_debug_line(wPos, wPos + to_point3(getPointVelocity(object, to_btVector3(wPos))), E3DCOLOR(120, 120, 120, 255));

    draw_cached_debug_line(to_point3(cp), to_point3(cp + w.groundNorm * (w.wheelFz * imass / 9.81)), E3DCOLOR(120, 120, 120, 255));

    draw_cached_debug_line(to_point3(cp), to_point3(cp + w.longForce * imass / iterCount / 9.81), E3DCOLOR(255, 120, 255, 255));
    draw_cached_debug_line(to_point3(cp), to_point3(cp + w.latForce * imass / iterCount / 9.81), E3DCOLOR(255, 255, 120, 255));
    draw_cached_debug_line(to_point3(cp), to_point3(cp + (w.longForce + w.latForce) * imass / iterCount / 9.81),
      E3DCOLOR(120, 120, 255, 255));
  }
  btVector3 pos = body_m44.getOrigin();
  btVector3 linVel = object->getLinearVelocity();
  // btVector3 angVel = object->getAngularVelocity();

  draw_cached_debug_sphere(to_point3(pos), 0.05, E3DCOLOR(255, 255, 255, 255), 12);
  /*draw_cached_debug_line(to_point3(pos),
                         to_point3(pos+lastForce*imass/9.81),
                         E3DCOLOR(120,120,255,255));*/
  draw_cached_debug_line(to_point3(pos), to_point3(pos + linVel), E3DCOLOR(120, 120, 120, 255));

  /*
    float a, b, c, m = object->getMass();
    btVector3 j = object->getMassSpaceInertiaTensor();
    a = sqrt(1.5*(-j.x+j.y+j.z)/m);
    b = sqrt(1.5*(j.x-j.y+j.z)/m);
    c = sqrt(1.5*(j.x+j.y-j.z)/m);
    draw_cached_debug_box(to_point3(tm*btVector3(-a,-b,-c)),
      to_point3(body_m44.getBasis()*btVector3(2*a,0,0)), to_point3(body_m44.getBasis()*btVector3(0, 2*b,0)),
      to_point3(body_m44.getBasis()*btVector3(0, 0, 2*c)),
      object->isSleeping()?E3DCOLOR(0,0,255,255):E3DCOLOR(255,255,255,255));
  */
  ::end_draw_cached_debug_lines();
#endif
}

IPhysVehicle *IPhysVehicle::createRayCarBullet(PhysBody *car, int iter_count)
{
  if (!car || !car->getActor())
    return NULL;

  BulletRbRayCar *pv = new (midmem) BulletRbRayCar(car, iter_count);
  car->getActor()->setDamping(0, 0);
  // car->disableGravity(true);
  return pv;
}

void IPhysVehicle::bulletSetStaticTracer(bool (*traceray)(const Point3 &, const Point3 &, float &, Point3 &, int &))
{
  custom_tracer = traceray;
}
