#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/destructableRendObject.h>
#include <EASTL/algorithm.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_geomTree.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <phys/dag_physics.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>
#include <memory/dag_fixedBlockAllocator.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/physLayers.h>
#include <gamePhys/props/atmosphere.h>

using namespace gamephys;

namespace destructables
{
static int default_fgroup = dacoll::EPL_DEFAULT;
static constexpr int default_fmask = dacoll::EPL_ALL;
} // namespace destructables

DestructableObject::DestructableObject(DynamicPhysObjectData *po_data, float scale_dt, const TMatrix &tm, PhysWorld *phys_world,
  int res_idx, uint32_t hash_val, const DataBlock *blk) :
  scaleDt(scale_dt),
  physObj(DynamicPhysObject::create(po_data, phys_world, tm, destructables::default_fgroup, destructables::default_fmask)),
  resIdx(res_idx)
{
  mat44f tm44;
  v_mat44_make_from_43cu_unsafe(tm44, tm.array);
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm44);
  v_stu(&intialTmAndHash[0].x, m43.row0);
  v_stu(&intialTmAndHash[1].x, m43.row1);
  v_stu(&intialTmAndHash[2].x, m43.row2);
  memcpy(&intialTmAndHash[3].x, &hash_val, sizeof(hash_val));

  if (blk)
  {
    ttl = blk->getReal("timeToLive", ttl);
    defaultTimeToLive = blk->getReal("timeForBodies", defaultTimeToLive);
    timeToKinematic = blk->getReal("timeToKinematic", timeToKinematic);
  }
  clear_and_resize(ttlBodies, physObj->getPhysSys()->getBodyCount());
  for (int i = 0; i < ttlBodies.size(); ++i)
    ttlBodies[i] = defaultTimeToLive;
  rendData.reset(destructables::init_rend_data(physObj.get()));
}

int DestructableObject::getNumActiveBodies() const { return ttlBodies.size(); }

bool DestructableObject::hasInteractableBodies() const
{
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    if (body->getInteractionLayer() != 0 && body->isInWorld())
      return true;
  }
  return false;
}

struct gamephys::DestructableObjectAddImpulse final : public AfterPhysUpdateAction
{
  // Note: it's okay to use direct ref since it would be pointing to pool's memory (`destructablesListAllocator`)
  DestructableObject &dobj;
  int gen;
  float speedLimit;
  Point3 pos, impulse;

  DestructableObjectAddImpulse(DestructableObject &dobj_, const Point3 &p, const Point3 &i, float sl) :
    dobj(dobj_), gen(dobj_.gen), pos(p), impulse(i), speedLimit(sl)
  {}

  void doAction(PhysWorld &pw, bool) override
  {
    if (gen == dobj.gen) // Wasn't destroyed?
      dobj.doAddImpulse(pw, pos, impulse, speedLimit);
  }
};

void DestructableObject::addImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit)
{
  exec_or_add_after_phys_action<DestructableObjectAddImpulse>(pw, *this, pos, impulse, speedLimit);
}

void DestructableObject::doAddImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit)
{
  float impulseLen = length(impulse);
  for (int i = 0, n = physObj->getPhysSys()->getBodyCount(); i < n; ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    float mass = body->getMass();
    TMatrix bodyTm;
    body->getTm(bodyTm);
    Point3 dirToBody = bodyTm.getcol(3) - pos;
    float distToBody = length(dirToBody);
    dirToBody *= safeinv(distToBody);
    const float impulseMult = 0.1f;
    body->addImpulse(pos, dirToBody * clamp(safeinv(sqr(distToBody)) * impulseLen * impulseMult, mass * 0.5f, mass * speedLimit));
  }
  if (impulseLen < 1e-5f)
    return;
  PhysRayCast rayCast(pos - normalize(impulse), normalize(impulse), 10.f, &pw);
  rayCast.setFilterMask(destructables::default_fgroup);
  rayCast.forceUpdate();
  if (!rayCast.hasContact())
    return;
  PhysBody *body = rayCast.getBody();
  if (!body)
    return;
  float mass = body->getMass();
  Point3 impulseDir = impulse * safeinv(impulseLen);
  body->addImpulse(pos, impulseDir * clamp(impulseLen, mass * 4.f, mass * 20.f));
}

void DestructableObject::keepFloatable(float dt, float at_time)
{
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    float mass = body->getMass();
    TMatrix bodyTm;
    body->getTm(bodyTm);
    Point3 pos = bodyTm.getcol(3);

    bool underwater = false;
    float waterHt = dacoll::traceht_water_at_time(pos, 0.5f, at_time, underwater);
    if (!dacoll::is_valid_water_height(waterHt))
      continue;

    float waterDist = pos.y - waterHt;
    if (waterDist < 0.0f && timeToFloat > 2.0f)
    {
      const float defDensityRatio = 1.2f;
      float fa = min(sqr(waterDist), sqr(-1.0f)) * defDensityRatio;
      Point3 impulse = Point3(0.f, fa * mass * gamephys::atmosphere::g() * dt, 0.f);
      body->addImpulse(pos, impulse);
    }
  }
}

bool DestructableObject::update(float dt, bool force_update_ttl)
{
  TIME_PROFILE(DestructableObject__update);

  float updateDt = dt * scaleDt;
  bool forceUpdateTtl = force_update_ttl && scaleDt > 1.f;
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    if (!body->isInWorld())
      continue;
    if (body->isActive() && ttlBodies[i] >= 0.f && !forceUpdateTtl)
    {
      ttlBodies[i] = defaultTimeToLive;
    }
    else
    {
      if (updateDt > ttlBodies[i] && ttlBodies[i] >= 0.f)
      {
        // Disable physics, just play animation
        body->setGroupAndLayerMask(0, 0);
        body->setGravity(Point3(0.f, -0.1f, 0.f));
      }
      if (ttlBodies[i] < 0.f)
        body->activateBody(true);
      ttlBodies[i] -= updateDt;
    }
  }
  if (timeToKinematic >= 0.f)
  {
    timeToKinematic -= updateDt;
    if (timeToKinematic < 0.f)
    {
      for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
      {
        PhysBody *body = physObj->getPhysSys()->getBody(i);
        if (body->getInteractionLayer() && body->isInWorld())
          body->setGroupAndLayerMask(destructables::default_fgroup, destructables::default_fmask ^ dacoll::EPL_KINEMATIC);
      }
    }
  }
  ttl -= updateDt;
  if (timeToFloat >= 0.f)
    timeToFloat -= updateDt;
  return isAlive();
}

namespace destructables
{
struct DestructableObjectDeleter
{
  void operator()(gamephys::DestructableObject *object);
};
static dag::Vector<eastl::unique_ptr<DestructableObject, DestructableObjectDeleter>> destructablesList;
static FixedBlockAllocator destructablesListAllocator;
void DestructableObjectDeleter::operator()(DestructableObject *object)
{
  (*(volatile int *)&object->gen)++; // Cast to disable DSE
  object->~DestructableObject();
  destructablesListAllocator.freeOneBlock(object);
}
int maxNumberOfDestructableBodies;
static int numActiveBodies;
static int numOfDestrBodiesForScaleDt;
static float distForScaleDtSq;
static float maxScaleDt;
float minDestrRadiusSq;
static float overflowReportTimeout = 0.0f;

void init(const DataBlock *blk, int fgroup)
{
  const DataBlock *destrBlk = blk->getBlockByNameEx("destructables");
  maxNumberOfDestructableBodies = destrBlk->getInt("maxNumberOfDestructableBodies", 100);
  numOfDestrBodiesForScaleDt = destrBlk->getInt("numOfDestrBodiesForScaleDt", maxNumberOfDestructableBodies * 0.5);
  distForScaleDtSq = sqr(destrBlk->getReal("distForScaleDt", 100.f));
  maxScaleDt = destrBlk->getReal("maxScaleDt", 3.f);
  minDestrRadiusSq = sqr(destrBlk->getReal("minDestrRadius", 40.f));
  default_fgroup = fgroup;
  if (!destructablesListAllocator.isInited())
    destructablesListAllocator.init(sizeof(DestructableObject), (4096 * 2 - 16) / sizeof(DestructableObject));
}

destructables::id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx, uint32_t hash_val, const DataBlock *blk)
{
  float scaleDt = 1.f;
  if (distForScaleDtSq > 0.f)
  {
    float distSq = (cam_pos - tm.getcol(3)).lengthSq();
    if (distSq > distForScaleDtSq)
      scaleDt = clamp(sqrtf(distSq / distForScaleDtSq), 1.f, maxScaleDt);
  }
  void *mem = destructablesListAllocator.allocateOneBlock();
  DestructableObject *obj = new (mem, _NEW_INPLACE) DestructableObject(po_data, scaleDt, tm, phys_world, res_idx, hash_val, blk);
  destructablesList.emplace_back(obj);
  if (out_destr)
    *out_destr = obj;
  return obj; // To consider: we can use high 16 bit of pointer for storing generation (for safety)
}

void clear()
{
  clear_and_shrink(destructablesList);
  destructablesListAllocator.clear();
}

void removeDestructableById(id_t id)
{
  if (id != INVALID_ID && destructablesListAllocator.isValidPtr(id))
    static_cast<DestructableObject *>(id)->markForDelete();
}

void update(float dt)
{
  TIME_PROFILE(destructables_update);

  float dtUpdate = dt * cvt(numActiveBodies, numOfDestrBodiesForScaleDt, maxNumberOfDestructableBodies, 1.f, maxScaleDt);
  bool forceUpdateTtl = numOfDestrBodiesForScaleDt > 0 && numActiveBodies > numOfDestrBodiesForScaleDt;
  numActiveBodies = 0;
  int deleteLimit = DESTRUCTABLES_DELETE_MAX_PER_FRAME;
  destructablesList.erase(eastl::remove_if(destructablesList.begin(), destructablesList.end(),
                            [&](auto &object) {
                              if (!object->isAlive())
                              {
                                // delete objects marked at previous frames
                                if (deleteLimit-- > 0)
                                  return true; // ~18 deletions at 1 msec on AMD 5800H. Move to thread?
                              }
                              else if (object->update(dtUpdate, forceUpdateTtl))
                                numActiveBodies += object->getNumActiveBodies();
                              // else it already considered marked for deletion
                              return false;
                            }),
    destructablesList.end());

  overflowReportTimeout -= dt;
  if (numActiveBodies > maxNumberOfDestructableBodies)
  {
    if (overflowReportTimeout <= 0.0f)
    {
      overflowReportTimeout = 1.0f;
      logwarn("destructables::update: too many destructable bodies %d, max - %d", numActiveBodies, maxNumberOfDestructableBodies);
    }

    // Cleanup first those bodies that are falling through, then all old ones
    for (auto &object : destructablesList)
    {
      if (object->isAlive())
      {
        bool shouldDelete = !object->hasInteractableBodies() && numActiveBodies > maxNumberOfDestructableBodies;
        if (shouldDelete)
        {
          numActiveBodies -= object->getNumActiveBodies();
          object->markForDelete();
        }
      }
    }

    // First ones are oldest ones, as they were added in this order.
    if (numActiveBodies > maxNumberOfDestructableBodies)
    {
      for (auto &object : destructablesList)
      {
        if (object->isAlive())
        {
          bool shouldDelete = numActiveBodies > maxNumberOfDestructableBodies;
          if (shouldDelete)
          {
            numActiveBodies -= object->getNumActiveBodies();
            object->markForDelete();
          }
        }
      }
    }
  }
}

dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects()
{
  G_STATIC_ASSERT(sizeof(typename decltype(destructablesList)::value_type) == sizeof(gamephys::DestructableObject *));
  return dag::ConstSpan<gamephys::DestructableObject *>((gamephys::DestructableObject **)destructablesList.data(), //-V1032
    destructablesList.size());
}

} // namespace destructables
