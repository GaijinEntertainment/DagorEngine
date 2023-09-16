#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/collisionInstances.h>
#include <gamePhys/collision/physLayers.h>
#include <phys/dag_physics.h>
#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <gamePhys/collision/cachedCollisionObject.h>
#include <gamePhys/collision/rendinstCollisionWrapper.h>
#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gameMath/interpolateTabUtils.h>
#include <memory/dag_framemem.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>


float WrapperRendinstContactResultCB::addSingleResult(contact_data_t &, obj_user_data_t *, obj_user_data_t *,
  gamephys::CollisionObjectInfo *)
{
  return 0.f;
}

void WrapperRendinstContactResultCB::applyRiGenDamage() const {}

CollisionObject WrapperRendInstCollisionImplCB::processCollisionInstance(const rendinst::RendInstCollisionCB::CollisionInfo &,
  CollisionObject &, TMatrix &)
{
  return CollisionObject();
}

void WrapperRendInstCollisionImplCB::addCollisionCheck(const rendinst::RendInstCollisionCB::CollisionInfo &) {}

void WrapperRendInstCollisionImplCB::addTreeCheck(const rendinst::RendInstCollisionCB::CollisionInfo &) {}
