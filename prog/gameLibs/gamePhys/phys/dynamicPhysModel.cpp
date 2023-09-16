#include <gamePhys/phys/dynamicPhysModel.h>
#include <gamePhys/props/atmosphere.h>
#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/contactData.h>

#include <memory/dag_framemem.h>

using namespace gamephys;
DynamicPhysModel::DynamicPhysModel(DynamicPhysModel &&) = default;
DynamicPhysModel &DynamicPhysModel::operator=(DynamicPhysModel &&) = default;
DynamicPhysModel::~DynamicPhysModel() = default;
DynamicPhysModel::DynamicPhysModel(const TMatrix &tm, float bodyMass, const Point3 &moment, const Point3 &CoG, PhysType phys_type) :
  velocity(0.f, 0.f, 0.f),
  omega(0.f, 0.f, 0.f),
  mass(bodyMass),
  momentOfInertia(moment),
  centerOfGravity(CoG),
  physType(phys_type),
  active(phys_type == E_FORCED_POS_BY_COLLISION),
  timeFromCollision(0.f)
{
  location.fromTM(tm);
  originalLoc = location;
}

void DynamicPhysModel::applyImpulse(const Point3 &impulse, const Point3 &arm, Point3 &outVel, Point3 &outOmega, float invMass,
  const Point3 &invMomentOfInertia)
{
  outVel += impulse * invMass;
  Point3 angularImpulseMomentum = impulse % (arm - centerOfGravity);
  angularImpulseMomentum.x *= invMomentOfInertia.x;
  angularImpulseMomentum.y *= invMomentOfInertia.y;
  angularImpulseMomentum.z *= invMomentOfInertia.z;
  outOmega += angularImpulseMomentum;
}

void DynamicPhysModel::update(float dt)
{
  if (physType == E_ANGULAR_SPRING || physType == E_FORCED_POS_BY_COLLISION)
  {
    float maxDepth = -FLT_MAX;
    int maxDepthIdx = -1;
    for (int i = 0; i < contacts.size(); ++i)
    {
      const CollisionContactData &contact = contacts[i];
      float depth = rabs(contact.depth);
      if (depth > maxDepth)
      {
        maxDepth = depth;
        maxDepthIdx = i;
      }
    }

    if (maxDepthIdx >= 0)
    {
      Point3 objectUpVect = location.O.getQuat().getUp();
      const CollisionContactData &contact = contacts[maxDepthIdx];
      Point3 contactPos = contact.wpos;
      float t = 0.f;
      Point3 pt = contactPos;
      distanceToLine(contactPos, Point3::xyz(location.P), objectUpVect, t, pt);
      Point3 toContact = contactPos - (Point3::xyz(location.P) + (contactPos - pt));
      float depth = rabs(contact.depth);
      Point3 contactNorm = contact.wnormB;

      float distToContactSq = lengthSq(toContact);
      if (distToContactSq < 0.001f)
        return;

      float angle = unsafe_atan2(depth, sqrtf(distToContactSq));
      Point3 axis = Point3::x0z(toContact % contactNorm);

      axis.normalize();
      axis = ::makeTM(inverse(location.O.getQuat())) % axis;

      Quat rot(axis, angle * 0.5f);
      Quat res = location.O.getQuat() * rot;
      location.O.setQuat(approach(location.O.getQuat(), res, dt, 0.1f));
      omega.zero();
      active = true;
      timeFromCollision = 0.f;
    }
    else if (physType == E_ANGULAR_SPRING)
    {
      timeFromCollision += dt;
      Point3 from = location.O.getQuat() * Point3(0.f, 1.f, 0.f);
      Point3 to = originalLoc.O.getQuat() * Point3(0.f, 1.f, 0.f);
      float omegaToAdd = (1.f - from * to) * min(timeFromCollision * 2.f, 3.f);
      omega += (from % to) * omegaToAdd;
      omega *= 0.91f;

      if (timeFromCollision < 4.f)
      {
        Quat omegaInc(normalize(omega), length(omega) * dt);
        Quat res = approach(location.O.getQuat() * omegaInc, originalLoc.O.getQuat(), dt, 4.f);
        location.O.setQuat(res);
        active = true;
      }
      else
      {
        Quat res = qinterp(location.O.getQuat(), originalLoc.O.getQuat(), 0.5f);
        location.O.setQuat(res);
        active = false;
      }
    }
  }
  else if (physType == E_FALL_WITH_FIXED_POS)
  {
    TMatrix tm = TMatrix::IDENT;
    location.toTM(tm);
    TMatrix itm = inverse(tm);
    Quat invOrient = inverse(location.O.getQuat());
    Point3 pseudoVel = Point3(0, 0, 0);
    Point3 pseudoOmega = Point3(0, 0, 0);

    Tab<CollisionInfo> collision(framemem_ptr());
    for (int i = 0; i < contacts.size(); ++i)
    {
      CollisionInfo &info = collision.push_back();
      info.PnT = itm * contacts[i].wpos;
      info.d = rabs(contacts[i].depth);
      info.normal = invOrient * contacts[i].wnormB;
    }

    Point3 localVel = invOrient * velocity;
    Point3 addVel = Point3(0.f, 0.f, 0.f);
    Point3 addOmega = Point3(0.f, 0.f, 0.f);
    float invMass = safeinv(mass);
    Point3 invMomentOfInertia(safeinv(momentOfInertia.x), safeinv(momentOfInertia.y), safeinv(momentOfInertia.z));
    // Gravity pull
    velocity += Point3(0.f, -atmosphere::g(), 0.f) * dt;

    // Apply sequential impulses with pseudovelicities
    for (int iteration = 0; iteration < 5; ++iteration)
    {
      for (int i = 0; i < collision.size(); ++i)
      {
        CollisionInfo &info = collision[i];
        Point3 vel = localVel + addVel;
        Point3 localOmega = omega + addOmega;
        Point3 n1 = info.normal;
        Point3 w1 = Point3::xyz(info.normal) % (Point3::xyz(info.PnT) - centerOfGravity);
        float a = n1 * vel + w1 * localOmega;
        float b =
          (n1 * n1 * invMass + w1 * Point3(w1.x * invMomentOfInertia.x, w1.y * invMomentOfInertia.y, w1.z * invMomentOfInertia.z));
        float lambda = -safediv(a, b);
        if (lambda < 0.f)
          continue;
        applyImpulse(n1 * lambda, info.PnT, addVel, addOmega, invMass, invMomentOfInertia);
      }
    }

    velocity += location.O.getQuat() * addVel;
    omega += addOmega;

    // pseudovelocities
    localVel = invOrient * velocity;
    for (int iteration = 0; iteration < 5; ++iteration)
    {
      for (int i = 0; i < collision.size(); ++i)
      {
        CollisionInfo &info = collision[i];
        Point3 vel = localVel + pseudoVel;
        Point3 localOmega = omega + pseudoOmega;
        Point3 n1 = info.normal;
        Point3 w1 = Point3::xyz(info.normal) % (Point3::xyz(info.PnT) - centerOfGravity);
        float a = n1 * vel + w1 * localOmega;
        float b =
          (n1 * n1 * invMass + w1 * Point3(w1.x * invMomentOfInertia.x, w1.y * invMomentOfInertia.y, w1.z * invMomentOfInertia.z));
        const float ERP = 1.0;
        float lambda = safediv(ERP * rabs(info.d) - a, b);
        if (lambda < 0.f)
          continue;
        applyImpulse(n1 * lambda, info.PnT, pseudoVel, pseudoOmega, invMass, invMomentOfInertia);
      }
    }

    // Integrate
    Point3 omegaInc = (omega + pseudoOmega) * dt;
    location.O.increment(-RadToDeg(omegaInc.y), -RadToDeg(omegaInc.z), RadToDeg(omegaInc.x));
    location.P += (velocity + location.O.getQuat() * pseudoVel) * dt;
  }
  else if (physType == E_PHYS_OBJECT)
  {
    active = contacts.size() > 0;
  }
  clear_and_shrink(contacts);
}
