// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joltRayCar.h"
#include <phys/dag_physics.h>
#include <phys/dag_joltHelpers.h>
#include <scene/dag_physMat.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <workCycle/dag_workCyclePerf.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <debug/dag_debug3d.h>


constexpr float MAX_RAY_LENGTH = 1e9; // FLT_MAX can't be used, because physics engine can use square of this value

static bool (*custom_tracer)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid) = nullptr;


void JoltRbRayCar::Wheel::init(PhysBody *_body, PhysBody *_wheel, float wheel_rad, const Point3 &_axis, int physmat_id)
{
  body = _body;
  wheelPmid = physmat_id;
  wheel = _wheel ? PhysBody::from_body_id(_wheel->bodyId) : nullptr;

  axis = normalize(_axis);
  upDir = -spring.axis;
  radius = wheel_rad;

  integratePosition(0.0f);
  cwcf = NULL;
  wheelContactCb = NULL;
}


void JoltRbRayCar::Wheel::addRayQuery(float ifps)
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
    Point3 wNormal = hit.worldNormal;
    if (custom_tracer(spring_fixpt, wSpringAxis, hit.distance, wNormal, pmid))
    {
      hit.materialIndex = pmid;
      onWheelContact(true, hit, ifps);
    }
    else
      onWheelContact(false, hit, ifps);
  }
  else
  {
    PhysRayCast rayCast(spring_fixpt, wSpringAxis, hit.distance, physWorld);
    rayCast.setFilterMask(2); // The proper filter mask enum should be used here
    rayCast.forceUpdate();

    if (rayCast.hasContact())
    {
      Point3 point = rayCast.getPoint();
      Point3 normal = rayCast.getNormal();

      hit.worldNormal = normal;
      hit.body = rayCast.getBody();
      hit.distance = rayCast.getLength();
      if (hit.body)
        hit.materialIndex = rayCast.getMaterial();

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


//
//
//
JoltRbRayCar::JoltRbRayCar(PhysBody *pb, int iter) : PhysRbRayCarBase(pb, iter), object(pb->bodyId) { G_ASSERT(physBody); }

JoltRbRayCar::~JoltRbRayCar()
{
  for (int i = 0; i < wheels.size(); i++)
    delete wheels[i];
}


#if DAGOR_DBGLEVEL > 0
#include <debug/dag_debug3d.h>
#include <math/dag_TMatrix.h>
#endif

void JoltRbRayCar::debugRender()
{
#if DAGOR_DBGLEVEL > 0
  ::begin_draw_cached_debug_lines(false);
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  TMatrix mtx;
  physBody->getTm(mtx);
  float imass = 4.0 * physBody->getInvMass();

  for (int i = 0; i < wheels.size(); i++)
  {
    Wheel &w = *wheels[i];
    Point3 wUp = normalize(mtx % -w.upDir);
    Point3 wSide = makeTM(wUp, w.steering_angle * DEG_TO_RAD) * normalize(mtx % w.axis);
    Point3 wFwd = normalize(wUp % wSide);
    Point3 wPos = mtx * w.spring.wheelPos();

    TMatrix rot;
    rot.makeTM(wSide, w.rotation_angle);

    draw_cached_debug_circle(wPos, rot * wUp, rot * wFwd, w.radius, E3DCOLOR(120, w.contact ? 255 : 0, 255, 255));

    // draw_cached_debug_sphere(to_point3(forcePt), 0.05, E3DCOLOR(255,255,255,255), 12);
    Point3 cp = wPos - wUp * w.radius;
    draw_cached_debug_line(wPos, wPos + physBody->getPointVelocity(wPos), E3DCOLOR(120, 120, 120, 255));

    draw_cached_debug_line(cp, cp + w.groundNorm * (w.wheelFz * imass / 9.81), E3DCOLOR(120, 120, 120, 255));

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

IPhysVehicle *IPhysVehicle::createRayCarJolt(PhysBody *car, int iter_count)
{
  if (!car || !car->isValid())
    return NULL;

  JoltRbRayCar *pv = new (midmem) JoltRbRayCar(car, iter_count);
  pv->initAero();
  // car->getActor()->setDamping(0, 0); >> No matching api call
  return pv;
}

void IPhysVehicle::joltSetStaticTracer(bool (*traceray)(const Point3 &, const Point3 &, float &, Point3 &, int &))
{
  custom_tracer = traceray;
}