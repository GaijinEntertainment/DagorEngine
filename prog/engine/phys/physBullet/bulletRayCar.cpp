// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bulletRayCar.h"
#include <phys/dag_physics.h>
#include <scene/dag_physMat.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <workCycle/dag_workCyclePerf.h>
#if BT_BULLET_VERSION >= 325
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#endif

#define U_EPSILON 1e-6

static bool (*custom_tracer)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid) = nullptr;

BulletRbRayCar::BulletRbRayCar(PhysBody *pb, int iter) : PhysRbRayCarBase(pb, iter), object(pb->getActor())
{
  G_ASSERT(physBody);
  initAero();
}

BulletRbRayCar::~BulletRbRayCar()
{
  for (int i = 0; i < wheels.size(); i++)
    delete wheels[i];
}


void BulletRbRayCar::Wheel::init(PhysBody *_body, PhysBody *_wheel, float wheel_rad, const Point3 &_axis, int physmat_id)
{
  wheelPmid = physmat_id;
  body = _body;
  wheel = _wheel;

  axis = normalize(_axis);
  upDir = -spring.axis;
  radius = wheel_rad;

  integratePosition(0.0f);
  cwcf = NULL;
  wheelContactCb = NULL;
}

void BulletRbRayCar::Wheel::addRayQuery(float ifps)
{
  TMatrix mtx;
  body->getTm(mtx);
  Point3 spring_fixpt = mtx * spring.fixPt;
  wSpringAxis = normalize(mtx % spring.axis);

  longForce = latForce = Point3(0, 0, 0);
  Hit hit;
  hit.distance = spring.lmax + radius;
  if (custom_tracer)
  {
    int pmid;
    if (custom_tracer(spring_fixpt, wSpringAxis, hit.distance, hit.worldNormal, pmid))
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

    btVector3 p0 = to_btVector3(spring_fixpt), p1 = to_btVector3(spring_fixpt + wSpringAxis * hit.distance);
    RayCastCallback resultCallback(p0, p1, body->getActor(), wheel ? wheel->getActor() : nullptr);
#if BT_BULLET_VERSION >= 325
    resultCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
#endif
    physWorld->getScene()->rayTest(p0, p1, resultCallback);
    if (resultCallback.hasHit())
    {
      hit.worldNormal = to_point3(resultCallback.m_hitNormalWorld);
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
    Point3 pos = spring_fixpt + wSpringAxis * (spring.curL + radius);
    processContactHandler(contactPmid, pos, groundNorm);
  }
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

  TMatrix mtx;
  physBody->getTm(mtx);
  float imass = 4.0 * object->getInvMass();

  for (int i = 0; i < wheels.size(); i++)
  {
    Wheel &w = *wheels[i];
    Point3 wUp = normalize(mtx % w.upDir);
    Point3 wSide = makeTM(wUp, -w.steering_angle * DEG_TO_RAD) * normalize(mtx % w.axis);
    Point3 wFwd = normalize(wSide % wUp);
    Point3 wPos = mtx * w.spring.wheelPos();

    TMatrix rot;
    rot.makeTM(wSide, w.rotation_angle);

    draw_cached_debug_circle(wPos, rot * wUp, rot * wFwd, w.radius, E3DCOLOR(120, w.contact ? 255 : 0, 255, 255));

    // draw_cached_debug_sphere(to_point3(forcePt), 0.05, E3DCOLOR(255,255,255,255), 12);
    Point3 cp = wPos - wUp * w.radius;
    draw_cached_debug_line(wPos, wPos + physBody->getPointVelocity(wPos), E3DCOLOR(120, 120, 120, 255));

    // draw_cached_debug_line(cp, cp + w.groundNorm * (w.wheelFz * imass / 9.81), E3DCOLOR(120, 120, 120, 255));
    BBox3 bb(cp, 0.3f);
    draw_cached_debug_box(bb, E3DCOLOR(120, 120, 120, 255));
    draw_cached_debug_line(cp, cp + w.groundNorm * (w.wheelFz / 1000), E3DCOLOR(120, 120, 120, 255));

    draw_cached_debug_line(cp, cp + w.longForce * imass / iterCount / 9.81, E3DCOLOR(255, 120, 255, 255));
    draw_cached_debug_line(cp, cp + w.latForce * imass / iterCount / 9.81, E3DCOLOR(255, 255, 120, 255));
    draw_cached_debug_line(cp, cp + (w.longForce + w.latForce) * imass / iterCount / 9.81, E3DCOLOR(120, 120, 255, 255));
  }
  Point3 pos = mtx.getcol(3);
  Point3 linVel = physBody->getVelocity();
  // btVector3 angVel = object->getAngularVelocity();

  draw_cached_debug_sphere(pos, 0.05, E3DCOLOR(255, 255, 255, 255), 12);
  /*draw_cached_debug_line(to_point3(pos),
                         to_point3(pos+lastForce*imass/9.81),
                         E3DCOLOR(120,120,255,255));*/
  draw_cached_debug_line(pos, pos + linVel, E3DCOLOR(120, 120, 120, 255));

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
