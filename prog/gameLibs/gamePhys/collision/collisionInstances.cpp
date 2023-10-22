#include <gamePhys/collision/collisionInstances.h>
#include <gamePhys/collision/physLayers.h>

#include <phys/dag_physDecl.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <phys/dag_physics.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>


namespace dacoll
{

static bool add_instances_to_world = false;
static float default_time_to_live = 0.5f;

static CollisionObject create_bullet_scaled_copy(const CollisionObject &obj, const Point3 &scale, CollisionObjectUserData *user_ptr);

ScaledBulletInstance::ScaledBulletInstance(CICollisionObject original_obj, const Point3 &scl, const rendinst::RendInstDesc &in_desc,
  float time_of_death) :
  timeOfDeath(time_of_death), scale(scl), desc(in_desc)
{
  userData.matId = rendinst::getRIGenMaterialId(desc);
  obj = create_bullet_scaled_copy(original_obj, scale, &userData);
}

ScaledBulletInstance::~ScaledBulletInstance()
{
  if (obj)
    destroy_dynamic_collision(obj);
}

void ScaledBulletInstance::updateUserDataPtr()
{
  if (obj.body)
    obj.body->setUserData(&userData);
}

static CollisionObject create_bullet_scaled_copy(const CollisionObject &obj, const Point3 &scale, CollisionObjectUserData *user_ptr)
{
  PhysCollision *newShape = obj.body->getCollisionScaledCopy(scale);
  CollisionObject co = create_coll_obj_from_shape(*newShape, user_ptr, /*kinematic*/ false, /*add_to_world*/ false);
  PhysCollision::clearAllocatedData(*newShape);
  delete newShape;
  return co;
}

void set_add_instances_to_world(bool flag) { add_instances_to_world = flag; }

void set_ttl_for_collision_instances(float value) { default_time_to_live = value; }

extern void push_non_empty_ri_instance_list(CollisionInstances &ci);

const ScaledBulletInstance &CollisionInstances::getScaledInstance(const rendinst::RendInstDesc &desc, const Point3 &scale,
  bool &out_created)
{
  float timeOfDeath = get_ri_instances_time() + default_time_to_live;
  for (auto &si : bulletInstances)
  {
    if (desc != si.desc)
      continue;

    si.timeOfDeath = timeOfDeath;
    out_created = false;
    return si;
  }
  //
  // Nothing found, create a new one
  //
  auto prevData = bulletInstances.data();
  auto &ret = bulletInstances.emplace_back(originalObj, scale, desc, timeOfDeath);
  if (EASTL_UNLIKELY(prevData && bulletInstances.data() != prevData)) // reallocated?
    for (auto it = bulletInstances.begin(); it != &ret; ++it)
      it->updateUserDataPtr();

#ifndef ENABLE_APEX
  if (bulletInstances.size() == 1)
    push_non_empty_ri_instance_list(*this);
#endif

  out_created = true;
  return ret;
}

extern void remove_empty_ri_instance_list(CollisionInstances &ci);

void CollisionInstances::removeCollisionObject(const rendinst::RendInstDesc &desc)
{
  bool removed = false;
  for (auto it = bulletInstances.begin(); it != bulletInstances.end();)
  {
    if (it->desc.doesReflectedInstanceMatches(desc))
    {
      removed = true;
      it = bulletInstances.erase_unsorted(it);
      if (it != bulletInstances.end())
        it->updateUserDataPtr();
    }
    else
      ++it;
  }
#ifndef ENABLE_APEX
  if (removed && bulletInstances.empty())
    remove_empty_ri_instance_list(*this);
#endif
}

void CollisionInstances::enableDisableCollisionObject(const rendinst::RendInstDesc &desc, bool flag)
{
  const auto it = eastl::find(disabledInstances.begin(), disabledInstances.end(), desc);
  const bool isEnabled = it == disabledInstances.end();
  if (isEnabled == flag)
    return;

  if (!flag)
    disabledInstances.emplace_back(desc);
  else
    disabledInstances.erase_unsorted(it);
}

bool CollisionInstances::isCollisionObjectEnabled(const rendinst::RendInstDesc &desc) const
{
  const auto it = eastl::find(disabledInstances.begin(), disabledInstances.end(), desc);
  const bool isEnabled = it == disabledInstances.end();
  return isEnabled;
}

CollisionInstances::CollisionInstances(CollisionInstances &&rhs) :
  bulletInstances(eastl::move(rhs.bulletInstances)),
  disabledInstances(eastl::move(rhs.disabledInstances)),
  originalObj(rhs.originalObj)
{
  rhs.originalObj.clear_ptrs();
}

CollisionInstances::~CollisionInstances()
{
  if (originalObj)
    destroy_dynamic_collision(originalObj);
}

void CollisionInstances::clear()
{
  clearInstances();
  destroy_dynamic_collision(originalObj);
  originalObj.clear_ptrs();
}

void CollisionInstances::clearInstances() { bulletInstances.clear(); }

// TODO: remove this _almost_ copy-paste of CollisionInstances::updateTm. We should have one way of dealing with
// collision most times
CollisionObject CollisionInstances::getCollisionObject(const rendinst::RendInstDesc &desc, TMatrix &tm, bool instant)
{
  bool created = false;
  Point3 localScaling = Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length());
  CollisionObject cobj = getScaledInstance(desc, localScaling, created).obj;

  for (int i = 0; i < 3; ++i)
    tm.setcol(i, tm.getcol(i) * safeinv(localScaling[i]));
  if (!instant)
    cobj.body->setTm(tm);
  else
    cobj.body->setTmInstant(tm);

  if (created)
  {
    cobj.body->setGroupAndLayerMask(EPL_STATIC, EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC));
    if (add_instances_to_world)
      get_phys_world()->addBody(cobj.body, /*kinematic*/ false); // After initial tm setup
  }

  return cobj;
}

CollisionObject CollisionInstances::updateTm(const rendinst::RendInstDesc &desc, const TMatrix &tm)
{
  bool created = false;
  Point3 localScaling = Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length());
  const ScaledBulletInstance &scaledInstance = getScaledInstance(desc, localScaling, created);
  CollisionObject cobj = scaledInstance.obj;

  TMatrix normTm = tm;
  for (int i = 0; i < 3; ++i)
    normTm.setcol(i, tm.getcol(i) * safeinv(localScaling[i]));

  get_phys_world()->fetchSimRes(true);

  if (localScaling != scaledInstance.scale)
    cobj.body->patchCollisionScaledCopy(localScaling, originalObj.body);

  cobj.body->setTm(normTm);

  if (created)
  {
    cobj.body->setGroupAndLayerMask(EPL_STATIC, EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC));
    if (add_instances_to_world)
      get_phys_world()->addBody(cobj.body, /*kinematic*/ false); // After initial tm setup
  }

  return cobj;
}

CollisionObject CollisionInstances::updateTm(const rendinst::RendInstDesc &desc, const Point3 &vel, const Point3 &omega)
{
  if (!rendinst::isRiGenDescValid(desc) || !isCollisionObjectEnabled(desc))
    return CollisionObject();
  TMatrix tm = rendinst::getRIGenMatrix(desc);

  CollisionObject cobj = updateTm(desc, tm);

  if (cobj.body)
  {
    cobj.body->setVelocity(vel);
    cobj.body->setAngularVelocity(omega);
  }

  return cobj;
}

} // namespace dacoll
